#pragma once

#include <cstdint>

#include "config.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "vending/VendingController.hpp"
#include "wifi/WifiManager.hpp"

namespace vending::mqtt {

class MqttManager {
public:
    MqttManager(domain::VendingController &controller, wifi::WifiManager &wifiManager);

    esp_err_t init();
    esp_err_t start();
    void printStatus() const;
    bool isConnected() const;
    bool publishHeartbeat();

private:
    struct QueuedPayload {
        char data[config::MqttMaxPayloadLength];
        int length;
    };

    domain::VendingController &controller;
    wifi::WifiManager &wifiManager;
    esp_mqtt_client_handle_t client;
    QueueHandle_t commandQueue;
    TaskHandle_t commandTask;
    TaskHandle_t heartbeatTask;
    bool initialized;
    bool started;
    bool connected;
    bool hasLastCompletedCommand;
    int lastCompletedCommandId;
    char clientId[32];
    char actionsTopic[96];
    char statusTopic[96];
    char eventsTopic[96];

    domain::MachineState currentMachineState() const;
    uint64_t uptimeMs() const;
    bool publishDispenseEvent(const char *eventType, const domain::DispenseCommand &command,
                              int attempt, int attemptsExecuted, const char *reason);
    bool publishMachineError(const char *reason, const char *details);
    bool publishMachineLog(const char *level, const char *message);
    void enqueuePayload(const char *data, int dataLength);
    void processPayload(const char *payload);
    bool isActionsTopic(const char *topic, int topicLength) const;
    void handleEvent(esp_event_base_t eventBase, int32_t eventId, void *eventData);

    static void mqttEventHandler(void *handlerArgs, esp_event_base_t eventBase,
                                 int32_t eventId, void *eventData);
    static void commandTaskEntry(void *arg);
    static void heartbeatTaskEntry(void *arg);
};

}  // namespace vending::mqtt
