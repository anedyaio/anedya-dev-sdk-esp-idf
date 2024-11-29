#include <stdio.h>
#include "anedya.h"
#include "anedya_certs.h"
#include "anedya_json_parse.h"
#include "anedya_operations.h"
#include "anedya_op_commands.h"
#include "string.h"
#include "sys/time.h" // TODO: Remove time header dependency

anedya_err_t anedya_client_init(anedya_config_t *config, anedya_client_t *client)
{
    client->config = config;
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
    client->tx_buffer = anedya_interface_allocate_memory(config->tx_buffer_size);
    if (client->tx_buffer == NULL)
    {
        return ANEDYA_ERR_NO_MEMORY;
    }
    client->rx_buffer = anedya_interface_allocate_memory(config->rx_buffer_size);
    if (client->rx_buffer == NULL)
    {
        return ANEDYA_ERR_NO_MEMORY;
    }
#ifdef ANEDYA_ENABLE_DEVICE_LOGS
    client->log_buffer = anedya_interface_allocate_memory(config->log_buffer_size * config->log_batch_size);
    if (client->log_buffer == NULL)
    {
        return ANEDYA_ERR_NO_MEMORY;
    }
#endif
#endif

// Compute the broker url
#ifdef ANEDYA_CONNECTION_METHOD_MQTT
    char url[100];
    sprintf(url, "mqtt.%s.anedya.io", config->region);
    strcpy(client->broker_url, url);
    anedya_mqtt_client_handle_t handle = _anedya_interface_mqtt_init(client, client->broker_url, client->config->_device_id_str, client->config->connection_key);
    client->mqtt_client = handle;
    client->_message_handler = _anedya_message_handler;
    client->_anedya_on_connect_handler = _anedya_on_connect_handler;
    client->_anedya_on_disconnect_handler = _anedya_on_disconnect_handler;
    // Initialize txn store
    anedya_err_t err = _anedya_txn_store_init(&client->txn_store);
    if (err != ANEDYA_OK)
    {
        return err;
    }
#endif

    char topic_prefix[100];
    strcpy(topic_prefix, "$anedya/device/");
    strcat(topic_prefix, config->_device_id_str);
    strcat(topic_prefix, "/");

    strcpy(client->_message_topics[0], topic_prefix);
    strcat(client->_message_topics[0], "response");

    strcpy(client->_message_topics[1], topic_prefix);
    strcat(client->_message_topics[1], "errors");

    strcpy(client->_message_topics[2], topic_prefix);
    strcat(client->_message_topics[2], "commands");

    strcpy(client->_message_topics[3], topic_prefix);
    strcat(client->_message_topics[3], "valuestore/updates/json");

    return ANEDYA_OK;
}

