#pragma once
#include <cstdint>
#include <cstdio>
#include <functional>
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_ERR_NO_MEM=1, ESP_ERR_INVALID_STATE=2, ESP_ERR_NVS_NOT_FOUND=3;
using gpio_num_t=int;
enum {GPIO_NUM_1=1, GPIO_NUM_2=2, GPIO_NUM_4=4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10,
 GPIO_NUM_11,GPIO_NUM_12,GPIO_NUM_13,GPIO_NUM_14,GPIO_NUM_15,GPIO_NUM_16,GPIO_NUM_17, GPIO_NUM_40=40, GPIO_NUM_41, GPIO_NUM_42};
using gpio_mode_t=int;
enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE,GPIO_INTR_ANYEDGE};
struct gpio_config_t {uint64_t pin_bit_mask; int mode,pull_up_en,pull_down_en,intr_type;};
inline constexpr int GpioCount = 49;
inline int levels[GpioCount]{}, directions[GpioCount]{};
inline uint16_t shiftWord{}, latched{}, feedback=0xffff, serial{};
inline int64_t micros=1000;
inline int64_t motorOnAt{}, motorOffAt{};
inline unsigned motorStarts{}, feedbackReads{}, feedbackWrites{};
inline void (*handlers[GpioCount])(void*){};
inline void* args[GpioCount]{};
inline int gpio_config(const gpio_config_t* c){
 for(int i=0;i<GpioCount;i++) if(c->pin_bit_mask&(1ULL<<i)) directions[i]=c->mode;
 return 0;
}
inline int gpio_set_level(int pin,int level){
 if(pin==15 || pin==16) ++feedbackWrites;
 if(pin==15 && !level) serial=feedback;
 if(level && !levels[pin]){
   if(pin==5) shiftWord=(shiftWord<<1)|levels[4];
   if(pin==6) latched=shiftWord;
   if(pin==16) serial<<=1;
 }
 bool changed=levels[pin]!=level;
 if(changed && pin==GPIO_NUM_1) {
   if(level) {motorOnAt=micros;++motorStarts;}
   else motorOffAt=micros;
 }
 levels[pin]=level;
 if(changed && handlers[pin]) handlers[pin](args[pin]);
 return 0;
}
inline int gpio_get_level(int p){if(p==17) ++feedbackReads;return p==17 ? serial>>15 : levels[p];}
inline int gpio_install_isr_service(int){return 0;}
inline int gpio_set_intr_type(int,int){return 0;}
inline int gpio_isr_handler_add(int p,void(*f)(void*),void*a){handlers[p]=f;args[p]=a;return 0;}
inline int64_t esp_timer_get_time(){return micros;}
inline void esp_rom_delay_us(int n){micros+=n;}
inline std::function<void()> tick;
inline void vTaskDelay(int){micros+=1000;if(tick)tick();}
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
#define portENTER_CRITICAL_ISR(x) ((void)0)
#define portEXIT_CRITICAL_ISR(x) ((void)0)
using SemaphoreHandle_t=bool*;
inline SemaphoreHandle_t xSemaphoreCreateMutex(){static bool free=true;return &free;}
inline bool xSemaphoreTake(bool*m,int){if(!*m)return false;*m=false;return true;}
inline void xSemaphoreGive(bool*m){*m=true;}
constexpr bool pdTRUE=true;
using nvs_handle_t=int;
inline uint16_t saved=0;
constexpr int NVS_READWRITE=0;
inline int nvs_flash_init(){return 0;}
inline int nvs_open(const char*,int,int*){return 0;}
inline int nvs_get_u16(int,const char*,uint16_t*p){*p=saved;return 0;}
inline int nvs_set_u16(int,const char*,uint16_t v){saved=v;return 0;}
inline int nvs_commit(int){return 0;}
#define ESP_LOGI(tag,fmt,...) ((void)0)
