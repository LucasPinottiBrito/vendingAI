#pragma once

#include <cstdint>

#include "esp_err.h"
#include "motor/StepperMotor.hpp"
#include "sensor/IrBreakBeamSensor.hpp"
#include "vending/VendingSlot.hpp"
#include "vending/VendingTypes.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace vending::domain {

class VendingController {
public:
    VendingController();

    esp_err_t init();
    bool vendSlot(int slotId);
    bool vendSlot(int slotId, uint32_t timeoutMs);
    bool testMotor(int slotId);
    void printStatus() const;
    void printSensors() const;
    MachineState getMachineState() const;
    bool isVendInProgress() const;
    bool getSlotState(int slotId, SlotState *state) const;
    bool getSensorState(int slotId, bool *beamBroken, bool *rawHigh) const;

private:
    motor::StepperMotor motor1;
    motor::StepperMotor motor2;
    sensor::IrBreakBeamSensor sensor1;
    sensor::IrBreakBeamSensor sensor2;
    VendingSlot slot1;
    VendingSlot slot2;
    MachineState machineState;
    bool vendInProgress;
    SemaphoreHandle_t operationMutex;

    VendingSlot *findSlot(int slotId);
    const VendingSlot *findSlot(int slotId) const;
};

}  // namespace vending::domain
