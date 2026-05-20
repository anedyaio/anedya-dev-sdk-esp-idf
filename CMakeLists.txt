FILE(GLOB AnedyaSDKSource         ./core/src/*.c
     GLOB AnedyaInterfaceQuectel  ./interfaces/esp32-quectel/*.c
     GLOB AnedyaInterfaceWifi     ./interfaces/esp32-wifi/*.c
     GLOB AnedyaInterfaceWifiQuectel  ./interfaces/esp32-wifi-quectel/*.c)
idf_component_register(SRCS ${AnedyaSDKSource} ${AnedyaInterfaceQuectel} ${AnedyaInterfaceWifi} ${AnedyaInterfaceWifiQuectel}
                    INCLUDE_DIRS "./core/include" "./interfaces/include"
                    PRIV_REQUIRES esp_event esp_netif esp-tls mqtt driver)

# Handle embedded certificates
if(CONFIG_AN_INTERFACE_ESP32_WIFI)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ANEDYA_EMBED_DER)
endif()

if(CONFIG_AN_INTERFACE_ESP32_QUETEL)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ANEDYA_EMBED_PEM)
endif()

if(CONFIG_AN_INTERFACE_ESP32_WIFI_QUETEL)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ANEDYA_EMBED_PEM)
endif()