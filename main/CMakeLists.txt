idf_component_register(SRCS "main.cpp" "sd_card/sd_card.cpp"
"wifi_manager/wifi_manager.cpp" "wifi_manager/sta_persistence.cpp" "wifi_manager/sta_persistence_async.cpp" 
"wifi_manager/ap_persistence.cpp" "wifi_manager/ap_persistence_async.cpp" "wifi_manager/wifi_api" 
                    INCLUDE_DIRS "sd_card"
                                 "wifi_manager")
                                 
