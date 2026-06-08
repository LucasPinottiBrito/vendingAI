#include "mqtt/MqttManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "config.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt/MqttPayloads.hpp"
#include "mqtt/MqttTopics.hpp"

namespace vending::mqtt {

namespace {

constexpr const char *Tag = "MQTT";
constexpr int QueueSendTimeoutMs = 50;
constexpr int CommandTaskStackWords = 8192;
constexpr int HeartbeatTaskStackWords = 4096;
constexpr int CommandTaskPriority = 5;
constexpr int HeartbeatTaskPriority = 4;

bool isKnownMotor(int motorId) {
    return motorId == 1 || motorId == 2;
}

bool isKnownSensor(int sensorColumnId) {
    return sensorColumnId == 1 || sensorColumnId == 2;
}

bool isSupportedPhysicalPair(int motorId, int sensorColumnId) {
    return (motorId == 1 && sensorColumnId == 1) || (motorId == 2 && sensorColumnId == 2);
}

}  // namespace

MqttManager::MqttManager(domain::VendingController &controller, wifi::WifiManager &wifiManager)
    : controller(controller),
      wifiManager(wifiManager),
      client(nullptr),
      commandQueue(nullptr),
      commandTask(nullptr),
      heartbeatTask(nullptr),
      initialized(false),
      started(false),
      connected(false),
      hasLastCompletedCommand(false),
      lastCompletedCommandId(0),
      clientId{},
      actionsTopic{},
      statusTopic{},
      eventsTopic{} {}

esp_err_t MqttManager::init() {
    if (initialized) {
        return ESP_OK;
    }

    formatActionsTopic(actionsTopic, sizeof(actionsTopic));
    formatStatusTopic(statusTopic, sizeof(statusTopic));
    formatEventsTopic(eventsTopic, sizeof(eventsTopic));

    commandQueue = xQueueCreate(config::MqttCommandQueueLength, sizeof(QueuedPayload));
    if (commandQueue == nullptr) {
        ESP_LOGE(Tag, "[MQTT] Failed to create command queue");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(&MqttManager::commandTaskEntry, "mqtt_cmd", CommandTaskStackWords, this,
                    CommandTaskPriority, &commandTask) != pdPASS) {
        ESP_LOGE(Tag, "[MQTT] Failed to create command task");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(&MqttManager::heartbeatTaskEntry, "mqtt_heartbeat", HeartbeatTaskStackWords, this,
                    HeartbeatTaskPriority, &heartbeatTask) != pdPASS) {
        ESP_LOGE(Tag, "[MQTT] Failed to create heartbeat task");
        return ESP_ERR_NO_MEM;
    }

    initialized = true;
    return ESP_OK;
}

esp_err_t MqttManager::start() {
    if (!initialized) {
        const esp_err_t initResult = init();
        if (initResult != ESP_OK) {
            return initResult;
        }
    }

    if (started) {
        return ESP_OK;
    }

    std::snprintf(clientId, sizeof(clientId), "vending-%d", config::MachineId);

    esp_mqtt_client_config_t mqttConfig = {};
    mqttConfig.broker.address.uri = config::MqttBrokerUri;
    mqttConfig.credentials.client_id = clientId;
    mqttConfig.network.reconnect_timeout_ms = static_cast<int>(config::MqttReconnectIntervalMs);
    mqttConfig.network.disable_auto_reconnect = false;
    mqttConfig.buffer.size = static_cast<int>(config::MqttMaxPayloadLength);
    mqttConfig.buffer.out_size = static_cast<int>(config::MqttMaxPayloadLength);

    if (std::strlen(config::MqttUsername) > 0) {
        mqttConfig.credentials.username = config::MqttUsername;
    }

    if (std::strlen(config::MqttPassword) > 0) {
        mqttConfig.credentials.authentication.password = config::MqttPassword;
    }

    client = esp_mqtt_client_init(&mqttConfig);
    if (client == nullptr) {
        ESP_LOGE(Tag, "[MQTT] Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_err_t result = esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, &MqttManager::mqttEventHandler, this);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[MQTT] Failed to register MQTT event handler: %s", esp_err_to_name(result));
        return result;
    }

    ESP_LOGI(Tag, "[MQTT] Starting client for broker: %s", config::MqttBrokerUri);
    ESP_LOGI(Tag, "[MQTT] Actions=%s Events=%s Status=%s", actionsTopic, eventsTopic, statusTopic);

    result = esp_mqtt_client_start(client);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[MQTT] Failed to start MQTT client: %s", esp_err_to_name(result));
        return result;
    }

