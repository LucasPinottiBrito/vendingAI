# CODEX_PROMPTS.md — Prompt Pack for Vending Machine TCC

Use these prompts sequentially with Codex. Do not ask Codex to implement everything in one pass. The goal is to keep each step buildable, testable, and easy to debug.

The current priority is firmware v0.1.

---

# Prompt 00 — Read project context before coding

You are working on an academic IoT vending machine TCC project.

Before writing code, read the repository context, especially:

- `AGENTS.md`
- `GEMINI.md`
- `docs/` if present

Current task: understand the project and report your implementation plan for firmware v0.1.

Firmware v0.1 scope:

- ESP32-S3 N16R8.
- C++.
- ESP-IDF only.
- PlatformIO with `framework = espidf`.
- No Arduino.
- Two 28BYJ-48 motors with ULN2003 drivers.
- Two IR break beam sensors.
- Wi-Fi with fixed config placeholders.
- Serial commands.
- Local vend flow with timeout.
- Logs via serial/ESP-IDF logging.
- No MQTT yet.
- No backend/web/payment/database logic yet.
- Reed switch reserved but inactive.

Respond with:

1. What you understood.
2. Files you expect to create or modify.
3. Risks you see.
4. Exact next implementation step.

Do not create code in this prompt unless explicitly asked after this planning response.

---

# Prompt 01 — Create firmware project skeleton

Create the initial project skeleton for the ESP32-S3 vending machine firmware v0.1.

Requirements:

- Use PlatformIO.
- Use ESP-IDF framework, not Arduino.
- Use C++.
- Target board: `esp32-s3-devkitc-1`.
- Create a minimal buildable project.
- Provide `platformio.ini`.
- Provide `sdkconfig.defaults` only if useful and minimal.
- Provide `src/main.cpp` with `app_main()`.
- Provide `src/config.hpp` with firmware version, Wi-Fi placeholders, motor timing constants, GPIO pin map, and sensor behavior.
- Create documentation files:
  - `README.md`
  - `docs/wiring.md`
  - `docs/commands.md`
  - `docs/architecture.md`

Initial pin map:

```cpp
// Motor 1
MOTOR1_IN1 = GPIO4
MOTOR1_IN2 = GPIO5
MOTOR1_IN3 = GPIO6
MOTOR1_IN4 = GPIO7

// Motor 2
MOTOR2_IN1 = GPIO15
MOTOR2_IN2 = GPIO16
MOTOR2_IN3 = GPIO17
MOTOR2_IN4 = GPIO18

// Sensors
IR_SENSOR_1 = GPIO10
IR_SENSOR_2 = GPIO11
REED_SWITCH_RESERVED = GPIO12
```

Avoid GPIO0, GPIO3, GPIO19, GPIO20, GPIO26-GPIO32, GPIO33-GPIO37, GPIO45, GPIO46.

Do not implement MQTT.
Do not implement Arduino code.
Do not create empty decorative files.

After creating the skeleton, run or explain how to run:

```bash
pio run
```

Respond with:

1. Files created.
2. What each file does.
3. Build instructions.
4. Any assumptions.

---

# Prompt 02 — Fix PlatformIO/ESP-IDF build errors

The firmware project failed to build. Analyze the error below and fix the smallest possible set of files.

Constraints:

- Keep PlatformIO.
- Keep `framework = espidf`.
- Keep C++.
- Do not migrate to Arduino.
- Do not remove project structure unless necessary.
- Do not add unrelated features.
- Do not implement MQTT.

Build error:

```text
[PASTE FULL BUILD LOG HERE]
```

Tasks:

1. Identify the actual root cause.
2. Patch the code/configuration.
3. Explain the fix.
4. Tell me the next command to run.

Expected command:

```bash
pio run
```

---

# Prompt 03 — Implement StepperMotor class

Implement the `StepperMotor` class for firmware v0.1.

Files:

- `src/motor/StepperMotor.hpp`
- `src/motor/StepperMotor.cpp`

Requirements:

- Use ESP-IDF GPIO APIs.
- Use `gpio_num_t` for pins.
- Do not use Arduino APIs.
- Configure four GPIOs as outputs.
- Use half-step sequence for 28BYJ-48 + ULN2003.
- Provide forward and backward step methods.
- Provide configurable step delay in milliseconds.
- Use `vTaskDelay(pdMS_TO_TICKS(...))` or a proper ESP-IDF-compatible delay mechanism.
- Provide `release()` that turns all motor pins off.
- Ensure `release()` is safe to call multiple times.
- Keep implementation simple.

Suggested interface:

```cpp
class StepperMotor {
public:
    StepperMotor(gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4);

    esp_err_t init();
    void stepForward();
    void stepBackward();
    void release();
    void setStepDelayMs(uint32_t delayMs);

private:
    gpio_num_t pins[4];
    int currentStep;
    uint32_t stepDelayMs;

    void writeStep(int stepIndex);
};
```

Half-step sequence:

```text
1 0 0 0
1 1 0 0
0 1 0 0
0 1 1 0
0 0 1 0
0 0 1 1
0 0 0 1
1 0 0 1
```

