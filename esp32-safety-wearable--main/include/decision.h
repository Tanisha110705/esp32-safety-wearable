// decision.h — rule-based event decision (hardware-independent).
//
// Inputs each loop: three boolean sensor conditions (computed in main.cpp from
// ECG, GSR and PIR), whether calibration has finished, and the button event.
//
// Rules, in priority order:
//   1. Panic button (manual trigger) -> EMERGENCY from any state, including
//      during calibration. This is the primary, explicit emergency path.
//   2. EMERGENCY is latched. Only a Cancel button event returns to NORMAL,
//      so a brief drop in sensor conditions never silently clears an alert.
//   3. While calibrating, sensor conditions are ignored (no baseline yet).
//   4. If >= autoMinConditions conditions stay true continuously for
//      autoConfirmMs -> EMERGENCY (automatic). Requiring agreement between
//      sensors *and* persistence over time is the main defence against single
//      noisy readings (e.g. a PIR blip or electrode movement).
//   5. If >= warningMinConditions conditions are true -> WARNING (local
//      indication only, no notification).
//   6. Otherwise NORMAL.

#pragma once

#include <stdint.h>

#include "button.h"

enum class SystemState : uint8_t { Calibrating, Normal, Warning, Emergency };
enum class EmergencySource : uint8_t { None, Manual, Automatic };

struct SensorConditions {
  bool ecg = false;
  bool gsr = false;
  bool pir = false;

  uint8_t count() const { return static_cast<uint8_t>(ecg) + gsr + pir; }
};

class DecisionEngine {
 public:
  DecisionEngine(uint8_t warningMinConditions, uint8_t autoMinConditions, uint32_t autoConfirmMs)
      : warnMin_(warningMinConditions), autoMin_(autoMinConditions), confirmMs_(autoConfirmMs) {}

  SystemState update(uint32_t nowMs, const SensorConditions& c, bool calibrated, ButtonEvent button) {
    if (button == ButtonEvent::Panic && state_ != SystemState::Emergency) {
      enterEmergency(EmergencySource::Manual);
      return state_;
    }

    if (state_ == SystemState::Emergency) {
      if (button == ButtonEvent::Cancel) {
        state_ = calibrated ? SystemState::Normal : SystemState::Calibrating;
        source_ = EmergencySource::None;
        sustaining_ = false;
      }
      return state_;
    }

    if (!calibrated) {
      state_ = SystemState::Calibrating;
      sustaining_ = false;
      return state_;
    }

    const uint8_t n = c.count();
    if (n >= autoMin_) {
      if (!sustaining_) {
        sustaining_ = true;
        sustainStartMs_ = nowMs;
      } else if (nowMs - sustainStartMs_ >= confirmMs_) {
        enterEmergency(EmergencySource::Automatic);
        return state_;
      }
    } else {
      sustaining_ = false;  // any interruption restarts the confirmation timer
    }

    state_ = (n >= warnMin_) ? SystemState::Warning : SystemState::Normal;
    return state_;
  }

  SystemState state() const { return state_; }
  EmergencySource source() const { return source_; }

 private:
  void enterEmergency(EmergencySource s) {
    state_ = SystemState::Emergency;
    source_ = s;
    sustaining_ = false;
  }

  uint8_t warnMin_, autoMin_;
  uint32_t confirmMs_;
  SystemState state_ = SystemState::Calibrating;
  EmergencySource source_ = EmergencySource::None;
  bool sustaining_ = false;
  uint32_t sustainStartMs_ = 0;
};
