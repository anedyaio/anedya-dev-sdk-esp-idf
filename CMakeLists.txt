FILE(GLOB AnedyaSDKSource         ./core/src/*.c
     GLOB AnedyaInterfaceQuectel  ./interfaces/esp32-quectel/*.c
     GLOB AnedyaInterfaceWifi     ./interfaces/esp32-wifi/*.c)
idf_component_register(SRCS ${AnedyaSDKSource} ${AnedyaInterfaceQuectel} ${AnedyaInterfaceWifi}
                    INCLUDE_DIRS "./core/include" "./interfaces/include"
                    PRIV_REQUIRES esp_event esp_netif esp-tls mqtt driver)