    started = true;
    return ESP_OK;
}

void MqttManager::printStatus() const {
    ESP_LOGI(Tag, "[MQTT] Started=%s Connected=%s Broker='%s'",
             started ? "YES" : "NO",
             connected ? "YES" : "NO",
             config::MqttBrokerUri);
    ESP_LOGI(Tag, "[MQTT] Topics actions='%s' events='%s' status='%s'",
             actionsTopic,
             eventsTopic,
             statusTopic);
    ESP_LOGI(Tag, "[MQTT] Last completed command=%s%d",
             hasLastCompletedCommand ? "" : "NONE/",
             hasLastCompletedCommand ? lastCompletedCommandId : 0);
}

bool MqttManager::isConnected() const {
    return connected;
}

bool MqttManager::publishHeartbeat() {
    if (client == nullptr || !connected) {
        ESP_LOGW(Tag, "[MQTT] Heartbeat skipped; MQTT is not connected");
        return false;
    }

    char payload[512] = {};
    if (!buildHeartbeatPayload(payload,
                               sizeof(payload),
                               currentMachineState(),
                               wifiManager.getIpAddress(),
                               wifiManager.isConnected(),
                               connected,
                               uptimeMs())) {
        ESP_LOGW(Tag, "[MQTT] Failed to build heartbeat payload");
        return false;
    }

    const int messageId = esp_mqtt_client_publish(client, statusTopic, payload, 0, 1, 0);
    ESP_LOGI(Tag, "[MQTT] Heartbeat publish msg_id=%d", messageId);
    return messageId >= 0;
}

domain::MachineState MqttManager::currentMachineState() const {
    const domain::MachineState physicalState = controller.getMachineState();
    if (physicalState == domain::MachineState::BOOTING ||
        physicalState == domain::MachineState::VENDING ||
        physicalState == domain::MachineState::ERROR) {
        return physicalState;
    }

    if (!wifiManager.isConnected()) {
        return domain::MachineState::WIFI_CONNECTING;
    }

    if (!connected) {
        return domain::MachineState::MQTT_CONNECTING;
    }

    return domain::MachineState::READY;
}

