# Tests

## `test_logic.cpp`: host-side unit tests

Tests the hardware-independent parts of the firmware on a PC:

| Module | What is checked |
|---|---|
| `include/button.h` | Short tap ignored; panic after 1.5 s exactly once; bounce rejected; cancel only for presses started in EMERGENCY; one long hold cannot both raise and cancel |
| `include/decision.h` | Calibration gating; panic during calibration; NORMAL/WARNING thresholds; 2 conditions never escalate; 3 conditions → EMERGENCY at exactly 10 s; interruption restarts the timer; EMERGENCY latches until Cancel |
| `include/ecg_rate.h` | Synthetic 800 ms / 500 ms pulse trains → 75 / 120 per minute (±1); flat signal rejected; stale after 3 s; reset clears history |

All inputs are **synthetic**, generated in the test file. These tests show
that the code implements the documented rules. They are not evidence of
detection performance on a person.

```bash
# from the repository root
g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_logic.cpp -o tests/test_logic
./tests/test_logic
```

## Hardware tests

There are no automated hardware tests. A manual procedure is in
[`docs/testing.md`](../docs/testing.md#suggested-hardware-test-procedure).
