#pragma once
#ifndef __SD_CARD_H__
#define __SD_CARD_H__   
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

namespace sd_card
{
    static constexpr const char* TAG = "SD_CARD";
    extern esp_err_t g_sd_status;

    void init_task(void*);
    void start_async_init();
    
}

#endif