#include "event_bus.h"
#include "features.hpp"
#include "fssys.h"
#include "wifi_manager.h"
#include "MqttClient.hpp"
#include "api.h"
#include "LedManager.h"

#ifdef CONFIG_IOT_FEATURE_CAMERA
#include "camera_controller.h"
#include "camera.h"
#endif

#include "esp_log.h"
#include "macro.h"

static void essential_init(void)
{
    INIT_NVS();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI("[BOOT]", "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
}

extern "C" void app_main(void)
{
    essential_init();

    Event evt = {};
    evt.type = EventType::BOOT_CAN_START;
    EventBus::getInstance().emit(evt);

    MqttClient::getInstance().registerMqttActuator(
        "iot/led",
        "iot/response",
        [](const cJSON *root, cJSON *resp)
        {
            cJSON *get = cJSON_GetObjectItemCaseSensitive(root, "get");
            cJSON *led = cJSON_GetObjectItemCaseSensitive(resp, "led");

            if (get)
            { // GET = demande d'état
                bool isOn = LedManager::getInstance().isOn();
                if (led)
                    cJSON_SetIntValue(led, isOn);
                else
                    cJSON_AddNumberToObject(resp, "led", isOn);
                ESP_LOGI("[LED]", "MQTT GET - LED state is %d", isOn);
            }
            else
            { // SET = changement d'état
                if (!led || !cJSON_IsNumber(led))
                {
                    ESP_LOGE("[LED]", "LED invalid");
                    return;
                }
                int value = led->valueint;
                if (value == 1)
                {
                    LedManager::getInstance().on();
                    cJSON_SetIntValue(led, 1);
                }
                else
                {
                    LedManager::getInstance().off();
                    cJSON_SetIntValue(led, 0);
                }
                ESP_LOGI("[LED]", "MQTT SET - LED changed to %d", led->valueint);
            }
        },
        MqttClient::MqttActuatorFilter::SAME_IP);
}