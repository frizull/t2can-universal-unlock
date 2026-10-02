# Universal TMR / T-2CAN validation

Based directly on upstream `main` at
`2e7a2f04ccc186d33929f8f8f8d355565ba346c9`, with one runtime-selected build target.

## Completed on 2026-10-02

- ESP32-S3 target build: Arduino ESP32 core 3.3.11, autowp-mcp2515 1.3.1,
  16 MB flash, OPI PSRAM, hardware USB CDC, 3 MB application partitions.
- Host adapter tests with AddressSanitizer and UndefinedBehaviorSanitizer:
  all 64 stable strap combinations, unstable detection, unknown-board pin
  isolation, profile save/load, Highland turn selection, distinct same-ID
  Body/Party frames, explicit TX destinations, controller initialization
  failure, B FIFO ordering/completion, queue saturation, overflow, stuck TX,
  bus-off recovery, YL standby and native T-2CAN TWAI delegation.
- Original profile/topology capability, turn-control and Summon routing
  results compared with upstream for 32 combinations: unchanged.
- All 25 pure headers compiled independently. Source and embedded dashboard
  scripts passed JavaScript syntax checks; embedded regeneration was deterministic.
- Embedded setup UI inspected with simulated API responses: TMR activates
  all three standard buses, Highland still requires turn-control selection,
  and T-2CAN retains its original two-bus topology choices.

Run the committed host tests from the repository root:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
  -fsanitize=address,undefined tests/board_can_test.cpp -o /tmp/board_can_test
/tmp/board_can_test
```

Build the same image for either board with `python3 tools/build.py`. The helper
accepts `--config-file`, `--libraries` and `--ctags` for isolated toolchains.
Build output is ignored by Git; no release or board-specific image is created.

## Hardware verification outstanding

No board was flashed and no physical CAN/vehicle behavior was validated here.
Before deployment, verify:

1. The same image boots on both boards, detects the correct resistor signature,
   exposes USB and the correct default SSID, and preserves saved custom Wi-Fi settings.
2. A23 standard profiles simultaneously receive J2 Body, J3 Chassis and J4 Party;
   duplicate numeric IDs remain distinct and each transmitted frame uses its
   intended connector. YL uses J3 VH/J4 Party with J2 held in standby.
3. Sustained three-bus load, latency, overflow and BLE/Wi-Fi coexistence on the
   physical hardware. The host mocks do not model SPI timing or electrical behavior.
4. Missing ACK, bus-off, disconnect/reconnect and controller initialization failures;
   stale queued transmissions must not survive recovery. Body/Party share the
   upstream CAN-A recovery group; Chassis has its own queue and recovery path.

The existing LAB capture/export remains A/B-only. Party/C receive counters are
reported separately by `/api/profile/status`; third-bus capture/export is outside
this minimal hardware port.
