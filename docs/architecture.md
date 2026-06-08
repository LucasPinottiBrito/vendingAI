# Firmware v0.2 Architecture

Firmware v0.2 is an ESP32-S3 firmware for local hardware validation plus MQTT dispense integration. It uses ESP-IDF APIs through PlatformIO and does not use Arduino.

## Main Modules

- `src/main.cpp`: ESP-IDF `app_main()` entrypoint. Configures stdio, starts Wi-Fi, initializes motors/sensors, starts MQTT, and enters the serial command loop.
- `src/config.hpp`: firmware version, machine ID, Wi-Fi placeholders, MQTT placeholders, timing constants, GPIO pin map, and sensor behavior.
- `src/wifi/WifiManager.*`: initializes ESP-IDF Wi-Fi, attempts station connection, tracks state/IP, and periodically reconnects after disconnection.
- `src/mqtt/MqttManager.*`: starts ESP-MQTT, subscribes to actions, queues incoming payloads, executes validated `DISPENSE` commands, publishes events, and sends heartbeat.
- `src/mqtt/MqttPayloads.*`: parses MQTT JSON commands with cJSON and builds JSON payloads for events and heartbeat.
- `src/mqtt/MqttTopics.hpp`: formats MQTT topics from `MachineId`.
- `src/vending/VendingTypes.hpp`: shared machine/slot states and `DispenseCommand`.
- `src/motor/StepperMotor.*`: controls one 28BYJ-48 motor through a ULN2003 driver using ESP-IDF GPIO APIs and a half-step sequence.
- `src/sensor/IrBreakBeamSensor.*`: reads one IR break beam sensor and maps raw GPIO level to logical beam-broken state.
- `src/vending/VendingSlot.*`: owns per-slot vend/test flows using one motor and one sensor.
- `src/vending/VendingController.*`: creates two slots, tracks physical machine state, validates slots, blocks simultaneous motor operations with a FreeRTOS mutex, and exposes APIs for serial and MQTT.
- `src/serial/CommandHandler.*`: parses line-based serial commands and dispatches to vending, Wi-Fi, and MQTT services.

## State Flow

Machine states are:

- `BOOTING`
- `WIFI_CONNECTING`
- `MQTT_CONNECTING`
- `READY`
- `VENDING`
- `ERROR`

`VendingController` owns physical states. `MqttManager` computes the heartbeat state from physical state plus Wi-Fi/MQTT connectivity.

Boot flow:

1. Configure stdio for the ESP-IDF console.
2. Initialize Wi-Fi and attempt initial connection with timeout.
3. Continue local serial testing even if Wi-Fi fails.
4. Initialize motors and sensors through `VendingController`.
5. Start MQTT even if Wi-Fi is not connected; ESP-MQTT auto reconnect handles later network availability.
6. Enter the serial command loop.

## Vend Flow

For serial `vend 1` / `vend 2`, the controller validates the slot before motor actuation. If another operation is active, the motor is not moved.

For a valid vend:

1. Machine state changes to `VENDING`.
2. Slot state changes to `RUNNING`.
3. The motor steps forward.
4. The matching IR sensor is read between motor steps.
5. If the sensor detects a product, the motor is released and slot state becomes `SUCCESS`.
6. If timeout expires, the motor is released and slot state becomes `FAILED`.
7. Machine state returns to `READY`.

Motor test commands run a fixed number of steps and release coils afterward.

## MQTT Flow

For `MachineId = 1`, MQTT uses:

- `vending/1/actions`
- `vending/1/events`
- `vending/1/status`

The MQTT event callback does not move motors. It validates topic/retained/fragmentation status, copies action payloads into a FreeRTOS queue, and a command task processes them.

Only `DISPENSE` is handled. The firmware validates JSON, command type, machine ID, duplicate command ID, machine busy state, motor ID, sensor column ID, supported physical pair, quantity, attempts, and timeout. The firmware uses `issued_at` for command metadata only; v0.2 does not expire commands because the requested payload does not include `expires_at`.

Published events include:

- `DISPENSE_STARTED`
- `SENSOR_TRIGGERED`
- `DISPENSE_RETRY`
- `DISPENSE_SUCCESS`
- `DISPENSE_FAILED`
- `MOTOR_ERROR`
- `MACHINE_ERROR`
- `MACHINE_LOG`

Heartbeat is published every 30 seconds while MQTT is connected.

## Future v0.3 Note

Future work may add TLS MQTT, command authentication, persistent idempotency, six-motor expansion, OTA, and richer diagnostics. Payment, database, stock, saldo, and backend business rules remain outside the firmware.
