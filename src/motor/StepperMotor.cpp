#include "motor/StepperMotor.hpp"

#include "config.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace vending::motor {

namespace {

constexpr int StepCount = 8;
constexpr int CoilCount = 4;

constexpr int HalfStepSequence[StepCount][CoilCount] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
};

}  // namespace

StepperMotor::StepperMotor(gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4)
    : pins{in1, in2, in3, in4},
      currentStep(0),
      stepDelayMs(config::MotorStepDelayMs) {}

esp_err_t StepperMotor::init() {
    uint64_t pinMask = 0;
    for (gpio_num_t pin : pins) {
        pinMask |= (1ULL << static_cast<uint32_t>(pin));
    }

    gpio_config_t ioConfig = {};
    ioConfig.pin_bit_mask = pinMask;
    ioConfig.mode = GPIO_MODE_OUTPUT;
    ioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    ioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConfig.intr_type = GPIO_INTR_DISABLE;

    const esp_err_t result = gpio_config(&ioConfig);
    if (result == ESP_OK) {
        release();
    }

    return result;
}

void StepperMotor::stepForward() {
    currentStep = (currentStep + 1) % StepCount;
    writeStep(currentStep);
    vTaskDelay(pdMS_TO_TICKS(stepDelayMs));
}

void StepperMotor::stepBackward() {
    currentStep = (currentStep + StepCount - 1) % StepCount;
    writeStep(currentStep);
    vTaskDelay(pdMS_TO_TICKS(stepDelayMs));
}

void StepperMotor::release() {
    for (gpio_num_t pin : pins) {
        gpio_set_level(pin, 0);
    }
}

void StepperMotor::setStepDelayMs(uint32_t delayMs) {
    stepDelayMs = delayMs;
}

void StepperMotor::writeStep(int stepIndex) {
    const int sequenceIndex = ((stepIndex % StepCount) + StepCount) % StepCount;
    for (int i = 0; i < CoilCount; ++i) {
        gpio_set_level(pins[i], HalfStepSequence[sequenceIndex][i]);
    }
}

}  // namespace vending::motor
