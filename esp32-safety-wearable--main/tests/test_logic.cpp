// Host-side unit tests for the hardware-independent firmware logic:
//   include/button.h    (debounce, panic hold, cancel hold)
//   include/decision.h  (state rules, sustained confirmation, latching)
//   include/ecg_rate.h  (beat-interval estimator)
//
// These run on a PC, not on the ESP32, and use SYNTHETIC inputs generated in
// this file. They check that the code implements the documented rules; they
// say nothing about how the sensors behave on a real wearer.
//
// Build and run from the repository root:
//   g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_logic.cpp -o tests/test_logic && ./tests/test_logic

#include <cmath>
#include <cstdio>

#include "button.h"
#include "config.h"
#include "decision.h"
#include "ecg_rate.h"

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    ++checks;                                                          \
    if (!(cond)) {                                                     \
      ++failures;                                                      \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
    }                                                                  \
  } while (0)

// ---------------------------------------------------------------------------
// Button
// ---------------------------------------------------------------------------
static ButtonEvent holdButton(ButtonTracker& b, uint32_t& t, uint32_t durationMs, bool inEmergency) {
  ButtonEvent result = ButtonEvent::None;
  for (uint32_t i = 0; i < durationMs; ++i, ++t) {
    ButtonEvent e = b.update(true, t, inEmergency);
    if (e != ButtonEvent::None) result = e;
  }
  return result;
}

static void releaseButton(ButtonTracker& b, uint32_t& t, uint32_t durationMs) {
  for (uint32_t i = 0; i < durationMs; ++i, ++t) b.update(false, t, false);
}

