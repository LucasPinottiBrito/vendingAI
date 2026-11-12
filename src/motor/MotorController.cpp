#include "motor/MotorController.hpp"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include <algorithm>
namespace vending::motor {
using namespace config;
namespace {
void pulse(gpio_num_t pin) {
    esp_rom_delay_us(1);
    gpio_set_level(pin, 1);
    esp_rom_delay_us(1);
    gpio_set_level(pin, 0);
}
esp_err_t configure(uint64_t mask, gpio_mode_t mode) {
    gpio_config_t cfg{};
    cfg.pin_bit_mask = mask;
    cfg.mode = mode;
    return gpio_config(&cfg);
}
}
esp_err_t MotorController::init() {
    gpio_set_level(Enable, 0);
    esp_err_t err = configure(1ULL << Enable, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) return err;
    uint64_t mask = 0;
    for (auto pin : {Data, Clock, Latch}) {
        gpio_set_level(pin, 0);
        mask |= 1ULL << pin;
    }
    err = configure(mask, GPIO_MODE_OUTPUT);
    if (err == ESP_OK) stopAll();
    return err;
}
void MotorController::shift(uint16_t word) const {
    for (int bit = 15; bit >= 0; --bit) {
        gpio_set_level(Data, (word >> bit) & 1);
        pulse(Clock);
    }
    pulse(Latch);
}
bool MotorController::installed(int id) {
    return id >= 1 && id <= 16 && Channels[id - 1].installed;
}
bool MotorController::start(int id, uint32_t timeoutMs) {
    if (active || !installed(id) || !timeoutMs) return false;
    active = id;
    const uint32_t limitMs = std::min(timeoutMs, VendTimeoutMs);
    stopResult = MotorRunMs <= limitMs ? Cycle::Complete : Cycle::Timeout;
    shift(uint16_t(1u << Channels[id - 1].outputBit));
    gpio_set_level(Enable, 1);
    stopAt = esp_timer_get_time() + 1000LL * std::min(MotorRunMs, limitMs);
    return true;
}
Cycle MotorController::update() {
    if (!active) return Cycle::Complete;
    if (esp_timer_get_time() >= stopAt) {
        stopAll();
        return stopResult;
    }
    return Cycle::Running;
}
void MotorController::stopAll() {
    gpio_set_level(Enable, 0);
    shift(0);
    active = 0;
}
}
