FILE(GLOB AnedyaSDKSource ./core/src/*.c)
idf_component_register(SRCS ${AnedyaSDKSource}
                    INCLUDE_DIRS "./core/include"
                    PRIV_REQUIRES esp_event esp_netif esp-tls mqtt)