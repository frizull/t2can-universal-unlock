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
- Exit comparison: the user observed an automatic indicator and crossing into
  a newly forming exit lane. A subsequent replay requiring a crossing into an
  established adjacent lane still displayed turn-signal confirmation and stayed
  in its lane. The latter capture spans 86.3 s / 56 USB snapshots, with 77
  accepted Confirm-Free enqueues, three rejected attempts, no software RX drops,
  Chassis hardware TX failures or recoveries, and 106 hardware RX-overrun events.
  Auto Blinker remained OFF with zero sends. Observed NOA stock `0x3F8` and
  outgoing R79 mux-1 payloads matched the failed passing capture. The successful
  exit does not establish that Confirm-Free caused that behavior.
- Added read-only USB `/api/das/bus-rx`: latest eight-byte `0x24A` planner and
  `0x3E9` indicator-reason frames, separated by physical A/B/C connector, with RX
  count, board timestamp and age. These are snapshots, not a lossless event log;
  zero RX and empty raw payload mean unseen. Host tests cover bus isolation,
  malformed/short frames, unseen data, clock wrap and GET-only access. No CAN
  transmission policy changes accompany this diagnostic.
- The diagnostic image passed the ESP32-S3 build and app-only flash validation.
  Fresh USB boot checks preserved the Highland/stalkless three-bus profile,
  Confirm-Free AP-active timing and Auto Blinker OFF. In 34.4 s, 152 snapshots
  completed without USB errors, software RX drops, hardware TX failures or CAN
  recovery. Hardware RX-overrun counters increased by 26 on Chassis and 34 on
  the CAN-A group. `0x24A` was observed on B/Chassis and `0x3E9` separately on
  A/Body and C/Party; the bench was AP-inactive during this runtime check.
- With the expanded diagnostics, the user again confirmed an automatic signalled
  crossing into a newly forming exit lane. The 78.7 s / 333-snapshot capture had
  zero USB errors or software RX drops. Chassis had no hardware TX failures or
  recovery; the CAN-A group recorded 29 TX failures, with zero consecutive
  failures at the final status sample and no bus-off. Auto Blinker sent zero
  frames. The public DBC's `0x3E9` reason bits 18-21 stayed zero in all 146 unique
  observed RX snapshots; bits 33-36 instead varied with indicator actions
  (0/3/4/5/8). That alternate field is an unvalidated candidate, not an established
  signal mapping or a Confirm-Free fix. Raw frames and bus identities are saved
  for comparison with a blocked request; firmware transmission policy is unchanged.
- The matching blocked replay again required turn-signal confirmation and did
  not maneuver. Across 146.1 s / 589 snapshots there were no observed `0x3E9`
  left/right start requests on A or C, despite B reporting NAV and SPEED ULC
  activity. The legacy reason field stayed zero; candidate bits 33-36 were
  0/4/15 alongside NONE/CANCEL/DEFER requests. This supports investigating the
  confirmation input, not treating a reported ULC-progress bit as execution.
  Confirm-Free enqueued 142 messages and rejected two; Auto Blinker sent zero.
  USB errors, software RX drops, Chassis hardware TX failures and recoveries
  remained zero. The CAN-A group recorded 226 TX failures, zero consecutive
  failures at the final sample and no bus-off; this is separate from the
  Chassis confirmation path. The user confirms only TMR USB/CAN access is
  available, without an Autopilot diagnostic/Ethernet configuration readout.
- Native setting reference: the user changed Speed Based Lane Changes from
  Mad Max to Disabled and back. Stock `0x3F8` on B changed byte 6 from `9F` to
  `93` and back to `9F`, matching bits 50-51, raw 3/0/3. The original native
  setting was restored and observed in CAN. This identifies the emitted field;
  it does not establish acceptance of an injected copy.
- The temporary speed-setting control completed with 111 USB snapshots over
  165.6 s and no USB errors. Stock stayed Mad Max throughout; injected copies
  carried Disabled for 120 s, then returned to Mad Max. The user observed no
  passing suggestion during the override and a turn-signal-confirmation prompt
  after expiry. The first sampled SPEED request occurred at 123.7 s. This
  supports acceptance of the injected speed setting; it does not prove that the
  separate confirmation flag is honored. AP remained NOA, Auto Blinker OFF.
  Confirm-Free counters increased by 159 accepted and seven rejected enqueues;
  Chassis had no software RX drops, hardware TX failures or recovery, with 155
  hardware RX overruns. The CAN-A group recorded 128 hardware TX failures.
  The speed probe was explicitly disarmed and its firmware code removed.
- Temporary USB matrix (subsequently removed) `/api/ulc/confirm-probe`: mode 1 clears public-DBC
  `UI_driverMonitorConfirmation` (0x3FD mux1 bit17) alongside normal Confirm-Free;
  mode 2 clears that flag while leaving the stock 0x3F8 stalk-confirmation flag.
  Both modes expire after 120 s, start OFF, write no NVS, and use only existing
  transmission paths. The field's effect on 2026.32.7 is unverified. Camera
  enable, remaining payload bits, native Mad Max and Auto Blinker are unchanged.
  Mode 0 stopped the experiment. All probe code was removed after the batch.
