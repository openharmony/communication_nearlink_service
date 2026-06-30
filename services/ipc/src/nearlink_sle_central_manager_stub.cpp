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

#include <map>
#include <vector>
#include "nearlink_permission_manager.h"
#include "log.h"
#include "ipc_types.h"
#include "nearlink_sle_central_manager_stub.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkSleCentralManagerInterfaceCode::code, \
    {NearlinkSleCentralManagerStub::func, perm}

namespace OHOS {
namespace Nearlink {
constexpr size_t FILTER_PARCEL_MAX_SIZE = 0xFF;

NearlinkSleCentralManagerStub::NearlinkSleCentralManagerStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(SLE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, RegisterSleCentralManagerCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, DeregisterSleCentralManagerCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_START_FULL_SCAN, StartFullScanInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(SLE_START_SCAN_WITH_FILTER, StartScanWithFilterInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_STOP_SCAN, StopScanInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
    };
}

NearlinkSleCentralManagerStub::~NearlinkSleCentralManagerStub()
{}

int32_t NearlinkSleCentralManagerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSleCentralManager", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSleCentralManagerStub);
}

int32_t NearlinkSleCentralManagerStub::RegisterSleCentralManagerCallbackInner(NearlinkSleCentralManagerStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleCentralManagerCallback> callBack =
        OHOS::iface_cast<INearlinkSleCentralManagerCallback>(remote);
    bool enableRandomAddrMode = data.ReadBool();
    uint32_t scannerId = 0;
    NlErrCode status = stub->RegisterSleCentralManagerCallback(scannerId, enableRandomAddrMode, callBack);
    if (!reply.WriteInt32(status)) {
        HILOGE("reply writing status failed");
        return TRANSACTION_ERR;
    }
    if (!reply.WriteUint32(scannerId)) {
        HILOGE("reply writing failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleCentralManagerStub::DeregisterSleCentralManagerCallbackInner(NearlinkSleCentralManagerStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    uint32_t scannerId = data.ReadUint32();
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleCentralManagerCallback> callBack =
        OHOS::iface_cast<INearlinkSleCentralManagerCallback>(remote);
    NlErrCode status = stub->DeregisterSleCentralManagerCallback(scannerId, callBack);
    if (!reply.WriteInt32(status)) {
        HILOGE("reply writing status failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleCentralManagerStub::StartFullScanInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    uint32_t scannerId = data.ReadUint32();
    std::shared_ptr<NearlinkSleScanSettings> settings(data.ReadParcelable<NearlinkSleScanSettings>());
    if (settings == nullptr) {
        HILOGW("[StartFullScanInner] fail: read settings failed");
        return ERR_INVALID_VALUE;
    }
    NlErrCode status = stub->StartFullScan(scannerId, *settings);
    if (!reply.WriteInt32(status)) {
        HILOGE("reply writing status failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleCentralManagerStub::StartScanWithFilterInner(NearlinkSleCentralManagerStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    uint32_t scannerId = data.ReadUint32();
    std::shared_ptr<NearlinkSleScanSettings> settings(data.ReadParcelable<NearlinkSleScanSettings>());
    if (settings == nullptr) {
        HILOGW("[StartScanWithFilterInner] fail: read settings failed");
        return ERR_INVALID_VALUE;
    }
    std::vector<NearlinkSleScanFilter> filters;
    uint32_t filterSize = data.ReadUint32();
    if (filterSize > FILTER_PARCEL_MAX_SIZE) {
        HILOGE("Parcel size is too big");
        return ERR_INVALID_VALUE;
    }
    for (uint32_t i = 0; i < filterSize; i++) {
        std::shared_ptr<NearlinkSleScanFilter> filter(data.ReadParcelable<NearlinkSleScanFilter>());
        if (filter == nullptr) {
            HILOGW("[StartScanWithFilterInner] fail: read filter failed");
            return ERR_INVALID_VALUE;
        }
        filters.push_back(*filter);
    }
    NlErrCode status = stub->StartScanWithFilter(scannerId, *settings, filters);
    if (!reply.WriteInt32(status)) {
        HILOGE("reply writing status failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleCentralManagerStub::StopScanInner(NearlinkSleCentralManagerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    uint32_t scannerId = data.ReadUint32();
    NlErrCode status = stub->StopScan(scannerId);
    if (!reply.WriteInt32(status)) {
        HILOGE("reply writing status failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS