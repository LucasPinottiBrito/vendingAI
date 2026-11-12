#pragma once

#include <cstdint>

#include "driver/gpio.h"

namespace vending::config {

inline constexpr const char *FirmwareVersion = "0.3.0";

inline constexpr const char *WifiSsid = "Ap_42";
inline constexpr const char *WifiPassword = "Lu81833923";
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

inline constexpr uint32_t VendTimeoutMs = 10000;
// Provisional motor on-time per attempt; calibrate for the installed mechanism.
inline constexpr uint32_t MotorRunMs = 1000;
static_assert(MotorRunMs > 0 && MotorRunMs <= VendTimeoutMs,
              "MotorRunMs must be within the motor safety limit");
inline constexpr uint32_t DropWindowMs = 750;
inline constexpr gpio_num_t Data = GPIO_NUM_4, Clock = GPIO_NUM_5, Latch = GPIO_NUM_6, Enable = GPIO_NUM_1;
inline constexpr gpio_num_t IrPins[] = {GPIO_NUM_2, GPIO_NUM_42, GPIO_NUM_41, GPIO_NUM_40};
inline constexpr bool IrSensorsActiveLow = true;
struct Channel { unsigned outputBit; bool installed; };
inline constexpr Channel Channels[] = {
    {0,true}, {1,true}, {2,true}, {3,false}, {4,false}, {5,false},
    {6,false}, {7,false}, {8,false}, {9,false}, {10,false}, {11,false},
    {12,false}, {13,false}, {14,false}, {15,false}
};
} // namespace vending::config
