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

#include <cmath>
#include "log.h"
#include "ipc_types.h"
#include "nearlink_sle_controller_stub.h"
#include "nearlink_permission_manager.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm)             \
    NearlinkSleControllerInterfaceCode::code, \
    {                                           \
        NearlinkSleControllerStub::func, perm \
    }

namespace OHOS::Nearlink {
constexpr size_t MAX_COEX_DEVICE_NUM = 32;

NearlinkSleControllerStub::NearlinkSleControllerStub()
{
    HILOGI("enter");
    memberFuncMap_ = {
        {STUB_FUNC(NL_SET_SLE_COEX_PARAM, SetSleCoexParamInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_SLE_UPDATE_INTERVAL, UpdateConnectIntervalInner,
            CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_SET_SLE_COEX_MODE, SetSleCoexModeInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
    };
}

NearlinkSleControllerStub::~NearlinkSleControllerStub()
{}

int32_t NearlinkSleControllerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSleController", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSleControllerStub);
}

int32_t NearlinkSleControllerStub::SetSleCoexParamInner(NearlinkSleControllerStub *stub, MessageParcel &data,
                                                         MessageParcel &reply)
{
    HILOGD("enter");
    uint16_t maxBitRate = 0;
    uint8_t dutyCycle = 0;

    NL_CHECK_RETURN_RET(data.ReadUint16(maxBitRate), TRANSACTION_ERR, "ReadUint16 failed.");
    NL_CHECK_RETURN_RET(data.ReadUint8(dutyCycle), TRANSACTION_ERR, "ReadUint8 failed.");
    NlErrCode result = stub->SetSleCoexParam(maxBitRate, dutyCycle);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkSleControllerStub::UpdateConnectIntervalInner(
    NearlinkSleControllerStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::string device;
    NL_CHECK_RETURN_RET(data.ReadString(device), TRANSACTION_ERR, "Read address failed.");
    int32_t intervalType;
    NL_CHECK_RETURN_RET(data.ReadInt32(intervalType), TRANSACTION_ERR, "Read type failed.");

    NlErrCode status = stub->UpdateConnectInterval(device, intervalType);
    HILOGI("status: %{public}d, intervalType: %{public}d", status, intervalType);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkSleControllerStub::SetSleCoexModeInner(NearlinkSleControllerStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    int32_t mode = 0;
    int32_t deviceSize = 0;
    std::vector<std::string> deviceList = {};
    std::vector<ConnectionInterval> paramList = {};
    NL_CHECK_RETURN_RET(data.ReadInt32(mode), TRANSACTION_ERR, "Read mode failed.");
    NL_CHECK_RETURN_RET(data.ReadInt32(deviceSize), TRANSACTION_ERR, "Read size failed.");

    deviceSize = std::min(deviceSize, static_cast<int32_t>(MAX_COEX_DEVICE_NUM));
    for (int32_t i = 0; i < deviceSize; i++) {
        std::string device;
        NL_CHECK_RETURN_RET(data.ReadString(device), TRANSACTION_ERR, "Read address failed.");
        deviceList.emplace_back(device);
        int32_t param;
        NL_CHECK_RETURN_RET(data.ReadInt32(param), TRANSACTION_ERR, "Read param failed.");
        paramList.emplace_back(static_cast<ConnectionInterval>(param));
    }
 
    NL_CHECK_RETURN_RET(deviceList.size() == paramList.size(), TRANSACTION_ERR,
        "device list and param list size mismatch");
    NlErrCode result = stub->SetSleCoexMode(mode, deviceList, paramList);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}
}  // namespace OHOS::Nearlink
