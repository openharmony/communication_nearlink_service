/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "nearlink_hadm_client_stub.h"
#include "nearlink_permission_manager.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkHadmClientInterfaceCode::code, \
    {NearlinkHadmClientStub::func, perm}
namespace OHOS {
namespace Nearlink {

NearlinkHadmClientStub::NearlinkHadmClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_REGISTER_HADM_CLIENT_CALLBACK, RegisterNearlinkHadmClientCallbackInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_DE_REGISTER_HADM_CLIENT_CALLBACK, DeregisterNearlinkHadmClientCallbackInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_HADM_CLIENT_START_SOUNDING, StartSoundingInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_HADM_CLIENT_STOP_SOUNDING, StopSoundingInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_HADM_FEATURE, GetHadmFeatureInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
    };
}

NearlinkHadmClientStub::~NearlinkHadmClientStub()
{}

int NearlinkHadmClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkHadmClient", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkHadmClientStub);
}

int32_t NearlinkHadmClientStub::RegisterNearlinkHadmClientCallbackInner(NearlinkHadmClientStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkHadmClientCallback> callback = OHOS::iface_cast<INearlinkHadmClientCallback>(remote);
    NL_CHECK_RETURN_RET(callback, NL_ERR_IPC_TRANS_FAILED, "callback is nullptr.");
    uint32_t hadmId = 0;
    NlErrCode status = stub->RegisterNearlinkHadmClientCallback(hadmId, callback);
    bool ret = reply.WriteInt32(status);
    bool idRet = reply.WriteInt32(hadmId);
    NL_CHECK_RETURN_RET(ret && idRet, TRANSACTION_ERR, "reply WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHadmClientStub::DeregisterNearlinkHadmClientCallbackInner(NearlinkHadmClientStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    uint32_t hadmId = data.ReadUint32();
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkHadmClientCallback> callBack = OHOS::iface_cast<INearlinkHadmClientCallback>(remote);
    NL_CHECK_RETURN_RET(callBack, NL_ERR_IPC_TRANS_FAILED, "callBack is nullptr.");
    NlErrCode status = stub->DeregisterNearlinkHadmClientCallback(hadmId, callBack);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHadmClientStub::StartSoundingInner(NearlinkHadmClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    uint32_t hadmId = data.ReadUint32();
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "addr read failed.");
    NlErrCode result = stub->StartSounding(hadmId, *addr);
    bool ret = reply.WriteInt32(result);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply writing failed.");
    return NO_ERROR;
}

int32_t NearlinkHadmClientStub::StopSoundingInner(NearlinkHadmClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    uint32_t hadmId = data.ReadUint32();
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "addr read failed.");
    NlErrCode result = stub->StopSounding(hadmId, *addr);
    bool ret = reply.WriteInt32(result);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply writing failed.");
    return NO_ERROR;
}

int32_t NearlinkHadmClientStub::GetHadmFeatureInner(NearlinkHadmClientStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    uint8_t capability = 0;
    NlErrCode status = stub->GetHadmFeature(capability);
    bool ret = reply.WriteInt32(status);
    bool capabilityRet = reply.WriteUint8(capability);
    NL_CHECK_RETURN_RET(ret && capabilityRet, TRANSACTION_ERR, "reply WriteUint8 failed.");
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS