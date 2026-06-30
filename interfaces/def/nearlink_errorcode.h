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

#ifndef NEARLINK_ERRORCODE_H
#define NEARLINK_ERRORCODE_H

namespace OHOS {
namespace Nearlink {

/* Nearlink errcode defines */
enum NlErrCode {
    // Common error codes
    NL_ERR_PERMISSION_FAILED = 201,
    NL_ERR_SYSTEM_PERMISSION_FAILED = 202,
    NL_ERR_PROHIBITED_BY_EDM = 203,
    NL_ERR_INVALID_PARAM = 401,
    NL_ERR_API_NOT_SUPPORT = 801,
    NL_ERR_DYNAMIC_POWER_NOT_SUPPORT = 901,
    NL_ERR_DYNAMIC_POWER_SET_FAILED = 1001,

    // Customized error codes
    NL_NO_ERROR = 0,

    NL_ERR_BASE_SYSCAP = 1009700000,
    NL_ERR_SERVICE_DISCONNECTED     = NL_ERR_BASE_SYSCAP + 1,
    NL_ERR_UNBONDED_DEVICE          = NL_ERR_BASE_SYSCAP + 2,
    NL_ERR_SLE_OFF                  = NL_ERR_BASE_SYSCAP + 3,
    NL_ERR_PROFILE_DISABLED         = NL_ERR_BASE_SYSCAP + 4,
    NL_ERR_DEVICE_DISCONNECTED      = NL_ERR_BASE_SYSCAP + 5,
    NL_ERR_MAX_CONNECTION           = NL_ERR_BASE_SYSCAP + 6,
    NL_ERR_TIMEOUT                  = NL_ERR_BASE_SYSCAP + 7,
    NL_ERR_UNAVAILABLE_PROXY        = NL_ERR_BASE_SYSCAP + 8,
    NL_ERR_INVALID_STATE            = NL_ERR_BASE_SYSCAP + 9,
    NL_ERR_PROFILE_PROHIBITED_BY_EDM    = NL_ERR_BASE_SYSCAP + 10,
    NL_ERR_PROFILE_CONNECTION_NOT_ESTABLISHED = NL_ERR_BASE_SYSCAP + 11,
    NL_ERR_DATATRANSFER_DUPLICATE_REGISTER    = NL_ERR_BASE_SYSCAP + 20,
    NL_ERR_DATATRANSFER_LIMITED     = NL_ERR_BASE_SYSCAP + 21,
    NL_ERR_DATATRANSFER_NO_REGISTER = NL_ERR_BASE_SYSCAP + 22,
    NL_ERR_DATATRANSFER_WRITE_DATA_CONGESTED = NL_ERR_BASE_SYSCAP + 23,
    NL_RANGING_RESULT_ERR           = NL_ERR_BASE_SYSCAP + 24,
    NL_ERR_CONNECTION_NOT_ESTABLISHED = NL_ERR_BASE_SYSCAP + 30,
    NL_ERR_INVALID_INTERGER         = NL_ERR_BASE_SYSCAP + 40,
    NL_ERR_INVALID_ADDRESS          = NL_ERR_BASE_SYSCAP + 41,
    NL_ERR_EMPTY_ARRAY              = NL_ERR_BASE_SYSCAP + 42,
    NL_ERR_INVALID_UUID             = NL_ERR_BASE_SYSCAP + 43,
    NL_ERR_STANDARD_UUID_NOT_ALLOWED = NL_ERR_BASE_SYSCAP + 44,
    NL_ERR_INVALID_PASSCODE         = NL_ERR_BASE_SYSCAP + 45,
    NL_ERR_STRING_LENGTH_LIMITED    = NL_ERR_BASE_SYSCAP + 46,
    NL_ERR_CDSM_NOT_SUPPORT         = NL_ERR_BASE_SYSCAP + 50,
    NL_ERR_PEER_NOT_SUPPORT_BATTERY_SERVICE = NL_ERR_BASE_SYSCAP + 51,
    NL_ERR_IMPL_ERROR               = NL_ERR_BASE_SYSCAP + 98,
    NL_ERR_INTERNAL_ERROR           = NL_ERR_BASE_SYSCAP + 99,
    NL_ERR_IPC_TRANS_FAILED         = NL_ERR_BASE_SYSCAP + 100,
    NL_ERR_SOCKET_TRANS_FAILED      = NL_ERR_BASE_SYSCAP + 101,
    NL_ERR_FEATURE_NOT_SUPPORT      = NL_ERR_BASE_SYSCAP + 102,

    // Inner error codes
    NL_ERR_INVALID_ADV_ID           = -40,
    NL_ERR_INVALID_SWITCH_OPERATION = -100,
};

}  // namespace Nearlink
}  // namespace OHOS

#endif
