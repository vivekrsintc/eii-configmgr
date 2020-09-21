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
 * @brief ConfigMgr C Implementation
 * Holds the implementaion of APIs supported by ConfigMgr
 */


#include "eis/config_manager/server_cfg.h"
#include <stdarg.h>

#define MAX_CONFIG_KEY_LENGTH 250

// To fetch endpoint from config
config_value_t* cfgmgr_get_endpoint_server(base_cfg_t* base_cfg) {
    config_value_t* ep = get_endpoint_base(base_cfg);
    if (ep == NULL) {
        LOG_ERROR_0("Endpoint not found");
        return NULL;
    }
    return ep;
}

// To fetch list of allowed clients from config
config_value_t* cfgmgr_get_allowed_clients_server(base_cfg_t* base_cfg) {
    config_value_t* client_list = get_allowed_clients_base(base_cfg);
    if (client_list == NULL) {
        LOG_ERROR_0("client_list initialization failed");
        return NULL;
    }
    return client_list;
}

// To fetch msgbus config
config_t* cfgmgr_get_msgbus_config_server(base_cfg_t* base_cfg) {

    // Initializing base_cfg variables
    config_value_t* serv_config = base_cfg->msgbus_config;
    char* app_name = base_cfg->app_name;
    int dev_mode = base_cfg->dev_mode;
    kv_store_client_t* m_kv_store_handle = base_cfg->m_kv_store_handle;

    // Creating cJSON object
    cJSON* c_json = cJSON_CreateObject();
    if (c_json == NULL) {
        LOG_ERROR_0("c_json initialization failed");
        return NULL;
    }

    // Fetching Name from name
    config_value_t* server_name = config_value_object_get(serv_config, NAME);
    if (server_name == NULL) {
        LOG_ERROR_0("server_name initialization failed");
        return NULL;
    }

    // Fetching Type from config
    config_value_t* server_type = config_value_object_get(serv_config, TYPE);
    if (server_type == NULL) {
        LOG_ERROR_0("server_type initialization failed");
        return NULL;
    }
    char* type = server_type->body.string;
    cJSON_AddStringToObject(c_json, "type", type);

    // Fetching EndPoint from config
    config_value_t* server_endpoint = config_value_object_get(serv_config, ENDPOINT);
    if (server_endpoint == NULL) {
        LOG_ERROR_0("server_endpoint initialization failed");
        return NULL;
    }
    const char* end_point = server_endpoint->body.string;

    // // Over riding endpoint with SERVER_<Name>_ENDPOINT if set
    size_t init_len = strlen("SERVER_") + strlen(server_name->body.string) + strlen("_ENDPOINT") + 2;
    char* ep_override_env = concat_s(init_len, 3, "SERVER_", server_name->body.string, "_ENDPOINT");
    char* ep_override = getenv(ep_override_env);
    if (ep_override != NULL) {
        if (strlen(ep_override) != 0) {
            LOG_DEBUG("Over riding endpoint with %s", ep_override_env);
            end_point = (const char*)ep_override;
        }
    }

    // Over riding endpoint with SERVER_ENDPOINT if set
    // Note: This overrides all the server endpoints if set
    char* server_ep = getenv("SERVER_ENDPOINT");
    if (server_ep != NULL) {
        LOG_DEBUG_0("Over riding endpoint with SERVER_ENDPOINT");
        if (strlen(server_ep) != 0) {
            end_point = (const char*)server_ep;
        }
    }

    // Adding zmq_recv_hwm value
    config_value_t* zmq_recv_hwm_value = config_value_object_get(serv_config, ZMQ_RECV_HWM);
    if (zmq_recv_hwm_value != NULL) {
        cJSON_AddNumberToObject(c_json, ZMQ_RECV_HWM, zmq_recv_hwm_value->body.integer);
    }

    if (!strcmp(type, "zmq_ipc")) {
        // Add Endpoint directly to socket_dir if IPC mode
        cJSON_AddStringToObject(c_json, "socket_dir", end_point);
    } else if (!strcmp(type, "zmq_tcp")) {

        // Add host & port to zmq_tcp_publish cJSON object
        cJSON* server_topic = cJSON_CreateObject();
        if (server_topic == NULL) {
            LOG_ERROR_0("server_topic initialization failed");
            return NULL;
        }
        char** host_port = get_host_port(end_point);
        char* host = host_port[0];
        trim(host);
        char* port = host_port[1];
        trim(port);
        __int64_t i_port = atoi(port);

        cJSON_AddStringToObject(server_topic, "host", host);
        cJSON_AddNumberToObject(server_topic, "port", i_port);

        if (dev_mode != 0) {

            // Initializing m_kv_store_handle to fetch public & private keys
            void *handle = m_kv_store_handle->init(m_kv_store_handle);

            // Fetching AllowedClients from config
            config_value_t* server_json_clients = config_value_object_get(serv_config, ALLOWED_CLIENTS);
            if (server_json_clients == NULL) {
                LOG_ERROR_0("server_json_clients initialization failed");
                return NULL;
            }
            // Fetch the first item in allowed_clients
            config_value_t* temp_array_value = config_value_array_get(server_json_clients, 0);
            if (temp_array_value == NULL) {
                LOG_ERROR_0("temp_array_value initialization failed");
                return NULL;
            }
            int result;
            strcmp_s(temp_array_value->body.string, strlen(temp_array_value->body.string), "*", &result);
            // If only one item in allowed_clients and it is *
            // Add all available Publickeys
            if ((config_value_array_len(server_json_clients) == 1) && (result == 0)) {
                cJSON* all_clients = cJSON_CreateArray();
                if (all_clients == NULL) {
                    LOG_ERROR_0("all_clients initialization failed");
                    return NULL;
                }
                config_value_t* pub_key_values = m_kv_store_handle->get_prefix(handle, "/Publickeys");
                if (pub_key_values == NULL) {
                    LOG_ERROR_0("pub_key_values initialization failed");
                    return NULL;
                }
                config_value_t* value;
                for (int i = 0; i < config_value_array_len(pub_key_values); i++) {
                    value = config_value_array_get(pub_key_values, i);
                    if (value == NULL) {
                        LOG_ERROR_0("value initialization failed");
                        return NULL;
                    }
                    cJSON_AddItemToArray(all_clients, cJSON_CreateString(value->body.string));
                }
                cJSON_AddItemToObject(c_json, "allowed_clients",  all_clients);
            } else {
                config_value_t* array_value;
                cJSON* all_clients = cJSON_CreateArray();
                if (all_clients == NULL) {
                    LOG_ERROR_0("all_clients initialization failed");
                    return NULL;
                }
                for (int i =0; i < config_value_array_len(server_json_clients); i++) {
                    // Fetching individual public keys of all AllowedClients
                    array_value = config_value_array_get(server_json_clients, i);
                    if (array_value == NULL) {
                        LOG_ERROR_0("array_value initialization failed");
                        return NULL;
                    }
                    size_t init_len = strlen(PUBLIC_KEYS) + strlen(array_value->body.string) + 2;
                    char* grab_public_key = concat_s(init_len, 2, PUBLIC_KEYS, array_value->body.string);
                    const char* sub_public_key = m_kv_store_handle->get(handle, grab_public_key);
                    if(sub_public_key == NULL){
                        LOG_ERROR("Value is not found for the key: %s", grab_public_key);
                    }

                    cJSON_AddItemToArray(all_clients, cJSON_CreateString(sub_public_key));
                }
                // Adding all public keys of clients to allowed_clients of config
                cJSON_AddItemToObject(c_json, "allowed_clients",  all_clients);
            }

            // Fetching Publisher private key & adding it to server_topic object
            size_t init_len = strlen("/") + strlen(PRIVATE_KEY) + strlen(app_name) + 2;
            char* pub_pri_key = concat_s(init_len, 3, "/", app_name, PRIVATE_KEY);
            const char* server_secret_key = m_kv_store_handle->get(handle, pub_pri_key);
            if (server_secret_key == NULL) {
                LOG_ERROR("Value is not found for the key: %s", pub_pri_key);
            }
            cJSON_AddStringToObject(server_topic, "server_secret_key", server_secret_key);
        }
        // Creating the final cJSON config object
        // server_name
        cJSON_AddItemToObject(c_json, server_name->body.string, server_topic);
    }

    // Constructing char* object from cJSON object
    char* config_value_cr = cJSON_Print(c_json);
    if (config_value_cr == NULL) {
        LOG_ERROR_0("config_value_cr initialization failed");
        return NULL;
    }
    LOG_DEBUG("Env server Config is : %s \n", config_value_cr);

    // Constructing config_t object from cJSON object
    config_t* m_config = config_new(
            (void*) c_json, free_json, get_config_value);
    if (m_config == NULL) {
        LOG_ERROR_0("Failed to initialize configuration object");
        return NULL;
    }
    return m_config;
}

// function to initialize server_cfg_t
server_cfg_t* server_cfg_new() {
    LOG_DEBUG_0("In server_cfg_new mthod");
    server_cfg_t *serv_cfg_mgr = (server_cfg_t *)malloc(sizeof(server_cfg_t));
    if (serv_cfg_mgr == NULL) {
        LOG_ERROR_0("Malloc failed for pub_cfg_t");
        return NULL;
    }
    serv_cfg_mgr->cfgmgr_get_msgbus_config_server = cfgmgr_get_msgbus_config_server;
    serv_cfg_mgr->cfgmgr_get_endpoint_server = cfgmgr_get_endpoint_server;
    serv_cfg_mgr->cfgmgr_get_allowed_clients_server = cfgmgr_get_allowed_clients_server;
    return serv_cfg_mgr;
}

// function to destroy server_cfg_t
void server_cfg_config_destroy(server_cfg_t *server_cfg_config) {
    if (server_cfg_config != NULL) {
        free(server_cfg_config);
    }
}