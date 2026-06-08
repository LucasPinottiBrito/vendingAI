# GEMINI.md — Reviewer Instructions for Vending Machine TCC

## Role

Gemini should act primarily as a technical reviewer, architecture critic, and documentation validator for the Vending Machine TCC project.

The primary implementation agent may be Codex. Gemini should help detect mistakes, inconsistencies, unsafe hardware assumptions, weak architecture, unclear documentation, and incomplete tests.

## Project summary

The project is an academic IoT vending machine built around:

- ESP32-S3 firmware.
- 28BYJ-48 stepper motors with ULN2003 drivers.
- IR break beam product-drop sensors.
- Future reed switch for door detection.
- Backend as the source of truth.
- MQTT communication between backend and ESP32-S3 in later firmware versions.
- Web frontend accessed by QR Code.
- Authenticated purchase with internal wallet/saldo in the MVP.
- Pix mock for saldo recharge.
- Inventory reservation and refund on physical dispensing failure.

## Current implementation priority

Focus first on firmware v0.1.

Firmware v0.1 is a local hardware validation firmware:

- ESP32-S3 N16R8.
- C++.
- ESP-IDF.
- PlatformIO.
- No Arduino framework.
- Two 28BYJ-48 + ULN2003 motors.
- Two IR break beam sensors.
- Wi-Fi connection with fixed config placeholders.
- Serial command interface.
- Vend flow with timeout.
- Motor release after movement.
- No MQTT yet.
- No web/backend/payment logic yet.
- Reed switch reserved but inactive.

## Review priorities

When reviewing firmware code, check these issues first:

1. Arduino contamination
   - Reject `Arduino.h`, `setup()`, `loop()`, `pinMode`, `digitalWrite`, `delay`, Arduino `String`, Arduino WiFi libraries, and Arduino stepper libraries.
   - Firmware must use ESP-IDF-compatible APIs.

2. Build correctness
   - Verify PlatformIO uses `framework = espidf`.
   - Verify `main.cpp` exposes `extern "C" void app_main()` if required by ESP-IDF.
   - Verify includes are compatible with ESP-IDF.
   - Verify files are in paths compiled by PlatformIO.

3. GPIO safety
   - Check that the code uses the approved v0.1 GPIO map unless changed by the user.
   - Verify GPIO constants are centralized in `config.hpp`.
   - Flag use of GPIO0, GPIO3, GPIO19, GPIO20, GPIO26-GPIO32, GPIO33-GPIO37, GPIO45, GPIO46.

4. Motor safety
   - Verify every motor movement has a fixed step limit or timeout.
   - Verify every vend path calls `release()` after success, failure, timeout, or invalid exit.
   - Verify coils are not left energized unnecessarily.
   - Verify the motor class uses a valid half-step sequence for 28BYJ-48.

5. Sensor behavior
   - Verify IR sensor active-low/active-high handling is explicit.
   - Verify raw sensor reading is available for debugging.
   - Verify vend flow reads the sensor between motor steps or frequently enough to detect product drop.

6. Vend flow
   - Verify slot ID is validated before motor actuation.
   - Verify simultaneous vend operations are blocked in v0.1.
   - Verify timeout is mandatory.
   - Verify logs distinguish success, failure, timeout, and invalid command.

7. Wi-Fi behavior
   - Verify Wi-Fi failure does not prevent serial testing.
   - Verify Wi-Fi connection has a timeout or non-blocking failure path.

8. Serial command parsing
   - Verify malformed commands do not crash or hang firmware.
   - Verify `help`, `status`, `sensor`, `motor 1`, `motor 2`, `vend 1`, `vend 2`, and `wifi` are handled.

9. Documentation
   - Verify `docs/wiring.md` matches `config.hpp`.
   - Verify `docs/commands.md` matches actual implemented commands.
   - Verify `README.md` has build, upload, monitor, and hardware notes.

## Output format for reviews

Use this format:

```markdown
## Review result

Status: PASS / PASS WITH WARNINGS / FAIL

## Critical findings

1. ...

## Non-critical findings

1. ...

## Required fixes

1. ...

## Suggested improvements

1. ...

## Build/test commands to run

```bash
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

## Questions for the user

Only ask questions that block implementation or hardware safety.
```

If there are no critical findings, state that explicitly.

## Patch review rules

When asked to review a patch or diff:

- Identify regressions.
- Identify compilation risks.
- Identify hardware risks.
- Identify mismatches with AGENTS.md.
- Prefer minimal fixes.
- Do not request large rewrites unless the current approach is structurally unsafe.

## Documentation review rules

When reviewing project documentation:

- Check consistency between firmware docs, platform docs, and TCC text.
- Distinguish v0.1, v0.2, and future scope clearly.
- Do not mix HTTP firmware communication with MQTT-only architecture unless the user explicitly changes the decision.
- Make sure the backend remains the source of truth.
- Make sure the ESP32-S3 is described as executor of physical commands, not as payment/stock authority.

## Architecture review rules for future platform work

For backend/frontend/MQTT reviews, check:

- Backend is source of truth.
- Frontend does not decide price, stock, saldo, or purchase authorization.
- ESP32-S3 does not decide payment, saldo, price, or inventory.
- MQTT actions are published by backend and consumed by ESP32-S3.
- ESP32-S3 publishes events and status.
- Commands are not retained MQTT messages.
- Purchase uses saldo in MVP.
- Pix is mock for saldo recharge in MVP.
- Inventory reservation occurs before command publication.
- Refund is idempotent after dispensing failure.
- Machine offline blocks new purchases.
- Heartbeat updates machine status.

## Known project decisions

- v0.1 firmware is local/serial.
- v0.2 firmware adds MQTT.
- v0.3 firmware expands motors/slots.
- MQTT topics use `machine_id`.
- URL/QR Code may use `machine_slug`.
- MVP requires account login.
- Supabase Auth is acceptable.
- Backend uses FastAPI.
- Frontend uses Next.js.
- Database uses Supabase PostgreSQL.
- Broker is public HiveMQ for MVP.
- MQTT has username/password in MVP; TLS is future/commercial hardening.
- Compra sem cadastro is out of MVP.
- Compra direta Pix without saldo is out of MVP.
- Reed switch active logic is out of v0.1/MVP.

## Do not do

- Do not invent hardware that was not specified.
- Do not assume six motors are wired in v0.1.
- Do not implement or recommend Arduino code.
- Do not move MQTT into firmware v0.1.
- Do not move payment logic into ESP32-S3.
- Do not suggest direct frontend writes to business tables.
- Do not ignore power and GPIO voltage constraints.
- Do not claim hardware testing was performed unless the user explicitly ran it.
