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

#include "log.h"
#include "ipc_types.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_def.h"
#include "nearlink_sle_advertiser_stub.h"
#include "nearlink_permission_manager.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkSleAdvertiserInterfaceCode::code, {NearlinkSleAdvertiserStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkSleAdvertiserStub::NearlinkSleAdvertiserStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(SLE_REGISTER_SLE_ADVERTISER_CALLBACK, RegisterSleAdvertiserCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_DE_REGISTER_SLE_ADVERTISER_CALLBACK, DeregisterSleAdvertiserCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_START_ADVERTISING, StartAdvertisingInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_STOP_ADVERTISING, StopAdvertisingInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_GET_ADVERTISER_HANDLE, GetAdvertiserHandleInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_SET_ADVERTISING_DATA, SetAdvertisingDataInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SLE_ENABLE_ADVERTISING, EnableAdvertisingInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SLE_DISABLE_ADVERTISING, DisableAdvertisingInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
    };
}

NearlinkSleAdvertiserStub::~NearlinkSleAdvertiserStub()
{}

int32_t NearlinkSleAdvertiserStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSleAdvertiser", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSleAdvertiserStub);
}

int32_t NearlinkSleAdvertiserStub::RegisterSleAdvertiserCallbackInner(NearlinkSleAdvertiserStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleAdvertiseCallback> callBack = OHOS::iface_cast<INearlinkSleAdvertiseCallback>(remote);
    NL_CHECK_RETURN_RET(callBack, TRANSACTION_ERR, "callBack is nullptr");
    NlErrCode status = stub->RegisterSleAdvertiserCallback(callBack);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::DeregisterSleAdvertiserCallbackInner(NearlinkSleAdvertiserStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleAdvertiseCallback> callBack = OHOS::iface_cast<INearlinkSleAdvertiseCallback>(remote);
    NL_CHECK_RETURN_RET(callBack, TRANSACTION_ERR, "callBack is nullptr");
    NlErrCode status = stub->DeregisterSleAdvertiserCallback(callBack);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::StartAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleAdvertiserSettings> settings(data.ReadParcelable<NearlinkSleAdvertiserSettings>());
    if (settings == nullptr) {
        HILOGW("[StartAdvertisingInner] fail: read settings failed");
        return TRANSACTION_ERR;
    }

    std::shared_ptr<NearlinkSleAdvertiserData> advData(data.ReadParcelable<NearlinkSleAdvertiserData>());
    if (advData == nullptr) {
        HILOGW("[StartAdvertisingInner] fail: read advData failed");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSleAdvertiserData> scanResponse(data.ReadParcelable<NearlinkSleAdvertiserData>());
    if (scanResponse == nullptr) {
        HILOGW("[StartAdvertisingInner] fail: read scanResponse failed");
        return TRANSACTION_ERR;
    }

    int32_t advHandle = data.ReadInt32();
    NlErrCode status = stub->StartAdvertising(*settings, *advData, *scanResponse, advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::StopAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int32_t advHandle = data.ReadInt32();
    NlErrCode status = stub->StopAdvertising(advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::GetAdvertiserHandleInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int advHandle = static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
    NlErrCode status = stub->GetAdvertiserHandle(advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply writing failed");
        return TRANSACTION_ERR;
    }
    ret = reply.WriteInt32(advHandle);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply writing failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::SetAdvertisingDataInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleAdvertiserData> advData(data.ReadParcelable<NearlinkSleAdvertiserData>());
    if (advData == nullptr) {
        HILOGW("[SetAdvertisingDataInner] fail: read advData failed");
        return TRANSACTION_ERR;
    }

    std::shared_ptr<NearlinkSleAdvertiserData> scanResponse(data.ReadParcelable<NearlinkSleAdvertiserData>());
    if (scanResponse == nullptr) {
        HILOGW("[SetAdvertisingDataInner] fail: read scanResponse failed");
        return TRANSACTION_ERR;
    }

    int32_t advHandle = data.ReadInt32();
    NlErrCode status = stub->SetAdvertisingData(*advData, *scanResponse, advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::EnableAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int32_t advHandle = data.ReadInt32();
    NlErrCode status = stub->EnableAdvertising(advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkSleAdvertiserStub::DisableAdvertisingInner(NearlinkSleAdvertiserStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int32_t advHandle = data.ReadInt32();
    NlErrCode status = stub->DisableAdvertising(advHandle);
    bool ret = reply.WriteInt32(status);
    if (!ret) {
        HILOGE("NearlinkSleAdvertiserStub: reply WriteInt32 failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
