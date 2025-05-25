#include "event_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <algorithm>

// Constructeur Singleton
EventBus::EventBus()
    : evt_queue(nullptr)
{
}

// Accès Singleton (C++11+ thread-safe)
EventBus &EventBus::getInstance()
{
    static EventBus instance;
    if (!instance.evt_queue)
    {
        instance.init();
    }
    return instance;
}

void EventBus::init()
{
    if (evt_queue)
        return;

    evt_queue = xQueueCreate(QUEUE_LEN, sizeof(Event));
    // On lance la tâche FreeRTOS qui dispatch les events (pointeur sur l'instance)
    xTaskCreate([](void *arg)
                { static_cast<EventBus *>(arg)->eventbus_task(); }, "eventbus_task", STACK_SIZE, this, 5, nullptr);
    ESP_LOGI(TAG, "EventBus initialized");
}

void EventBus::emit(const Event &evt)
{
    if (!evt_queue)
    {
        ESP_LOGW(TAG, "EventBus not initialized");
        EventBus::getInstance().init();
        return EventBus::getInstance().emit(evt);
    }

    if (xQueueSend(static_cast<QueueHandle_t>(evt_queue), &evt, 0) != pdTRUE)
    {
        // Option : log overflow
        // ESP_LOGW(TAG, "EventBus overflow (event dropped)");
    }
}

void EventBus::subscribe(const EventCallback &cb, const char *topic)
{
    ESP_LOGI(TAG, "EventBus %s subscribe", topic);
    std::lock_guard<std::mutex> lock(sub_mutex);
    subscribers.push_back(cb);
}

void EventBus::unsubscribe(const EventCallback &cb)
{
    std::lock_guard<std::mutex> lock(sub_mutex);
    // On ne peut pas comparer std::function simplement : laisser vide ou gérer l'ID callback selon besoins
    // std::remove_if, etc, à adapter selon usage réel
}

void EventBus::eventbus_task()
{
    Event evt;
    while (true)
    {
        if (xQueueReceive(static_cast<QueueHandle_t>(evt_queue), &evt, portMAX_DELAY) == pdTRUE)
        {
            std::lock_guard<std::mutex> lock(sub_mutex);
            for (auto &cb : subscribers)
            {
#ifdef CONFIG_COMPILER_CXX_EXCEPTIONS
                try
                {
#endif
                    cb(evt);
#ifdef CONFIG_COMPILER_CXX_EXCEPTIONS
                }
                catch (...)
                {
                    ESP_LOGE(TAG, "Exception in callback!");
                }
#endif
            }
        }
        vTaskDelay(1);
    }

   
}