uint64_t MqttManager::uptimeMs() const {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

bool MqttManager::publishDispenseEvent(const char *eventType, const domain::DispenseCommand &command,
                                       int attempt, int attemptsExecuted, const char *reason) {
    if (client == nullptr || !connected) {
        ESP_LOGW(Tag, "[MQTT] Event %s skipped; MQTT is not connected", eventType);
        return false;
    }

    char payload[512] = {};
    if (!buildDispenseEventPayload(payload,
                                   sizeof(payload),
                                   eventType,
                                   command,
                                   attempt,
                                   attemptsExecuted,
                                   reason,
                                   uptimeMs())) {
        ESP_LOGW(Tag, "[MQTT] Failed to build %s payload", eventType);
        return false;
    }

    const int messageId = esp_mqtt_client_publish(client, eventsTopic, payload, 0, 1, 0);
    ESP_LOGI(Tag, "[MQTT] Event %s publish msg_id=%d", eventType, messageId);
    return messageId >= 0;
}

bool MqttManager::publishMachineError(const char *reason, const char *details) {
    if (client == nullptr || !connected) {
        ESP_LOGW(Tag, "[MQTT] MACHINE_ERROR skipped; MQTT is not connected reason=%s details=%s",
                 reason != nullptr ? reason : "",
                 details != nullptr ? details : "");
        return false;
    }

    char payload[512] = {};
    if (!buildMachineErrorPayload(payload,
                                  sizeof(payload),
                                  reason,
                                  details,
                                  currentMachineState(),
                                  uptimeMs())) {
        ESP_LOGW(Tag, "[MQTT] Failed to build MACHINE_ERROR payload");
        return false;
    }

    const int messageId = esp_mqtt_client_publish(client, eventsTopic, payload, 0, 1, 0);
    ESP_LOGI(Tag, "[MQTT] MACHINE_ERROR publish msg_id=%d", messageId);
    return messageId >= 0;
}

bool MqttManager::publishMachineLog(const char *level, const char *message) {
    if (client == nullptr || !connected) {
        return false;
    }

    char payload[384] = {};
    if (!buildMachineLogPayload(payload,
                                sizeof(payload),
                                level,
                                message,
                                currentMachineState(),
                                uptimeMs())) {
        return false;
    }

    const int messageId = esp_mqtt_client_publish(client, eventsTopic, payload, 0, 0, 0);
    return messageId >= 0;
}

void MqttManager::enqueuePayload(const char *data, int dataLength) {
    if (commandQueue == nullptr || data == nullptr || dataLength <= 0) {
        return;
    }

    if (dataLength >= static_cast<int>(config::MqttMaxPayloadLength)) {
        ESP_LOGW(Tag, "[MQTT] Ignoring oversized action payload: %d bytes", dataLength);
        publishMachineError("INVALID_COMMAND", "payload_too_large");
        return;
    }

    QueuedPayload item = {};
    std::memcpy(item.data, data, dataLength);
    item.data[dataLength] = '\0';
    item.length = dataLength;

    if (xQueueSend(commandQueue, &item, pdMS_TO_TICKS(QueueSendTimeoutMs)) != pdPASS) {
        ESP_LOGW(Tag, "[MQTT] Command queue full; action ignored");
        publishMachineError("INTERNAL_ERROR", "command_queue_full");
    }
}

void MqttManager::processPayload(const char *payload) {
    const ParseResult parsed = parseDispenseCommandPayload(payload);
    if (!parsed.ok) {
        ESP_LOGW(Tag, "[MQTT] Invalid action payload reason=%s details=%s",
                 parsed.reason,
                 parsed.details);
        publishMachineError(parsed.reason, parsed.details);
        return;
    }

    domain::DispenseCommand command = parsed.command;
    if (!connected) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; MQTT disconnected before processing",
                 command.commandId);
        return;
    }

    if (command.machineId != config::MachineId) {
        ESP_LOGW(Tag, "[MQTT] Ignoring command %d for machine_id=%d",
                 command.commandId,
                 command.machineId);
        return;
    }

    if (hasLastCompletedCommand && command.commandId == lastCompletedCommandId) {
        ESP_LOGW(Tag, "[MQTT] Duplicate command ignored: %d", command.commandId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "COMMAND_DUPLICATED");
        publishMachineLog("WARN", "duplicate_command_ignored");
        return;
    }

    if (controller.isVendInProgress() || controller.getMachineState() != domain::MachineState::READY) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; machine is busy", command.commandId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "MACHINE_BUSY");
        return;
    }

    if (!isKnownMotor(command.motorId)) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; unknown motor_id=%d",
                 command.commandId,
                 command.motorId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "UNKNOWN_MOTOR_ID");
        return;
    }

    if (!isKnownSensor(command.sensorColumnId)) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; unknown sensor_column_id=%d",
                 command.commandId,
                 command.sensorColumnId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "UNKNOWN_SENSOR_COLUMN_ID");
        return;
    }

    if (!isSupportedPhysicalPair(command.motorId, command.sensorColumnId)) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; unsupported motor/sensor pair %d/%d",
                 command.commandId,
                 command.motorId,
                 command.sensorColumnId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "INVALID_COMMAND");
        return;
    }

    if (command.quantity != 1) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; unsupported quantity=%d",
                 command.commandId,
                 command.quantity);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "UNSUPPORTED_QUANTITY");
        return;
    }

    if (command.attemptsAllowed <= 0 || command.timeoutMsPerAttempt == 0) {
        ESP_LOGW(Tag, "[MQTT] Rejecting command %d; invalid attempts or timeout",
                 command.commandId);
        publishDispenseEvent("DISPENSE_FAILED", command, 0, 0, "INVALID_COMMAND");
        return;
    }

    const int attemptsAllowed = static_cast<int>(
        std::min(static_cast<uint32_t>(command.attemptsAllowed), config::MqttMaxAttemptsAllowed));
    const uint32_t timeoutMs = std::clamp(command.timeoutMsPerAttempt,
                                          config::MqttMinVendTimeoutMs,
                                          config::MqttMaxVendTimeoutMs);

    ESP_LOGI(Tag, "[MQTT] DISPENSE command=%d sale=%d slot=%s motor=%d sensor=%d attempts=%d timeout_ms=%lu issued_at=%s",
             command.commandId,
             command.saleId,
             command.slotCode,
             command.motorId,
             command.sensorColumnId,
             attemptsAllowed,
             static_cast<unsigned long>(timeoutMs),
             command.issuedAt);

    publishDispenseEvent("DISPENSE_STARTED", command, 1, 0, "");

    for (int attempt = 1; attempt <= attemptsAllowed; ++attempt) {
        const bool success = controller.vendSlot(command.motorId, timeoutMs);
        if (success) {
            bool beamBroken = false;
            bool rawHigh = false;
            controller.getSensorState(command.sensorColumnId, &beamBroken, &rawHigh);

            if (beamBroken) {
                publishDispenseEvent("SENSOR_TRIGGERED", command, attempt, attempt, "");
            }

            publishDispenseEvent("DISPENSE_SUCCESS", command, attempt, attempt, "");
            hasLastCompletedCommand = true;
            lastCompletedCommandId = command.commandId;
            publishHeartbeat();
            return;
        }

        if (controller.isVendInProgress()) {
            publishDispenseEvent("DISPENSE_FAILED", command, attempt, attempt, "MACHINE_BUSY");
            return;
        }

        if (attempt < attemptsAllowed) {
            publishDispenseEvent("DISPENSE_RETRY", command, attempt + 1, attempt, "PRODUCT_NOT_DETECTED");
        }
    }

    publishDispenseEvent("DISPENSE_FAILED", command, attemptsAllowed, attemptsAllowed, "PRODUCT_NOT_DETECTED");
    hasLastCompletedCommand = true;
    lastCompletedCommandId = command.commandId;
    publishHeartbeat();
}

