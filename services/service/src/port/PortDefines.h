/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PORT_DEFINES_H
#define PORT_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"

namespace OHOS {
namespace Nearlink {
constexpr int PORT_STATE_DISCONNECTED = 0;
constexpr int PORT_STATE_CONNECTING = 1;
constexpr int PORT_STATE_DISCONNECTING = 2;
constexpr int PORT_STATE_CONNECTED = 3;

constexpr int PORT_SUCCESS = 0;
constexpr int PORT_FAILURE = 1;

// Service start/stop
constexpr int PORT_SERVICE_STARTUP_EVT = 1;
constexpr int PORT_SERVICE_SHUTDOWN_EVT = 2;

// port client state
constexpr int PORT_CONNECT_START_EVT = 3;
constexpr int PORT_DISCONNECT_START_EVT = 4;
constexpr int PORT_CONNECT_CMPL_EVT = 5;
constexpr int PORT_DISCONNECT_CMPL_EVT = 6;

// Connection/disconnection timeout
constexpr int PORT_CONNECTION_TIMEOUT_EVT = 7;
constexpr int PORT_DISCONNECTION_TIMEOUT_EVT = 8;

// port profile add/delete port
constexpr int PORT_ADD_PORT_EVT = 9;
constexpr int PORT_DELETE_PORT_EVT = 10;
constexpr int PORT_GET_REMOTE_PORT_EVT = 11;

// shift 8 bit
constexpr uint8_t PORT_SHIFT_OPRATURN_8 = 8;
// group num of the property is 2
constexpr uint8_t PORT_PROPERTY_GROUP_BYTES_NUM_2 = 2;
// single property size 20 bytes
constexpr uint8_t PORT_PROPERTY_BYTES_NUM_20 = 20;
// single uuid of property value size 16 bytes
constexpr uint8_t PORT_PROPERTY_VAL_UUID_BYTES_NUM_16 = 16;
// uuid of property value start from 6th byte
constexpr uint8_t PORT_PROPERTY_VAL_UUID_START_6 = 6;
// uuid of property value end at 21th byte
constexpr uint8_t PORT_PROPERTY_VAL_UUID_END_21 = 21;
// high 8bit of portid of property value start in 2nd byte every group
constexpr uint8_t PORT_PROPERTY_VAL_PORTID_HIGH_2 = 2;
// low 8bit of portid of property value start in 3rd byte every group
constexpr uint8_t PORT_PROPERTY_VAL_PORTID_LOW_3 = 3;
// bytes num of one group property
constexpr uint8_t PORT_PROPERTY_ONE_GROUP_BYTES_NUM_22 = 22;

constexpr int PORT_PROFILE_SSAP_THREAD_WAIT_TIMEOUT = 5;

//port property uuid
constexpr uint16_t PORT_UUID_SSAP_PROPERTY_NAME = 0x1101;
constexpr const char* NEARLINK_PORT_SERVICE_UUID = "37BEA880-FC70-11EA-B720-000000000660";
constexpr const char* NEARLINK_PORT_PROPERTY_UUID = "37BEA880-FC70-11EA-B720-000000001101";

enum PortOperationIndication {
    PORT_OPERATION_READ = 0x01, /**< readable */
    PORT_OPERATION_WRITE_NO_RESPONSE = 0x02,
    PORT_OPERATION_WRITE_WITH_RESPONSE = 0x04,
    PORT_OPERATION_NOTIFY = 0x08,
    PORT_OPERATION_INDICATION = 0x10,
    PORT_OPERATION_BROADCAST = 0x20,
};

} // namespace Sle
} // namespace OHOS
#endif // PORT_DEFINES_H