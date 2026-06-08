#include "mqtt/MqttPayloads.hpp"

#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>

#include "cJSON.h"
#include "config.hpp"

namespace vending::mqtt {

namespace {

void copyText(char *destination, size_t destinationSize, const char *source) {
    if (destination == nullptr || destinationSize == 0) {
        return;
    }

    std::snprintf(destination, destinationSize, "%s", source != nullptr ? source : "");
}

ParseResult fail(const char *reason, const char *details) {
    ParseResult result = {};
    result.ok = false;
    copyText(result.reason, sizeof(result.reason), reason);
    copyText(result.details, sizeof(result.details), details);
    return result;
}

bool isSafeToken(const char *value) {
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    for (size_t i = 0; value[i] != '\0'; ++i) {
        const unsigned char ch = static_cast<unsigned char>(value[i]);
        if (!(std::isalnum(ch) || ch == '-' || ch == '_' || ch == ':' || ch == '.' || ch == '@')) {
            return false;
        }
    }

    return true;
}

bool parseIntegerString(const char *value, int *out) {
    if (value == nullptr || out == nullptr || value[0] == '\0') {
        return false;
    }

    int parsed = 0;
    for (size_t i = 0; value[i] != '\0'; ++i) {
        const unsigned char ch = static_cast<unsigned char>(value[i]);
        if (!std::isdigit(ch)) {
            return false;
        }

        parsed = parsed * 10 + (value[i] - '0');
    }

    *out = parsed;
    return true;
}

bool getRequiredInt(const cJSON *root, const char *key, int *out) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr || out == nullptr) {
        return false;
    }

    if (cJSON_IsNumber(item)) {
        *out = item->valueint;
        return true;
    }

    if (cJSON_IsString(item)) {
        return parseIntegerString(item->valuestring, out);
    }

    return false;
}

bool getRequiredString(const cJSON *root, const char *key, char *out, size_t outSize) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr || out == nullptr || outSize == 0 || !cJSON_IsString(item)) {
        return false;
    }

    if (std::strlen(item->valuestring) >= outSize || !isSafeToken(item->valuestring)) {
        return false;
    }

    copyText(out, outSize, item->valuestring);
    return true;
}

bool getIssuedAt(const cJSON *root, char *out, size_t outSize) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "issued_at");
    if (item == nullptr || out == nullptr || outSize == 0 || !cJSON_IsString(item)) {
        return false;
    }

    const char *value = item->valuestring;
    if (value == nullptr || std::strlen(value) >= outSize || std::strchr(value, '"') != nullptr) {
        return false;
    }

    copyText(out, outSize, value);
    return true;
}

bool writePayload(char *buffer, size_t bufferSize, int written) {
    return buffer != nullptr && bufferSize > 0 && written > 0 &&
           written < static_cast<int>(bufferSize);
}

}  // namespace

ParseResult parseDispenseCommandPayload(const char *payload) {
    if (payload == nullptr || payload[0] == '\0') {
        return fail("INVALID_JSON", "empty_payload");
    }

    cJSON *root = cJSON_Parse(payload);
    if (root == nullptr || !cJSON_IsObject(root)) {
        if (root != nullptr) {
            cJSON_Delete(root);
        }
        return fail("INVALID_JSON", "payload_is_not_valid_json_object");
    }

    char type[16] = {};
    if (!getRequiredString(root, "type", type, sizeof(type))) {
        cJSON_Delete(root);
        return fail("INVALID_COMMAND", "missing_or_invalid_type");
    }

    if (std::strcmp(type, "DISPENSE") != 0) {
        cJSON_Delete(root);
        return fail("UNKNOWN_COMMAND_TYPE", "type_is_not_dispense");
    }

    ParseResult result = {};
    result.ok = true;
    domain::DispenseCommand &command = result.command;
    int timeoutMs = 0;

    const bool valid =
        getRequiredInt(root, "command_id", &command.commandId) &&
        getRequiredInt(root, "sale_id", &command.saleId) &&
        getRequiredInt(root, "machine_id", &command.machineId) &&
        getRequiredInt(root, "product_id", &command.productId) &&
        getRequiredInt(root, "slot_id", &command.slotId) &&
        getRequiredString(root, "slot_code", command.slotCode, sizeof(command.slotCode)) &&
        getRequiredInt(root, "motor_id", &command.motorId) &&
        getRequiredInt(root, "sensor_column_id", &command.sensorColumnId) &&
        getRequiredInt(root, "quantity", &command.quantity) &&
        getRequiredInt(root, "attempts_allowed", &command.attemptsAllowed) &&
        getRequiredInt(root, "timeout_ms_per_attempt", &timeoutMs) &&
        getIssuedAt(root, command.issuedAt, sizeof(command.issuedAt));

    cJSON_Delete(root);

    if (!valid) {
        return fail("INVALID_COMMAND", "missing_or_invalid_required_field");
    }

    if (timeoutMs < 0) {
        return fail("INVALID_COMMAND", "timeout_ms_per_attempt_out_of_range");
    }

    command.timeoutMsPerAttempt = static_cast<uint32_t>(timeoutMs);
    return result;
}

