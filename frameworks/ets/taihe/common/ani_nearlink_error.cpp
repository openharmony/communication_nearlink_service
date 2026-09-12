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

#ifndef LOG_TAG
#define LOG_TAG "nearlink_ani_error"
#endif

#include <map>
#include "ani_nearlink_error.h"
#include "log.h"
#include "taihe/runtime.hpp"

namespace OHOS {
namespace Nearlink {
constexpr int32_t NL_ERR_SYSCAP_BASE_CODE = 36100000;
constexpr int32_t NL_ERR_CODE_MAX = 99999;
static std::map<int32_t, std::string> aniErrCommonMsgMap {
    { NlErrCode::NL_ERR_PERMISSION_FAILED, "Permission denied." },
    { NlErrCode::NL_ERR_SYSTEM_PERMISSION_FAILED, "Non-system applications are not allowed to use system APIs."},
    { NlErrCode::NL_ERR_PROHIBITED_BY_EDM, "Nearlink is prohibited by EDM."},
    { NlErrCode::NL_ERR_INVALID_PARAM, "Invalid parameter." },
    { NlErrCode::NL_ERR_API_NOT_SUPPORT, "Capability not supported." },
    { NlErrCode::NL_ERR_SLE_OFF, "NearLink disabled." },
    { NlErrCode::NL_ERR_DATATRANSFER_DUPLICATE_REGISTER, "The port associated with this UUID is already registered." },
    { NlErrCode::NL_ERR_DATATRANSFER_LIMITED, "The number of ports exceeds the upper limit." },
    { NlErrCode::NL_ERR_DATATRANSFER_NO_REGISTER, "No port associated with this UUID is registered." },
    { NlErrCode::NL_ERR_DATATRANSFER_WRITE_DATA_CONGESTED, "Data transmission congested." },
    { NlErrCode::NL_ERR_CONNECTION_NOT_ESTABLISHED, "No SSAP connection with the remote device." },
    { NlErrCode::NL_ERR_INVALID_INTERGER, "Integer out of range." },
    { NlErrCode::NL_ERR_INVALID_ADDRESS, "Invalid address." },
    { NlErrCode::NL_ERR_EMPTY_ARRAY, "Empty array." },
    { NlErrCode::NL_ERR_INVALID_UUID, "Invalid UUID." },
    { NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED, "NearLink standard UUIDs not allowed." },
    { NlErrCode::NL_ERR_INVALID_PASSCODE, "Passcode must be a 6-digit number." },
    { NlErrCode::NL_ERR_STRING_LENGTH_LIMITED, "String exceeds maximum length." },
    { NlErrCode::NL_ERR_PEER_NOT_SUPPORT_BATTERY_SERVICE, "The remote device does not support battery service." },
    { NlErrCode::NL_ERR_INTERNAL_ERROR, "Operation failed." },
    { NlErrCode::NL_ERR_CDSM_NOT_SUPPORT, "Coordinated Devices Set Management not supported."},
    { NlErrCode::NL_ERR_INVALID_ADV_ID, "Invalid advertising ID."},
};

std::string GetAniErrMsg(const int32_t errCode)
{
    auto iter = aniErrCommonMsgMap.find(errCode);
    if (iter != aniErrCommonMsgMap.end()) {
        return iter->second;
    }
    return "";
}

void ConvertAniError(int32_t &errCode, std::string &errMsg)
{
    errMsg = GetAniErrMsg(errCode);
    // 如果错误信息无效，表示未识别的错误码，判定为内部错误
    if (errMsg == "") {
        HILOGW("unknown errCode: %{public}d, covert to internal error", errCode);
        errCode = NlErrCode::NL_ERR_INTERNAL_ERROR;
        errMsg = "Operation failed.";
    }

    // 内部错误码转换为标准错误码抛出，错误信息差异化
    if (errCode == NlErrCode::NL_ERR_INVALID_ADV_ID) {
        errCode = NlErrCode::NL_ERR_INVALID_INTERGER;
    } else if (errCode == NlErrCode::NL_ERR_PROFILE_PROHIBITED_BY_EDM) {
        errCode = NlErrCode::NL_ERR_INTERNAL_ERROR;
    }

    // 将错误码转换到正确范围
    if (errCode >= static_cast<int32_t>(NlErrCode::NL_ERR_BASE_SYSCAP) &&
        errCode <= static_cast<int32_t>(NlErrCode::NL_ERR_BASE_SYSCAP) + NL_ERR_CODE_MAX) {
        // 业务错误码10097XXXXX，转换为361XXXXX
        errCode = errCode - static_cast<int32_t>(NlErrCode::NL_ERR_BASE_SYSCAP) + NL_ERR_SYSCAP_BASE_CODE;
    }
}

void HandleSyncErr(int32_t errCode)
{
    if (errCode == NlErrCode::NL_NO_ERROR) {
        return;
    }
    std::string errMsg = "";
    ConvertAniError(errCode, errMsg);
    taihe::set_business_error(errCode, errMsg.c_str());
}
} // namespace Nearlink
} // namespace OHOS