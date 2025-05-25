#pragma once

#include "esp_err.h"
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
class LedManager
{
public:
    static LedManager &getInstance();

    esp_err_t init();
    void on();
    void off();
    void toggle();
    void blink(int times, int ms_on, int ms_off);
    bool isOn();
private:
    LedManager() = default;
    ~LedManager() = default;
    LedManager(const LedManager &) = delete;
    LedManager &operator=(const LedManager &) = delete;
    static void blink_task(void *param);
    TaskHandle_t blink_task_handle_ = nullptr;

    struct BlinkParams
    {
        int times;
        int ms_on;
        int ms_off;
    };
#ifdef CONFIG_IOT_FEATURE_CAMERA
    static constexpr gpio_num_t LED_GPIO_ = GPIO_NUM_4;
#else
    static constexpr gpio_num_t LED_GPIO_ = GPIO_NUM_8;
#endif
};
