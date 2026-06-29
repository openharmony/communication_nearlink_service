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

#include <memory>
#include "log.h"
#include "nearlink_permission_manager.h"
#include "nearlink_cdsm_client_stub.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkCdsmClientInterfaceCode::code, {NearlinkCdsmClientStub::func, perm}
namespace OHOS {
namespace Nearlink {

NearlinkCdsmClientStub::NearlinkCdsmClientStub()
{
    HILOGD("[Cdsm]enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
            {STUB_FUNC(NL_REGISTER_CDSM_CLIENT_CALLBACK, RegisterCdsmClientCallbackInner,
                       CHECK_PERM(false, {ACCESS_NEARLINK}))},
            {STUB_FUNC(NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, DeregisterCdsmClientCallbackInner,
                       CHECK_PERM(false, {ACCESS_NEARLINK}))},
            {STUB_FUNC(NL_CDSM_CLIENT_GET_CDS_INFO, GetCdsInfoInner,
                        CHECK_PERM(false, {ACCESS_NEARLINK}))},
    };
}

NearlinkCdsmClientStub::~NearlinkCdsmClientStub()
{
    HILOGD("[Cdsm]enter");
}

int NearlinkCdsmClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkCdsmClient", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkCdsmClientStub);
}

int32_t NearlinkCdsmClientStub::RegisterCdsmClientCallbackInner(
    NearlinkCdsmClientStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("[Cdsm]NearlinkCdsmClientStub::RegisterCdsmClientCallbackInner starts");
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "[Cdsm]read device failed.");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkCdsmClientCallback> callback = OHOS::iface_cast<INearlinkCdsmClientCallback>(remote);
    NL_CHECK_RETURN_RET(callback != nullptr, TRANSACTION_ERR, "[Cdsm]callback is nullptr.");
    NlErrCode result = stub->RegisterCdsmClientCallback(*addr, callback);
    bool ret = reply.WriteInt32(result);
    HILOGI("[Cdsm]NearlinkCdsmClientStub::Register result=%{public}d", result);
    NL_CHECK_RETURN_RET(ret, ERR_INVALID_VALUE, "[Cdsm]reply writing failed.");
    HILOGD("[Cdsm]NearlinkCdsmClientStub::RegisterCdsmClientCallbackInner end");
    return NO_ERROR;
}

int32_t NearlinkCdsmClientStub::DeregisterCdsmClientCallbackInner(
    NearlinkCdsmClientStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("[Cdsm]Enter");
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "[Cdsm]read device failed.");
    NlErrCode status = stub->DeregisterCdsmClientCallback(*addr);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "[Cdsm]reply WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkCdsmClientStub::GetCdsInfoInner(
    NearlinkCdsmClientStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("[Cdsm]Enter");
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "[Cdsm]read device failed.");
    NearlinkCdsInfoParcel cdsInfoInner;
    NlErrCode result = stub->GetCdsmInfo(*addr, cdsInfoInner);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "[Cdsm]WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteParcelable(&cdsInfoInner), TRANSACTION_ERR, "[Cdsm]WriteParcelable failed.");
    return NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS