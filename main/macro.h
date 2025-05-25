#pragma once
#ifndef __MACRO_H__
#define __MACRO_H__
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"

using http_handler_t = esp_err_t (*)(httpd_req_t *);

/*
 *
 */
#define HANDLE_OPTIONS(req)                   \
    if (req->method == HTTP_OPTIONS)          \
    {                                         \
        httpd_resp_set_status(req, "200 OK"); \
        httpd_resp_send(req, NULL, 0);        \
    }                                         \
/**                                           \
 *                                            \
 */
#define SET_CORS_HEADERS(req)                                                                            \
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");                                         \
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization, Credentials"); \
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");                       \
    /**                                                                                                  \
     *                                                                                                   \
     */
#define SET_RESP_HEADERS(req)                     \
    SET_CORS_HEADERS(req);                        \
    httpd_resp_set_type(req, "application/json"); \
/**                                               \
 *                                                \
 */
/**
 *
 */
#define HTTP_RECV_BODY_OR_FAIL(req, body_var, log_tag, error_msg) \
    int __len = req->content_len;                                 \
    std::string body_var(__len, '\0');                            \
    if (httpd_req_recv(req, &body_var[0], __len) <= 0)            \
    {                                                             \
        ESP_LOGE(log_tag, "%s", error_msg);                       \
        httpd_resp_send_500(req);                                 \
        return ESP_FAIL;                                          \
    }
/**
 *
 */
#define INIT_NVS()                                                                    \
    do                                                                                \
    {                                                                                 \
        esp_err_t ret = nvs_flash_init();                                             \
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) \
        {                                                                             \
            nvs_flash_erase();                                                        \
            nvs_flash_init();                                                         \
        }                                                                             \
    } while (0)
/**
 *
 *
 */

static void logWatermark(const char *frm)
{
    ESP_LOGW(frm, "Task %s high water mark = %u bytes", frm,
             uxTaskGetStackHighWaterMark(nullptr) * sizeof(uint32_t));
}
#define END_TASK(x, y)                \
    {                                 \
        logWatermark(y);              \
        vTaskDelete(x ? x : nullptr); \
        return;                       \
    }

#define BIND_DATA_EVENT_JSON(evt, buf)  \
    do                                  \
    {                                   \
        size_t n = strlen(buf);         \
        if (n >= sizeof((evt).data))    \
            n = sizeof((evt).data) - 1; \
        memcpy((evt).data, (buf), n);   \
        (evt).data[n] = 0;              \
        (evt).data_len = n;             \
    } while (0)

#endif