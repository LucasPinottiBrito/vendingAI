#pragma once
#include "sensor/IrBreakBeamSensor.hpp"
#include "config.hpp"
#include "freertos/FreeRTOS.h"
namespace vending::sensor {
struct DropSample {
    uint8_t beams = 0;
    int64_t durationUs[4]{};
};
class DropDetector {
public:
    esp_err_t init();
    uint8_t blocked() const;
    uint8_t arm();
    DropSample finish();
private:
    struct Beam {
        DropDetector *owner;
        int index;
        IrBreakBeamSensor sensor;
    };
    Beam beams[4] = {
        {this,0,{config::IrPins[0],config::IrSensorsActiveLow}},
        {this,1,{config::IrPins[1],config::IrSensorsActiveLow}},
        {this,2,{config::IrPins[2],config::IrSensorsActiveLow}},
        {this,3,{config::IrPins[3],config::IrSensorsActiveLow}}
    };
    portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
    bool armed = false;
    DropSample result{};
    int64_t began[4]{};
    static void edge(void *arg);
};
}
