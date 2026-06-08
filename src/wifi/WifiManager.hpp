#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

namespace vending::wifi {

enum class WifiState {
    NOT_STARTED,
    INITIALIZED,
    CONNECTING,
    CONNECTED,
    FAILED
};

class WifiManager {
public:
    WifiManager();

    esp_err_t init();
    void connect();
    void printStatus() const;
    bool isConnected() const;
    WifiState getState() const;
    const char *getIpAddress() const;

private:
    EventGroupHandle_t eventGroup;
    esp_netif_t *stationNetif;
    esp_event_handler_instance_t wifiEventHandler;
    esp_event_handler_instance_t ipEventHandler;
    TaskHandle_t reconnectTask;
    WifiState state;
    char ipAddress[16];
    bool initialized;

    esp_err_t requestConnect(bool waitForResult);
    static void onWifiEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static void onIpEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static void reconnectTaskEntry(void *arg);
};

const char *wifiStateToString(WifiState state);

}  // namespace vending::wifi