#ifdef ANEDYA_CONNECTION_METHOD_MQTT
anedya_err_t anedya_client_connect(anedya_client_t *client)
{
    anedya_err_t err;
    err = anedya_interface_mqtt_connect(client->mqtt_client);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_client_disconnect(anedya_client_t *client)
{
    anedya_err_t err;
    // Set is_connected to 0 to signify intentional shutdown
    client->is_connected = 0;
    err = anedya_interface_mqtt_disconnect(client->mqtt_client);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_client_destroy(anedya_client_t *client) {
    anedya_err_t err;
    err = anedya_interface_mqtt_destroy(client->mqtt_client);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

#endif

anedya_err_t _anedya_txn_store_init(anedya_txn_store_t *store)
{
    for (int i = 0; i < ANEDYA_MAX_CONCURRENT_TXN; i++)
    {
        store->txn_slot_free[i] = 1;
        store->txns[i] = NULL;
    }
    store->_lock = 0;
    return ANEDYA_OK;
}

anedya_err_t _anedya_txn_store_aquire_slot(anedya_txn_store_t *store, anedya_txn_t *txn)
{
    // Aquire lock
    if (store->_lock == 1)
    {
        //printf("Lock failed while aquiring\r\n");
        return ANEDYA_ERR_LOCK_FAILED;
    }
    // Take lock
    store->_lock = 1;
    // Loop through the slots, and aquire the first available one
    for (int i = 0; i < ANEDYA_MAX_CONCURRENT_TXN; i++)
    {
        int64_t now = time(NULL);
        if(store->txn_slot_free[i] == 0 && (now - store->aquired_time[i]) > 30)
        {
            // TXN has timed out. Free the aquired slot
            store->txn_slot_free[i] = 1;
            store->txns[i] = NULL;
            // printf("Slot timed out: %d", i + 1);
        }
        if (store->txn_slot_free[i] == 1)
        {
            store->txn_slot_free[i] = 0;
            store->txns[i] = txn;
            store->aquired_time[i] = (int64_t)time(NULL);
            txn->desc = i + 1;
            store->_lock = 0;
            //printf("Aquired slot: %d\r\n", txn->desc);
            return ANEDYA_OK;
        }
    }
    // Release the lock
    store->_lock = 0;
    return ANEDYA_ERR_MAX_TXN_EXCEEDED;
}

anedya_err_t _anedya_txn_store_release_slot(anedya_txn_store_t *store, anedya_txn_t *txn)
{
    // Aquire lock
    if (store->_lock == 1)
    {
        //printf("Lock failed while releasing\r\n");
        return ANEDYA_ERR_LOCK_FAILED;
    }
    // Take lock
    store->_lock = 1;
    store->txn_slot_free[txn->desc - 1] = 1;
    store->txns[txn->desc - 1] = NULL;
    //printf("Released slot: %d\r\n", txn->desc);
    // Release the lock
    store->_lock = 0;
    return ANEDYA_OK;
}

void _anedya_message_handler(anedya_client_t *cl, char *topic, int topic_len, char *payload, int payload_len)
{
    // Just received the message, now determine for which topic this message is delivered
    //_anedya_interface_std_out("Processing message");
    //printf("Matching from: %.*s Len: %d\r\n", topic_len, topic, topic_len);
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        //printf("Matching with: %s Len: %d\n", cl->_message_topics[i], strlen(cl->_message_topics[i]));
        if (strncmp(topic, cl->_message_topics[i], strlen(cl->_message_topics[i]) - 1) == 0)
        {
            //_anedya_interface_std_out("Topic matched");
            //printf("Matching from: %.*s Len: %d\r\n", topic_len, topic, topic_len);
            break;
        }
    }
    //printf("Index: %d\r\n", i);
    switch (i)
    {
    case 0:
        _anedya_handle_txn_response(cl, payload, payload_len, 0);
        break;
    case 1:
        //printf("Error case\r\n");
        _anedya_handle_txn_response(cl, payload, payload_len, 1);
        break;
    case 2:
        _anedya_handle_event(cl, payload, payload_len, 2);
        // Handle command
        break;
    case 3:
        // Handle valuestore update
        _anedya_handle_event(cl, payload, payload_len, 3);
        break;
    }
}

void _anedya_on_connect_handler(anedya_client_t *client)
{
    // Client just got connected to the broker, now subscribe to the topics
    client->is_connected = 1;
    anedya_interface_mqtt_subscribe(client->mqtt_client, client->_message_topics[0], strlen(client->_message_topics[0]), 0);
    anedya_interface_mqtt_subscribe(client->mqtt_client, client->_message_topics[1], strlen(client->_message_topics[1]), 0);
    anedya_interface_mqtt_subscribe(client->mqtt_client, client->_message_topics[2], strlen(client->_message_topics[2]), 0);
    anedya_interface_mqtt_subscribe(client->mqtt_client, client->_message_topics[3], strlen(client->_message_topics[2]), 0);
    if (client->config->on_connect != NULL)
    {
        client->config->on_connect(client->config->on_connect_ctx);
    }
}

void _anedya_on_disconnect_handler(anedya_client_t *client)
{
    if(client->is_connected == 1) {
        // This means the flow is coming from unintentional connection close
        // Process retry logic
        client->is_connected = 0;
        anedya_interface_mqtt_connect(client->mqtt_client);
        // TODO: Implement retry logic
    }
    // Call the callback
    if (client->config->on_disconnect != NULL)
    {
        client->config->on_disconnect(client->config->on_disconnect_ctx);
    }
}

void _anedya_handle_txn_response(anedya_client_t *cl, char *payload, int payload_len, uint8_t topic)
{
    // Parse the payload, and get the txn id
    //printf("Handling txn response\r\n");
    char buffer[ANEDYA_RX_BUFFER_SIZE];
    char str[ANEDYA_RX_BUFFER_SIZE];
    int str_len = payload_len;
    for(int i = 0;i< ANEDYA_RX_BUFFER_SIZE; i++)
    {
        buffer[i] = 0;
        str[i] = 0;
    }
    memcpy(str, payload, payload_len);
    memcpy(buffer, str, str_len);
    //printf("Payload Received: %s\r\n", str);
    str[str_len] = '\0';
    buffer[str_len] = '\0';
    json_t mem[32];
    // Parse the json and get the txn id
    json_t const *json = json_create(str, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body in TXN handler");
    }
    // Get the txn id
    json_t const *txn_id = json_getProperty(json, "reqId");
    if (!txn_id || JSON_TEXT != json_getType(txn_id))
    {
        _anedya_interface_std_out("Error, the first name property is not found.");
    }
    char const *txn_index = json_getValue(txn_id);
    int index = atoi(txn_index);
    // If the txn id is 0, then it is an error
    //printf("Txn id: %d\r\n", index);
    if (index == 0)
    {
        _anedya_interface_std_out("Error, invalid txn id");
    }
    // Search for the txn in the txn store
    anedya_txn_t *txn = cl->txn_store.txns[index - 1];
    strcpy(txn->_rxbody, buffer);
    txn->_rx_len = str_len + 1;
    if (txn->_rx_len > ANEDYA_RX_BUFFER_SIZE)
    {
        txn->_op_err = ANEDYA_ERR_RX_BUFFER_OVERFLOW;
        txn->is_complete = true;
        txn->is_success = false;
        _anedya_txn_complete(cl, txn);
        return;    
    }
    //printf("Rx Body: %s", txn->_rxbody);
    // Call the Operation handler
    switch (txn->_op)
    {
    case ANEDYA_OP_BIND_DEVICE:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_HEARTBEAT:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_OTA_NEXT:
        _anedya_op_ota_next_resp(cl, txn);
        break;
    case ANEDYA_OP_SUBMIT_DATA:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_VALUESTORE_SET:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_SUBMIT_EVENT:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_CMD_UPDATE_STATUS:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    case ANEDYA_OP_SUBMIT_LOG:
        _anedya_device_handle_generic_resp(cl, txn);
        break;
    default:
        // Do nothing
        break;
    }
    // Mark transaction as completed
    _anedya_txn_complete(cl, txn);
    // If not found, handle error
    return;
}

void _anedya_handle_event(anedya_client_t *cl, char *payload, int payload_len, uint8_t topic)
{
    // A new event has been triggerred
    char buffer[ANEDYA_RX_BUFFER_SIZE];
    int buffer_len = payload_len;
    memcpy(buffer, payload, payload_len);
    //anedya_event_t event;
    //void *event_data = NULL;
    switch(topic) {
        case 2:
            // Handle command
            anedya_command_obj_t cmd;
            cmd.cmd_data_type= ANEDYA_DATATYPE_UNKNOWN;
            _anedya_parse_inbound_command(buffer, buffer_len, &cmd);
            if(cl->config->event_handler != NULL)
            {
                cl->config->event_handler(cl, ANEDYA_EVENT_COMMAND, &cmd);
            }
            break;
        case 3:
            // Handle valuestore update
            // First decode the valuestore object
            uint8_t type = _anedya_parse_valuestore_type(buffer, buffer_len);
            switch(type) {
                case ANEDYA_VALUESTORE_TYPE_FLOAT:
                    anedya_valustore_obj_float_t float_data;
                    //printf("Buffer in: %s", buffer);
                    _anedya_parse_valuestore_float(buffer, buffer_len, &float_data);
                    // Call the event handler with data
                    if(cl->config->event_handler != NULL)
                    {
                        cl->config->event_handler(cl, ANEDYA_EVENT_VS_UPDATE_FLOAT, &float_data);
                    }
                    break;
                case ANEDYA_VALUESTORE_TYPE_STR:
                    //event_data = _anedya_parse_valuestore_string(payload, payload_len);
                    break;
                case ANEDYA_VALUESTORE_TYPE_BOOL:
                    //event_data = _anedya_parse_valuestore_json(payload, payload_len);
                    break;
                case ANEDYA_VALUESTORE_TYPE_BIN:
                    // Handle binary data
                    break;
            }
            break;
    }
}