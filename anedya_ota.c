#include "anedya_operations.h"
#include "anedya_json_builder.h"
#include "anedya_json_parse.h"
#include "anedya_ota.h"

anedya_err_t anedya_op_ota_next_req(anedya_client_t *client, anedya_txn_t *txn)
{
    anedya_op_next_ota_resp_t *resp = (anedya_op_next_ota_resp_t *)txn->response;
    resp->deployment_available = false;
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_OTA_NEXT;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK)
    {
        return err;
    }

    // Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);
    char topic[100];

    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/ota/next/json");
    // printf("REQ: %s", txbuffer);
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

void _anedya_op_ota_next_resp(anedya_client_t *client, anedya_txn_t *txn)
{
    // Parse JSON and check for error
    anedya_op_next_ota_resp_t *resp = (anedya_op_next_ota_resp_t *)txn->response;
    json_t mem[32];
    // Parse the json and get the txn id
    json_t const *json = json_create(txn->_rxbody, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body:response handler OTA Next");
        return;
    }
    // Check if success
    json_t const *success = json_getProperty(json, "success");
    if (!success || JSON_BOOLEAN != json_getType(success))
    {
        _anedya_interface_std_out("Error, the success property is not found.");
    }
    bool s = json_getBoolean(success);
    if (s == true)
    {
        txn->is_success = true;
    }
    else
    {
        txn->is_success = false;
        json_t const *error = json_getProperty(json, "errCode");
        if (!error || JSON_INTEGER != json_getType(error))
        {
            _anedya_interface_std_out("Error, the error property is not found.");
        }
        int err = json_getInteger(error);
        txn->_op_err = err;
        return;
    }
    // Flow reaches here means, request was successful.
    // Now, parse the response
    _anedya_op_ota_next_parser((json_t *)json, resp);
}