- Confirmation matrix results so far: with NOA active, the user observed a
  turn-signal-confirmation prompt both with bit17 cleared alongside Confirm-Free
  and with bit17 cleared while stock stalk confirmation passed through. USB
  verified the outgoing mux1 payload and, for the latter test, absence of any
  0x3F8 override. Neither removed confirmation. The initial setup capture
  included a hard CAN recovery; its resetting counters are not event deltas.
- The first pre-engagement run was inconclusive: flags were observed clear in
  Park, but only AP states 1/3 occurred during the armed capture and native
  route-following remained off (0x3F8 bit49). The user subsequently enabled NOA.
  Its later return must not be attributed to probe removal. The existing R79
  manual-driving gate also suspends 0x3FD overrides during part of the transition;
  this run does not establish continuous pre-engagement delivery. Probe and
  timing were restored to OFF and AP_ACTIVE_ONLY at the end.

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

- The repeated pre-engagement run reached NOA and still required the turn
  signal. Bit17 was not continuously overlaid during manual-drive suppression.
  Temporary mode 3 added a received-stock mux1 echo changing only bit17 during
  that interval; administrative holds and offline states still block it. This
  enables a continuous engagement comparison without applying R79 overlays in
  the manual-drive interval. Mode 3 has the same RAM-only expiry and remains OFF
  at boot. The continuous comparison reached NOA and still asked for the turn
  signal without maneuvering (135 snapshots, 220.95 s, zero USB errors). Mode 0
  and AP_ACTIVE_ONLY timing were restored afterward.
- The final temporary comparison preserved native bit18 using the existing R79
  STOCK policy: mode 4 with normal Confirm-Free, mode 5 also clearing bit17.
  Both fast echo and periodic/retry output use that temporary policy; saved NVS
  policy remains unchanged. This tests interaction with the normal bit18=0
  override without assigning an unverified meaning to bit18. Both modes retain
  the same expiry and are OFF at boot.

- Both native-bit18 comparisons failed: the user observed “Use the turn signal
  to confirm” and no maneuver with normal Confirm-Free (31 snapshots / 47.64 s)
  and with bit17 also cleared (49 snapshots / 77.99 s). NOA remained active in
  every snapshot, with zero USB errors, Chassis hardware TX failures, software
  RX drops or hard recoveries. The outgoing mux1 payloads were respectively
  `0100060006881B80` and `0100040006881B80`; 0x3F8 remained
  `8128080059DD9FA0`. Chassis hardware RX overruns increased by 49 and 112;
  the CAN-A group recorded 90 and 28 TX failures. These captures establish
  the emitted overrides and failed observed outcome, not controller acceptance
  of the confirmation settings.
- The temporary confirmation endpoint, modes, manual-drive echo and related
  test code were removed after explicit disarm. Production firmware sources
  match the pre-probe baseline `07807b4`; saved bit18 policy, Mad Max,
  AP_ACTIVE_ONLY timing and Auto Blinker OFF are retained. Confirm-Free remains
  unresolved on this 2026.32.7 bench. The speed-setting control influenced
  suggestions, so a completely ineffective injection path does not explain
  all results; an additional controller condition or changed confirmation
  signal remains possible, without evidence yet distinguishing them.

### Region comparison bring-up (2026-10-03)

- Reread the Discord Confirm-Free thread: working reports cluster in Korea;
  EU and Japan reports still require confirmation, including similar software
  versions. These are anecdotal reports, not proof that region is causal. The
  20 ms R79 suggestion in that thread concerned Summon, not confirmed passing.
- Extended passive USB `/api/das/bus-rx` with 0x293, 0x238 and separate 0x7FF
  pages 1/3, retaining Body A, Chassis B and Party C identities. The third-bus
  branch had skipped the legacy 0x293 observer. Live capture closes that gap:
  stock enable is already ON on all three buses. Native road country=250,
  country configuration bytes=52 46 (packed FR), map region=1 (EU).
- Current public Model3CAN.dbc documents 0x238 country bits16..25, 0x7FF page1
  country bits16..31 and page3 map-region bits8..11 (EU=1, KR=7):
  https://github.com/joshwardell/model3dbc/blob/master/Model3CAN.dbc
  Road country=410 and packed KR follow the numeric-code/native-byte-order
  hypothesis; controller interpretation on this software remains to be tested.
- Temporary USB region-probe modes: 1 map only, 2 road/config country only,
  3 both. OFF at boot, no NVS writes, 180 s expiry, explicit mode0 disarm.
  Only received same-bus stock templates are copied, with existing TX recovery
  barriers/admission retained. 0x238 counter/checksum are updated; other fields
  and other 0x7FF pages remain untouched. T2CAN cannot arm these bench probes.
  Host checks cover field preservation, counter wrap/checksum, mode separation,
  invalid RX and expiry; firmware build and live replay results are recorded
  separately. An accepted transmission does not establish AP acceptance.
