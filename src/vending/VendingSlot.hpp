#pragma once

#include <cstdint>

#include "motor/StepperMotor.hpp"
#include "sensor/IrBreakBeamSensor.hpp"
#include "vending/VendingTypes.hpp"

namespace vending::domain {

class VendingSlot {
public:
    VendingSlot(int slotId, motor::StepperMotor &motor, sensor::IrBreakBeamSensor &sensor);

    bool vend(uint32_t timeoutMs);
    bool testMotor(uint32_t stepLimit);
    bool isAvailable() const;
    bool isSensorBlocked() const;
    bool readSensorRaw() const;
    int getId() const;
    SlotState getState() const;

private:
    int slotId;
    motor::StepperMotor &motor;
    sensor::IrBreakBeamSensor &sensor;
    SlotState state;
};

}  // namespace vending::domain
