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
"""EIS Message Bus echo service Python example.
"""

import json
import argparse
import os
import eis.msgbus as mb
import cfgmgr.config_manager as cfg


msgbus = None
service = None

try:
    # For DEV_MODE true tests
    # os.environ["DEV_MODE"] = "TRUE"
    # os.environ["CONFIGMGR_CERT"] = ""
    # os.environ["CONFIGMGR_KEY"] = ""
    # os.environ["CONFIGMGR_CACERT"] = ""
    
    os.environ["DEV_MODE"] = "FALSE"
    # Set path to certs here
    os.environ["CONFIGMGR_CERT"] = ""
    os.environ["CONFIGMGR_KEY"] = ""
    os.environ["CONFIGMGR_CACERT"] = ""
    os.environ["AppName"] = "VideoIngestion"

    ctx = cfg.ConfigMgr()
    server_ctx = ctx.get_server_by_name("sample_server")
    config = server_ctx.get_msgbus_config()
    print('[INFO] Obtained config is {}'.format(config))
    print('[INFO] Obtained endpoint is {}'.format(server_ctx.get_endpoint()))
    print('[INFO] Obtained allowed clients is {}'.format(server_ctx.get_allowed_clients()))

    print('[INFO] Initializing message bus context')
    msgbus = mb.MsgbusContext(config)
    service = msgbus.new_service("echo_service")

    print('[INFO] Running...')
    while True:
        request = service.recv()
        print(f'[INFO] Received request: {request.get_meta_data()}')
        service.response(request.get_meta_data())
except KeyboardInterrupt:
    print('[INFO] Quitting...')
finally:
    if service is not None:
        service.close()