static void testButton() {
  std::printf("button\n");
  uint32_t t = 0;

  {  // A short tap does not raise an emergency.
    ButtonTracker b(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
    CHECK(holdButton(b, t, 300, false) == ButtonEvent::None);
    releaseButton(b, t, 200);
  }
  {  // Holding past debounce + PANIC_HOLD_MS fires Panic exactly once.
    ButtonTracker b(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
    int panics = 0;
    for (uint32_t i = 0; i < BUTTON_DEBOUNCE_MS + PANIC_HOLD_MS + 2000; ++i, ++t)
      if (b.update(true, t, false) == ButtonEvent::Panic) ++panics;
    CHECK(panics == 1);
  }
  {  // Contact bounce shorter than the debounce time is ignored.
    ButtonTracker b(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
    for (int i = 0; i < 100; ++i, t += 10) b.update(i % 2 == 0, t, false);
    CHECK(!b.pressed());
  }
  {  // A press started during an emergency cancels after CANCEL_HOLD_MS, not before.
    ButtonTracker b(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
    CHECK(holdButton(b, t, BUTTON_DEBOUNCE_MS + PANIC_HOLD_MS + 100, true) == ButtonEvent::None);
    CHECK(holdButton(b, t, CANCEL_HOLD_MS, true) == ButtonEvent::Cancel);
  }
  {  // One long hold from NORMAL raises Panic but never also Cancel.
    ButtonTracker b(BUTTON_DEBOUNCE_MS, PANIC_HOLD_MS, CANCEL_HOLD_MS);
    bool sawPanic = false, sawCancel = false;
    for (uint32_t i = 0; i < CANCEL_HOLD_MS * 2; ++i, ++t) {
      // After Panic fires the caller would report inEmergency = true.
      ButtonEvent e = b.update(true, t, sawPanic);
      sawPanic |= e == ButtonEvent::Panic;
      sawCancel |= e == ButtonEvent::Cancel;
    }
    CHECK(sawPanic);
    CHECK(!sawCancel);
  }
}

// ---------------------------------------------------------------------------
// Decision engine
// ---------------------------------------------------------------------------
static SensorConditions conds(bool ecg, bool gsr, bool pir) {
  SensorConditions c;
  c.ecg = ecg;
  c.gsr = gsr;
  c.pir = pir;
  return c;
}

static void testDecision() {
  std::printf("decision\n");
  const auto none = ButtonEvent::None;

  {  // Calibrating until calibrated; sensor conditions ignored meanwhile.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    for (uint32_t t = 0; t < AUTO_CONFIRM_MS * 2; t += 10)
      CHECK(d.update(t, conds(true, true, true), false, none) == SystemState::Calibrating);
  }
  {  // Panic works even during calibration.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    CHECK(d.update(0, conds(false, false, false), false, ButtonEvent::Panic) == SystemState::Emergency);
    CHECK(d.source() == EmergencySource::Manual);
  }
  {  // 0/1 conditions -> NORMAL, 2 -> WARNING.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    CHECK(d.update(0, conds(false, false, false), true, none) == SystemState::Normal);
    CHECK(d.update(10, conds(false, false, true), true, none) == SystemState::Normal);
    CHECK(d.update(20, conds(false, true, true), true, none) == SystemState::Warning);
    CHECK(d.update(30, conds(true, true, false), true, none) == SystemState::Warning);
  }
  {  // Two conditions held indefinitely never escalate to EMERGENCY.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    for (uint32_t t = 0; t < AUTO_CONFIRM_MS * 5; t += 10)
      CHECK(d.update(t, conds(true, false, true), true, none) == SystemState::Warning);
  }
  {  // All three for AUTO_CONFIRM_MS -> automatic EMERGENCY, and not earlier.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    uint32_t t = 1000;
    d.update(t, conds(true, true, true), true, none);
    CHECK(d.update(t + AUTO_CONFIRM_MS - 1, conds(true, true, true), true, none) == SystemState::Warning);
    CHECK(d.update(t + AUTO_CONFIRM_MS, conds(true, true, true), true, none) == SystemState::Emergency);
    CHECK(d.source() == EmergencySource::Automatic);
  }
  {  // An interruption restarts the confirmation timer.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    d.update(0, conds(true, true, true), true, none);
    d.update(AUTO_CONFIRM_MS - 100, conds(true, true, false), true, none);  // PIR drops briefly
    d.update(AUTO_CONFIRM_MS - 50, conds(true, true, true), true, none);
    CHECK(d.update(AUTO_CONFIRM_MS + 100, conds(true, true, true), true, none) == SystemState::Warning);
    CHECK(d.update(2 * AUTO_CONFIRM_MS - 50, conds(true, true, true), true, none) == SystemState::Emergency);
  }
  {  // EMERGENCY latches when conditions clear; only Cancel returns to NORMAL.
    DecisionEngine d(WARNING_MIN_CONDITIONS, AUTO_MIN_CONDITIONS, AUTO_CONFIRM_MS);
    d.update(0, conds(false, false, false), true, ButtonEvent::Panic);
    CHECK(d.update(100, conds(false, false, false), true, none) == SystemState::Emergency);
    CHECK(d.update(200, conds(false, false, false), true, ButtonEvent::Cancel) == SystemState::Normal);
    CHECK(d.source() == EmergencySource::None);
  }
}

// ---------------------------------------------------------------------------
// ECG rate estimator — synthetic pulse train, not real ECG data.
// ---------------------------------------------------------------------------
static EcgRateEstimator::Params ecgParams() {
  return {ECG_WINDOW_MS, ECG_THRESHOLD_FRACTION, ECG_MIN_PEAK_TO_PEAK, ECG_REFRACTORY_MS,
          ECG_MIN_RR_MS, ECG_MAX_RR_MS, ECG_STALE_MS};
}

// Mid-rail baseline (~2048 counts) with a narrow 40 ms spike every periodMs
// and a smaller, wider secondary bump 250 ms later (a crude stand-in for a
// T wave) to exercise the threshold and refractory logic.
static int syntheticSample(uint32_t tMs, uint32_t periodMs, int spike) {
  const uint32_t phase = tMs % periodMs;
  int v = 2048;
  if (phase < 40) v += spike;
  if (phase >= 250 && phase < 330) v += spike / 4;
  return v;
}

static void testEcg() {
  std::printf("ecg_rate\n");
  {  // 800 ms period -> ~75 per minute.
    EcgRateEstimator e(ecgParams());
    uint32_t t = 0;
    for (; t < 15000; t += 4) e.addSample(syntheticSample(t, 800, 1200), t);
    const float bpm = e.rateBpm(t);
    std::printf("  synthetic 800 ms period -> %.1f\n", bpm);
    CHECK(std::fabs(bpm - 75.0f) < 1.0f);
  }
  {  // 500 ms period -> ~120 per minute.
    EcgRateEstimator e(ecgParams());
    uint32_t t = 0;
    for (; t < 15000; t += 4) e.addSample(syntheticSample(t, 500, 1200), t);
    const float bpm = e.rateBpm(t);
    std::printf("  synthetic 500 ms period -> %.1f\n", bpm);
    CHECK(std::fabs(bpm - 120.0f) < 1.0f);
  }
  {  // Flat signal (amplitude below ECG_MIN_PEAK_TO_PEAK) -> invalid.
    EcgRateEstimator e(ecgParams());
    uint32_t t = 0;
    for (; t < 15000; t += 4) e.addSample(syntheticSample(t, 800, 100), t);
    CHECK(e.rateBpm(t) < 0);
  }
  {  // Signal stops -> rate becomes invalid after ECG_STALE_MS.
    EcgRateEstimator e(ecgParams());
    uint32_t t = 0;
    for (; t < 10000; t += 4) e.addSample(syntheticSample(t, 800, 1200), t);
    CHECK(e.rateBpm(t) > 0);
    CHECK(e.rateBpm(t + ECG_STALE_MS + 1000) < 0);
  }
  {  // reset() clears history.
    EcgRateEstimator e(ecgParams());
    uint32_t t = 0;
    for (; t < 10000; t += 4) e.addSample(syntheticSample(t, 800, 1200), t);
    e.reset();
    CHECK(e.rateBpm(t) < 0);
  }
}

int main() {
  testButton();
  testDecision();
  testEcg();
  std::printf("\n%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