Add no MQTT.
Add no Arduino.
After implementation, ensure project still builds with:

```bash
pio run
```

---

# Prompt 04 — Implement IR break beam sensor class

Implement the `IrBreakBeamSensor` class for firmware v0.1.

Files:

- `src/sensor/IrBreakBeamSensor.hpp`
- `src/sensor/IrBreakBeamSensor.cpp`

Requirements:

- Use ESP-IDF GPIO APIs.
- Use `gpio_num_t`.
- Configure GPIO as input.
- Support active-low sensors through constructor parameter.
- Provide `readRaw()` for debug.
- Provide `isBeamBroken()` for logical detection.
- Do not overcomplicate with advanced filtering unless needed.
- If debounce is added, keep it simple and documented.
- Do not use Arduino APIs.
- Do not implement MQTT.

Suggested interface:

```cpp
class IrBreakBeamSensor {
public:
    IrBreakBeamSensor(gpio_num_t gpio, bool activeLow = true);

    esp_err_t init();
    bool isBeamBroken() const;
    bool readRaw() const;

private:
    gpio_num_t gpio;
    bool activeLow;
};
```

Update docs only if needed.
Ensure project builds after the change.

---

# Prompt 05 — Implement VendingSlot and VendingController

Implement the vending domain layer for firmware v0.1.

Files:

- `src/vending/VendingSlot.hpp`
- `src/vending/VendingSlot.cpp`
- `src/vending/VendingController.hpp`
- `src/vending/VendingController.cpp`

Requirements:

- Use the existing `StepperMotor` and `IrBreakBeamSensor` classes.
- Define and use `MachineState` and `SlotState` enums.
- Create two slots:
  - Slot 1 uses motor 1 and IR sensor 1.
  - Slot 2 uses motor 2 and IR sensor 2.
- Implement `vendSlot(int slotId)`.
- Prevent simultaneous vending in v0.1.
- Validate slot ID before motor movement.
- Vend flow must:
  1. Set machine state to `VENDING`.
  2. Set slot state to `RUNNING`.
  3. Rotate motor slowly.
  4. Read the associated IR sensor between steps.
  5. Stop and release motor if sensor detects product.
  6. Stop and release motor if timeout occurs.
  7. Return machine to `READY` after success or failure.
  8. Log each important transition.
- Timeout must use `VEND_TIMEOUT_MS` from `config.hpp`.
- Do not implement retry yet unless explicitly requested.
- Do not implement MQTT.
- Do not use Arduino.

Also implement:

- `printStatus()`.
- `printSensors()`.
- Safe motor test method for `motor 1` and `motor 2` commands if appropriate.

Ensure project builds.

---

# Prompt 06 — Implement serial CommandHandler

Implement serial command handling for firmware v0.1.

Files:

- `src/serial/CommandHandler.hpp`
- `src/serial/CommandHandler.cpp`
- Update `src/main.cpp` as needed.

Required commands:

```text
help
status
sensor
motor 1
motor 2
vend 1
vend 2
wifi
```

Requirements:

- Read commands from serial monitor/USB console in an ESP-IDF-compatible way.
- Do not use Arduino `Serial`.
- Commands should tolerate `\n`, `\r\n`, and extra spaces.
- Invalid commands must not crash or hang.
- `help` prints all available commands.
- `status` calls vending controller status.
- `sensor` calls sensor status.
- `motor N` runs a limited safe test movement.
- `vend N` executes vending flow.
- `wifi` prints current Wi-Fi status if available.
- Keep the loop simple and reliable.

If reading from stdin with ESP-IDF is more practical for the PlatformIO monitor, use that approach and document it.

Do not implement MQTT.
Do not use Arduino.
Update `docs/commands.md` to match the real commands.
Ensure project builds.

---

# Prompt 07 — Implement Wi-Fi manager

Implement Wi-Fi connection for firmware v0.1.

Files:

- `src/wifi/WifiManager.hpp`
- `src/wifi/WifiManager.cpp`
- Update `main.cpp` and `config.hpp` as needed.

Requirements:

- Use ESP-IDF Wi-Fi APIs.
- Use fixed SSID/password placeholders from `config.hpp`.
- Do not hardcode real credentials.
- Connect during boot.
- Log connection attempt.
- Log success and IP address.
- Log failure after timeout.
- Wi-Fi failure must not block local serial testing.
- Expose a method to print Wi-Fi status for the `wifi` serial command.
- Keep implementation simple.

Do not implement MQTT in this step.
Do not use Arduino WiFi libraries.
Ensure project builds.

---

# Prompt 08 — Complete firmware documentation

Update firmware documentation after the v0.1 implementation.

Files:

- `README.md`
- `docs/wiring.md`
- `docs/commands.md`
- `docs/architecture.md`

Requirements:

`README.md` must include:

- Project purpose.
- Hardware required.
- Build command.
- Upload command.
- Serial monitor command.
- Safety notes.
- Firmware v0.1 scope.
- Explicit note: no MQTT yet.

`docs/wiring.md` must include:

