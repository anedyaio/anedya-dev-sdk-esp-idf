#include "anedya.h"

anedya_err_t anedya_bind_device(anedya_client_t *client, anedya_bind_secret_t secret)
{
    // Check whether MQTT is connected or not.
    if (!client->is_connected)
    {
        // Start MQTT connection
        anedya_client_connect(client);
    }
    return ANEDYA_OK;
}