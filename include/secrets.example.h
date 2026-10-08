// secrets.example.h — template for network and notification settings.
//
// Copy this file to include/secrets.h and fill in your own values.
// include/secrets.h is listed in .gitignore and must never be committed.
// If secrets.h is missing, the firmware builds against these placeholders and
// notification attempts will fail (useful for compile checks only).

#pragma once

#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

// HTTP(S) endpoint that receives the JSON alert and forwards it to the
// emergency contact (for example a small server, or an automation service
// that sends an SMS / chat message). The device itself does not know the
// contact's phone number or address.
#define NOTIFY_URL     "https://YOUR_NOTIFICATION_ENDPOINT/alert"

// Optional bearer token sent as "Authorization: Bearer <token>".
// Leave empty ("") to omit the header.
#define NOTIFY_TOKEN   "YOUR_API_TOKEN"

// Identifier included in every alert so the receiver knows which wearable sent it.
#define DEVICE_ID      "wearable-01"

// Optional: PEM root certificate of NOTIFY_URL's CA. When left undefined the
// HTTPS connection is NOT certificate-verified (see docs/limitations.md).
// #define NOTIFY_ROOT_CA "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n"
