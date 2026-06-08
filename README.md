# Vending Machine TCC Firmware

Firmware for the academic project "Maquina de Venda Inteligente com Arquitetura IoT e Monitoramento Remoto em Tempo Real".

Firmware v0.2 validates the ESP32-S3 hardware path and adds MQTT command integration: Wi-Fi connection with automatic reconnect, MQTT connection with automatic reconnect, two stepper motors, two IR break beam sensors, serial debug commands, local vend flow with timeout, MQTT `DISPENSE` commands, events, and heartbeat publishing. It uses C++ with ESP-IDF through PlatformIO. Arduino APIs and Arduino libraries are not used.

## Hardware Required

- ESP32-S3 N16R8 board.
- External 5V power supply for motors.
- Two 28BYJ-48 5V stepper motors.
- Two ULN2003 driver boards.
- Two IR break beam sensors.
- USB cable for flashing and serial monitor.

## Firmware v0.2 Scope

- ESP32-S3 boot logs.
- GPIO initialization for two motors and two sensors.
- Wi-Fi connection using placeholders in `src/config.hpp`.
- Automatic Wi-Fi reconnect after disconnection.
- MQTT connection using TCP broker placeholders in `src/config.hpp`.
- Automatic MQTT reconnect and resubscribe.
- Subscribe to `vending/1/actions` by default.
- Publish heartbeat to `vending/1/status` every 30 seconds while MQTT is connected.
- Publish dispense and machine events to `vending/1/events`.
- Handle MQTT `DISPENSE` commands only.
- Validate command JSON, `type`, `machine_id`, `motor_id`, `sensor_column_id`, `quantity`, attempts, timeout, and duplicate `command_id`.
- Retry MQTT dispense attempts using `attempts_allowed`, capped by firmware config.
- Keep serial debug commands for local motor, sensor, vend, Wi-Fi, MQTT, and heartbeat checks.
- Release motor coils after motor tests, vend success, vend failure, timeout, or rejected movement.

Backend business logic, payment logic, database access, HTTP requests, display/UI, OTA, persistent configuration, six-motor expansion, active reed switch logic, and TLS MQTT are not implemented in firmware v0.2.

## Build

```bash
pio run -e esp32-s3-devkitc-1
```

## Upload

```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

## Serial Monitor

```bash
pio device monitor -b 115200
```

The ESP-IDF console is configured for the ESP32-S3 USB Serial/JTAG port. Commands are line-based; press Enter after each command.

Useful commands:

```text
help
status
sensor
motor 1
motor 2
vend 1
vend 2
wifi
mqtt
heartbeat
```

Short aliases are also supported: `h`, `c`, `s`, `a`, `b`, `1`, `2`, `w`, and `m`.

## MQTT Configuration

Before MQTT testing, replace placeholders in `src/config.hpp`:

- `WifiSsid`
- `WifiPassword`
- `MachineId` if this board is not machine `1`
- `MqttBrokerUri`, for example `mqtt://broker.hivemq.com:1883`
- `MqttUsername` and `MqttPassword` if the broker requires authentication

Firmware v0.2 uses plain TCP MQTT for the academic MVP. TLS/certificate validation is left for a later hardening step.

## MQTT Test

Subscribe to all machine topics:

```bash
mosquitto_sub -h <broker-host> -t 'vending/1/#' -v
```

Publish a command:

```bash
mosquitto_pub -h <broker-host> -t 'vending/1/actions' -q 1 -m '{"type":"DISPENSE","command_id":123,"sale_id":456,"machine_id":1,"product_id":10,"slot_id":3,"slot_code":"A1","motor_id":1,"sensor_column_id":1,"quantity":1,"attempts_allowed":2,"timeout_ms_per_attempt":10000,"issued_at":"2026-06-08T12:00:00Z"}'
```

Expected event sequence on success:

```text
DISPENSE_STARTED
SENSOR_TRIGGERED
DISPENSE_SUCCESS
```

Expected event sequence on timeout with retry:

```text
DISPENSE_STARTED
DISPENSE_RETRY
DISPENSE_FAILED
```

Publishing the same `command_id` again after completion emits `DISPENSE_FAILED` with reason `COMMAND_DUPLICATED` and does not move a motor.

## Safety Notes

- Do not power the stepper motors from the ESP32-S3 board.
- Power the 28BYJ-48 motors from an external 5V rail.
- Connect ESP32-S3 GND and motor power supply GND together.
- ESP32-S3 GPIOs are 3.3V logic only.
- Verify IR sensor output voltage before connecting it to ESP32-S3 GPIOs.
- Use a voltage divider or level shifter if a sensor output is 5V.
- Stop testing if a ULN2003 board, motor, or power wire overheats.

Hardware upload and physical validation must be performed on the actual circuit; this repository does not claim hardware testing was completed.
