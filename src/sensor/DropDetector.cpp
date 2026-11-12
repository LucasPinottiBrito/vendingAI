#include "sensor/DropDetector.hpp"
#include "esp_timer.h"
namespace vending::sensor {
esp_err_t DropDetector::init() {
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    for (auto &beam : beams) {
        err = beam.sensor.init();
        if (err != ESP_OK) return err;
        auto pin = config::IrPins[beam.index];
        err = gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
        if (err != ESP_OK) return err;
        err = gpio_isr_handler_add(pin, edge, &beam);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
uint8_t DropDetector::blocked() const {
    uint8_t mask = 0;
    for (const auto &b : beams) if (b.sensor.isBeamBroken()) mask |= 1u << b.index;
    return mask;
}
uint8_t DropDetector::arm() {
    portENTER_CRITICAL(&lock);
    const auto mask = blocked();
    result = {};
    for (auto &stamp : began) stamp = 0;
    armed = mask == 0;
    portEXIT_CRITICAL(&lock);
    return mask;
}
void DropDetector::edge(void *arg) {
    auto &beam = *static_cast<Beam *>(arg);
    auto &self = *beam.owner;
    portENTER_CRITICAL_ISR(&self.lock);
    if (self.armed) {
        const auto now = esp_timer_get_time();
        auto &start = self.began[beam.index];
        if (beam.sensor.isBeamBroken()) {
            self.result.beams |= 1u << beam.index;
            start = now;
        } else if (start) {
            self.result.durationUs[beam.index] = now - start;
            start = 0;
        }
    }
    portEXIT_CRITICAL_ISR(&self.lock);
}
DropSample DropDetector::finish() {
    portENTER_CRITICAL(&lock);
    armed = false;
    auto snapshot = result;
    portEXIT_CRITICAL(&lock);
    return snapshot;
}
}
