# Firmware v0.2 Wiring

The v0.2 firmware still controls only two physical motors and two IR break beam sensors.

## Motor 1

| ULN2003 input | ESP32-S3 GPIO |
| --- | --- |
| IN1 | GPIO4 |
| IN2 | GPIO5 |
| IN3 | GPIO6 |
| IN4 | GPIO7 |

## Motor 2

| ULN2003 input | ESP32-S3 GPIO |
| --- | --- |
| IN1 | GPIO15 |
| IN2 | GPIO16 |
| IN3 | GPIO17 |
| IN4 | GPIO18 |

## Sensors

| Device | ESP32-S3 GPIO | Firmware behavior |
| --- | --- | --- |
| IR break beam sensor 1 | GPIO10 | Active-low by default |
| IR break beam sensor 2 | GPIO11 | Active-low by default |
| Reed switch | GPIO12 | Reserved, inactive in v0.2 |

## Physical Mapping

| Physical channel | `motor_id` | `sensor_column_id` |
| --- | ---: | ---: |
| A1 | 1 | 1 |
| B1 | 2 | 2 |

The firmware rejects unsupported motor/sensor pairs and does not expand to six motors in v0.2.

## GPIOs To Avoid

Do not use GPIO0, GPIO3, GPIO19, GPIO20, GPIO26-GPIO32, GPIO33-GPIO37, GPIO45, or GPIO46.

These pins are avoided to reduce risk around boot strapping, USB-JTAG, flash, PSRAM, and board-specific conflicts.

## Power Notes

- Motors must use an external 5V supply.
- Do not power motors from ESP32-S3 USB or 3.3V pins.
- ESP32-S3 GND and external motor supply GND must be common.
- ESP32-S3 GPIOs are not 5V tolerant.
- If an IR sensor outputs 5V, use a level shifter or voltage divider before connecting it to GPIO10 or GPIO11.
- ULN2003 motor power and ESP32-S3 GPIO signals must share a common ground reference.