- Motor 1 pinout.
- Motor 2 pinout.
- IR sensor pinout.
- Reed switch reserved pin.
- GPIOs to avoid.
- Power wiring note: motors on external 5V, common GND, 3.3V GPIO safety.

`docs/commands.md` must include:

- All serial commands.
- Expected behavior.
- Example monitor session.

`docs/architecture.md` must include:

- Main modules/classes.
- Firmware state flow.
- Vend flow.
- Future v0.2 MQTT note.

Do not invent features not implemented.
Do not claim hardware testing was completed.

---

# Prompt 09 — Hardware validation checklist and debug logs

Create or update a hardware validation checklist for firmware v0.1.

Create:

- `firmware/docs/test-plan-v0.1.md`

Content required:

1. Pre-power electrical checks.
2. ESP32-S3 only boot test.
3. Wi-Fi test.
4. Sensor raw reading test.
5. Motor 1 standalone test.
6. Motor 2 standalone test.
7. Vend slot 1 success test.
8. Vend slot 2 success test.
9. Timeout/failure test.
10. Invalid command test.
11. Logs to capture for the TCC.
12. Common failures and likely causes.

Include warnings:

- Do not power motors from ESP32-S3 USB.
- Use common GND.
- Check sensor output voltage before connecting to ESP32-S3 GPIO.
- Stop testing if the ULN2003 board or motor overheats.

Do not change firmware code unless documentation reveals a mismatch.

---

# Prompt 10 — Prepare code for future MQTT without implementing MQTT

Refactor firmware v0.1 only if necessary to make future MQTT integration easier, but do not implement MQTT.

Goal:

- Keep `VendingController` usable by both serial commands now and MQTT commands later.
- Keep vend operation callable through a simple method like `vendSlot(slotId)`.
- Keep status methods reusable.
- Keep slot/motor/sensor mapping centralized.

Restrictions:

- Do not add MQTT dependencies.
- Do not add MQTT config.
- Do not add JSON parsing.
- Do not change behavior that already works.
- Do not break serial commands.
- Do not over-engineer.

After refactor:

- Run/build with `pio run`.
- Update docs only if behavior or structure changed.

---

# Prompt 11 — Implement firmware v0.2 MQTT only after v0.1 is validated

Use this prompt only after firmware v0.1 has been built, uploaded, and hardware-tested.

Implement firmware v0.2 MQTT integration.

Requirements:

- Preserve all v0.1 serial commands.
- Connect to Wi-Fi.
- Connect to HiveMQ broker.
- Use MQTT username/password placeholders.
- Subscribe to:

```text
vending/{machine_id}/actions
```

- Publish status to:

```text
vending/{machine_id}/status
```

- Publish events to:

```text
vending/{machine_id}/events
```

- Publish heartbeat every 30 seconds.
- Handle only command type `DISPENSE`.
- Ignore expired commands.
- Reject command if machine is already vending.
- Execute vend flow using existing `VendingController`.
- Publish:
  - `DISPENSE_STARTED`
  - `SENSOR_TRIGGERED` if available
  - `DISPENSE_SUCCESS`
  - `DISPENSE_RETRY` if retry is implemented
  - `DISPENSE_FAILED`
  - `MACHINE_ERROR` when appropriate

Expected DISPENSE payload:

```json
{
  "type": "DISPENSE",
  "command_id": "uuid",
  "sale_id": "uuid",
  "machine_id": "uuid",
  "product_id": "uuid",
  "slot_id": "uuid",
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "quantity": 1,
  "attempts_allowed": 2,
  "timeout_ms_per_attempt": 10000,
  "issued_at": "2026-06-03T12:00:00Z",
  "expires_at": "2026-06-03T12:01:00Z"
}
```

Do not implement payment, database, or backend logic in firmware.

---

# Prompt 12 — General bugfix prompt

Fix the bug described below with the smallest safe change.

Context:

- Project: Vending Machine TCC.
- Current priority: ESP32-S3 firmware.
- Language: C++.
- Framework: ESP-IDF.
- Environment: PlatformIO.
- No Arduino.

Bug description:

```text
[DESCRIBE THE BUG HERE]
```

Logs/output:

```text
[PASTE SERIAL LOGS OR BUILD LOGS HERE]
```

Constraints:

- Do not add unrelated features.
- Do not change pin mapping unless the bug is pin-related and you explain why.
- Do not remove safety timeouts.
- Do not bypass motor release.
- Do not introduce MQTT unless this is explicitly a v0.2 task.

Respond with:

1. Root cause.
2. Files changed.
3. Patch summary.
4. How to test.
5. Remaining risks.

---

# Prompt 13 — Ask Codex to explain code for TCC documentation

Explain the current firmware implementation for academic documentation.

Focus on:

- Why ESP32-S3 is used.
- Why ESP-IDF/PlatformIO is used.
- How motors are controlled.
- How IR sensors confirm dispensing.
- How serial commands are used for tests.
- How state machine improves safety.
- Why timeout and motor release are required.
- What is out of scope in v0.1.
- How v0.2 will add MQTT.

Do not invent hardware tests that were not performed.
Do not claim production readiness.
Write in Portuguese, in a formal academic tone.
