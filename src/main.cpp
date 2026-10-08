// ESP32 Safety Wearable — reference firmware
//
// REFERENCE IMPLEMENTATION. The original HerZion 2025 prototype firmware is
// not in this repository. This file re-implements the documented architecture
// (ECG + GSR + PIR sensing, rule-based event detection, panic button, LED
// indication, emergency-contact notification) so it can be built, read and
// tested. Pins and thresholds live in include/config.h; see docs/ for details.
//
// Main loop (non-blocking except while an HTTP request is in flight):
//   sample ECG (250 Hz) / GSR (20 Hz) / PIR / button
//   -> derive per-sensor conditions -> DecisionEngine -> LEDs
//   -> on entering EMERGENCY, POST an alert; retry until delivered or cancelled.

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "button.h"
#include "config.h"
#include "decision.h"
#include "ecg_rate.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
// Lets the project compile out of the box; alerts will fail until secrets.h exists.
#warning "include/secrets.h not found - building with placeholder values from secrets.example.h"
#include "secrets.example.h"
#endif

// ---------------------------------------------------------------------------
// Modules
// ---------------------------------------------------------------------------
static EcgRateEstimator ecg({ECG_WINDOW_MS, ECG_THRESHOLD_FRACTION, ECG_MIN_PEAK_TO_PEAK,
                             ECG_REFRACTORY_MS, ECG_MIN_RR_MS, ECG_MAX_RR_MS, ECG_STALE_MS});
