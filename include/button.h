// button.h — debounced panic-button state machine (hardware-independent).
//
// The caller passes the raw "pressed" level and the current time; the tracker
// debounces it and reports at most one event per physical press:
//   - Panic : a press that started outside an emergency has been held for
//             panicHoldMs. A long hold (not a tap) is required so that an
//             accidental bump does not send an alert.
//   - Cancel: a press that started *during* an emergency has been held for
//             cancelHoldMs. Remembering which state the press started in stops
//             one long hold from raising an emergency and then immediately
//             cancelling it.

#pragma once

#include <stdint.h>

enum class ButtonEvent : uint8_t { None, Panic, Cancel };

class ButtonTracker {
 public:
  ButtonTracker(uint32_t debounceMs, uint32_t panicHoldMs, uint32_t cancelHoldMs)
      : debounceMs_(debounceMs), panicHoldMs_(panicHoldMs), cancelHoldMs_(cancelHoldMs) {}

  ButtonEvent update(bool rawPressed, uint32_t nowMs, bool inEmergency) {
    // Debounce: accept a new level only after it has been stable for debounceMs.
    if (rawPressed != lastRaw_) {
      lastRaw_ = rawPressed;
      lastChangeMs_ = nowMs;
    }
    if (rawPressed != stable_ && (nowMs - lastChangeMs_) >= debounceMs_) {
      stable_ = rawPressed;
      if (stable_) {
        pressStartMs_ = nowMs;
        pressStartedInEmergency_ = inEmergency;
        eventFired_ = false;
      }
    }

    if (!stable_ || eventFired_) return ButtonEvent::None;

    const uint32_t held = nowMs - pressStartMs_;
    if (!pressStartedInEmergency_ && held >= panicHoldMs_) {
      eventFired_ = true;
      return ButtonEvent::Panic;
    }
    if (pressStartedInEmergency_ && held >= cancelHoldMs_) {
      eventFired_ = true;
      return ButtonEvent::Cancel;
    }
    return ButtonEvent::None;
  }

  bool pressed() const { return stable_; }

 private:
  uint32_t debounceMs_, panicHoldMs_, cancelHoldMs_;
  bool lastRaw_ = false;
  bool stable_ = false;
  uint32_t lastChangeMs_ = 0;
  uint32_t pressStartMs_ = 0;
  bool pressStartedInEmergency_ = false;
  bool eventFired_ = false;
};
