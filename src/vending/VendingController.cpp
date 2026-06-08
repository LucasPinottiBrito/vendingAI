#include "vending/VendingController.hpp"

#include "config.hpp"
#include "esp_log.h"

namespace vending::domain {

namespace {

constexpr const char *Tag = "VEND";
constexpr const char *SensorTag = "SENSOR";
constexpr const char *StatusTag = "STATUS";

esp_err_t logInitError(const char *name, esp_err_t result) {
    if (result != ESP_OK) {
        ESP_LOGE(Tag, "[VEND] %s init failed: %s", name, esp_err_to_name(result));
    }

    return result;
}

}  // namespace

VendingController::VendingController()
    : motor1(config::Motor1In1, config::Motor1In2, config::Motor1In3, config::Motor1In4),
      motor2(config::Motor2In1, config::Motor2In2, config::Motor2In3, config::Motor2In4),
      sensor1(config::IrSensor1, config::IrSensorsActiveLow),
      sensor2(config::IrSensor2, config::IrSensorsActiveLow),
      slot1(1, motor1, sensor1),
      slot2(2, motor2, sensor2),
      machineState(MachineState::BOOTING),
      vendInProgress(false),
      operationMutex(xSemaphoreCreateMutex()) {}

esp_err_t VendingController::init() {
    ESP_LOGI(Tag, "[VEND] Initializing motors and sensors");

    if (operationMutex == nullptr) {
        ESP_LOGE(Tag, "[VEND] Failed to create operation mutex");
        machineState = MachineState::ERROR;
        return ESP_ERR_NO_MEM;
    }

    esp_err_t result = logInitError("Motor 1", motor1.init());
    if (result != ESP_OK) {
        machineState = MachineState::ERROR;
        return result;
    }

    result = logInitError("Motor 2", motor2.init());
    if (result != ESP_OK) {
        machineState = MachineState::ERROR;
        return result;
    }

    result = logInitError("IR sensor 1", sensor1.init());
    if (result != ESP_OK) {
        machineState = MachineState::ERROR;
        return result;
    }

    result = logInitError("IR sensor 2", sensor2.init());
    if (result != ESP_OK) {
        machineState = MachineState::ERROR;
        return result;
    }

    machineState = MachineState::READY;
    ESP_LOGI(Tag, "[VEND] Vending controller ready");
    return ESP_OK;
}

bool VendingController::vendSlot(int slotId) {
    return vendSlot(slotId, config::VendTimeoutMs);
}

bool VendingController::vendSlot(int slotId, uint32_t timeoutMs) {
    ESP_LOGI(Tag, "[VEND] Slot %d requested", slotId);

    if (operationMutex == nullptr || xSemaphoreTake(operationMutex, 0) != pdTRUE) {
        ESP_LOGW(Tag, "[VEND] Slot %d rejected; another operation is active", slotId);
        return false;
    }

    if (vendInProgress || machineState == MachineState::VENDING) {
        ESP_LOGW(Tag, "[VEND] Slot %d rejected; machine is already vending", slotId);
        xSemaphoreGive(operationMutex);
        return false;
    }

    VendingSlot *slot = findSlot(slotId);
    if (slot == nullptr) {
        ESP_LOGW(Tag, "[VEND] Invalid slot %d; no motor movement", slotId);
        xSemaphoreGive(operationMutex);
        return false;
    }

    if (!slot->isAvailable()) {
        ESP_LOGW(Tag, "[VEND] Slot %d rejected; state=%s", slotId, slotStateToString(slot->getState()));
        xSemaphoreGive(operationMutex);
        return false;
    }

    vendInProgress = true;
    machineState = MachineState::VENDING;

    const bool result = slot->vend(timeoutMs);

    machineState = MachineState::READY;
    vendInProgress = false;
    xSemaphoreGive(operationMutex);

    return result;
}

bool VendingController::testMotor(int slotId) {
    ESP_LOGI(Tag, "[VEND] Slot %d motor test requested", slotId);

    if (operationMutex == nullptr || xSemaphoreTake(operationMutex, 0) != pdTRUE) {
        ESP_LOGW(Tag, "[VEND] Motor test rejected; another operation is active");
        return false;
    }

    if (vendInProgress || machineState == MachineState::VENDING) {
        ESP_LOGW(Tag, "[VEND] Motor test rejected; machine is vending");
        xSemaphoreGive(operationMutex);
        return false;
    }

    VendingSlot *slot = findSlot(slotId);
    if (slot == nullptr) {
        ESP_LOGW(Tag, "[VEND] Invalid slot %d; no motor movement", slotId);
        xSemaphoreGive(operationMutex);
        return false;
    }

    vendInProgress = true;
    machineState = MachineState::VENDING;

    const bool result = slot->testMotor(config::MotorTestStepLimit);

    machineState = MachineState::READY;
    vendInProgress = false;
    xSemaphoreGive(operationMutex);

    return result;
}

void VendingController::printStatus() const {
    ESP_LOGI(StatusTag, "[STATUS] Machine=%s Slot1=%s Slot2=%s",
             machineStateToString(machineState),
             slotStateToString(slot1.getState()),
             slotStateToString(slot2.getState()));
}

void VendingController::printSensors() const {
    ESP_LOGI(SensorTag, "[SENSOR] IR1=%s(raw=%s) IR2=%s(raw=%s)",
             slot1.isSensorBlocked() ? "BLOCKED" : "OK",
             slot1.readSensorRaw() ? "HIGH" : "LOW",
             slot2.isSensorBlocked() ? "BLOCKED" : "OK",
             slot2.readSensorRaw() ? "HIGH" : "LOW");
}

MachineState VendingController::getMachineState() const {
    return machineState;
}

bool VendingController::isVendInProgress() const {
    return vendInProgress || machineState == MachineState::VENDING;
}

bool VendingController::getSlotState(int slotId, SlotState *state) const {
    if (state == nullptr) {
        return false;
    }

    const VendingSlot *slot = findSlot(slotId);
    if (slot == nullptr) {
        return false;
    }

    *state = slot->getState();
    return true;
}

bool VendingController::getSensorState(int slotId, bool *beamBroken, bool *rawHigh) const {
    if (beamBroken == nullptr || rawHigh == nullptr) {
        return false;
    }

    const VendingSlot *slot = findSlot(slotId);
    if (slot == nullptr) {
        return false;
    }

    *beamBroken = slot->isSensorBlocked();
    *rawHigh = slot->readSensorRaw();
    return true;
}

VendingSlot *VendingController::findSlot(int slotId) {
    if (slotId == 1) {
        return &slot1;
    }

    if (slotId == 2) {
        return &slot2;
    }

    return nullptr;
}

const VendingSlot *VendingController::findSlot(int slotId) const {
    if (slotId == 1) {
        return &slot1;
    }

    if (slotId == 2) {
        return &slot2;
    }

    return nullptr;
}

const char *machineStateToString(MachineState state) {
    switch (state) {
        case MachineState::BOOTING:
            return "BOOTING";
        case MachineState::WIFI_CONNECTING:
            return "WIFI_CONNECTING";
        case MachineState::MQTT_CONNECTING:
            return "MQTT_CONNECTING";
        case MachineState::READY:
            return "READY";
        case MachineState::VENDING:
            return "VENDING";
        case MachineState::ERROR:
            return "ERROR";
    }

    return "UNKNOWN";
}

}  // namespace vending::domain