static ButtonTracker button(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
static DecisionEngine decision(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);

// ---------------------------------------------------------------------------
// Runtime state
// ---------------------------------------------------------------------------
static uint32_t lastEcgSampleUs = 0;
static uint32_t lastGsrSampleMs = 0;
static uint32_t lastReportMs = 0;
static uint32_t lastWifiAttemptMs = 0;

static bool ecgLeadsOff = true;
static float gsrFiltered = 0.0f;
static bool gsrFilterPrimed = false;
static uint32_t lastPirHighMs = 0;
static bool pirEverHigh = false;

// Calibration accumulators (resting baselines for ECG rate and GSR level).
static bool calibrated = false;
static double gsrBaselineSum = 0.0;
static uint32_t gsrBaselineCount = 0;
static float gsrBaseline = 0.0f;
static double ecgBaselineSum = 0.0;
static uint32_t ecgBaselineCount = 0;
static float ecgBaselineBpm = -1.0f;  // negative = no usable ECG during calibration

// Notification state for the current emergency.
static bool alertPending = false;
static bool alertDelivered = false;
static uint32_t lastAlertAttemptMs = 0;
static uint32_t alertAttempts = 0;

static SystemState lastState = SystemState::Calibrating;
static SensorConditions conditions;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static const char* stateName(SystemState s) {
  switch (s) {
    case SystemState::Calibrating: return "CALIBRATING";
    case SystemState::Normal:      return "NORMAL";
    case SystemState::Warning:     return "WARNING";
    case SystemState::Emergency:   return "EMERGENCY";
  }
  return "?";
}

static const char* sourceName(EmergencySource s) {
  switch (s) {
    case EmergencySource::Manual:    return "manual";
    case EmergencySource::Automatic: return "automatic";
    default:                         return "none";
  }
}

static float gsrDeviationFraction() {
  if (gsrBaseline < GSR_MIN_VALID_BASELINE) return 0.0f;
  return fabsf(gsrFiltered - gsrBaseline) / gsrBaseline;
}

// ---------------------------------------------------------------------------
// Sensor acquisition
// ---------------------------------------------------------------------------
static void sampleEcg(uint32_t nowUs, uint32_t nowMs) {
  if (nowUs - lastEcgSampleUs < ECG_SAMPLE_PERIOD_US) return;
  lastEcgSampleUs += ECG_SAMPLE_PERIOD_US;
  // If the loop was blocked (e.g. during an HTTP request), resynchronise
  // instead of trying to "catch up" with a burst of back-to-back samples.
  if (nowUs - lastEcgSampleUs > ECG_SAMPLE_PERIOD_US) lastEcgSampleUs = nowUs;

  // The AD8232 drives LO+/LO- HIGH when an electrode loses contact. Its
  // output is meaningless then, so the estimator is reset rather than fed.
  const bool leadsOff = digitalRead(PIN_ECG_LO_PLUS) == HIGH || digitalRead(PIN_ECG_LO_MINUS) == HIGH;
  if (leadsOff) {
    if (!ecgLeadsOff) ecg.reset();
    ecgLeadsOff = true;
    return;
  }
  ecgLeadsOff = false;
  ecg.addSample(analogRead(PIN_ECG_OUT), nowMs);
}

static void sampleGsr(uint32_t nowMs) {
  if (nowMs - lastGsrSampleMs < GSR_SAMPLE_PERIOD_MS) return;
  lastGsrSampleMs = nowMs;

  // Skin conductance changes slowly; an exponential moving average removes
  // ADC noise and small contact jitter without needing a sample buffer.
  const float raw = static_cast<float>(analogRead(PIN_GSR));
  if (!gsrFilterPrimed) {
    gsrFiltered = raw;
    gsrFilterPrimed = true;
  } else {
    gsrFiltered += GSR_EMA_ALPHA * (raw - gsrFiltered);
  }
}

static void samplePir(uint32_t nowMs) {
  // HC-SR501: HIGH while motion is detected (plus its own on-board hold time).
  if (digitalRead(PIN_PIR) == HIGH) {
    lastPirHighMs = nowMs;
    pirEverHigh = true;
  }
}

// ---------------------------------------------------------------------------
// Calibration: capture resting baselines once after power-up.
// ---------------------------------------------------------------------------
static void updateCalibration(uint32_t nowMs) {
  if (calibrated) return;

  // Accumulate at the GSR rate so both baselines weight time equally.
  static uint32_t lastAccumMs = 0;
  if (nowMs - lastAccumMs >= GSR_SAMPLE_PERIOD_MS) {
    lastAccumMs = nowMs;
    if (gsrFilterPrimed) {
      gsrBaselineSum += gsrFiltered;
      ++gsrBaselineCount;
    }
    const float bpm = ecg.rateBpm(nowMs);
    if (bpm > 0) {
      ecgBaselineSum += bpm;
      ++ecgBaselineCount;
    }
  }

  if (nowMs < CALIBRATION_MS) return;

  gsrBaseline = gsrBaselineCount ? static_cast<float>(gsrBaselineSum / gsrBaselineCount) : 0.0f;
  ecgBaselineBpm = ecgBaselineCount ? static_cast<float>(ecgBaselineSum / ecgBaselineCount) : -1.0f;
  calibrated = true;

  Serial.printf("[CAL] done: gsr_baseline=%.1f ecg_baseline_bpm=%.1f%s%s\n", gsrBaseline, ecgBaselineBpm,
                gsrBaseline < GSR_MIN_VALID_BASELINE ? " (GSR condition disabled: no contact)" : "",
                ecgBaselineBpm < 0 ? " (ECG condition disabled: no usable signal)" : "");
}

// ---------------------------------------------------------------------------
// Per-sensor conditions fed to the decision engine.
// ---------------------------------------------------------------------------
static SensorConditions evaluateConditions(uint32_t nowMs) {
  SensorConditions c;
  if (!calibrated) return c;

  // ECG: beat rate clearly above this wearer's resting baseline. Disabled if
  // no baseline was captured or the electrodes are off.
  const float bpm = ecg.rateBpm(nowMs);
  c.ecg = !ecgLeadsOff && ecgBaselineBpm > 0 && bpm > 0 && bpm >= ecgBaselineBpm * ECG_RATE_RISE_FACTOR;

  // GSR: significant deviation of the conductance-related reading from baseline.
  c.gsr = gsrDeviationFraction() >= GSR_DEVIATION_FRACTION;

  // PIR: motion reported recently.
  c.pir = pirEverHigh && (nowMs - lastPirHighMs) <= PIR_HOLD_MS;
  return c;
}

// ---------------------------------------------------------------------------
// LED indication
// ---------------------------------------------------------------------------
static void updateLeds(SystemState s, uint32_t nowMs) {
  const bool slowBlink = (nowMs / 500) % 2;   // 1 Hz
  const bool fastBlink = (nowMs / 125) % 2;   // 4 Hz
  bool green = false, yellow = false, red = false;

  switch (s) {
    case SystemState::Calibrating:
      yellow = slowBlink;
      break;
    case SystemState::Normal:
      // Solid green when Wi-Fi is up; blinking green means "armed, but an
      // alert could not be sent right now" (local panic still works).
      green = (WiFi.status() == WL_CONNECTED) ? true : slowBlink;
      break;
    case SystemState::Warning:
      yellow = true;
      break;
    case SystemState::Emergency:
      // Fast blink until the receiver acknowledges with HTTP 2xx, then solid.
      red = alertDelivered ? true : fastBlink;
      break;
  }
  digitalWrite(PIN_LED_GREEN, green);
  digitalWrite(PIN_LED_YELLOW, yellow);
  digitalWrite(PIN_LED_RED, red);
}

// ---------------------------------------------------------------------------
// Network + notification
// ---------------------------------------------------------------------------
static void maintainWifi(uint32_t nowMs) {
  if (WiFi.status() == WL_CONNECTED) return;
  if (lastWifiAttemptMs != 0 && nowMs - lastWifiAttemptMs < WIFI_RECONNECT_MS) return;
  lastWifiAttemptMs = nowMs;
  Serial.println("[WIFI] connecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // non-blocking; status polled next loop
}

// Sends one alert attempt. Blocks for up to HTTP_TIMEOUT_MS; sensor sampling
// pauses during that time, which is acceptable because the device is already
// in the latched EMERGENCY state. Returns true on HTTP 2xx.
static bool sendAlert(uint32_t nowMs) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ALERT] no Wi-Fi, will retry");
    return false;
  }

  char body[256];
  snprintf(body, sizeof(body),
           "{\"device\":\"%s\",\"event\":\"EMERGENCY\",\"trigger\":\"%s\","
           "\"ecg_bpm\":%.0f,\"gsr_deviation\":%.2f,\"pir_recent\":%s,\"uptime_ms\":%lu}",
           DEVICE_ID, sourceName(decision.source()), ecg.rateBpm(nowMs), gsrDeviationFraction(),
           conditions.pir ? "true" : "false", static_cast<unsigned long>(nowMs));

  HTTPClient http;
  WiFiClientSecure secureClient;
  WiFiClient plainClient;
  const String url = NOTIFY_URL;
  bool begun;
  if (url.startsWith("https://")) {
#ifdef NOTIFY_ROOT_CA
    secureClient.setCACert(NOTIFY_ROOT_CA);
#else
    secureClient.setInsecure();  // encrypted but NOT authenticated; see docs/limitations.md
#endif
    begun = http.begin(secureClient, url);
  } else {
    begun = http.begin(plainClient, url);
  }
  if (!begun) {
    Serial.println("[ALERT] invalid NOTIFY_URL");
    return false;
  }

  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  if (strlen(NOTIFY_TOKEN) > 0) http.addHeader("Authorization", String("Bearer ") + NOTIFY_TOKEN);

  const int code = http.POST(reinterpret_cast<uint8_t*>(body), strlen(body));
  http.end();
  Serial.printf("[ALERT] attempt %lu -> HTTP %d\n", static_cast<unsigned long>(alertAttempts), code);
  return code >= 200 && code < 300;
}

