/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "log.h"
#include "ipc_types.h"
#include "nearlink_cloud_pair_stub.h"
#include "nearlink_cloud_pair_device.h"
#include "nearlink_permission_manager.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm)             \
    NearlinkCloudPairInterfaceCode::code, \
    {                                           \
        NearlinkCloudPairStub::func, perm \
    }

namespace OHOS::Nearlink {

NearlinkCloudPairStub::NearlinkCloudPairStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_UPDATE_CLOUD_DEVICE_INFO_LIST, UpdateCloudDeviceInfoListInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},

        {STUB_FUNC(NL_GET_CLOUD_PAIR_STATE, GetCloudPairStateInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
    };
}

NearlinkCloudPairStub::~NearlinkCloudPairStub()
{}

int32_t NearlinkCloudPairStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkCloudPair", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkCloudPairStub);
}

int32_t NearlinkCloudPairStub::UpdateCloudDeviceInfoListInner(NearlinkCloudPairStub *stub, MessageParcel &data,
                                                             MessageParcel &reply)
{
    uint32_t size;
    NL_CHECK_RETURN_RET(data.ReadUint32(size), TRANSACTION_ERR, "ReadUint32 failed");
    NL_CHECK_RETURN_RET(size < CLOUD_PAIR_DEVICE_SIZE_MAX, INVALID_DATA, "Read cloudDevices size error");

    std::vector<NearlinkCloudPairDevice> trustDevices;
    for (uint32_t i = 0; i < size; i++) {
        std::shared_ptr<NearlinkCloudPairDevice> device(data.ReadParcelable<NearlinkCloudPairDevice>());
        if (device == nullptr) {
            HILOGE("[CLOUD_PAIR] UpdateCloudDeviceInfoList create device failed");
            continue;
        }
        trustDevices.push_back(*device);
    }

    NlErrCode result = stub->UpdateCloudDeviceInfoList(trustDevices);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("reply writing failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkCloudPairStub::GetCloudPairStateInner(NearlinkCloudPairStub *stub, MessageParcel &data,
                                                     MessageParcel &reply)
{
    std::string address;
    int32_t cloudPairState = 0;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode status = stub->GetCloudPairState(address, cloudPairState);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(cloudPairState), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

} // namespace OHOS::Nearlink