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
#include "nearlink_def.h"
#include "log.h"
#include "log_util.h"
#include "nearlink_errorcode.h"
#include "nearlink_sa_manager.h"
#include "i_nearlink_cdsm_client.h"
#include "nearlink_cdsm_client_callback_stub.h"
#include "nearlink_host.h"
#include "nearlink_cdsm_client.h"

namespace OHOS {
namespace Nearlink {
struct NearlinkCdsmClient::impl {
    impl(const NearlinkRemoteDevice &device, const std::shared_ptr<NearlinkCdsmClientCallback> &callback);
    ~impl();

    void Init(const std::weak_ptr<NearlinkCdsmClient> &client);

    NearlinkRemoteDevice device_;
    std::weak_ptr<NearlinkCdsmClientCallback> callback_;
    class NearlinkCdsmClientCallbackStubImpl;
    sptr<NearlinkCdsmClientCallbackStubImpl> callbackStubImpl_ = nullptr;

    int32_t profileRegisterId_{0};
};

class NearlinkCdsmClient::impl::NearlinkCdsmClientCallbackStubImpl : public NearlinkCdsmClientCallbackStub {
public:
    explicit NearlinkCdsmClientCallbackStubImpl(std::weak_ptr<NearlinkCdsmClient> client)
        : client_(std::move(client)) {}

    ~NearlinkCdsmClientCallbackStubImpl() = default;

    void OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo) override
    {
        HILOGI("[Cdsm]Enter size=%{public}zu", cdsInfo.GetCdsMemberList().size());
        std::shared_ptr<NearlinkCdsmClient> nearlinkCdsmClientSptr = client_.lock();
        NL_CHECK_RETURN(nearlinkCdsmClientSptr, "[Cdsm]nearlinkCdsmClientSptr is nullptr.");

        std::shared_ptr<NearlinkCdsmClientCallback> callbackSptr = nearlinkCdsmClientSptr->pimpl->callback_.lock();
        NL_CHECK_RETURN(callbackSptr, "[Cdsm]callbackSptr is nullptr.");

        callbackSptr->OnCdsInfoChanged(cdsInfo);
    }

private:
    std::weak_ptr<NearlinkCdsmClient> client_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCdsmClientCallbackStubImpl);
};

NearlinkCdsmClient::impl::impl(const NearlinkRemoteDevice &device,
                               const std::shared_ptr<NearlinkCdsmClientCallback> &callback)
    : device_(device), callback_(callback) {}

NearlinkCdsmClient::impl::~impl()
{
    HILOGD("[Cdsm]Enter");
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("[Cdsm]profileRegisterId_ is invalid");
        return;
    }
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkCdsmClient> proxy = GetProxy<INearlinkCdsmClient>(PROFILE_CDSM_CLIENT_SERVER);
    NL_CHECK_RETURN(proxy, "[Cdsm]proxy is nullptr.");
    proxy->DeregisterCdsmClientCallback(NearlinkRawAddress(device_.GetDeviceAddr()));
}

NearlinkCdsmClient::NearlinkCdsmClient(NearlinkRemoteDevice &device,
                                       std::shared_ptr<NearlinkCdsmClientCallback> &callback)
{
    HILOGD("[Cdsm]Enter");
    pimpl = std::make_unique<impl>(device, callback);
    NL_CHECK_RETURN(pimpl, "[Cdsm]pimpl is nullptr");
}

NearlinkCdsmClient::~NearlinkCdsmClient()
{
    HILOGD("[Cdsm]Enter");
}

std::shared_ptr<NearlinkCdsmClient> NearlinkCdsmClient::CreateNearlinkCdsmClient(
    NearlinkRemoteDevice &device, std::shared_ptr<NearlinkCdsmClientCallback> &callback)
{
    HILOGD("[Cdsm]Enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), nullptr,
        "[Cdsm]nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), nullptr, "[Cdsm]nearlink is off");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), nullptr, "[Cdsm]device is nullptr.");
    auto nearlinkCdsmClient = std::make_shared<NearlinkCdsmClient>(device, callback);
    NL_CHECK_RETURN_RET(nearlinkCdsmClient, nullptr, "[Cdsm]Create NearlinkCdsmClient failed.");
    nearlinkCdsmClient->pimpl->Init(nearlinkCdsmClient);
    return nearlinkCdsmClient;
}

NlErrCode NearlinkCdsmClient::GetCdsmInfo(NearlinkCdsInfo& cdsInfo) const
{
    HILOGD("[Cdsm]Enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "[Cdsm]nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "[Cdsm]nearlink is off");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkAudioSupport(), NL_ERR_CDSM_NOT_SUPPORT,
        "[Cdsm]Coordinated Devices Set Management not supported.");

    sptr<INearlinkCdsmClient> proxy = GetProxy<INearlinkCdsmClient>(PROFILE_CDSM_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "[Cdsm]proxy is nullptr.");

    NearlinkRawAddress device(pimpl->device_.GetDeviceAddr());
    NearlinkCdsInfoParcel parcelCdsInfo;
    NlErrCode ret = proxy->GetCdsmInfo(device, parcelCdsInfo);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "[Cdsm]get cdsm info failed, ret=%{public}d", ret);
    cdsInfo = parcelCdsInfo;
    return NL_NO_ERROR;
}

void NearlinkCdsmClient::impl::Init(const std::weak_ptr<NearlinkCdsmClient> &client)
{
    HILOGD("[Cdsm]Enter");
    callbackStubImpl_ = sptr<NearlinkCdsmClientCallbackStubImpl>::MakeSptr(client);
    NL_CHECK_RETURN(callbackStubImpl_, "[Cdsm]callbackStubImpl_ is nullptr.");
    const auto info = std::make_shared<NearlinkRegisterInfo>(PROFILE_CDSM_CLIENT_SERVER);
    NL_CHECK_RETURN(info, "[Cdsm]info is nullptr.");
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkCdsmClient> proxy = iface_cast<INearlinkCdsmClient>(remote);
        NL_CHECK_RETURN(proxy, "[Cdsm]proxy is nullptr.");
        NL_CHECK_RETURN(callbackStubImpl_, "[Cdsm]callbackStubImpl_ is nullptr.");
        int result = proxy->RegisterCdsmClientCallback(NearlinkRawAddress(device_.GetDeviceAddr()), callbackStubImpl_);
        HILOGI("[Cdsm]NearlinkCdsmClient::Init result=%{public}d", result);
        if (result != NL_NO_ERROR) {
            HILOGE("[Cdsm]Can not Register to cdsm client service! result(%{public}d)", result);
        }
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("[Cdsm]profileRegisterId_ is invalid");
    }
}

} // namespace Nearlink
} // namespace OHOS