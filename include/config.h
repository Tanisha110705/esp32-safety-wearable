// config.h — pin map, timing and decision thresholds for the reference firmware.
//
// IMPORTANT: The original HerZion 2025 prototype firmware is not in this
// repository. Every value below is a REFERENCE value chosen for this
// re-implementation, not a value recovered from the original prototype or
// measured on it. Check pins against your own wiring and calibrate the
// thresholds on your own hardware before relying on them.

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Pin map (ESP32 DevKit V1 / ESP32-WROOM-32)
// ---------------------------------------------------------------------------
// Analog inputs are on ADC1 deliberately: ADC2 cannot be read while the
// Wi-Fi driver is active, and this firmware needs Wi-Fi for notification.
// GPIO34/35 are input-only pins, which suits sensor outputs.

constexpr uint8_t PIN_ECG_OUT       = 34;  // AD8232 OUTPUT  -> ADC1_CH6
constexpr uint8_t PIN_ECG_LO_PLUS   = 32;  // AD8232 LO+     (HIGH = electrode off)
constexpr uint8_t PIN_ECG_LO_MINUS  = 33;  // AD8232 LO-     (HIGH = electrode off)
constexpr uint8_t PIN_GSR           = 35;  // GSR module SIG -> ADC1_CH7
constexpr uint8_t PIN_PIR           = 27;  // HC-SR501 OUT   (HIGH = motion)
constexpr uint8_t PIN_PANIC_BUTTON  = 13;  // momentary switch to GND, internal pull-up
constexpr uint8_t PIN_LED_GREEN     = 18;  // normal / armed
constexpr uint8_t PIN_LED_YELLOW    = 19;  // calibrating / warning
constexpr uint8_t PIN_LED_RED       = 21;  // emergency

// ---------------------------------------------------------------------------
// Sampling
// ---------------------------------------------------------------------------
constexpr uint32_t ECG_SAMPLE_PERIOD_US = 4000;   // 250 Hz ECG sampling
constexpr uint32_t GSR_SAMPLE_PERIOD_MS = 50;     // 20 Hz GSR sampling
constexpr float    GSR_EMA_ALPHA        = 0.10f;  // GSR smoothing (exponential moving average)
constexpr uint32_t SERIAL_REPORT_MS     = 500;    // debug line period

// Baselines for ECG rate and GSR level are captured over this window after
// power-up. The wearer should be at rest while the yellow LED blinks.
constexpr uint32_t CALIBRATION_MS = 15000;

// ---------------------------------------------------------------------------
// ECG beat-interval estimator (see include/ecg_rate.h)
// ---------------------------------------------------------------------------
constexpr uint32_t ECG_WINDOW_MS          = 2000;  // min/max tracking window for adaptive threshold
constexpr float    ECG_THRESHOLD_FRACTION = 0.60f; // threshold = min + fraction * (max - min)
constexpr int      ECG_MIN_PEAK_TO_PEAK   = 300;   // ADC counts; below this the signal is treated as unusable
constexpr uint32_t ECG_REFRACTORY_MS      = 300;   // ignore crossings this soon after a detected beat
constexpr uint32_t ECG_MIN_RR_MS          = 300;   // reject intervals shorter than this
constexpr uint32_t ECG_MAX_RR_MS          = 2000;  // reject intervals longer than this
constexpr uint32_t ECG_STALE_MS           = 3000;  // no beat for this long -> rate invalid

// ---------------------------------------------------------------------------
// Per-sensor conditions (see include/decision.h and docs/decision-logic.md)
// ---------------------------------------------------------------------------
// ECG condition: estimated beat rate at least this factor above the resting
// baseline captured during calibration.
constexpr float ECG_RATE_RISE_FACTOR = 1.25f;

// GSR condition: smoothed reading deviates from the calibration baseline by at
// least this fraction. A baseline below GSR_MIN_VALID_BASELINE is treated as
// "no sensor contact" and the condition is disabled.
constexpr float GSR_DEVIATION_FRACTION = 0.15f;
constexpr float GSR_MIN_VALID_BASELINE = 100.0f;

// PIR condition: motion reported within the last PIR_HOLD_MS.
constexpr uint32_t PIR_HOLD_MS = 5000;

// ---------------------------------------------------------------------------
// Decision rules
// ---------------------------------------------------------------------------
// WARNING  : at least WARNING_MIN_CONDITIONS sensor conditions are true.
// EMERGENCY: at least AUTO_MIN_CONDITIONS conditions stay true for
//            AUTO_CONFIRM_MS without interruption, OR the panic button fires.
constexpr uint8_t  WARNING_MIN_CONDITIONS = 2;
constexpr uint8_t  AUTO_MIN_CONDITIONS    = 3;
constexpr uint32_t AUTO_CONFIRM_MS        = 10000;

// ---------------------------------------------------------------------------
// Panic button (see include/button.h)
// ---------------------------------------------------------------------------
constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;    // contact must be stable this long
constexpr uint32_t PANIC_HOLD_MS      = 1500;  // hold to raise a manual emergency
constexpr uint32_t CANCEL_HOLD_MS     = 5000;  // hold (press started during emergency) to cancel

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------
constexpr uint32_t WIFI_RECONNECT_MS   = 10000;  // re-attempt Wi-Fi association this often
constexpr uint32_t HTTP_TIMEOUT_MS     = 5000;   // per-request timeout
constexpr uint32_t NOTIFY_RETRY_MS     = 10000;  // retry an undelivered alert this often
