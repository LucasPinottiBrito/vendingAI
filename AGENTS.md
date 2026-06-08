# AGENTS.md — Vending Machine TCC

## Project identity

Project: Máquina de Venda Inteligente com Arquitetura IoT e Monitoramento Remoto em Tempo Real.

This repository is for an academic vending machine project that integrates embedded firmware, web platform, backend, database, MQTT communication, inventory control, payment flow, and remote monitoring.

The current development priority is the ESP32-S3 firmware, starting with firmware v0.1.

## Current active phase

Implement firmware v0.1 for local hardware validation.

Firmware v0.1 must validate:

- ESP32-S3 boot and GPIO initialization.
- Wi-Fi connection using fixed credentials in code or config file placeholders.
- Two 28BYJ-48 stepper motors controlled through ULN2003 drivers.
- Two IR break beam sensors.
- Serial monitor command interface.
- Local vending flow: rotate motor slowly until product drop is detected by the IR sensor or until timeout.
- Clear serial logs.
- Safe stop and motor coil release after success, failure, or timeout.

Firmware v0.1 must not implement:

- MQTT.
- Web platform integration.
- Payment logic.
- Database access.
- User authentication.
- Wallet/saldo logic.
- Full six-motor control.
- Active reed switch logic.
- OTA.
- NVS persistence.
- Display/UI logic.
- Arduino framework.

## Hardware baseline

Target board:

- ESP32-S3 N16R8.
- 16 MB Flash.
- 8 MB PSRAM.

Firmware stack:

- Language: C++.
- Framework: ESP-IDF.
- Development environment: PlatformIO using `framework = espidf`.
- Do not use Arduino APIs, Arduino libraries, `Arduino.h`, `setup()`, `loop()`, `digitalWrite`, `pinMode`, or `delay`.

Motors:

- 28BYJ-48 5V stepper motors.
- ULN2003 driver boards.
- Four GPIO signals per motor: IN1, IN2, IN3, IN4.
- v0.1 uses only two motors.
- Future versions may expand to six motors through GPIO expansion or multiplexing.

Sensors:

- Two IR break beam sensors.
- Each sensor covers a physical product drop column.
- Sensor detection confirms that an item crossed the beam, but does not identify which product fell.

Reed switch:

- Reserved for future door-open detection.
- No active logic in firmware v0.1.

Power:

- Motors must be powered by an external 5V rail, not by the ESP32-S3 board.
- ESP32-S3 and motor power supply must share common GND.
- ESP32-S3 GPIOs are 3.3V logic; do not feed 5V sensor outputs directly into GPIOs.
- If any IR sensor output is 5V, use a voltage divider or level shifter.

## Initial firmware pin map

Use this pin map unless the user explicitly changes it after hardware testing.

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

Avoid using these GPIOs in v0.1:

- GPIO0.
- GPIO3.
- GPIO19.
- GPIO20.
- GPIO26 to GPIO32.
- GPIO33 to GPIO37.
- GPIO45.
- GPIO46.

Reason: avoid boot, USB-JTAG, flash, PSRAM, strapping, and board-specific conflicts.

## Expected firmware architecture

Preferred structure:

```text
firmware/
├── platformio.ini
├── sdkconfig.defaults
├── README.md
├── src/
│   ├── main.cpp
│   ├── config.hpp
│   ├── wifi/
│   │   ├── WifiManager.hpp
│   │   └── WifiManager.cpp
│   ├── motor/
│   │   ├── StepperMotor.hpp
│   │   └── StepperMotor.cpp
│   ├── sensor/
│   │   ├── IrBreakBeamSensor.hpp
│   │   └── IrBreakBeamSensor.cpp
│   ├── vending/
│   │   ├── VendingSlot.hpp
│   │   ├── VendingSlot.cpp
│   │   ├── VendingController.hpp
│   │   └── VendingController.cpp
│   └── serial/
│       ├── CommandHandler.hpp
│       └── CommandHandler.cpp
└── docs/
    ├── wiring.md
    ├── commands.md
    └── architecture.md
```

Do not create empty files, decorative folders, unused abstractions, or placeholder modules that are not compiled or documented.

## Firmware states

Use explicit state enums.

```cpp
enum class MachineState {
    BOOTING,
    WIFI_CONNECTING,
    READY,
    VENDING,
    ERROR
};

enum class SlotState {
    IDLE,
    RUNNING,
    SUCCESS,
    FAILED,
    DISABLED
};
```

## Core classes

### StepperMotor

Responsibility: control one 28BYJ-48 motor through a ULN2003 board.

Required behavior:

- Configure four GPIOs as outputs.
- Use ESP-IDF GPIO APIs.
- Use half-step sequence by default.
- Allow forward and backward steps.
- Allow step delay configuration.
- Provide `release()` to de-energize all coils.
- Never leave coils energized after a vend attempt, test movement, timeout, or error.

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

### IrBreakBeamSensor

Responsibility: read one IR break beam sensor.

Required behavior:

- Configure GPIO as input.
- Support active-low or active-high sensors.
- Provide raw read for debugging.
- Provide logical `isBeamBroken()` method.
- Optional simple debounce/filtering is allowed if it does not overcomplicate v0.1.

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

