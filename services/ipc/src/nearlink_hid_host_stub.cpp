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

#include "nearlink_hid_host_stub.h"
#include "nearlink_errorcode.h"
#include "ipc_types.h"
#include "nearlink_def.h"
#include "i_nearlink_hid_host.h"
#include "nearlink_permission_manager.h"
#include "nearlink_utils.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkHidHostInterfaceCode::code, {NearlinkHidHostStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkHidHostStub::NearlinkHidHostStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_SET_REPORT, HidHostSetReportInner, CHECK_PERM(false, {MANAGE_NEARLINK}))}
    };
}

NearlinkHidHostStub::~NearlinkHidHostStub()
{}

int32_t NearlinkHidHostStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkHidHost", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkHidHostStub);
}

int32_t NearlinkHidHostStub::HidHostSetReportInner(NearlinkHidHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "Read address failed.");
    uint8_t type;
    NL_CHECK_RETURN_RET(data.ReadUint8(type), TRANSACTION_ERR, "Read type failed.");
    std::string report;
    NL_CHECK_RETURN_RET(data.ReadString(report), TRANSACTION_ERR, "Read report failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address),  INVALID_DATA, "address is invalid, data trans err !");
    int result;
    NlErrCode ec = stub->HidHostSetReport(address, type, report, result);
    if (ec == NL_NO_ERROR) {
        NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "result writing failed.");
    }
    return TRANSACTION_ERR;
}

}
}