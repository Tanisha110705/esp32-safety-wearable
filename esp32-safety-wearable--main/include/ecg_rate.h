// ecg_rate.h — simple beat-interval estimator for an AD8232-style analog ECG
// front end (hardware-independent).
//
// This is deliberately basic signal handling for event detection, not ECG
// analysis. It finds the dominant positive peak of each cycle with an
// adaptive threshold and averages the intervals between peaks. It does not
// classify the waveform, identify P/QRS/T morphology, or detect arrhythmias,
// and its rate has not been compared against a reference instrument.
//
// Method:
//   1. Track min and max of the signal over a fixed window (windowMs). The
//      previous window's min/max set the threshold for the current window so
//      the threshold follows slow baseline drift and changes in amplitude.
//   2. threshold = min + thresholdFraction * (max - min).
//   3. A beat is an upward crossing of the threshold, ignoring crossings
//      within refractoryMs of the previous beat (T-wave / noise rejection).
//   4. Intervals outside [minRrMs, maxRrMs] are discarded as implausible.
//   5. The rate is the mean of the last kHistory accepted intervals and is
//      reported invalid if no beat was seen for staleMs, or if the signal's
//      peak-to-peak amplitude is below minPeakToPeak (flat / disconnected).

#pragma once

#include <stdint.h>

class EcgRateEstimator {
 public:
  struct Params {
    uint32_t windowMs;
    float thresholdFraction;
    int minPeakToPeak;
    uint32_t refractoryMs;
    uint32_t minRrMs;
    uint32_t maxRrMs;
    uint32_t staleMs;
  };

  explicit EcgRateEstimator(const Params& p) : p_(p) {}

  void reset() {
    haveThreshold_ = false;
    windowStartMs_ = 0;
    windowStarted_ = false;
    above_ = false;
    haveLastBeat_ = false;
    rrCount_ = 0;
    rrNext_ = 0;
  }

  void addSample(int sample, uint32_t nowMs) {
    if (!windowStarted_) {
      windowStarted_ = true;
      windowStartMs_ = nowMs;
      winMin_ = winMax_ = sample;
    }
    if (sample < winMin_) winMin_ = sample;
    if (sample > winMax_) winMax_ = sample;

    if (nowMs - windowStartMs_ >= p_.windowMs) {
      // Close the window: its range sets the threshold for the next one.
      threshMin_ = winMin_;
      threshMax_ = winMax_;
      haveThreshold_ = true;
      windowStartMs_ = nowMs;
      winMin_ = winMax_ = sample;
    }

    if (!haveThreshold_ || !signalUsable()) {
      above_ = false;
      return;
    }

    const float threshold = threshMin_ + p_.thresholdFraction * (threshMax_ - threshMin_);
    const bool nowAbove = sample >= threshold;
    if (nowAbove && !above_) onCrossing(nowMs);
    above_ = nowAbove;
  }

  // Mean beat rate in beats per minute, or a negative value when invalid.
  float rateBpm(uint32_t nowMs) const {
    if (!signalUsable() || !haveLastBeat_ || rrCount_ == 0) return -1.0f;
    if (nowMs - lastBeatMs_ > p_.staleMs) return -1.0f;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < rrCount_; ++i) sum += rr_[i];
    return 60000.0f * rrCount_ / static_cast<float>(sum);
  }

  int peakToPeak() const { return haveThreshold_ ? threshMax_ - threshMin_ : 0; }

 private:
  static constexpr uint8_t kHistory = 4;

  bool signalUsable() const { return haveThreshold_ && peakToPeak() >= p_.minPeakToPeak; }

  void onCrossing(uint32_t nowMs) {
    if (haveLastBeat_) {
      const uint32_t rr = nowMs - lastBeatMs_;
      if (rr < p_.refractoryMs) return;  // too soon: treat as the same beat
      if (rr >= p_.minRrMs && rr <= p_.maxRrMs) {
        rr_[rrNext_] = rr;
        rrNext_ = (rrNext_ + 1) % kHistory;
        if (rrCount_ < kHistory) ++rrCount_;
      }
    }
    lastBeatMs_ = nowMs;
    haveLastBeat_ = true;
  }

  Params p_;
  bool windowStarted_ = false;
  uint32_t windowStartMs_ = 0;
  int winMin_ = 0, winMax_ = 0;
  bool haveThreshold_ = false;
  int threshMin_ = 0, threshMax_ = 0;
  bool above_ = false;
  bool haveLastBeat_ = false;
  uint32_t lastBeatMs_ = 0;
  uint32_t rr_[kHistory] = {0};
  uint8_t rrCount_ = 0;
  uint8_t rrNext_ = 0;
};
