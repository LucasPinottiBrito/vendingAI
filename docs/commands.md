# Firmware v0.2 Commands

Firmware v0.2 keeps local serial debugging and adds MQTT command handling. Serial commands are read from the ESP-IDF console using `stdin`; press Enter after each command.

```bash
pio device monitor -b 115200
```

## Serial Commands

| Command | Alias | Behavior |
| --- | --- | --- |
| `help` | `h` | Lists available commands. |
| `status` | `c` | Prints machine state and slot states. |
| `sensor` | `s` | Prints logical and raw IR sensor states. |
| `motor 1` | `a` | Runs motor 1 for `MotorTestStepLimit` steps, then releases coils. |
| `motor 2` | `b` | Runs motor 2 for `MotorTestStepLimit` steps, then releases coils. |
| `vend 1` | `1` | Runs local vend flow for motor/sensor 1 until detection or timeout. |
| `vend 2` | `2` | Runs local vend flow for motor/sensor 2 until detection or timeout. |
| `wifi` | `w` | Prints Wi-Fi connection state and IP address. |
| `mqtt` | `m` | Prints MQTT connection state and topics. |
| `heartbeat` | none | Publishes one MQTT heartbeat if MQTT is connected. |

Invalid commands return a warning and do not actuate motors. Serial motor/vend tests do not publish backend sale events.

## Safety Behavior

- `motor 1` and `motor 2` use `MotorTestStepLimit` from `src/config.hpp`.
- `vend 1` and `vend 2` use `VendTimeoutMs` from `src/config.hpp`.
- Invalid commands and invalid slots do not move motors.
- A FreeRTOS mutex prevents serial and MQTT operations from moving two motors at the same time.
- Motor coils are released after motor tests, successful vends, and failed/timeout vends.
- Firmware v0.2 does not implement payment, database, stock, saldo, or backend business logic.

## MQTT Topics

For `MachineId = 1`, firmware subscribes to:

```text
vending/1/actions
```

Firmware publishes events to:

```text
vending/1/events
```

Firmware publishes heartbeat to:

```text
vending/1/status
```

## MQTT DISPENSE Command

```json
{
  "type": "DISPENSE",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "product_id": 10,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "quantity": 1,
  "attempts_allowed": 2,
  "timeout_ms_per_attempt": 10000,
  "issued_at": "2026-06-08T12:00:00Z"
}
```

Required validations:

- JSON must be valid.
- `type` must be `DISPENSE`.
- `machine_id` must match `src/config.hpp`.
- Machine must not already be vending.
- `motor_id` and `sensor_column_id` must be known.
- The physical pair must be supported: `(1,1)` or `(2,2)`.
- `quantity` must be `1`.
- `attempts_allowed` and `timeout_ms_per_attempt` must be greater than zero.
- Completed `command_id` values are rejected as duplicates until reboot.

## MQTT CLI Example

```bash
mosquitto_sub -h <broker-host> -t 'vending/1/#' -v
mosquitto_pub -h <broker-host> -t 'vending/1/actions' -q 1 -m '{"type":"DISPENSE","command_id":123,"sale_id":456,"machine_id":1,"product_id":10,"slot_id":3,"slot_code":"A1","motor_id":1,"sensor_column_id":1,"quantity":1,"attempts_allowed":2,"timeout_ms_per_attempt":10000,"issued_at":"2026-06-08T12:00:00Z"}'
```
