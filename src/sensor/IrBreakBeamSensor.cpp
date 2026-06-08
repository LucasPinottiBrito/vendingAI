#include "sensor/IrBreakBeamSensor.hpp"

namespace vending::sensor {

IrBreakBeamSensor::IrBreakBeamSensor(gpio_num_t gpio, bool activeLow)
    : gpio(gpio),
      activeLow(activeLow) {}

esp_err_t IrBreakBeamSensor::init() {
    gpio_config_t ioConfig = {};
    ioConfig.pin_bit_mask = 1ULL << static_cast<uint32_t>(gpio);
    ioConfig.mode = GPIO_MODE_INPUT;
    ioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    ioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConfig.intr_type = GPIO_INTR_DISABLE;

    return gpio_config(&ioConfig);
}

bool IrBreakBeamSensor::isBeamBroken() const {
    const bool rawHigh = readRaw();
    return activeLow ? !rawHigh : rawHigh;
}

bool IrBreakBeamSensor::readRaw() const {
    return gpio_get_level(gpio) != 0;
}

}  // namespace vending::sensor
