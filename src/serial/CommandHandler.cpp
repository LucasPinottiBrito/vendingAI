#include "serial/CommandHandler.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace vending::serial {

namespace {

constexpr const char *Tag = "CMD";
constexpr size_t CommandBufferSize = 96;

std::string normalizeLine(const char *line) {
    std::string normalized;
    normalized.reserve(CommandBufferSize);

    bool previousWasSpace = true;
    for (size_t i = 0; line[i] != '\0'; ++i) {
        const unsigned char value = static_cast<unsigned char>(line[i]);
        if (value == '\r' || value == '\n') {
            break;
        }

        if (std::isspace(value)) {
            if (!previousWasSpace) {
                normalized.push_back(' ');
                previousWasSpace = true;
            }
            continue;
        }

        normalized.push_back(static_cast<char>(std::tolower(value)));
        previousWasSpace = false;
    }

    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }

    return normalized;
}

void splitCommand(const std::string &line, char *command, size_t commandSize,
                  char *arg, size_t argSize, char *extra, size_t extraSize) {
    command[0] = '\0';
    arg[0] = '\0';
    extra[0] = '\0';

    std::sscanf(line.c_str(), "%15s %15s %15s", command, arg, extra);

    command[commandSize - 1] = '\0';
    arg[argSize - 1] = '\0';
    extra[extraSize - 1] = '\0';
}

}  // namespace

CommandHandler::CommandHandler(domain::VendingController &controller, wifi::WifiManager &wifiManager,
                               mqtt::MqttManager *mqttManager)
    : controller(controller),
      wifiManager(wifiManager),
      mqttManager(mqttManager) {}

void CommandHandler::run() {
    char buffer[CommandBufferSize] = {};
    size_t length = 0;

    printHelp();
    std::printf("\n> ");
    std::fflush(stdout);

    while (true) {
        const int input = std::getchar();
        if (input == EOF) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }

        const unsigned char value = static_cast<unsigned char>(input);
        if (value == '\r' || value == '\n') {
            buffer[length] = '\0';
            handleLine(buffer);
            length = 0;
            buffer[0] = '\0';
            std::printf("\n> ");
            std::fflush(stdout);
            continue;
        }

        if (length + 1 >= sizeof(buffer)) {
            ESP_LOGW(Tag, "[CMD] Command too long; input cleared");
            length = 0;
            buffer[0] = '\0';
            continue;
        }

        buffer[length++] = static_cast<char>(value);
    }
}

void CommandHandler::handleLine(const char *line) {
    const std::string normalized = normalizeLine(line);
    if (normalized.empty()) {
        return;
    }

    char command[16] = {};
    char arg[16] = {};
    char extra[16] = {};
    splitCommand(normalized, command, sizeof(command), arg, sizeof(arg), extra, sizeof(extra));

    if ((std::strcmp(command, "help") == 0 || std::strcmp(command, "h") == 0) &&
        arg[0] == '\0') {
        printHelp();
        return;
    }

    if ((std::strcmp(command, "status") == 0 || std::strcmp(command, "c") == 0) &&
        arg[0] == '\0') {
        controller.printStatus();
        return;
    }

    if ((std::strcmp(command, "sensor") == 0 || std::strcmp(command, "s") == 0) &&
        arg[0] == '\0') {
        controller.printSensors();
        return;
    }

    if ((std::strcmp(command, "wifi") == 0 || std::strcmp(command, "w") == 0) &&
        arg[0] == '\0') {
        printWifiStatus();
        return;
    }

    if ((std::strcmp(command, "mqtt") == 0 || std::strcmp(command, "m") == 0) &&
        arg[0] == '\0') {
        printMqttStatus();
        return;
    }

    if (std::strcmp(command, "heartbeat") == 0 && arg[0] == '\0') {
        if (mqttManager == nullptr || !mqttManager->publishHeartbeat()) {
            ESP_LOGW(Tag, "[CMD] Manual heartbeat could not be published");
        }
        return;
    }

    if ((std::strcmp(command, "a") == 0 && arg[0] == '\0') ||
        (std::strcmp(command, "motor") == 0 && std::strcmp(arg, "1") == 0 && extra[0] == '\0')) {
        controller.testMotor(1);
        return;
    }

    if ((std::strcmp(command, "b") == 0 && arg[0] == '\0') ||
        (std::strcmp(command, "motor") == 0 && std::strcmp(arg, "2") == 0 && extra[0] == '\0')) {
        controller.testMotor(2);
        return;
    }

    if ((std::strcmp(command, "1") == 0 && arg[0] == '\0') ||
        (std::strcmp(command, "vend") == 0 && std::strcmp(arg, "1") == 0 && extra[0] == '\0')) {
        controller.vendSlot(1);
        return;
    }

    if ((std::strcmp(command, "2") == 0 && arg[0] == '\0') ||
        (std::strcmp(command, "vend") == 0 && std::strcmp(arg, "2") == 0 && extra[0] == '\0')) {
        controller.vendSlot(2);
        return;
    }

    printInvalidCommand();
}

void CommandHandler::printHelp() const {
    std::printf("\nAvailable commands:\n");
    std::printf("  help | h - list commands\n");
    std::printf("  status | c - print machine and slot state\n");
    std::printf("  sensor | s - print IR sensor raw/logical state\n");
    std::printf("  motor 1 | a - run a limited motor test for slot 1\n");
    std::printf("  motor 2 | b - run a limited motor test for slot 2\n");
    std::printf("  vend 1 | 1 - vend slot 1 until sensor detection or timeout\n");
    std::printf("  vend 2 | 2 - vend slot 2 until sensor detection or timeout\n");
    std::printf("  wifi | w - print Wi-Fi status\n");
    std::printf("  mqtt | m - print MQTT status and topics\n");
    std::printf("  heartbeat - publish one MQTT heartbeat when connected\n");
}

void CommandHandler::printWifiStatus() const {
    wifiManager.printStatus();
}

void CommandHandler::printMqttStatus() const {
    if (mqttManager != nullptr) {
        mqttManager->printStatus();
        return;
    }

    ESP_LOGW(Tag, "[CMD] MQTT manager is not available");
}

void CommandHandler::printInvalidCommand() const {
    ESP_LOGW(Tag, "[CMD] Invalid command. Type 'help' to list available commands.");
}

}  // namespace vending::serial
