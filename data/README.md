# Data

This folder is **empty on purpose**. No sensor recordings, serial logs or
waveform captures from the original HerZion 2025 prototype are in this
repository, and none have been made with the reference firmware yet.

Nothing here should be read as measured data.

## Adding recordings

If you capture data, add it here with a short note covering:

- date, hardware and firmware commit
- sensor placement and conditions (at rest, walking, …)
- the raw file, for example serial output saved as `.log` or converted to
  `.csv`

The firmware's status line (every 500 ms, format in
[`docs/sensor-acquisition.md`](../docs/sensor-acquisition.md)) is the
simplest source. Remove anything that could identify a person before
committing.
