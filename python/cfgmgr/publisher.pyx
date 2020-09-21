# Copyright (c) 2020 Intel Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
"""EIS Message Bus Publisher wrapper object
"""

import json
import logging

from .libneweisconfigmgr cimport *
from libc.stdlib cimport malloc


cdef class Publisher:
    """EIS Message Bus Publisher object
    """

    def __init__(self):
        """Constructor
        """
        pass

    def __cinit__(self, *args, **kwargs):
        """Cython base constructor
        """
        self.app_cfg = NULL
        self.pub_cfg = NULL

    @staticmethod
    cdef create(app_cfg_t* app_cfg, pub_cfg_t* pub_cfg):
        """Helper method for initializing the client object.

        :param app_cfg: Applications config struct
        :type: struct
        :param pub_cfg: Publisher config struct
        :type: struct
        :return: Publisher class object
        :rtype: obj
        """
        p = Publisher()
        p.app_cfg = app_cfg
        p.pub_cfg = pub_cfg
        return p

    def __dealloc__(self):
        """Cython destructor
        """
        self.destroy()

    def destroy(self):
        """Destroy the publisher.
        """
        if self.app_cfg != NULL:
            app_cfg_config_destroy(self.app_cfg)
        if self.pub_cfg != NULL:
            pub_cfg_config_destroy(self.pub_cfg)

    def get_msgbus_config(self):
        """Calling the base C get_msgbus_config() API

        :return: Messagebus config
        :rtype: dict
        """
        cdef char* config
        new_config_new = self.pub_cfg.cfgmgr_get_msgbus_config_pub(self.app_cfg.base_cfg)
        config = configt_to_char(new_config_new)
        config_str = config.decode('utf-8')
        return json.loads(config_str)

    def get_endpoint(self):
        """Calling the base C cfgmgr_get_endpoint_pub() API

        :return: Endpoint config
        :rtype: string
        """
        cdef config_value_t* ep
        ep = self.pub_cfg.cfgmgr_get_endpoint_pub(self.app_cfg.base_cfg)
        endpoint = ep.body.string.decode('utf-8')
        config_value_destroy(ep)
        return endpoint
        
    def get_topics(self):
        """ Calling the base C cfgmgr_get_topics_pub() API
        :return: List of topics
        :rtype: List
        """
        topics_list = []
        cdef config_value_t* topics
        topics = self.pub_cfg.cfgmgr_get_topics_pub(self.app_cfg.base_cfg)
        cdef config_value_t* topic_value
        for i in range(config_value_array_len(topics)):
            topic_value = config_value_array_get(topics, i)
            topics_list.append(topic_value.body.string.decode('utf-8'))
            config_value_destroy(topic_value)
        config_value_destroy(topics)
        return topics_list

    def get_allowed_clients(self):
        """Calling the base C get_allowed_clients() API
        
        :return: List of clients
        :rtype: List
        """
        clients_list = []
        cdef config_value_t* clients
        clients = self.pub_cfg.cfgmgr_get_allowed_clients_pub(self.app_cfg.base_cfg)
        cdef config_value_t* client_value
        for i in range(config_value_array_len(clients)):
            client_value = config_value_array_get(clients, i)
            clients_list.append(client_value.body.string.decode('utf-8'))
            config_value_destroy(client_value)
        config_value_destroy(clients)
        return clients_list

    def set_topics(self, topics_list):
        """Calling the base C cfgmgr_set_topics_pub() API

        :return: whether topic is set
        :rtype: int
        """
        cdef int topics_set
        cdef char** topics_to_be_set
        # Allocate memory
        topics_to_be_set = <char**>malloc(len(topics_list) * sizeof(char*))
        # Check if allocation went fine
        if topics_to_be_set is NULL:
            logging.info("Out of memory")
        # Convert str to char* and store it into our char**
        for i in range(len(topics_list)):
            topics_list[i] = topics_list[i].encode()
            topics_to_be_set[i] = topics_list[i]
        # Calling the base C cfgmgr_set_topics_pub() API
        topics_set = self.pub_cfg.cfgmgr_set_topics_pub(topics_to_be_set, len(topics_list), self.app_cfg.base_cfg)
        return topics_set