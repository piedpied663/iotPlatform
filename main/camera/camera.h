#pragma once
#ifndef __IOT_CAMERA_H__
#define __IOT_CAMERA_H__

#include "esp_camera.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "camera_controller.h"
#include "event_bus.h"

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif
// ESP32S3 (WROOM) PIN Map
#ifdef BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif

#ifdef CONFIG_OV2640_SUPPORT
#define CAM_MODEL OV2640
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#endif

#ifndef CONFIG_IOT_FILE_PATH_CAMERA
#define CONFIG_IOT_FILE_PATH_CAMERA "/sdcard/camera.txt"
#endif

#ifndef CONFIG_IOT_BACKEND_CAMERA_URL
#define CONFIG_IOT_BACKEND_CAMERA_URL "/iot/camera"
#endif

#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <esp_http_server.h>
#include <sys/socket.h>
#include <unistd.h>

namespace camera
{

    static constexpr const char *TAG = "[IotCamera]";

    // esp_err_t init();
    // __attribute__((noinline))
    // bool startMjpegStream(std::function<bool(const uint8_t *, size_t)> , size_t);

    // esp_err_t capture_handler(httpd_req_t *req);
    // void stream_task(void *arg);
    // esp_err_t stream_handler_s(httpd_req_t *req);

    // esp_err_t stream_handler(httpd_req_t *req);

    void stream_socket_task(void *arg);
    void mjpeg_socket_server_task(void *arg);
    static bool stream_socket_server_started = false;
    static bool wifi_connected = false;
    static bool camera_init = false;

    static void on_event(const Event &event)
    {
        if (event.type == EventType::WIFI_STA_CONNECTED)
        {
            wifi_connected = true;
        }else if (event.type == EventType::WIFI_STA_DISCONNECTED)
        {
            wifi_connected = false;
        }else if (event.type == EventType::CAMERA_INIT_DONE)
        {
            camera_init = true;
        }else if(camera_init && wifi_connected && !stream_socket_server_started)
        {
            xTaskCreate(mjpeg_socket_server_task, "mjpeg_socket_server_task", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);
            stream_socket_server_started = true;
        }
    }
    static bool subscribed = false;
    struct register_event_bus
    {
        register_event_bus()
        {
            if (!subscribed)
            {
                EventBus::getInstance().subscribe(on_event, TAG);
                subscribed = true;
            }
        }
    };

    static register_event_bus _register_event_bus;
} // namespace camera
#endif