anedya_err_t _anedya_op_ota_next_parser(json_t *json, anedya_op_next_ota_resp_t *resp)
{
    // Parse whether deployment is available or not
    json_t const *da = json_getProperty(json, "deploymentAvailable");
    if (!da || JSON_BOOLEAN != json_getType(da))
    {
        _anedya_interface_std_out("Error, the success property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    bool dep_available = json_getBoolean(da);
    if (dep_available == false)
    {
        resp->deployment_available = false;
        return ANEDYA_OK;
    }

    // Flow reaches here, that means the OTA is available.
    resp->deployment_available = true;
    json_t const *dpdata = json_getProperty(json, "data");
    if (!dpdata || JSON_OBJ != json_getType(dpdata))
    {
        _anedya_interface_std_out("Error, the data property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }

    // Parse deploymentId
    json_t const *dpID = json_getProperty(dpdata, "deploymentId");
    if (!dpID || JSON_TEXT != json_getType(dpID))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *deployment_id = json_getValue(dpID);

    // Parse assetId
    json_t const *aID = json_getProperty(dpdata, "assetId");
    if (!aID || JSON_TEXT != json_getType(aID))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *asset_id = json_getValue(aID);

    // Parse assetIdentifier
    json_t const *aidentifier = json_getProperty(dpdata, "assetIdentifier");
    if (!aidentifier || JSON_TEXT != json_getType(aidentifier))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *asset_identifier = json_getValue(aidentifier);
    strcpy(resp->asset.asset_identifier, asset_identifier);
    resp->asset.asset_identifier_len = strlen(resp->asset.asset_identifier);

    // Parse assetVersion
    json_t const *aversion = json_getProperty(dpdata, "assetVersion");
    if (!aversion || JSON_TEXT != json_getType(aversion))
    {
        _anedya_interface_std_out("Error, the assetVersion property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *asset_version = json_getValue(aversion);
    strcpy(resp->asset.asset_version, asset_version);
    resp->asset.asset_version_len = strlen(resp->asset.asset_version);

    // Parse assetSigned
    json_t const *asigned = json_getProperty(dpdata, "assetSigned");
    if (!asigned || JSON_BOOLEAN != json_getType(asigned))
    {
        _anedya_interface_std_out("Error, the assetSigned property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const bool asset_signed = json_getBoolean(asigned);
    resp->asset.asset_signed = asset_signed;

    if (asset_signed)
    {
        resp->asset.asset_signed = true;
        // Parse assetSignature
        json_t const *asign = json_getProperty(dpdata, "assetSignature");
        if (!asign || JSON_TEXT != json_getType(asign))
        {
            _anedya_interface_std_out("Error, the assetSignature property is not found.");
            return ANEDYA_ERR_PARSE_ERROR;
        }
        const char *asset_signature = json_getValue(asign);
        strcpy(resp->asset.asset_signature, asset_signature);
        resp->asset.asset_signature_len = strlen(resp->asset.asset_signature);
    }

    // Parse Asset Metadata
    json_t const *ametadata = json_getProperty(dpdata, "assetMeta");
    size_t i = 0;
    json_t const *child;
    for (child = json_getChild(ametadata); i < resp->asset.asset_metadata_len; child = json_getSibling(child))
    {
        if (child == 0)
        {
            // List terminated
            break;
        }
        if (JSON_TEXT != json_getType(child))
        {
            continue;
        }
        const char *key = json_getName(child);
        if (key == NULL)
        {
            continue;
        }
        const char *value = json_getValue(child);
        if (strlen(key) > 50 || strlen(value) > 50)
        {
            continue;
        }
        strcpy(resp->asset.asset_metadata[i].key, key);
        strcpy(resp->asset.asset_metadata[i].value, value);
        i++;
    }

    // Parse asset checksum
    json_t const *achecksum = json_getProperty(dpdata, "assetChecksum");
    if (!achecksum || JSON_TEXT != json_getType(achecksum))
    {
        _anedya_interface_std_out("Error, the assetChecksum property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *asset_checksum = json_getValue(achecksum);
    strcpy(resp->asset.asset_checksum, asset_checksum);
    resp->asset.asset_checksum_len = strlen(resp->asset.asset_checksum);

    // Parse asset size
    json_t const *asize = json_getProperty(dpdata, "assetSize");
    if (!asize || JSON_INTEGER != json_getType(asize))
    {
        _anedya_interface_std_out("Error, the assetSize property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    int asset_size = json_getInteger(asize);
    resp->asset.asset_size = asset_size;

    // Parse asset URL
    json_t const *aurl = json_getProperty(dpdata, "asseturl");
    if (!aurl || JSON_TEXT != json_getType(aurl))
    {
        _anedya_interface_std_out("Error, the asseturl property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *asset_url = json_getValue(aurl);
    bool firstChar = 0;
    strcpy(resp->asset.asset_url, asset_url);
    for (int i = 0; i < strlen(resp->asset.asset_url) + 1; i++)
    {
        char c;
        if (resp->asset.asset_url[i] == '?')
        {
            if (firstChar)
            {
                c = '&';
            }
            else
            {
                firstChar = 1;
                c = resp->asset.asset_url[i];
            }
        }
        else
        {
            c = resp->asset.asset_url[i];
        }
        resp->asset.asset_url[i] = c;
    }
    resp->asset.asset_url_len = strlen(resp->asset.asset_url);

    // All data has been parsed now set the data in response structure.
    anedya_err_t err = _anedya_uuid_parse(deployment_id, resp->deployment_id);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    err = _anedya_uuid_parse(asset_id, resp->asset.asset_id);
    if (err != ANEDYA_OK)
    {
        return err;
    }

    return ANEDYA_OK;
}

anedya_err_t anedya_op_ota_update_status_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_ota_update_status_t *req)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_SUBMIT_DATA;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK)
    {
        return err;
    }
// Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif
    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    char dep_id[37];
    _anedya_uuid_marshal(*req->deployment_id, dep_id);
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_str(p, "deploymentId", dep_id, &marker);
    p = anedya_json_str(p, "status", req->status, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);
    // Body is ready now publish it to the MQTT
    char topic[100];
    // printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/ota/updateStatus/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}