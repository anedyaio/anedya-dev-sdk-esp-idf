FILE(GLOB AnedyaSDKSource ./src/*.c)
idf_component_register(SRCS ${AnedyaSDKSource}
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_event esp_netif esp-tls mqtt)