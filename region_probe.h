#pragma once
#include "region_probe_pure.h"

static portMUX_TYPE regionProbeMux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t regionProbeMode = 0;
static uint32_t regionProbeStart = 0;
static bool regionProbePersistent = false;
static struct RegionProbeTx { uint32_t ok, fail; uint16_t id; uint8_t raw[8]; } regionProbeTx[3] = {};

static void regionProbeLoad() {
  if (board != BOARD_TMR || !boardTripleCan()) return;
  Preferences p;
  if (p.begin("regionLab", true)) {
    regionProbePersistent = p.getBool("countryKR", false);
    p.end();
  }
}

static bool regionProbeSavePersistent(bool enabled) {
  Preferences p;
  if (!p.begin("regionLab", false)) return false;
  const bool ok = p.putBool("countryKR", enabled) > 0 && p.getBool("countryKR", !enabled) == enabled;
  p.end();
  if (ok) { portENTER_CRITICAL(&regionProbeMux); regionProbePersistent = enabled; portEXIT_CRITICAL(&regionProbeMux); }
  return ok;
}

static uint8_t regionProbeActiveMode() {
  const uint32_t now = millis();
  portENTER_CRITICAL(&lab3f8Mux);
  const bool confirm = ulcNoConfirmEnabled && activeProfileUlcNoConfirmSupported();
  portEXIT_CRITICAL(&lab3f8Mux);
  portENTER_CRITICAL(&regionProbeMux);
  if (!regionProbeLivePure(regionProbeMode, regionProbeStart, now)) regionProbeMode = 0;
  const uint8_t mode = regionProbeSelectPure(regionProbeMode, regionProbePersistent, confirm);
  portEXIT_CRITICAL(&regionProbeMux);
  return mode;
}

static String regionProbeStats() {
  const uint8_t mode = regionProbeActiveMode();
  RegionProbeTx tx[3]; uint32_t start; bool persistent;
  portENTER_CRITICAL(&regionProbeMux);
  memcpy(tx, regionProbeTx, sizeof(tx)); start = regionProbeStart; persistent = regionProbePersistent;
  portEXIT_CRITICAL(&regionProbeMux);
  String s = "{\"mode\":" + String(mode) + ",\"remainingMs\":" +
      String(mode && !persistent ? 180000 - (uint32_t)(millis() - start) : 0) +
      ",\"persistentCountry\":" + (persistent ? "true" : "false") + ",\"buses\":[";
  for (uint8_t b = 0; b < 3; ++b) {
    char raw[17] = {}, line[192];
    if (tx[b].ok || tx[b].fail) for (uint8_t j = 0; j < 8; ++j) snprintf(raw + j * 2, 3, "%02X", tx[b].raw[j]);
    snprintf(line, sizeof(line), "%s{\"bus\":\"%c\",\"ok\":%lu,\"fail\":%lu,\"id\":%u,\"raw\":\"%s\"}",
      b ? "," : "", 'A' + b, (unsigned long)tx[b].ok, (unsigned long)tx[b].fail, tx[b].id, raw);
    s += line;
  }
  return s + "]}";
}

static String regionProbeSet(uint8_t mode) {
  if (board != BOARD_TMR || !boardTripleCan() || mode > 3) return "{\"error\":\"TMR three-bus bench only\"}";
  portENTER_CRITICAL(&regionProbeMux);
  const bool persistent = regionProbePersistent;
  portEXIT_CRITICAL(&regionProbeMux);
  if (persistent && !regionProbeSavePersistent(false)) return "{\"error\":\"NVS write failed\"}";
  portENTER_CRITICAL(&regionProbeMux);
  regionProbeMode = mode; regionProbeStart = millis();
  if (mode) memset(regionProbeTx, 0, sizeof(regionProbeTx));
  portEXIT_CRITICAL(&regionProbeMux);
  return regionProbeStats();
}

static String regionProbePersistCountry() {
  if (board != BOARD_TMR || !boardTripleCan() || !activeProfileUlcNoConfirmSupported())
    return "{\"error\":\"TMR Confirm-Free bench only\"}";
  if (!regionProbeSavePersistent(true)) return "{\"error\":\"NVS write failed\"}";
  portENTER_CRITICAL(&regionProbeMux);
  regionProbeMode = 0; memset(regionProbeTx, 0, sizeof(regionProbeTx));
  portEXIT_CRITICAL(&regionProbeMux);
  return regionProbeStats();
}

static void regionProbeObserve(uint8_t bus, uint32_t id, uint8_t dlc, const uint8_t *data) {
  if (bus >= 3 || (id != 0x238 && id != 0x7FF) || dlc != 8 || !data ||
      board != BOARD_TMR || !boardTripleCan()) return;
  const uint8_t mode = regionProbeActiveMode();
  if (!mode) return;
  uint8_t raw[8]; memcpy(raw, data, 8);
  if (!regionProbePatchPure(mode, id, dlc, raw)) return;
  const uint32_t epoch = canTxEpochSnapshot();
  if (regionProbeActiveMode() != mode) return;
  bool ok = false;
  if (bus == 1) {
    if (twaiNonSummonAdmissionOpen()) {
      twai_message_t out = {}; out.identifier = id; out.data_length_code = 8;
      memcpy(out.data, raw, 8);
      ok = canTxTwaiTransmit(&out, epoch) == ESP_OK;
    }
  } else {
    struct can_frame out = {}; out.can_id = id; out.can_dlc = 8;
    memcpy(out.data, raw, 8);
    MCP2515::ERROR err = MCP2515::ERROR_FAIL;
    const bool attempted = canTxMcpSend(&out, epoch, err, nullptr, bus == 2);
    ok = attempted && err == MCP2515::ERROR_OK;
    if (attempted) {
      if (ok) { mcpTxOk++; mcpTxFailConsecutive = 0; }
      else { mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
    }
  }
  portENTER_CRITICAL(&regionProbeMux);
  RegionProbeTx &tx = regionProbeTx[bus];
  if (ok) tx.ok++; else tx.fail++;
  tx.id = (uint16_t)id; memcpy(tx.raw, raw, 8);
  portEXIT_CRITICAL(&regionProbeMux);
}
