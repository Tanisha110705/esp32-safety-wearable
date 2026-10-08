# Decision Logic

> **Provenance:** That the prototype used **sensor-based, rule-based decision
> logic** together with a panic-button override is **confirmed**. The exact
> rules and thresholds of the original are not documented. This page
> describes the **reference implementation**
> ([`include/decision.h`](../include/decision.h),
> [`include/config.h`](../include/config.h)).

![Decision flow](../assets/decision-flow.svg)

## Approach

The logic is a plain **rule-based state machine** with fixed thresholds. It
uses no machine learning, trained model or probabilistic fusion. Each
sensor's reading is reduced to one boolean "condition". The engine then counts
how many conditions are true and how long they have been true.

```
             ┌────────────┐
 ECG rate ──►│ ECG cond.  │──┐
             └────────────┘  │
             ┌────────────┐  │   count n   ┌──────────────────────────┐
 GSR level ─►│ GSR cond.  │──┼────────────►│ n ≥ 3 for 10 s → EMERGENCY│
             └────────────┘  │             │ n ≥ 2         → WARNING   │
             ┌────────────┐  │             │ otherwise     → NORMAL    │
 PIR output ►│ PIR cond.  │──┘             └──────────────────────────┘
             └────────────┘
 Panic button (held 1.5 s) ───────────────────────────► EMERGENCY (overrides all)
```

## States

| State | Entered when | Leaves when | Notification |
|---|---|---|---|
| `CALIBRATING` | Power-up | 15 s elapsed (or panic) | none |
| `NORMAL` | Fewer than 2 conditions true | Conditions change, or panic | none |
| `WARNING` | Exactly 2, or 3 not yet sustained for 10 s | Conditions change, or panic | **none** (local LED only) |
| `EMERGENCY` | Panic, **or** all 3 true continuously for 10 s | Only a cancel hold (5 s) | HTTP(S) alert, retried until delivered |

## Rules in priority order

1. **Panic** → EMERGENCY (source `manual`), from any state including
   CALIBRATING.
2. **EMERGENCY is latched.** It is not cleared when the sensor conditions go
   away, so a short lull cannot silently end an alert. Only a deliberate
   cancel clears it.
3. **During calibration** sensor conditions are ignored, because there is no
   baseline yet to compare against.
4. **Automatic emergency:** all 3 conditions true **continuously** for 10 s.
   If any condition drops, even briefly, the 10 s timer restarts.
5. **Warning:** at least 2 conditions true.
6. Otherwise **NORMAL**.

### Why require agreement and persistence

Each sensor alone has common, harmless causes:

| Condition | Common harmless cause |
|---|---|
| ECG rate rise | Walking quickly, stairs, exercise |
| GSR deviation | Heat, sweating, electrode movement |
| PIR motion | Wearer's own movement, people passing by |

Requiring **all three** at once, and **sustained for 10 s**, is meant to make
an automatic alert from everyday activity less likely. The trade-off is lower
sensitivity: a real emergency that does not satisfy all three conditions will
not trigger automatically. That is why the panic button is the primary path.
None of these false-positive or false-negative rates have been measured.

## Threshold table

All values are hard-coded in [`include/config.h`](../include/config.h).
**Origin: configured reference values.** None were derived from recorded
data or experiment in this repository.

| Signal | Threshold | Meaning | Constant |
|---|---:|---|---|
| ECG | rate ≥ 1.25 × resting baseline | Beat-interval rate clearly above the wearer's calibration value | `ECG_RATE_RISE_FACTOR` |
| ECG | peak-to-peak ≥ 300 counts | Below this the signal is treated as unusable | `ECG_MIN_PEAK_TO_PEAK` |
| GSR | \|filtered − baseline\| / baseline ≥ 0.15 | Conductance-related reading moved ≥ 15 % from rest | `GSR_DEVIATION_FRACTION` |
| GSR | baseline ≥ 100 counts | Below this, no sensor contact is assumed and the condition is disabled | `GSR_MIN_VALID_BASELINE` |
| PIR | HIGH within last 5000 ms | Recent motion near the wearer | `PIR_HOLD_MS` |
| Rule | ≥ 2 conditions | WARNING | `WARNING_MIN_CONDITIONS` |
| Rule | 3 conditions for ≥ 10 000 ms | Automatic EMERGENCY | `AUTO_MIN_CONDITIONS`, `AUTO_CONFIRM_MS` |
| Button | stable for 50 ms | Debounce | `BUTTON_DEBOUNCE_MS` |
| Button | held ≥ 1500 ms | Manual EMERGENCY | `PANIC_HOLD_MS` |
| Button | held ≥ 5000 ms (press started in EMERGENCY) | Cancel | `CANCEL_HOLD_MS` |
| Calibration | 15 000 ms after boot | Baseline capture window | `CALIBRATION_MS` |

## Verification

`tests/test_logic.cpp` checks the rules on a PC with synthetic inputs:

- conditions are ignored while calibrating, and panic works while calibrating
- 0–1 conditions give NORMAL, and 2 give WARNING
- 2 conditions held indefinitely never escalate
- 3 conditions give EMERGENCY at exactly 10 s and not before
- an interruption restarts the 10 s timer
- EMERGENCY stays latched when conditions clear, and only Cancel clears it

These tests check that the code matches this specification. They do not show
that the specification detects real emergencies.
