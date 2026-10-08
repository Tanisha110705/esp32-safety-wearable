# Limitations and Scope

## Safety and ethical scope

This project is a **prototype embedded safety system**. It is **not** a
certified emergency-response device, and it is **not** a medical or diagnostic
device. It has no regulatory approval or safety certification of any kind.

It cannot reliably detect an assault, a medical emergency or any other
dangerous situation, and no such claim is made. The **panic button is the
explicit, manual emergency trigger**. The automatic sensor rule is an
experimental secondary path whose detection performance has not been
measured.

Nobody should depend on this prototype as their only means of getting help.

## Repository limitations

- The original HerZion 2025 firmware, schematics, photographs and test records
  are **not in this repository**. The firmware here is a reference
  re-implementation. See [overview.md](overview.md).
- The reference firmware has been logic-tested on a PC with synthetic inputs
  only. It has not been cross-compiled in the authoring environment, and it
  has not been run on hardware.

## Sensing limitations

- **Threshold logic is not medical-grade.** The ECG channel provides only a
  rough beat-interval rate, used as one input to a rule. It is not a validated
  heart-rate measurement and performs no cardiac analysis.
- **ECG quality depends on electrode contact and placement.** A single-lead
  analog front end on a wrist wearable is very sensitive to motion artefacts.
- **GSR depends on contact, skin moisture, temperature and movement.** It is
  not a measurement of stress or emotion.
- **PIR reports motion, not identity or intent.** On a moving wearer it can
  trigger from the wearer's own motion.
- **Baselines are captured once per boot.** Activity or stress during the
  15 s calibration window skews them, and they are not updated as conditions
  drift.
- **All thresholds are untuned reference values.** No data-driven calibration
  has been performed.
- **No ADC linearity calibration.** The ESP32 ADC is non-linear near both ends
  of its range.

## Decision limitations

- **False positives and false negatives are both possible**, and neither rate
  has been measured.
- Requiring 3-of-3 sustained for 10 s cuts false alerts but also means many
  real situations will not trigger automatically.

## Notification limitations

- **Network-dependent.** Without a known Wi-Fi network in range, no alert
  leaves the device. There is no cellular, Bluetooth or offline fallback.
- **No location** is sent; there is no GPS.
- **No "cancelled" message** is sent after a cancel.
- **TLS is not authenticated** unless a root certificate is configured.
- An HTTP 2xx confirms the endpoint accepted the alert, **not** that a person
  received it.
- The device and the endpoint use a simple bearer token. There is no
  per-message signature or replay protection.

## Hardware and system limitations

- **Power management is not implemented or documented:** no battery
  monitoring, sleep modes or charging circuit.
- **No watchdog recovery** is configured beyond the ESP32 defaults.
- **No enclosure or ergonomic design** is documented in this repository.
- **No on-device data logging.**

## Unquantified

The following have **not** been measured: detection accuracy,
false-positive rate, false-negative rate, ECG rate accuracy, GSR repeatability,
PIR range, end-to-end notification latency, Wi-Fi range, battery life, power
consumption.
