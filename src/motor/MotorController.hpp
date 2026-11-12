#pragma once
#include "config.hpp"
#include "esp_err.h"
namespace vending::motor {
enum class Cycle { Running, Complete, Timeout };
class MotorController {
public:
    esp_err_t init();
    bool start(int id, uint32_t timeoutMs);
    Cycle update();
    void stopAll();
    static bool installed(int id);
private:
    int active = 0;
    int64_t stopAt = 0;
    Cycle stopResult = Cycle::Complete;
    void shift(uint16_t word) const;
};
}
