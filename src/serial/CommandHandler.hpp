#pragma once

#include "mqtt/MqttManager.hpp"
#include "vending/VendingController.hpp"
#include "wifi/WifiManager.hpp"

namespace vending::serial {

class CommandHandler {
public:
    CommandHandler(domain::VendingController &controller, wifi::WifiManager &wifiManager,
                   mqtt::MqttManager *mqttManager = nullptr);

    void run();
    void handleLine(const char *line);
    void printHelp() const;

private:
    domain::VendingController &controller;
    wifi::WifiManager &wifiManager;
    mqtt::MqttManager *mqttManager;

    void printWifiStatus() const;
    void printMqttStatus() const;
    void printInvalidCommand() const;
};

}  // namespace vending::serial
