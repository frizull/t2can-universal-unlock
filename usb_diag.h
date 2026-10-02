#pragma once

static String usbDiagConfirmTiming(uint8_t timing) {
  portENTER_CRITICAL(&lab3f8Mux);
  const uint8_t before = ulcNoConfirmTimingMode;
  ulcNoConfirmTimingMode = timing;
  portEXIT_CRITICAL(&lab3f8Mux);
  if (!ulcCfgSave()) {
    portENTER_CRITICAL(&lab3f8Mux);
    ulcNoConfirmTimingMode = before;
    portEXIT_CRITICAL(&lab3f8Mux);
    return "{\"error\":\"NVS write failed\"}";
  }
  return ulcStatsToJson();
}

// Dashboard snapshots and the existing Confirm-Free timing selector over USB.
// Independent of synchronous HTTP; only the exact requests below are accepted.
static void usbDiagTick() {
  static const struct { const char *request; String (*json)(); } routes[] = {
    {"GET /api/profile/status", vehicleProfileStatusJson},
    {"GET /api/features/status", v3FeaturePolicyJson},
    {"GET /api/blinkA/stats", blinkAStatsToJson},
    {"GET /api/das/stats", dasTelemetryStatsToJson},
    {"GET /api/ulc/stats", ulcStatsToJson},
    {"GET /api/lab/auto-lane-change/stats", ulcStatsToJson},
    {"GET /api/system/stats", systemStatsToJson},
    {"GET /api/researchcapture/stats", researchCaptureStatsToJson},
    {"POST /api/ulc/update?timing=0", []() { return usbDiagConfirmTiming(0); }},
    {"POST /api/ulc/update?timing=1", []() { return usbDiagConfirmTiming(1); }},
  };
  static char line[64];
  static size_t used = 0, sent = 0;
  static bool overflow = false;
  static String response;
  static uint32_t responseAt = 0;
  if (!Serial) {
    used = sent = 0; overflow = false; response = "";
    return;
  }
  if (response.length()) {
    // A slow/disconnected reader must never hold up CAN or the dashboard.
    if ((uint32_t)(millis() - responseAt) >= 2000) { response = ""; return; }
    const int free = Serial.availableForWrite();
    size_t count = response.length() - sent;
    if (free <= 0) return;
    if (count > (size_t)free) count = (size_t)free;
    if (count > 256) count = 256;
    sent += Serial.write((const uint8_t *)response.c_str() + sent, count);
    if (sent == response.length()) response = "";
    return;
  }
  for (unsigned budget = 0; budget < sizeof(line) && Serial.available(); ++budget) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (c < 32 || c > 126 || used == sizeof(line) - 1) overflow = true;
      else if (!overflow) line[used++] = c;
      continue;
    }
    if (!used && !overflow) continue;
    line[used] = 0;
    response = "{\"error\":\"unsupported USB request\"}\n";
    if (!overflow) {
      for (const auto &route : routes) if (strcmp(line, route.request) == 0) {
        response = String("{\"path\":\"") + (strchr(route.request, ' ') + 1) + "\",\"data\":" + route.json() + "}\n";
        break;
      }
    }
    used = sent = 0; overflow = false; responseAt = millis();
    return;
  }
}