static void serviceNotification(uint32_t nowMs) {
  if (!alertPending || alertDelivered) return;
  if (alertAttempts > 0 && nowMs - lastAlertAttemptMs < NOTIFY_RETRY_MS) return;
  lastAlertAttemptMs = nowMs;
  ++alertAttempts;
  if (sendAlert(nowMs)) {
    alertDelivered = true;
    Serial.println("[ALERT] delivered");
  }
}

// ---------------------------------------------------------------------------
// Serial debug output (one CSV-style line every SERIAL_REPORT_MS)
// ---------------------------------------------------------------------------
static void report(uint32_t nowMs, SystemState s) {
  if (nowMs - lastReportMs < SERIAL_REPORT_MS) return;
  lastReportMs = nowMs;
  Serial.printf("t=%lu state=%s ecg_off=%d ecg_p2p=%d bpm=%.1f gsr=%.0f gsr_dev=%.2f pir=%d btn=%d cond=%d%d%d wifi=%d\n",
                static_cast<unsigned long>(nowMs), stateName(s), ecgLeadsOff, ecg.peakToPeak(), ecg.rateBpm(nowMs),
                gsrFiltered, gsrDeviationFraction(), digitalRead(PIN_PIR), button.pressed(), conditions.ecg,
                conditions.gsr, conditions.pir, WiFi.status() == WL_CONNECTED);
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_ECG_LO_PLUS, INPUT);
  pinMode(PIN_ECG_LO_MINUS, INPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_PANIC_BUTTON, INPUT_PULLUP);  // pressed = LOW
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  // 12-bit ADC with 11 dB attenuation covers roughly 0-3.1 V, enough for the
  // AD8232 output (mid-rail around 1.65 V) and a 3.3 V-powered GSR module.
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  WiFi.mode(WIFI_STA);
  maintainWifi(millis());

  lastEcgSampleUs = micros();
  Serial.println("[BOOT] ESP32 safety wearable (reference firmware). Calibrating - keep still.");
}

void loop() {
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();

  sampleEcg(nowUs, nowMs);
  sampleGsr(nowMs);
  samplePir(nowMs);
  updateCalibration(nowMs);

  const bool rawPressed = digitalRead(PIN_PANIC_BUTTON) == LOW;
  const ButtonEvent ev = button.update(rawPressed, nowMs, decision.state() == SystemState::Emergency);

  conditions = evaluateConditions(nowMs);
  const SystemState s = decision.update(nowMs, conditions, calibrated, ev);

  if (s != lastState) {
    Serial.printf("[STATE] %s -> %s", stateName(lastState), stateName(s));
    if (s == SystemState::Emergency) Serial.printf(" (trigger=%s)", sourceName(decision.source()));
    Serial.println();

    if (s == SystemState::Emergency) {
      alertPending = true;
      alertDelivered = false;
      alertAttempts = 0;
    } else if (lastState == SystemState::Emergency) {
      // Cancelled locally. No "cancelled" message is sent (see docs/limitations.md).
      alertPending = false;
    }
    lastState = s;
  }

  updateLeds(s, nowMs);
  maintainWifi(nowMs);
  serviceNotification(nowMs);
  report(nowMs, s);
}
