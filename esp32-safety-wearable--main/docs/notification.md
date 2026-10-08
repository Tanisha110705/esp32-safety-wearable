# Emergency Notification

> **Provenance:** That the prototype notified an emergency contact is
> **confirmed**. The transport the original used (Wi-Fi/HTTP, a messaging
> API, SMS through a GSM module, Bluetooth to a phone, …) is **not documented**
> in this repository. This page describes the **reference implementation**:
> Wi-Fi + HTTP(S) POST to a user-provided endpoint (`sendAlert()`,
> `serviceNotification()`, `maintainWifi()` in
> [`src/main.cpp`](../src/main.cpp)).

![Notification flow](../assets/notification-flow.svg)

## Design

The device does not talk to the emergency contact directly. It sends a
JSON message to one configurable URL (`NOTIFY_URL`). The service behind that
URL is responsible for reaching a person, for example a small server or an
automation service that sends an SMS or a chat message. This approach:

- keeps phone numbers and messaging-service credentials **off the device**
- lets the delivery channel change without reflashing the firmware
- needs only the ESP32's built-in Wi-Fi, with no extra modem

No endpoint service is included in this repository.

## Sequence

1. **Trigger.** The decision engine enters EMERGENCY, either `manual` (panic
   button) or `automatic` (3-of-3 sustained).
2. **ESP32 processing.** On that transition the firmware sets
   `alertPending = true`, resets the attempt counter, and starts the red LED's
   fast blink.
3. **Communication.** If Wi-Fi is connected, the firmware builds the payload
   and sends an HTTP(S) POST.
4. **Delivery.** An HTTP 2xx response marks the alert as delivered and turns
   the red LED solid.
5. **Local feedback.** The LED shows whether the alert has been acknowledged
   (see [indication.md](indication.md)).
6. **Failure.** No Wi-Fi, a connection error, a timeout or a non-2xx response
   all count as "not delivered". The firmware retries every 10 s
   (`NOTIFY_RETRY_MS`) until delivery succeeds or the user cancels. The number
   of retries is not limited.

## Payload

```json
{
  "device": "wearable-01",
  "event": "EMERGENCY",
  "trigger": "manual",
  "ecg_bpm": 92,
  "gsr_deviation": 0.21,
  "pir_recent": true,
  "uptime_ms": 183400
}
```

*(Example of the format only. These numbers are illustrative, not a
recorded alert.)* `ecg_bpm` is `-1` when no valid estimate exists.

Headers: `Content-Type: application/json`, and
`Authorization: Bearer <NOTIFY_TOKEN>` if a token is configured.

The payload contains **no location**. The prototype has no GPS, so the
receiving side cannot tell where the wearer is.

## Network Communication

| Aspect | Behaviour |
|---|---|
| Wi-Fi mode | Station (`WIFI_STA`), credentials from `include/secrets.h` |
| Initialisation | `WiFi.begin()` in `setup()`; non-blocking, so sensing starts at once |
| Connection monitoring | `WiFi.status()` checked every loop iteration |
| Reconnect | `disconnect()` + `begin()` every 10 s while disconnected (`WIFI_RECONNECT_MS`) |
| Request | `HTTPClient` POST, JSON body |
| Timeout | 5 s per request (`HTTP_TIMEOUT_MS`) |
| TLS | `https://` URLs use `WiFiClientSecure`. If `NOTIFY_ROOT_CA` is defined, the server certificate is verified; otherwise the connection is encrypted but **not authenticated** (`setInsecure()`). |
| Retry | Every 10 s until HTTP 2xx or cancel |
| Blocking | The POST blocks the loop for up to 5 s. Sensor sampling and LED blinking pause during that time. This is acceptable because the device is already latched in EMERGENCY. |

No network performance (latency, delivery rate, range) has been measured.

## Configuration

Copy `include/secrets.example.h` to `include/secrets.h`, which is git-ignored,
and set:

| Setting | Placeholder |
|---|---|
| `WIFI_SSID` | `YOUR_WIFI_SSID` |
| `WIFI_PASSWORD` | `YOUR_WIFI_PASSWORD` |
| `NOTIFY_URL` | `https://YOUR_NOTIFICATION_ENDPOINT/alert` |
| `NOTIFY_TOKEN` | `YOUR_API_TOKEN` (or `""`) |
| `DEVICE_ID` | `wearable-01` |
| `NOTIFY_ROOT_CA` | optional PEM certificate |

Never commit real credentials or contact details.

## Limitations

- It needs a known Wi-Fi network in range. Outdoors, the wearer would need a
  phone hotspot, and there is no cellular fallback.
- No "cancelled" follow-up message is sent.
- No location is included.
- Without `NOTIFY_ROOT_CA`, the device cannot detect an impostor endpoint.
