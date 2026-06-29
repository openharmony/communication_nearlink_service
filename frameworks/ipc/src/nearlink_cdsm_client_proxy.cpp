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
#include "nearlink_cdsm_client_proxy.h"
#include "log.h"
#include "nearlink_service_ipc_interface_code.h"

namespace OHOS {
namespace Nearlink {
NearlinkCdsmClientProxy::NearlinkCdsmClientProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkCdsmClient>(impl)
{
    HILOGD("[Cdsm]Enter");
}

NearlinkCdsmClientProxy::~NearlinkCdsmClientProxy()
{
    HILOGD("[Cdsm]Enter");
}

NlErrCode NearlinkCdsmClientProxy::RegisterCdsmClientCallback(const NearlinkRawAddress &addr,
                                                              const sptr<INearlinkCdsmClientCallback> &callback)
{
    HILOGD("[Cdsm]Enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkCdsmClientProxy::GetDescriptor()),
                        NL_ERR_INTERNAL_ERROR, "[Cdsm]Write Token error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "[Cdsm]Write addr error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_INTERNAL_ERROR,
        "[Cdsm]Write callback error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "[Cdsm]done fail, ErrCode: %{public}d", result);

    NlErrCode ret = static_cast<NlErrCode>(reply.ReadInt32());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "[Cdsm]register cds callback fail:%{public}d", ret);
    return ret;
}

NlErrCode NearlinkCdsmClientProxy::DeregisterCdsmClientCallback(const NearlinkRawAddress &addr)
{
    HILOGD("[Cdsm]Enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkCdsmClientProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "[Cdsm]Write Token error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "[Cdsm]Write addr error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(
        NearlinkCdsmClientInterfaceCode::NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "[Cdsm]done fail, ErrCode: %{public}d", result);
    NlErrCode ret = static_cast<NlErrCode>(reply.ReadInt32());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "[Cdsm]deregister cds callback fail:%{public}d", ret);
    return ret;
}

NlErrCode NearlinkCdsmClientProxy::GetCdsmInfo(const NearlinkRawAddress &addr, NearlinkCdsInfoParcel &cdsInfo)
{
    HILOGD("[Cdsm]Enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkCdsmClientProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "[Cdsm]Write Token error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "[Cdsm]Write addr error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkCdsmClientInterfaceCode::NL_CDSM_CLIENT_GET_CDS_INFO, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "[Cdsm]done fail, error: %{public}d", error);
    NlErrCode ret = static_cast<NlErrCode>(reply.ReadInt32());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "[Cdsm]get cdsm info fail:%{public}d", ret);
    std::shared_ptr<NearlinkCdsInfoParcel> cdsInfoInner(reply.ReadParcelable<NearlinkCdsInfoParcel>());
    NL_CHECK_RETURN_RET(cdsInfoInner, NL_ERR_IPC_TRANS_FAILED, "[Cdsm]get cds info error");
    cdsInfo = *cdsInfoInner;
    return ret;
}

ErrCode NearlinkCdsmClientProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[Cdsm]fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGW("[Cdsm]fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("[Cdsm]fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS