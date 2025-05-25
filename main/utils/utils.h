#pragma once
#ifndef __UTILS_H__
#define __UTILS_H__
#include <cstdint>
#include "types.h"
#include "event_bus.h"
#include <string>

namespace utils
{
    uint32_t hash_struct(const void *, size_t);
    std::string make_unique_client_id(const char* base_id);
    template <typename T>
    bool send_struct_if_changed(const T &current, uint32_t &last_hash, EventType type, const char *source = nullptr)
    {
        uint32_t cur_hash = utils::hash_struct(&current, sizeof(T));
        if (cur_hash == last_hash)
            return false;
        last_hash = cur_hash;
        Event evt = {};
        evt.type = type;
        if (source)
            strncpy(evt.source, source, sizeof(evt.source) - 1);
        memcpy(evt.data, &current, sizeof(T));
        evt.data_len = sizeof(T);
        EventBus::getInstance().emit(evt);
        return true;
    }

}

#endif