#pragma once

#include <cstdint>

#include "driver/gpio.h"

namespace vending::config {

inline constexpr const char *FirmwareVersion = "0.2.0";

inline constexpr const char *WifiSsid = "UnivapWifi";
inline constexpr const char *WifiPassword = "universidade";
inline constexpr uint32_t WifiConnectTimeoutMs = 10000;
inline constexpr uint32_t WifiReconnectIntervalMs = 5000;

inline constexpr int MachineId = 1;
inline constexpr const char *MachineCode = "1";
inline constexpr const char *MqttBrokerUri = "mqtts://ef9ae0ae01774b9896c153998578af8b.s1.eu.hivemq.cloud:8883";
inline constexpr const char *MqttUsername = "machine1";
inline constexpr const char *MqttPassword = "Machine123";
inline constexpr uint32_t MqttReconnectIntervalMs = 5000;
inline constexpr uint32_t MqttHeartbeatIntervalMs = 30000;
inline constexpr uint32_t MqttCommandQueueLength = 3;
inline constexpr uint32_t MqttMaxPayloadLength = 1024;
inline constexpr uint32_t DefaultAttemptsAllowed = 2;
inline constexpr uint32_t MqttMaxAttemptsAllowed = 3;
inline constexpr uint32_t MqttMinVendTimeoutMs = 1;
inline constexpr uint32_t MqttMaxVendTimeoutMs = 30000;

inline constexpr uint32_t MotorStepDelayMs = 10;
inline constexpr uint32_t MotorTestStepLimit = 512;
inline constexpr uint32_t VendTimeoutMs = 10000;

inline constexpr gpio_num_t Motor1In1 = GPIO_NUM_4;
inline constexpr gpio_num_t Motor1In2 = GPIO_NUM_5;
inline constexpr gpio_num_t Motor1In3 = GPIO_NUM_6;
inline constexpr gpio_num_t Motor1In4 = GPIO_NUM_7;

inline constexpr gpio_num_t Motor2In1 = GPIO_NUM_15;
inline constexpr gpio_num_t Motor2In2 = GPIO_NUM_16;
inline constexpr gpio_num_t Motor2In3 = GPIO_NUM_17;
inline constexpr gpio_num_t Motor2In4 = GPIO_NUM_18;

inline constexpr gpio_num_t IrSensor1 = GPIO_NUM_10;
inline constexpr gpio_num_t IrSensor2 = GPIO_NUM_11;
inline constexpr gpio_num_t ReedSwitchReserved = GPIO_NUM_12;

inline constexpr bool IrSensorsActiveLow = true;

}  // namespace vending::config
