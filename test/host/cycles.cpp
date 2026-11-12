#include "vending/VendingController.hpp"
#include <cassert>
#include <cstring>
using namespace vending;
int main() {
    for (auto &direction : directions) direction = -1;
    motor::MotorController driver;
    assert(driver.init()==ESP_OK && !levels[config::Enable] && latched==0);
    for (int pin : {15,16,17}) assert(directions[pin]==-1);
    for (int id : {0,4,17}) assert(!driver.start(id,config::VendTimeoutMs));
    assert(!driver.start(1,0));
    // Absent, stuck or changing feedback cannot end the timed pulse early.
    for (int id=1;id<=3;id++) {
        feedback = id % 2 ? 0 : 0xffff;
        assert(driver.start(id,config::VendTimeoutMs) && levels[config::Enable] && latched==(1u<<(id-1)));
        assert(!driver.start(1,config::VendTimeoutMs));
        const auto started=micros;
        micros=started+1000LL*config::MotorRunMs-1;
        assert(driver.update()==motor::Cycle::Running && levels[config::Enable]);
        feedback^=0xffff;
        ++micros;
        assert(driver.update()==motor::Cycle::Complete && !levels[config::Enable] && !latched);
        assert(motorOffAt-motorOnAt==1000LL*config::MotorRunMs);
        assert(driver.update()==motor::Cycle::Complete);
    }
    // Equal duration/timeout completes; a shorter timeout aborts the attempt.
    assert(driver.start(1,config::MotorRunMs));
    micros+=1000LL*config::MotorRunMs;
    assert(driver.update()==motor::Cycle::Complete && !levels[config::Enable] && !latched);
    assert(driver.start(1,50));
    micros+=49999;
    assert(driver.update()==motor::Cycle::Running);
    ++micros;
    assert(driver.update()==motor::Cycle::Timeout && !levels[config::Enable] && !latched);
    assert(driver.start(1,UINT32_MAX));
    driver.stopAll();
    assert(!levels[config::Enable] && !latched && driver.update()==motor::Cycle::Complete);
    for(auto pin:config::IrPins) levels[pin]=1;
    domain::VendingController controller;
    assert(controller.init()==ESP_OK);
    assert(directions[1]==GPIO_MODE_OUTPUT);
    for (int pin : {2,42,41,40}) assert(directions[pin]==GPIO_MODE_INPUT && handlers[pin]);
    for (int pin : {7,13,14,9,10}) assert(directions[pin]==-1 && !handlers[pin]);
    gpio_set_level(config::IrPins[0],0);
    assert(!controller.vendSlot(3).attempts && !levels[config::Enable]);
    gpio_set_level(config::IrPins[0],1);
    // Short IR pulses confirm delivery without ending the motor pulse early.
    auto start=micros;
    bool emitted=false;
    tick=[&] {
        const auto elapsed=micros-start;
        if(elapsed<1000LL*config::MotorRunMs) assert(levels[config::Enable]);
        if(elapsed>35000 && !emitted) {
            emitted=true;
            assert(!controller.vendSlot(1).attempts && levels[config::Enable] && latched==4);
            gpio_set_level(config::IrPins[1],0); micros+=80; gpio_set_level(config::IrPins[1],1);
            gpio_set_level(config::IrPins[3],0); micros+=40; gpio_set_level(config::IrPins[3],1);
        }
    };
    auto sale=controller.vendSlot(3,config::VendTimeoutMs,2);
    assert(sale.delivered && !sale.mechanicalFault && sale.attempts==1 && saved==0);
    assert(motorOffAt-motorOnAt>=1000LL*config::MotorRunMs);
    assert(motorOffAt-motorOnAt<1000LL*config::MotorRunMs+1000);
    assert(!std::strcmp(sale.reason,"COMPLETE"));
    // Early safety timeout, then a late drop: delivered once, channel stays blocked.
    bool late=false;
    tick=[&]{if(!levels[config::Enable] && !late){late=true;gpio_set_level(config::IrPins[2],0);micros+=90;gpio_set_level(config::IrPins[2],1);}};
    sale=controller.vendSlot(3,50,2);
    assert(sale.delivered && sale.mechanicalFault && sale.attempts==1 && saved==4);
    assert(!std::strcmp(sale.reason,"MOTOR_TIMEOUT"));
    assert(!controller.vendSlot(3).attempts && !levels[config::Enable] && !latched);
    controller.recover(3);
    assert(!saved);
    // No drop: two complete timed pulses, stopped before the retry.
    int retries=0;
    const auto starts=motorStarts;
    tick={};
    sale=controller.vendSlot(3,config::VendTimeoutMs,2,[&](int next){
        assert(next==2 && !levels[config::Enable] && !latched);
        assert(motorOffAt-motorOnAt>=1000LL*config::MotorRunMs);
        ++retries;
    });
    assert(!sale.delivered && !sale.mechanicalFault && sale.attempts==2 && retries==1);
    assert(!std::strcmp(sale.reason,"PRODUCT_NOT_DETECTED"));
    assert(motorStarts==starts+2 && !levels[config::Enable] && !latched && !saved);
    controller.printStatus();
    assert(!feedbackReads && !feedbackWrites);
    for (int pin : {15,16,17}) assert(directions[pin]==-1);
    puts("PASS: timed duration, selection, no feedback IO, safety timeout, short IR, concurrency, late drop, lockout and retry");
}
