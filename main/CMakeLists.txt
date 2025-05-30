idf_component_register(SRCS "main.cpp" "utils/utils.cpp" "bus_event/event_bus.cpp" "filesystem/Fs.cpp" "filesystem/Sd.cpp" "persistence/persistence.cpp"
                        "wifi_manager/WiFiConfig.cpp"  "wifi_manager/WiFiManager.cpp" "wifi_manager/WiFiScanner.cpp" "ota/ota.cpp"
                            "api/api.cpp" "led_manager/LedManager.cpp" "camera/camera_api.cpp" "camera/camera_controller.cpp" "camera/camera.cpp"
                    INCLUDE_DIRS . "features" "bus_event" "filesystem" "wifi_manager" "utils" "persistence" "api" "led_manager" "mqtt_client" "camera" "ota")
                                 