### VendingSlot

Responsibility: represent a physical slot/channel in the vending machine.

Required behavior:

- Hold slot ID.
- Hold motor reference.
- Hold IR sensor reference.
- Execute vend flow.
- Return success/failure.
- Enforce timeout.
- Release motor at the end of every attempt.

Vend flow:

1. Verify slot is available.
2. Set slot state to `RUNNING`.
3. Log vend start.
4. Rotate motor slowly.
5. Read associated sensor between motor steps.
6. If sensor detects product: stop, release motor, set state `SUCCESS`, log success, return true.
7. If timeout occurs: stop, release motor, set state `FAILED`, log failure, return false.

### VendingController

Responsibility: coordinate slots and global machine state.

Required behavior:

- Initialize motors and sensors.
- Keep global machine state.
- Prevent simultaneous vend operations in v0.1.
- Validate slot ID before motor actuation.
- Print machine status.
- Print sensor status.
- Provide simple APIs that can later be reused by MQTT handling in firmware v0.2.

### CommandHandler

Responsibility: parse serial commands.

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

Rules:

- Do not crash on malformed commands.
- Commands should be reasonably tolerant of extra spaces and line endings.
- Invalid commands must return a clear message.
- `motor N` must run only a limited, safe number of steps.
- `vend N` must use timeout.

## PlatformIO baseline

Use:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf

monitor_speed = 115200
upload_speed = 921600

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L

build_flags =
    -D CONFIG_PROJECT_VERSION=\"0.1.0\"
    -D VENDING_FW_VERSION=\"0.1.0\"
```

If the ESP32-S3 N16R8 board requires flash/PSRAM-specific build options, identify the issue clearly and propose the smallest required change. Do not randomly add board flags without explaining why.

## Logging rules

Use ESP-IDF logging (`ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`) or a consistent serial logging approach compatible with ESP-IDF.

Expected log style:

```text
[BOOT] Firmware version: 0.1.0
[BOOT] Initializing GPIO...
[WIFI] Connecting to SSID...
[WIFI] Connected. IP: xxx.xxx.xxx.xxx
[SENSOR] IR1=OK IR2=BLOCKED
[VEND] Slot 1 requested
[VEND] Slot 1 motor started
[VEND] Slot 1 product detected
[VEND] Slot 1 success
[VEND] Slot 2 timeout
[VEND] Slot 2 failed
```

Logs should support academic testing and debugging.

## Safety rules

These are mandatory:

- Every vend operation must have a timeout.
- Every test motor operation must have a fixed step limit.
- Motor coils must be released after every movement, timeout, or error.
- Only one vend operation at a time in v0.1.
- Invalid slots must not actuate any motor.
- Wi-Fi failure must not block local serial testing.
- Commands must not hang the firmware indefinitely.
- Do not assume sensors are always connected or active.

## Development discipline

When changing code:

1. Keep changes small and focused.
2. Prefer simple, readable C++ over clever abstractions.
3. Keep hardware constants in `config.hpp`.
4. Do not hardcode GPIO numbers in implementation files.
5. Do not introduce MQTT before firmware v0.2.
6. Do not introduce web/backend/payment logic into firmware v0.1.
7. Do not create unnecessary dependencies.
8. Do not use Arduino libraries.
9. Keep code buildable after each meaningful step.
10. Update documentation when changing commands, pinout, or flow.

## Build and validation commands

Expected local commands:

```bash
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

If `pio run` fails, fix compilation before adding new functionality.

## Firmware v0.1 acceptance criteria

The implementation is acceptable when:

1. Project compiles with PlatformIO and ESP-IDF.
2. Firmware uploads to ESP32-S3 N16R8.
3. Serial monitor shows boot logs.
4. ESP32-S3 attempts Wi-Fi connection.
5. `help` lists available commands.
6. `status` shows machine state.
7. `sensor` shows both IR sensor states.
8. `motor 1` rotates motor 1 for a limited test movement.
9. `motor 2` rotates motor 2 for a limited test movement.
10. `vend 1` rotates motor 1 until IR sensor 1 detects product or timeout.
11. `vend 2` rotates motor 2 until IR sensor 2 detects product or timeout.
12. Motors stop and coils are released after success or failure.
13. Firmware does not hang if Wi-Fi fails.
14. Firmware does not hang on invalid serial input.

## Future phases

Do not implement these until explicitly requested.

Firmware v0.2:

- MQTT connection.
- Subscribe to `vending/{machine_id}/actions`.
- Publish to `vending/{machine_id}/events`.
- Publish to `vending/{machine_id}/status`.
- Heartbeat every 30 seconds.
- DISPENSE command handling.
- Retry before final failure.

Firmware v0.3:

- Expand to six motors.
- Consider GPIO expanders or multiplexers.
- Keep the high-level `VendingSlot` API stable when changing the motor driver implementation.

## Response format for coding agents

When completing a task, respond with:

1. What was changed.
2. Files modified.
3. How to build/test.
4. Any assumptions made.
5. Any hardware checks required before powering the circuit.

Do not claim the code was tested on hardware unless it actually was.
