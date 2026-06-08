# MQTT Contract - Firmware v0.2

Firmware v0.2 uses MQTT as the only backend communication channel. The backend publishes actions and the ESP32-S3 publishes status/events.

## Topics

For `MachineId = 1`:

```text
vending/1/actions
vending/1/events
vending/1/status
```

Commands must not be retained. The firmware ignores retained action messages.

## DISPENSE Command

Publish to `vending/1/actions`:

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

Numeric IDs may be sent as JSON numbers or numeric strings. Non-numeric IDs are rejected.

## Validation

The firmware validates:

- JSON object is valid.
- `type == "DISPENSE"`.
- `machine_id` equals configured `MachineId`.
- No vend operation is already running.
- `motor_id` is `1` or `2`.
- `sensor_column_id` is `1` or `2`.
- Supported physical pair is `(1,1)` or `(2,2)`.
- `quantity == 1`.
- `attempts_allowed > 0`.
- `timeout_ms_per_attempt > 0`.
- `command_id` does not equal the last completed command ID in RAM.

`attempts_allowed` is capped by `MqttMaxAttemptsAllowed`; timeout is clamped to firmware min/max values.

## Events

Published to `vending/1/events`.

Success path:

```text
DISPENSE_STARTED
SENSOR_TRIGGERED
DISPENSE_SUCCESS
```

Failure path after retry:

```text
DISPENSE_STARTED
DISPENSE_RETRY
DISPENSE_FAILED
```

Validation failures publish `MACHINE_ERROR` or `DISPENSE_FAILED` with one of these reasons:

```text
INVALID_JSON
UNKNOWN_COMMAND_TYPE
INVALID_COMMAND
MACHINE_BUSY
UNKNOWN_MOTOR_ID
UNKNOWN_SENSOR_COLUMN_ID
UNSUPPORTED_QUANTITY
COMMAND_DUPLICATED
PRODUCT_NOT_DETECTED
INTERNAL_ERROR
```

## Heartbeat

Published to `vending/1/status` every 30 seconds while MQTT is connected:

```json
{
  "type": "HEARTBEAT",
  "machine_id": 1,
  "status": "ONLINE",
  "current_state": "READY",
  "firmware_version": "0.2.0",
  "ip_address": "192.168.0.120",
  "wifi_connected": true,
  "mqtt_connected": true,
  "uptime_ms": 60000
}
```

The firmware does not publish `OFFLINE`; the backend should infer offline state from missing heartbeats.
