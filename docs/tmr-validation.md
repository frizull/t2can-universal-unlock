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
- USB diagnostics host tests with sanitizers: fragmented/overlong/invalid
  requests, GET/POST route isolation, timing persistence/rollback, partial writes, backpressure, timeout,
  disconnect and queued requests. The Python client also passed a pseudo-terminal
  round trip with fragmented JSON and unrelated boot log lines.
- A23 `94:A9:90:31:F2:70`: app-only USB flash verified at the freshly read active
  slot `0x10000`; bootloader, partition table and NVS were not written. Live USB
  status confirmed the saved Highland/stalkless profile, three-bus topology,
  Body/Chassis traffic and Party RX, plus the saved Auto Lane Change/Confirm-Free
  settings. This proves boot and telemetry, not lane-change behavior.
- USB status runs independently of synchronous HTTP clients. The initial
  web-task implementation was moved after a live capture encountered a timeout;
  status reads then recovered without a board reset.
- A23 Chassis receive interrupts and queued transmissions now wake the SPI
  task immediately. On the dismantled simulation bench, receive-overrun events
  fell from 347.69/s over 19.9 s before deployment to 1.35/s over 24.5 s after
  deployment. Both captures had zero USB failures, software RX drops, Chassis
  TX failures or CAN recoveries; Body/Chassis traffic and Party RX remained live.
  This measures transport behavior only: a lane-change suggestion or reported
  progress state has not established an actual simulated lane change.
- The subsequent 109.2 s highway replay retained low hardware overruns
  (1.51/s), but exposed 2,307 software RX drops during AP activity. The user
  confirmed that the observed lane change required manual confirmation.
  A23 now gives the Chassis decoder the same priority as Body/Party, including
  after task recovery; the original T-2CAN priority remains unchanged.
- With equal decoder priority, a 139.2 s pre-AP/NOA replay produced zero
  software RX drops, USB failures, Chassis hardware TX failures or recoveries;
  hardware overrun events averaged 0.82/s. Confirm-Free reported 140 accepted
  sends and five rejected attempts. The capture included disengagement and NOA
  re-engagement with pre-AP timing selected, but the user observed two lane-change
  suggestions without a maneuver. Both timing variants have therefore failed
  the requested behavioral test; AP-active-only timing was restored over USB.
- The USB host helper holds an exclusive port lock before changing serial
  settings or reading replies. A pseudo-terminal test confirmed a competing
  helper is rejected before I/O and normal reads resume after lock release.
- Added read-only USB R79 status, CAN-B send traces and physical A/B/C `0x3F8`
  observations. Host tests cover trace-ring wrapping, complete payloads,
  per-bus isolation, rejected malformed input and age arithmetic across clock
  wrap. Send traces report enqueue results, not receiver acceptance.
- On the user-reported Tesla 2026.32.7 Highland bench, a 128.6 s capture
  contained 124 distinct Confirm-Free send attempts: 121 enqueued and three
  timed out. Every attempted payload cleared bit 1. There were no software RX
  drops, Chassis hardware TX failures or recoveries. The user still observed
  "Use the turn signal to confirm" and no unconfirmed maneuver. A subsequent
  capture also failed behaviorally and had one USB timeout, then recovered.
- A temporary RAM-only 150 ms delayed-copy experiment also failed the observed
  behavior. Its 149.5 s capture contained 223 accepted and nine rejected sends,
  including 109 accepted identical pairs 145–160 ms apart. No software RX
  drops, Chassis hardware TX failures or recoveries occurred. The experiment
  was disabled over USB and removed; the production stock-follow policy is
  unchanged. Successful sends and reported ALC states do not establish success.
- Physical-connector observations across two further NOA captures found stock
  `0x3F8` only on B/Chassis, always with confirmation bit 1 set; A/Body and
  C/Party had no received `0x3F8`, despite live traffic. This provides no
  same-bus template or evidence for moving the override to Party. One reported
  progress state still lacks confirmation that it occurred without manual input.
- The user reports that the native "Require Lane Change Confirmation" setting
  is absent. The public [signal map](https://github.com/joshwardell/model3dbc/blob/master/Model3CAN.dbc)
  identifies `UI_ulcStalkConfirm` at `0x3F8` bit 1, matching this firmware.
  [Tesla's manual](https://www.tesla.com/ownersmanual/model3/en_us/GUID-20F2262F-CDF6-408E-A752-2AD9B0CC2FD6.html)
  describes the setting as "if equipped"; neither source establishes how the
  bench's 2026.32.7 controller accepts it. A native ON/OFF reference capture
  from a comparable configuration, or receiver-side effective-setting evidence,
  is still needed to distinguish ignored input from an additional policy gate.
- A second trace timeout prompted a host reproduction: a large reply making
  steady progress could exceed the fixed two-second response deadline. USB now
  expires after two seconds without write progress. The regression, stopped
  reader, disconnect and backpressure cases pass with sanitizers; output remains
  bounded per loop and never waits for the host.
- Final app-only deployment and USB recheck passed. A 58.6 s observation span
  (41 samples in the one-minute run) had zero USB failures, software RX drops,
  Chassis hardware TX failures or recoveries, and 60 hardware RX-overrun events.
  Confirm-Free enqueued 46 messages and rejected three attempts. Saved profile,
  AP-only timing and Auto Blinker OFF were preserved; all three buses remained
  live. This still does not establish confirmation-free lane-change behavior.

Run the committed host tests from the repository root:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter \
  -fsanitize=address,undefined tests/board_can_test.cpp -o /tmp/board_can_test
/tmp/board_can_test
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  tests/usb_diag_test.cpp -o /tmp/usb_diag_test
/tmp/usb_diag_test
```

Build the same image for either board with `python3 tools/build.py`. The helper
accepts `--config-file`, `--libraries` and `--ctags` for isolated toolchains.
Build output is ignored by Git; no release or board-specific image is created.

## Hardware verification outstanding

The A23 boot and telemetry checks above do not establish vehicle behavior or
physical T-2CAN compatibility. Remaining bench verification:

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
reported separately by `/api/profile/status`; `/api/ulc/bus-rx` provides only a
passive `0x3F8` snapshot for each connector, not a third-bus capture/export.
