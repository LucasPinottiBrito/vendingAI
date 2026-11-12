#pragma once
#include <atomic>
#include <functional>
#include "motor/MotorController.hpp"
#include "sensor/DropDetector.hpp"
#include "vending/VendingTypes.hpp"
#include "freertos/semphr.h"
#include "nvs.h"

namespace vending::domain {
struct VendResult {
    bool delivered = false;
    bool mechanicalFault = false;
    int attempts = 0;
    const char *reason = "MACHINE_BUSY";
};
class VendingController {
public:
    esp_err_t init();
    VendResult vendSlot(int id, uint32_t timeout = config::VendTimeoutMs, int attempts = 1,
                        std::function<void(int)> retry = {});
    void testMotor(int id);
    void recover(int id);
    void printStatus();
    void printSensors() const;
    MachineState getMachineState() const { return state.load(); }
    bool isVendInProgress() const { return state.load() == MachineState::VENDING; }
private:
    VendResult run(int id, uint32_t timeout, int attempts, bool observeDrop,
                   const std::function<void(int)> &retry);
    bool setBlocked(uint16_t value);
    motor::MotorController motor;
    sensor::DropDetector detector;
    SemaphoreHandle_t mutex = nullptr;
    std::atomic<MachineState> state{MachineState::BOOTING};
    std::atomic<int> selected{0};
    std::atomic<const char *> lastReason{"BOOT"};
    std::atomic<uint16_t> blocked{0};
    nvs_handle_t storage{};
};
}
