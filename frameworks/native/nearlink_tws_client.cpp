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

#include "nearlink_tws_client.h"

#include "i_nearlink_tws_client.h"
#include "log.h"
#include "log_util.h"
#include "nearlink_utils.h"
#include "nearlink_host.h"
#include "nearlink_sa_manager.h"
#include "nearlink_safe_weak_list.h"
#include "nearlink_def.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "nearlink_errorcode.h"
#include "nearlink_tws_client_observer_stub.h"
#include "nearlink_ASC_source.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint32_t STREAM_DATA_MAX_LEN = 255;
}

struct NearlinkTwsClient::impl {
    impl();
    ~impl();
    class TwsClientObserverStubImpl;
    sptr<TwsClientObserverStubImpl> clientCallback_ = nullptr;

    // Stores the observers registered by the application.
    NearlinkSafeWeakList<NearlinkTwsClientObserver> twsClientObserverList;

    int32_t profileRegisterId_ = 0;
};
class NearlinkTwsClient::impl::TwsClientObserverStubImpl : public NearlinkTwsClientObserverStub {
public:
    explicit TwsClientObserverStubImpl(NearlinkTwsClient::impl &twsClient)
        : twsClient_(twsClient)
    {
        HILOGI("[NearlinkTwsClient] TwsClientObserverStubImpl ");
    }

    ~TwsClientObserverStubImpl()
    {
        HILOGI("[NearlinkTwsClient] ~TwsClientObserverStubImpl ");
    }

    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) override
    {
        HILOGI("[NearlinkTwsClient] OnTwsRemoteInfo %{public}s", GetEncryptAddr(address).c_str());
        twsClient_.twsClientObserverList.IterateAsync(
            [address, value](std::shared_ptr<NearlinkTwsClientObserver> observer) -> void {
                observer->OnTwsRemoteInfo(address, value);
            });
    }

private:
    NearlinkTwsClient::impl &twsClient_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(TwsClientObserverStubImpl);
};

NearlinkTwsClient::impl::impl()
{
    HILOGI("start");
    clientCallback_ = new (std::nothrow) TwsClientObserverStubImpl(*this);
    NL_CHECK_RETURN(clientCallback_, "clientCallback_ is nullptr.");
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN(info, "info is nullptr.");

    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkTwsClient> proxy = iface_cast<INearlinkTwsClient>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        NL_CHECK_RETURN(clientCallback_, "clientCallback_ is nullptr.");
        int result = proxy->RegisterApplication(clientCallback_);
        if (result != NL_NO_ERROR) {
            HILOGE("Can not Register to TWS client service! result(%{public}d)", result);
        }
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

NearlinkTwsClient::impl::~impl()
{
    HILOGI("starts");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN(proxy, "proxy is nullptr.");
    proxy->DeregisterApplication(clientCallback_);
}

NearlinkTwsClient::NearlinkTwsClient() : pimpl(std::make_unique<impl>())
{}

NearlinkTwsClient::~NearlinkTwsClient()
{}

NlErrCode NearlinkTwsClient::RegisterTwsClientObserver(std::shared_ptr<NearlinkTwsClientObserver> observer)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->twsClientObserverList.Insert(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClient::DeregisterTwsClientObserver(std::shared_ptr<NearlinkTwsClientObserver> observer)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->twsClientObserverList.Erase(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkTwsClient::EnableWearDetection(const std::string &address)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->EnableWearDetection(address);
}

NlErrCode NearlinkTwsClient::DisableWearDetection(const std::string &address)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->DisableWearDetection(address);
}

NlErrCode NearlinkTwsClient::GetWearDetectionState(const std::string &address, int32_t &state)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetWearDetectionState(address, state);
}

NlErrCode NearlinkTwsClient::IsDeviceWearing(const std::string &address, bool &isWearing)
{
    HILOGD("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->IsDeviceWearing(address, isWearing);
}

NlErrCode NearlinkTwsClient::IsWearDetectionSupported(const std::string &address, bool &isSupported)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->IsWearDetectionSupported(address, isSupported);
}

NlErrCode NearlinkTwsClient::GetTwsRoleInfo(const std::string &address, int32_t &roleInfo)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetTwsRoleInfo(address, roleInfo);
}

NlErrCode NearlinkTwsClient::GetTwsAudioDelay(const std::string &address, uint32_t &delayValue)
{
    HILOGD("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetTwsAudioDelay(address, delayValue);
}

NlErrCode NearlinkTwsClient::IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->IsSupportVirtualAutoConnect(address, isSupported);
}

NlErrCode NearlinkTwsClient::SetVirtualAutoConnectType(const std::string &address,
    int32_t connType, int32_t businessType)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SetVirtualAutoConnectType(address, connType, businessType);
}

NearlinkTwsClient &NearlinkTwsClient::GetInstance()
{
    static NearlinkTwsClient instance;
    return instance;
}

NlErrCode NearlinkTwsClient::SendUserSelection(const std::string &address,
    const std::vector<struct AudioStreamInfo> &streamData)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(streamData.size() < STREAM_DATA_MAX_LEN, NL_ERR_INVALID_PARAM, "invalid streamData");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SendUserSelection(address, streamData);
}

NlErrCode NearlinkTwsClient::QueryStreamState(const std::string &address,
    std::vector<struct AudioStreamInfo> &streamData)
{
    HILOGI("[NearlinkTwsClient] address %{public}s", GetEncryptAddr(address).c_str());
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
                        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INVALID_PARAM, "invalid address");
    NL_CHECK_RETURN_RET(streamData.size() < STREAM_DATA_MAX_LEN, NL_ERR_INVALID_PARAM, "invalid streamData");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkTwsClient> proxy = GetProxy<INearlinkTwsClient>(NEARLINK_TWS_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->QueryStreamState(address, streamData);
}

} // namespace Nearlink
} // namespace OHOS

