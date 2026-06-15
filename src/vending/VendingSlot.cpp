#include "vending/VendingSlot.hpp"

#include "esp_log.h"
#include "esp_timer.h"

namespace vending::domain {

namespace {

constexpr const char *Tag = "VEND";
constexpr uint64_t MicrosPerMs = 1000;

}  // namespace

VendingSlot::VendingSlot(int slotId, motor::StepperMotor &motor, sensor::IrBreakBeamSensor &sensor)
    : slotId(slotId),
      motor(motor),
      sensor(sensor),
      state(SlotState::IDLE) {}

bool VendingSlot::vend(uint32_t timeoutMs) {
    if (!isAvailable()) {
        ESP_LOGW(Tag, "[VEND] Slot %d is not available", slotId);
        return false;
    }

    state = SlotState::RUNNING;
    ESP_LOGI(Tag, "[VEND] Slot %d motor started", slotId);

    const int64_t startedAtUs = esp_timer_get_time();
    const uint64_t timeoutUs = static_cast<uint64_t>(timeoutMs) * MicrosPerMs;

    while (static_cast<uint64_t>(esp_timer_get_time() - startedAtUs) < timeoutUs) {
        motor.stepBackward();

        if (sensor.isBeamBroken()) {
            motor.release();
            state = SlotState::SUCCESS;
            ESP_LOGI(Tag, "[VEND] Slot %d product detected", slotId);
            ESP_LOGI(Tag, "[VEND] Slot %d success", slotId);
            return true;
        }
    }

    motor.release();
    state = SlotState::FAILED;
    ESP_LOGW(Tag, "[VEND] Slot %d timeout", slotId);
    ESP_LOGW(Tag, "[VEND] Slot %d failed", slotId);
    return false;
}

bool VendingSlot::testMotor(uint32_t stepLimit) {
    if (!isAvailable()) {
        ESP_LOGW(Tag, "[VEND] Slot %d motor test blocked; slot is not available", slotId);
        return false;
    }

    state = SlotState::RUNNING;
    ESP_LOGI(Tag, "[VEND] Slot %d motor test started: %lu steps",
             slotId,
             static_cast<unsigned long>(stepLimit));

    for (uint32_t step = 0; step < stepLimit; ++step) {
        motor.stepBackward();
    }

    motor.release();
    state = SlotState::IDLE;
    ESP_LOGI(Tag, "[VEND] Slot %d motor test finished; coils released", slotId);
    return true;
}

bool VendingSlot::isAvailable() const {
    return state != SlotState::RUNNING && state != SlotState::DISABLED;
}

bool VendingSlot::isSensorBlocked() const {
    return sensor.isBeamBroken();
}

bool VendingSlot::readSensorRaw() const {
    return sensor.readRaw();
}

int VendingSlot::getId() const {
    return slotId;
}

SlotState VendingSlot::getState() const {
    return state;
}

const char *slotStateToString(SlotState state) {
    switch (state) {
        case SlotState::IDLE:
            return "IDLE";
        case SlotState::RUNNING:
            return "RUNNING";
        case SlotState::SUCCESS:
            return "SUCCESS";
        case SlotState::FAILED:
            return "FAILED";
        case SlotState::DISABLED:
            return "DISABLED";
    }

    return "UNKNOWN";
}

}  // namespace vending::domain
