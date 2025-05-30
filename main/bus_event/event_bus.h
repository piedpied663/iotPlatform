#pragma once
#include <functional>
#include <cstdint>
#include <cstring>
#include <vector>
#include <mutex>
#include "types.h"

using EventCallback = std::function<void(const Event *)>;
#define STACK_SIZE 8192
class EventBus
{
public:
    static EventBus& getInstance();

    void init();
    void emit(const Event &evt);
    void subscribe(const EventCallback& cb, const char* topic);
    void unsubscribe(const EventCallback& cb);

    // Désactive la copie/assignation
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

private:
    EventBus();  // constructeur privé
    ~EventBus() = default;

    // Membres privés équivalents à tes statics d'avant :
    static constexpr const char *TAG = "BUS_EVENT";
    static constexpr size_t QUEUE_LEN = 10;
    void eventbus_task();

    // Variables membres
    void *evt_queue; // On utilise QueueHandle_t, mais on forward-déclare en void* pour ne pas inclure FreeRTOS dans le header
    std::vector<EventCallback> subscribers;
    std::mutex sub_mutex;

};
