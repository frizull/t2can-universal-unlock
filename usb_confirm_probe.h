#pragma once

// Temporary bench matrix: 1 = both confirmation flags cleared;
// 2 = driverMonitorConfirmation cleared, stalkConfirm left at stock.
// 3 = both, including a stock bit17-only echo across manual-drive transitions.
// RAM only, off at boot, with automatic expiry.
static constexpr uint32_t USB_CONFIRM_PROBE_MS = 120000;
static portMUX_TYPE usbConfirmProbeMux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t usbConfirmProbeMode;
static uint32_t usbConfirmProbeStarted;
struct UsbConfirmProbeState { uint8_t mode; uint32_t remainingMs; };
static UsbConfirmProbeState usbConfirmProbeState(int command = -1) {
  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&usbConfirmProbeMux);
  if (command >= 0 && command <= 3) {
    usbConfirmProbeMode = (uint8_t)command;
    usbConfirmProbeStarted = now;
  }
  const uint32_t elapsed = now - usbConfirmProbeStarted;
  if (elapsed >= USB_CONFIRM_PROBE_MS) usbConfirmProbeMode = 0;
  const UsbConfirmProbeState result = {
      usbConfirmProbeMode, usbConfirmProbeMode ? USB_CONFIRM_PROBE_MS - elapsed : 0};
  portEXIT_CRITICAL(&usbConfirmProbeMux);
  return result;
}
static void usbConfirmProbeApply3fd(uint8_t *data, uint8_t dlc, uint8_t mode) {
  if (data && dlc == 8 && (data[0] & 7) == 1 && mode >= 1 && mode <= 3)
    data[2] &= (uint8_t)~0x02u; // Public DBC: UI_driverMonitorConfirmation, bit17.
}
