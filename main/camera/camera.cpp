#include "camera.h"
#include "../macro.h"
#include "camera_controller.h"

using namespace camera_controller;
namespace camera
{
    void stream_socket_task(void *arg)
    {
        int sockfd = (int)(intptr_t)arg;
        char headers[256];

        ESP_LOGI(TAG, "stream task started");

        while (true)
        {
            camera_fb_t *fb = CameraController::getInstance().tryCapture();
            if (!fb)
            {
                ESP_LOGE(TAG, "capture failed");
                break;
            }

            int hdr_len = snprintf(headers, sizeof(headers),
                                   "--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                                   "frame", fb->len);

            if (send(sockfd, headers, hdr_len, 0) < 0 ||
                send(sockfd, fb->buf, fb->len, 0) < 0 ||
                send(sockfd, "\r\n", 2, 0) < 0)
            {
                ESP_LOGW(TAG, "send error, client likely disconnected");
                CameraController::getInstance().releaseCapture(fb);
                break;
            }

            CameraController::getInstance().releaseCapture(fb);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        ESP_LOGI(TAG, "closing stream socket");
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        vTaskDelete(nullptr);
    }

    void mjpeg_socket_server_task(void *arg)
    {
        const int port = 8081;
        int listen_fd, client_fd;
        struct sockaddr_in server_addr, client_addr;
        socklen_t addr_len = sizeof(client_addr);

        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0)
        {
            ESP_LOGE(TAG, "socket failed");
            vTaskDelete(nullptr);
            return;
        }

        int yes = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ||
            listen(listen_fd, 1) < 0)
        {
            ESP_LOGE(TAG, "bind/listen failed");
            close(listen_fd);
            vTaskDelete(nullptr);
            return;
        }

        ESP_LOGI(TAG, "MJPEG server listening on port %d", port);

        while (true)
        {
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0)
            {
                ESP_LOGE(TAG, "accept failed");
                continue;
            }

            ESP_LOGI(TAG, "MJPEG client connected");
            const char *resp =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n\r\n";
            send(client_fd, resp, strlen(resp), 0);

            xTaskCreatePinnedToCore(stream_socket_task, "cam_stream", 8192, (void *)(intptr_t)client_fd, 5, nullptr, tskNO_AFFINITY);
        }

        // never reached
        close(listen_fd);
        vTaskDelete(nullptr);
    }
}