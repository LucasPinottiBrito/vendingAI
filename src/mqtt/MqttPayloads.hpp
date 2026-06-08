#pragma once

#include <cstddef>
#include <cstdint>

#include "vending/VendingTypes.hpp"

namespace vending::mqtt {

struct ParseResult {
    bool ok;
    char reason[32];
    char details[96];
    domain::DispenseCommand command;
};

ParseResult parseDispenseCommandPayload(const char *payload);

bool buildHeartbeatPayload(char *buffer, size_t bufferSize, domain::MachineState state,
                           const char *ipAddress, bool wifiConnected, bool mqttConnected,
                           uint64_t uptimeMs);

bool buildDispenseEventPayload(char *buffer, size_t bufferSize, const char *eventType,
                               const domain::DispenseCommand &command, int attempt,
                               int attemptsExecuted, const char *reason, uint64_t uptimeMs);

bool buildMachineErrorPayload(char *buffer, size_t bufferSize, const char *reason,
                              const char *details, domain::MachineState state,
                              uint64_t uptimeMs);

bool buildMachineLogPayload(char *buffer, size_t bufferSize, const char *level,
                            const char *message, domain::MachineState state,
                            uint64_t uptimeMs);

const char *heartbeatStatusForState(domain::MachineState state);

}  // namespace vending::mqtt