bool MqttManager::isActionsTopic(const char *topic, int topicLength) const {
    const size_t expectedLength = std::strlen(actionsTopic);
    return topic != nullptr &&
           topicLength == static_cast<int>(expectedLength) &&
           std::strncmp(topic, actionsTopic, expectedLength) == 0;
}

void MqttManager::handleEvent(esp_event_base_t eventBase, int32_t eventId, void *eventData) {
    (void)eventBase;

    auto *event = static_cast<esp_mqtt_event_handle_t>(eventData);
    if (event == nullptr) {
        return;
    }

    switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
        case MQTT_EVENT_CONNECTED:
            connected = true;
            ESP_LOGI(Tag, "[MQTT] Connected");
            esp_mqtt_client_subscribe(client, actionsTopic, 1);
            ESP_LOGI(Tag, "[MQTT] Subscribed to %s", actionsTopic);
            publishMachineLog("INFO", "mqtt_connected_and_subscribed");
            publishHeartbeat();
            break;

        case MQTT_EVENT_DISCONNECTED:
            connected = false;
            ESP_LOGW(Tag, "[MQTT] Disconnected; ESP-MQTT auto reconnect remains enabled");
            break;

        case MQTT_EVENT_DATA:
            if (!isActionsTopic(event->topic, event->topic_len)) {
                break;
            }

            if (event->retain) {
                ESP_LOGW(Tag, "[MQTT] Retained action ignored");
                publishMachineError("INVALID_COMMAND", "retained_action_ignored");
                break;
            }

            if (event->current_data_offset != 0 || event->data_len != event->total_data_len) {
                ESP_LOGW(Tag, "[MQTT] Fragmented action payload ignored");
                publishMachineError("INVALID_COMMAND", "fragmented_payload_not_supported");
                break;
            }

            enqueuePayload(event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGW(Tag, "[MQTT] Client error");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(Tag, "[MQTT] Subscription acknowledged msg_id=%d", event->msg_id);
            break;

        default:
            break;
    }
}

void MqttManager::mqttEventHandler(void *handlerArgs, esp_event_base_t eventBase,
                                   int32_t eventId, void *eventData) {
    auto *manager = static_cast<MqttManager *>(handlerArgs);
    if (manager != nullptr) {
        manager->handleEvent(eventBase, eventId, eventData);
    }
}

void MqttManager::commandTaskEntry(void *arg) {
    auto *manager = static_cast<MqttManager *>(arg);
    QueuedPayload item = {};

    while (true) {
        if (manager != nullptr && xQueueReceive(manager->commandQueue, &item, portMAX_DELAY) == pdPASS) {
            manager->processPayload(item.data);
        }
    }
}

void MqttManager::heartbeatTaskEntry(void *arg) {
    auto *manager = static_cast<MqttManager *>(arg);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(config::MqttHeartbeatIntervalMs));
        if (manager != nullptr) {
            manager->publishHeartbeat();
        }
    }
}

}  // namespace vending::mqtt
