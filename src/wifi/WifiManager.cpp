#include "wifi/WifiManager.hpp"

#include <cstdio>
#include <cstring>

#include "config.hpp"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

namespace vending::wifi {

namespace {

constexpr const char *Tag = "WIFI";
constexpr EventBits_t WifiConnectedBit = BIT0;
constexpr EventBits_t WifiFailedBit = BIT1;

esp_err_t initializeNvs() {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(Tag, "[WIFI] NVS needs reset for ESP-IDF Wi-Fi init");
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    return result;
}

}  // namespace

WifiManager::WifiManager()
    : eventGroup(nullptr),
      stationNetif(nullptr),
      wifiEventHandler(nullptr),
      ipEventHandler(nullptr),
      reconnectTask(nullptr),
      state(WifiState::NOT_STARTED),
      ipAddress{},
      initialized(false) {}

esp_err_t WifiManager::init() {
    if (initialized) {
        return ESP_OK;
    }

    eventGroup = xEventGroupCreate();
    if (eventGroup == nullptr) {
        ESP_LOGE(Tag, "[WIFI] Failed to create Wi-Fi event group");
        state = WifiState::FAILED;
        return ESP_ERR_NO_MEM;
    }

    esp_err_t result = initializeNvs();
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] NVS init failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_netif_init();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "[WIFI] esp_netif_init failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_event_loop_create_default();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(Tag, "[WIFI] event loop init failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    stationNetif = esp_netif_create_default_wifi_sta();
    if (stationNetif == nullptr) {
        ESP_LOGE(Tag, "[WIFI] Failed to create default Wi-Fi station netif");
        state = WifiState::FAILED;
        return ESP_FAIL;
    }

    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    result = esp_wifi_init(&wifiInitConfig);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_init failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_set_storage failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::onWifiEvent, this, &wifiEventHandler);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] Wi-Fi event handler registration failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::onIpEvent, this, &ipEventHandler);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] IP event handler registration failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    wifi_config_t wifiConfig = {};
    std::snprintf(reinterpret_cast<char *>(wifiConfig.sta.ssid),
                  sizeof(wifiConfig.sta.ssid),
                  "%s",
                  config::WifiSsid);
    std::snprintf(reinterpret_cast<char *>(wifiConfig.sta.password),
                  sizeof(wifiConfig.sta.password),
                  "%s",
                  config::WifiPassword);

    result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_set_mode failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_set_config failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    result = esp_wifi_start();
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_start failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    initialized = true;
    state = WifiState::INITIALIZED;

    if (xTaskCreate(&WifiManager::reconnectTaskEntry, "wifi_reconnect", 4096, this, 4, &reconnectTask) != pdPASS) {
        ESP_LOGE(Tag, "[WIFI] Failed to create reconnect task");
        state = WifiState::FAILED;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void WifiManager::connect() {
    (void)requestConnect(true);
}

esp_err_t WifiManager::requestConnect(bool waitForResult) {
    if (!initialized) {
        ESP_LOGW(Tag, "[WIFI] Connect requested before Wi-Fi init");
        state = WifiState::FAILED;
        return ESP_ERR_INVALID_STATE;
    }

    ipAddress[0] = '\0';
    if (eventGroup != nullptr) {
        xEventGroupClearBits(eventGroup, WifiConnectedBit | WifiFailedBit);
    }

    state = WifiState::CONNECTING;
    ESP_LOGI(Tag, "[WIFI] Connecting to SSID '%s'", config::WifiSsid);

    const esp_err_t result = esp_wifi_connect();
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[WIFI] esp_wifi_connect failed: %s", esp_err_to_name(result));
        state = WifiState::FAILED;
        return result;
    }

    if (!waitForResult) {
        return ESP_OK;
    }

    const EventBits_t bits = xEventGroupWaitBits(
        eventGroup,
        WifiConnectedBit | WifiFailedBit,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(config::WifiConnectTimeoutMs));

    if ((bits & WifiConnectedBit) != 0) {
        ESP_LOGI(Tag, "[WIFI] Connected. IP: %s", ipAddress);
        return ESP_OK;
    }

    if ((bits & WifiFailedBit) != 0) {
        ESP_LOGW(Tag, "[WIFI] Connection failed");
        return ESP_FAIL;
    }

    state = WifiState::FAILED;
    ESP_LOGW(Tag, "[WIFI] Connection timeout after %lu ms",
             static_cast<unsigned long>(config::WifiConnectTimeoutMs));
    return ESP_ERR_TIMEOUT;
}

void WifiManager::printStatus() const {
    if (state == WifiState::CONNECTED) {
        ESP_LOGI(Tag, "[WIFI] Status=%s SSID='%s' IP=%s",
                 wifiStateToString(state),
                 config::WifiSsid,
                 ipAddress);
        return;
    }

    ESP_LOGI(Tag, "[WIFI] Status=%s SSID='%s' timeout_ms=%lu",
             wifiStateToString(state),
             config::WifiSsid,
             static_cast<unsigned long>(config::WifiConnectTimeoutMs));
    ESP_LOGI(Tag, "[WIFI] Auto reconnect interval=%lu ms",
             static_cast<unsigned long>(config::WifiReconnectIntervalMs));
}

bool WifiManager::isConnected() const {
    return state == WifiState::CONNECTED;
}

WifiState WifiManager::getState() const {
    return state;
}

const char *WifiManager::getIpAddress() const {
    return ipAddress;
}

void WifiManager::onWifiEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
    (void)eventData;

    auto *manager = static_cast<WifiManager *>(arg);
    if (eventBase != WIFI_EVENT || manager == nullptr) {
        return;
    }

    if (eventId == WIFI_EVENT_STA_DISCONNECTED) {
        manager->ipAddress[0] = '\0';
        manager->state = WifiState::FAILED;
        ESP_LOGW(Tag, "[WIFI] Disconnected from access point");
        if (manager->eventGroup != nullptr) {
            xEventGroupSetBits(manager->eventGroup, WifiFailedBit);
        }
    }
}

void WifiManager::onIpEvent(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
    auto *manager = static_cast<WifiManager *>(arg);
    if (eventBase != IP_EVENT || eventId != IP_EVENT_STA_GOT_IP || manager == nullptr) {
        return;
    }

    auto *event = static_cast<ip_event_got_ip_t *>(eventData);
    std::snprintf(manager->ipAddress,
                  sizeof(manager->ipAddress),
                  IPSTR,
                  IP2STR(&event->ip_info.ip));
    manager->state = WifiState::CONNECTED;
    if (manager->eventGroup != nullptr) {
        xEventGroupSetBits(manager->eventGroup, WifiConnectedBit);
    }
}

void WifiManager::reconnectTaskEntry(void *arg) {
    auto *manager = static_cast<WifiManager *>(arg);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(config::WifiReconnectIntervalMs));

        if (manager == nullptr || !manager->initialized) {
            continue;
        }

        if (manager->state == WifiState::CONNECTED || manager->state == WifiState::CONNECTING) {
            continue;
        }

        ESP_LOGI(Tag, "[WIFI] Auto reconnect attempt");
        (void)manager->requestConnect(false);
    }
}

const char *wifiStateToString(WifiState state) {
    switch (state) {
        case WifiState::NOT_STARTED:
            return "NOT_STARTED";
        case WifiState::INITIALIZED:
            return "INITIALIZED";
        case WifiState::CONNECTING:
            return "CONNECTING";
        case WifiState::CONNECTED:
            return "CONNECTED";
        case WifiState::FAILED:
            return "FAILED";
    }

    return "UNKNOWN";
}

}  // namespace vending::wifi
