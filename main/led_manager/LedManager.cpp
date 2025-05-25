#include "LedManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "LedManager";
static bool initialized = false;

LedManager &LedManager::getInstance()
{
    static LedManager instance;
    return instance;
}

esp_err_t LedManager::init()
{
    if (initialized)
    {
        return ESP_OK;
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(LED_GPIO_));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_direction(LED_GPIO_, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(LED_GPIO_, 0));
    initialized = true;
    return ESP_OK;
}

void LedManager::on()
{
    if(!initialized) {
        init();
    };
    gpio_set_level(LED_GPIO_, 1); // LED ON
}

void LedManager::off()
{
    if(!initialized) {
        init();
    };
    gpio_set_level(LED_GPIO_, 0); // LED OFF
}

void LedManager::toggle()
{
    if(!initialized) {
        init();
    };
    gpio_set_level(LED_GPIO_, !gpio_get_level(LED_GPIO_));
}

void LedManager::blink(int times, int ms_on, int ms_off)
{
    if (blink_task_handle_ != nullptr)
    {
        // Si déjà en cours, on ignore (ou tu peux vouloir cancel / restart)
        return;
    }
    if(!initialized) {
        init();
    };

    auto *params = new BlinkParams{times, ms_on, ms_off};
    xTaskCreate(blink_task, "led_blink_task", 2048, params, 5, &blink_task_handle_);
}

bool LedManager::isOn()
{
    if(!initialized) {
        init();
    };
    return gpio_get_level(LED_GPIO_);
}

void LedManager::blink_task(void *param)
{
    auto *params = static_cast<BlinkParams *>(param);
    auto &led = LedManager::getInstance();

    for (int i = 0; i < params->times; i++)
    {
        gpio_set_level(LED_GPIO_, 1);
        vTaskDelay(pdMS_TO_TICKS(params->ms_on));
        gpio_set_level(LED_GPIO_, 0);
        vTaskDelay(pdMS_TO_TICKS(params->ms_off));
    }

    delete params;
    led.blink_task_handle_ = nullptr;
    vTaskDelete(nullptr); // Termine la tâche
}