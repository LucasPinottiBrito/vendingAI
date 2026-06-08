#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

namespace vending::sensor {

class IrBreakBeamSensor {
public:
    IrBreakBeamSensor(gpio_num_t gpio, bool activeLow = true);

    esp_err_t init();
    bool isBeamBroken() const;
    bool readRaw() const;

private:
    gpio_num_t gpio;
    bool activeLow;
};

}  // namespace vending::sensor
