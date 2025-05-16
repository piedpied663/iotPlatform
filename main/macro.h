#pragma once
#ifndef __MACRO_H__
#define __MACRO_H__

#define INIT_NVS() do { \
    esp_err_t ret = nvs_flash_init(); \
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { \
        nvs_flash_erase(); \
        nvs_flash_init(); \
    } \
} while(0)

#endif