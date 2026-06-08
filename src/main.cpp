#include "config.hpp"
#include "mqtt/MqttManager.hpp"
#include "serial/CommandHandler.hpp"
#include "vending/VendingController.hpp"
#include "wifi/WifiManager.hpp"

#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"

namespace {

constexpr const char *Tag = "BOOT";

}  // namespace

extern "C" void app_main() {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    ESP_LOGI(Tag, "[BOOT] Firmware version: %s", vending::config::FirmwareVersion);
    ESP_LOGI(Tag, "[BOOT] Platform: ESP32-S3 / ESP-IDF / PlatformIO");
    ESP_LOGI(Tag, "[BOOT] Firmware v0.2 MQTT build initialized");
    ESP_LOGI(Tag, "[BOOT] GPIO map loaded for two motors, two IR sensors, and reserved reed switch");
    ESP_LOGI(Tag, "[BOOT] MQTT enabled; payment, backend, display, OTA, and persistence remain out of firmware scope");

    vending::wifi::WifiManager wifiManager;
    const esp_err_t wifiInitResult = wifiManager.init();
    if (wifiInitResult == ESP_OK) {
        wifiManager.connect();
    } else {
        ESP_LOGW(Tag, "[BOOT] Wi-Fi init failed; local serial testing will continue: %s",
                 esp_err_to_name(wifiInitResult));
    }

    vending::domain::VendingController controller;
    const esp_err_t initResult = controller.init();
    if (initResult != ESP_OK) {
        ESP_LOGE(Tag, "[BOOT] Vending controller init failed: %s", esp_err_to_name(initResult));
        return;
    }

    controller.printStatus();
    controller.printSensors();

    vending::mqtt::MqttManager mqttManager(controller, wifiManager);
    const esp_err_t mqttInitResult = mqttManager.init();
    if (mqttInitResult != ESP_OK) {
        ESP_LOGW(Tag, "[BOOT] MQTT init failed; local serial testing will continue: %s",
                 esp_err_to_name(mqttInitResult));
    } else {
        const esp_err_t mqttStartResult = mqttManager.start();
        if (mqttStartResult != ESP_OK) {
            ESP_LOGW(Tag, "[BOOT] MQTT start failed; local serial testing will continue: %s",
                     esp_err_to_name(mqttStartResult));
        }
    }

    ESP_LOGI(Tag, "[BOOT] Serial command interface ready");
    vending::serial::CommandHandler commandHandler(controller, wifiManager, &mqttManager);
    commandHandler.run();
}