const char *heartbeatStatusForState(domain::MachineState state) {
    if (state == domain::MachineState::VENDING) {
        return "DISPENSING";
    }

    if (state == domain::MachineState::ERROR) {
        return "ERROR";
    }

    return "ONLINE";
}

bool buildHeartbeatPayload(char *buffer, size_t bufferSize, domain::MachineState state,
                           const char *ipAddress, bool wifiConnected, bool mqttConnected,
                           uint64_t uptimeMs) {
    const int written = std::snprintf(
        buffer,
        bufferSize,
        "{\"type\":\"HEARTBEAT\",\"machine_id\":%d,\"status\":\"%s\","
        "\"current_state\":\"%s\",\"firmware_version\":\"%s\","
        "\"ip_address\":\"%s\",\"wifi_connected\":%s,\"mqtt_connected\":%s,"
        "\"uptime_ms\":%llu}",
        config::MachineId,
        heartbeatStatusForState(state),
        domain::machineStateToString(state),
        config::FirmwareVersion,
        ipAddress != nullptr ? ipAddress : "",
        wifiConnected ? "true" : "false",
        mqttConnected ? "true" : "false",
        static_cast<unsigned long long>(uptimeMs));

    return writePayload(buffer, bufferSize, written);
}

bool buildDispenseEventPayload(char *buffer, size_t bufferSize, const char *eventType,
                               const domain::DispenseCommand &command, int attempt,
                               int attemptsExecuted, const char *reason, uint64_t uptimeMs) {
    const int written = std::snprintf(
        buffer,
        bufferSize,
        "{\"type\":\"%s\",\"command_id\":%d,\"sale_id\":%d,\"machine_id\":%d,"
        "\"product_id\":%d,\"slot_id\":%d,\"slot_code\":\"%s\","
        "\"motor_id\":%d,\"sensor_column_id\":%d,\"attempt\":%d,"
        "\"attempts_executed\":%d,\"reason\":\"%s\",\"uptime_ms\":%llu}",
        eventType,
        command.commandId,
        command.saleId,
        config::MachineId,
        command.productId,
        command.slotId,
        command.slotCode,
        command.motorId,
        command.sensorColumnId,
        attempt,
        attemptsExecuted,
        reason != nullptr ? reason : "",
        static_cast<unsigned long long>(uptimeMs));

    return writePayload(buffer, bufferSize, written);
}

bool buildMachineErrorPayload(char *buffer, size_t bufferSize, const char *reason,
                              const char *details, domain::MachineState state,
                              uint64_t uptimeMs) {
    const int written = std::snprintf(
        buffer,
        bufferSize,
        "{\"type\":\"MACHINE_ERROR\",\"machine_id\":%d,\"status\":\"%s\","
        "\"current_state\":\"%s\",\"reason\":\"%s\",\"details\":\"%s\","
        "\"uptime_ms\":%llu}",
        config::MachineId,
        heartbeatStatusForState(state),
        domain::machineStateToString(state),
        reason != nullptr ? reason : "INTERNAL_ERROR",
        details != nullptr ? details : "",
        static_cast<unsigned long long>(uptimeMs));

    return writePayload(buffer, bufferSize, written);
}

bool buildMachineLogPayload(char *buffer, size_t bufferSize, const char *level,
                            const char *message, domain::MachineState state,
                            uint64_t uptimeMs) {
    const int written = std::snprintf(
        buffer,
        bufferSize,
        "{\"type\":\"MACHINE_LOG\",\"machine_id\":%d,\"level\":\"%s\","
        "\"message\":\"%s\",\"current_state\":\"%s\",\"uptime_ms\":%llu}",
        config::MachineId,
        level != nullptr ? level : "INFO",
        message != nullptr ? message : "",
        domain::machineStateToString(state),
        static_cast<unsigned long long>(uptimeMs));

    return writePayload(buffer, bufferSize, written);
}

}  // namespace vending::mqtt
