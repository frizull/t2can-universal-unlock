#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
using String = std::string;
static uint32_t now;
static uint32_t millis() { return now; }
static unsigned snapshots;
#define STATUS(name) static String name() { ++snapshots; return "{\"source\":\"" #name "\"}"; }
STATUS(vehicleProfileStatusJson) STATUS(v3FeaturePolicyJson) STATUS(blinkAStatsToJson)
STATUS(dasTelemetryStatsToJson) STATUS(ulcStatsToJson) STATUS(systemStatsToJson)
STATUS(researchCaptureStatsToJson)
static uint8_t ulcNoConfirmTimingMode;
static bool saveOk = true;
static bool ulcCfgSave() { return saveOk; }
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
static struct {
  bool connected = true;
  int capacity = 512;
  size_t shortWrite = 512;
  String input, output;
  operator bool() const { return connected; }
  int available() { return input.size(); }
  int read() { char c = input.front(); input.erase(0, 1); return c; }
  int availableForWrite() { return capacity; }
  size_t write(const uint8_t *p, size_t count) {
    assert(count <= (size_t)capacity && count <= 256);
    count = std::min(count, shortWrite);
    output.append((const char *)p, count); return count;
  }
} Serial;
#include "../usb_diag.h"
static void ticks(unsigned n = 100) { while (n--) usbDiagTick(); }
static String request(const String &s) {
  Serial.output.clear(); Serial.input = s; ticks(); return Serial.output;
}
int main() {
  ticks(); assert(snapshots == 0 && Serial.output.empty());
  assert(request("GET /api/profile/status\r\n").find("vehicleProfileStatusJson") != String::npos);
  assert(request("GET /api/lab/auto-lane-change/stats\n").find("ulcStatsToJson") != String::npos);
  assert(request("GET /api/blinkA/").empty());
  assert(request("stats\n").find("blinkAStatsToJson") != String::npos);
  const unsigned before = snapshots;
  assert(request("POST /api/blinkA/enable\n").find("error") != String::npos);
  assert(request(String(80, 'x') + "GET /api/blinkA/stats\n").find("error") != String::npos);
  assert(request(String("GET /api/profile/status\0bad\n", 28)).find("error") != String::npos);
  assert(snapshots == before);
  assert(request("GET /api/ulc/update?timing=1\n").find("error") != String::npos);
  assert(ulcNoConfirmTimingMode == 0);
  assert(request("POST /api/ulc/update?timing=1\n").find("ulcStatsToJson") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  saveOk = false;
  assert(request("POST /api/ulc/update?timing=0\n").find("NVS write failed") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  saveOk = true;
  assert(request("POST /api/ulc/update?timing=2\n").find("error") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  request("POST /api/ulc/update?timing=0\n"); assert(ulcNoConfirmTimingMode == 0);
  assert(request("GET /api/das/stats\n").find("dasTelemetryStatsToJson") != String::npos);
  Serial.capacity = 0;
  assert(request("GET /api/system/stats\n").empty());
  const unsigned paused = snapshots;
  ticks(); assert(snapshots == paused && Serial.output.empty());
  Serial.capacity = 512; Serial.shortWrite = 3; ticks();
  assert(Serial.output == "{\"path\":\"/api/system/stats\",\"data\":{\"source\":\"systemStatsToJson\"}}\n");
  Serial.capacity = 0; request("GET /api/system/stats\n"); now += 2001; ticks();
  Serial.capacity = 512;
  assert(request("GET /api/features/status\n").find("v3FeaturePolicyJson") != String::npos);
  request("GET /api/profile/"); Serial.connected = false; ticks(); Serial.connected = true;
  assert(request("status\n").find("error") != String::npos);
  Serial.shortWrite = 512;
  const String replies = request("GET /api/das/stats\nGET /api/blinkA/stats\n");
  assert(std::count(replies.begin(), replies.end(), '\n') == 2);
  std::cout << "USB diagnostics: framing, routes, timing persistence/rollback, partial writes, backpressure, timeout and reconnect PASS\n";
}
