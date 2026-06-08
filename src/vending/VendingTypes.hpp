#pragma once

#include <cstdint>

namespace vending::domain {

enum class MachineState {
    BOOTING,
    WIFI_CONNECTING,
    MQTT_CONNECTING,
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

struct DispenseCommand {
    int commandId;
    int saleId;
    int machineId;
    int productId;
    int slotId;
    char slotCode[16];
    int motorId;
    int sensorColumnId;
    int quantity;
    int attemptsAllowed;
    uint32_t timeoutMsPerAttempt;
    char issuedAt[32];
};

const char *machineStateToString(MachineState state);
const char *slotStateToString(SlotState state);

}  // namespace vending::domain
