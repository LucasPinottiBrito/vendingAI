#include "vending/VendingController.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/task.h"
namespace vending::domain {
esp_err_t VendingController::init() {
    esp_err_t err = motor.init();
    if (err != ESP_OK) return err;
    mutex = xSemaphoreCreateMutex();
    if (!mutex) return ESP_ERR_NO_MEM;
    err = detector.init();
    if (err != ESP_OK) return err;
    err = nvs_flash_init();
    if (err != ESP_OK) return err;
    err = nvs_open("motors", NVS_READWRITE, &storage);
    if (err != ESP_OK) return err;
    uint16_t saved = 0;
    err = nvs_get_u16(storage, "blocked", &saved);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    blocked = saved;
    state = MachineState::READY;
    return ESP_OK;
}
bool VendingController::setBlocked(uint16_t value) {
    if (nvs_set_u16(storage,"blocked",value) != ESP_OK || nvs_commit(storage) != ESP_OK) {
        state = MachineState::ERROR;
        return false;
    }
    blocked = value;
    return true;
}
VendResult VendingController::run(int id, uint32_t timeout, int attempts, bool observeDrop,
                                  const std::function<void(int)> &retry) {
    VendResult result;
    if (xSemaphoreTake(mutex,0) != pdTRUE) return result;
    if (state != MachineState::READY || !motor.installed(id) || (blocked & (1u << (id-1)))) {
        result.reason = "CHANNEL_UNAVAILABLE";
    } else {
        state = MachineState::VENDING;
        const uint16_t bit = 1u << (id-1);
        for (int i = 1; i <= attempts; ++i) {
            // Persist uncertain position before power is applied, including reset mid-cycle.
            if (!setBlocked(blocked | bit)) { result.reason = "STORAGE_ERROR"; break; }
            if (observeDrop && detector.arm()) {
                result.reason = "BARRIER_BLOCKED";
                printSensors();
                setBlocked(blocked & ~bit);
                break;
            }
            if (!motor.start(id,timeout)) {
                detector.finish();
                setBlocked(blocked & ~bit);
                result.reason = "START_FAILED";
                break;
            }
            selected = id;
            result.attempts = i;
            motor::Cycle cycle;
            do {
                cycle = motor.update();
                if (cycle == motor::Cycle::Running) vTaskDelay(1);
            } while (cycle == motor::Cycle::Running);
            selected = 0;
            result.mechanicalFault = cycle == motor::Cycle::Timeout;
            if (observeDrop) {
                const auto end = esp_timer_get_time() + 1000LL * config::DropWindowMs;
                while (esp_timer_get_time() < end) vTaskDelay(1);
                const auto sample = detector.finish();
                result.delivered = sample.beams != 0;
                ESP_LOGI("DROP","motor=%d beams=0x%x last pulse us=%lld,%lld,%lld,%lld",
                         id,sample.beams,sample.durationUs[0],sample.durationUs[1],
                         sample.durationUs[2],sample.durationUs[3]);
            }
            if (result.mechanicalFault) { result.reason = "MOTOR_TIMEOUT"; break; }
            if (!setBlocked(blocked & ~bit)) { result.reason = "STORAGE_ERROR"; break; }
            if (result.delivered || !observeDrop) { result.reason = "COMPLETE"; break; }
            result.reason = "PRODUCT_NOT_DETECTED";
            if (detector.blocked() || i == attempts) break;
            if (retry) retry(i+1);
        }
        if (state != MachineState::ERROR) state = MachineState::READY;
    }
    lastReason = result.reason;
    ESP_LOGI("VEND","motor=%d attempts=%d delivered=%d reason=%s",id,result.attempts,result.delivered,result.reason);
    xSemaphoreGive(mutex);
    return result;
}
VendResult VendingController::vendSlot(int id,uint32_t timeout,int attempts,std::function<void(int)> retry) {
    return run(id,timeout,attempts,true,retry);
}
void VendingController::testMotor(int id) { run(id,config::VendTimeoutMs,1,false,{}); }
void VendingController::recover(int id) {
    if (!motor.installed(id) || xSemaphoreTake(mutex,0) != pdTRUE) return;
    if (state == MachineState::READY) setBlocked(blocked & ~(1u << (id-1)));
    xSemaphoreGive(mutex);
}
void VendingController::printStatus() {
    ESP_LOGI("STATUS","state=%s motor=%d mode=timed run_ms=%lu blocked=0x%04x last=%s",
             machineStateToString(state.load()),selected.load(),
             static_cast<unsigned long>(config::MotorRunMs),blocked.load(),lastReason.load());
}
void VendingController::printSensors() const {
    ESP_LOGI("IR","blocked mask=0x%x (bit0=IR1 .. bit3=IR4)",detector.blocked());
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
