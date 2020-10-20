// Copyright (c) 2020 Intel Corporation.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

/**
 * @brief ConfigMgr C Client Implementation
 * Holds the implementaion of APIs supported by ConfigMgr Client
 */


#include "eis/config_manager/client_cfg.h"
#include <stdarg.h>

#define MAX_CONFIG_KEY_LENGTH 250

// To fetch endpoint from config
config_value_t* cfgmgr_get_endpoint_client(void* cli_conf) {
    client_cfg_t* client_cfg = (client_cfg_t*) cli_conf;
    config_value_t* ep = get_endpoint_base(client_cfg->client_config);
    if (ep == NULL) {
        LOG_ERROR_0("Endpoint not found");
        return NULL;
    }
    return ep;
}

config_value_t* cfgmgr_get_interface_value_client(void* cli_conf, const char* key) {
    client_cfg_t* client_cfg = (client_cfg_t*)cli_conf;
    return config_value_object_get(client_cfg->client_config, key);
}

// function to get msgbus config for client
config_t* cfgmgr_get_msgbus_config_client(base_cfg_t* base_cfg, void* cli_conf) {

    client_cfg_t* client_cfg = (client_cfg_t*)cli_conf;
    // Initializing base_cfg variables
    config_value_t* cli_config = client_cfg->client_config;
    char* app_name = base_cfg->app_name;
    int dev_mode = base_cfg->dev_mode;
    kv_store_client_t* m_kv_store_handle = base_cfg->m_kv_store_handle;
    void* cfgmgr_handle = base_cfg->cfgmgr_handle;

    // Creating cJSON object
    cJSON* c_json = cJSON_CreateObject();
    if (c_json == NULL) {
        LOG_ERROR_0("c_json object initialization failed");
        return NULL;
    }

    // Fetching name from config
    config_value_t* client_name = config_value_object_get(cli_config, NAME);
    if (client_name == NULL) {
        LOG_ERROR_0("client_name initialization failed");
        return NULL;
    }

    // Fetching Type from config
    config_value_t* client_config_type = config_value_object_get(cli_config, TYPE);
    if (client_config_type == NULL) {
        LOG_ERROR_0("client_config_type object initialization failed");
        return NULL;
    }
    char* type = client_config_type->body.string;

    // Fetching EndPoint from config
    config_value_t* client_endpoint = config_value_object_get(cli_config, ENDPOINT);
    if (client_endpoint == NULL) {
        LOG_ERROR_0("client_endpoint object initialization failed");
        return NULL;
    }
    const char* end_point = client_endpoint->body.string;

    // Overriding endpoint with CLIENT_<Name>_ENDPOINT if set
    size_t init_len = strlen("CLIENT_") + strlen(client_name->body.string) + strlen("_ENDPOINT") + 2;
    char* ep_override_env = concat_s(init_len, 3, "CLIENT_", client_name->body.string, "_ENDPOINT");
    if (ep_override_env == NULL) {
        LOG_ERROR_0("concatenation for ep_override_env failed");
        return NULL;
    }
    char* ep_override = getenv(ep_override_env);
    if (ep_override != NULL) {
        if (strlen(ep_override) != 0) {
            LOG_DEBUG("Overriding endpoint with %s", ep_override_env);
            end_point = (const char*)ep_override;
        }
    }

    // Overriding endpoint with CLIENT_ENDPOINT if set
    // Note: This overrides all the client endpoints if set
    char* client_ep = getenv("CLIENT_ENDPOINT");
    if (client_ep != NULL) {
        LOG_DEBUG_0("Overriding endpoint with CLIENT_ENDPOINT");
        if (strlen(client_ep) != 0) {
            end_point = (const char*)client_ep;
        }
    }

    // Overriding endpoint with CLIENT_<Name>_TYPE if set
    init_len = strlen("CLIENT_") + strlen(client_name->body.string) + strlen("_TYPE") + 2;
    char* type_override_env = concat_s(init_len, 3, "CLIENT_", client_name->body.string, "_TYPE");
    if (type_override_env == NULL) {
        LOG_ERROR_0("concatenation for type_override_env failed");
        return NULL;
    }
    char* type_override = getenv(type_override_env);
    if (type_override != NULL) {
        if (strlen(type_override) != 0) {
            LOG_DEBUG("Overriding endpoint with %s", type_override_env);
            type = type_override;
        }
    }

    // Overriding endpoint with CLIENT_TYPE if set
    // Note: This overrides all the client type if set
    char* client_type = getenv("CLIENT_TYPE");
    if (client_type != NULL) {
        LOG_DEBUG_0("Overriding endpoint with CLIENT_TYPE");
        if (strlen(client_type) != 0) {
            type = client_type;
        }
    }
    cJSON_AddStringToObject(c_json, "type", type);

    // Adding zmq_recv_hwm value if available
    config_value_t* zmq_recv_hwm_value = config_value_object_get(cli_config, ZMQ_RECV_HWM);
    if (zmq_recv_hwm_value != NULL) {
        if (zmq_recv_hwm_value->type != CVT_INTEGER) {
            LOG_ERROR_0("zmq_recv_hwm type is not integer");
            return NULL;
        }
        cJSON_AddNumberToObject(c_json, ZMQ_RECV_HWM, zmq_recv_hwm_value->body.integer);
    }

    if (!strcmp(type, "zmq_ipc")) {
        // Add Endpoint directly to socket_dir if IPC mode
        cJSON_AddStringToObject(c_json, "socket_dir", end_point);
    } else if (!strcmp(type, "zmq_tcp")) {

        // Add host & port to client_topic cJSON object
        cJSON* client_topic = cJSON_CreateObject();
        if (client_topic == NULL) {
            LOG_ERROR_0("client_topic object initialization failed");
            return NULL;
        }
        char** host_port = get_host_port(end_point);
        char* host = host_port[0];
        trim(host);
        char* port = host_port[1];
        trim(port);
        __int64_t i_port = atoi(port);

        cJSON_AddStringToObject(client_topic, "host", host);
        cJSON_AddNumberToObject(client_topic, "port", i_port);

        if(dev_mode != 0) {

             // Fetching Server AppName from config
            config_value_t* server_appname = config_value_object_get(cli_config, SERVER_APPNAME);
            if (server_appname == NULL) {
                LOG_ERROR_0("server_appname object initialization failed");
                return NULL;
            }

            // Adding server public key to config
            size_t init_len = strlen(PUBLIC_KEYS) + strlen(server_appname->body.string) + 2;
            char* retreive_server_pub_key = concat_s(init_len, 2, PUBLIC_KEYS, server_appname->body.string);
            const char* server_public_key = m_kv_store_handle->get(cfgmgr_handle, retreive_server_pub_key);
            if(server_public_key == NULL){
                LOG_WARN("Value is not found for the key: %s", retreive_server_pub_key);
            }

            cJSON_AddStringToObject(client_topic, "server_public_key", server_public_key);

            // Adding client public key to config
            init_len = strlen(PUBLIC_KEYS) + strlen(app_name) + strlen(app_name) + 2;
            char* s_client_public_key = concat_s(init_len, 2, PUBLIC_KEYS, app_name);
            const char* sub_public_key = m_kv_store_handle->get(cfgmgr_handle, s_client_public_key);
            if(sub_public_key == NULL){
                LOG_ERROR("Value is not found for the key: %s", s_client_public_key);
                return NULL;
            }

            cJSON_AddStringToObject(client_topic, "client_public_key", sub_public_key);

            // Adding client private key to config
            init_len = strlen("/") + strlen(app_name) + strlen(PRIVATE_KEY) + 2;
            char* s_client_pri_key = concat_s(init_len, 3, "/", app_name, PRIVATE_KEY);
            const char* sub_pri_key = m_kv_store_handle->get(cfgmgr_handle, s_client_pri_key);
            if(sub_pri_key == NULL){
                LOG_ERROR("Value is not found for the key: %s", s_client_pri_key);
                return NULL;
            }

            cJSON_AddStringToObject(client_topic, "client_secret_key", sub_pri_key);
        }
        // Creating the final cJSON config object
        cJSON_AddItemToObject(c_json, client_name->body.string, client_topic);
    }

    char* config_value = cJSON_Print(c_json);
    if (config_value == NULL) {
        LOG_ERROR_0("config_value object initialization failed");
        return NULL;
    }
    LOG_DEBUG("Env client Config is : %s \n", config_value);

    config_t* m_config = config_new(
            (void*) c_json, free_json, get_config_value);
    if (m_config == NULL) {
        LOG_ERROR_0("Failed to initialize configuration object");
        return NULL;
    }

    return m_config;
}

// function to initialize client_cfg_t
client_cfg_t* client_cfg_new() {
    LOG_DEBUG_0("In client_cfg_new mthod");
    client_cfg_t *cli_cfg_mgr = (client_cfg_t *)malloc(sizeof(client_cfg_t));
    if (cli_cfg_mgr == NULL) {
        LOG_ERROR_0("Malloc failed for client_cfg_t");
        return NULL;
    }
    cli_cfg_mgr->cfgmgr_get_msgbus_config_client = cfgmgr_get_msgbus_config_client;
    cli_cfg_mgr->cfgmgr_get_interface_value_client = cfgmgr_get_interface_value_client;
    cli_cfg_mgr->cfgmgr_get_endpoint_client = cfgmgr_get_endpoint_client;
    return cli_cfg_mgr;
}

// function to destroy client_cfg_t
void client_cfg_config_destroy(client_cfg_t *cli_cfg_config) {
    if (cli_cfg_config != NULL) {
        free(cli_cfg_config);
    }
}