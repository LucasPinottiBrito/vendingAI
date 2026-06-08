#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "esp_err.h"

namespace vending::motor {

class StepperMotor {
public:
    StepperMotor(gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4);

    esp_err_t init();
    void stepForward();
    void stepBackward();
    void release();
    void setStepDelayMs(uint32_t delayMs);

private:
    gpio_num_t pins[4];
    int currentStep;
    uint32_t stepDelayMs;

    void writeStep(int stepIndex);
};

}  // namespace vending::motor
