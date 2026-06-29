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
#include "log_util.h"
#include "i_nearlink_hadm_client.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sa_manager.h"
#include "nearlink_raw_address.h"
#include "ranging_alogorithm_adapter.h"
#include "nearlink_hadm_client_callback_stub.h"
#include "nearlink_sle_ranging.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkSleRanging::impl {
    impl(std::shared_ptr<SleRangingCallback> callback);
    ~impl();
    void Init(std::weak_ptr<NearlinkSleRanging> nearlinkSleRanging);

    class HadmClientCallbackStubImpl;
    sptr<HadmClientCallbackStubImpl> callbackStubImp_ = nullptr;
    std::weak_ptr<SleRangingCallback> callback_;
    uint32_t hadmId_ = SLE_HADM_INVALID_ID;
    int32_t profileRegisterId_{0};
};

class NearlinkSleRanging::impl::HadmClientCallbackStubImpl : public NearlinkHadmClientCallbackStub {
public:
    explicit HadmClientCallbackStubImpl(std::weak_ptr<NearlinkSleRanging> nearlinkSleRanging)
        : nearlinkSleRanging_(nearlinkSleRanging)
    {}

    ~HadmClientCallbackStubImpl()
    {}

    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) override
    {
        std::shared_ptr<NearlinkSleRanging> nearlinkSleRangingSptr = nearlinkSleRanging_.lock();
        NL_CHECK_RETURN(nearlinkSleRangingSptr, "nearlinkSleRangingSptr is nullptr.");
        DisResult hdamRangingResult = {0};
        int ret = HadmRangingAdapter::GetInstance().CalculateHadmDistance(result, hdamRangingResult);
        std::shared_ptr<SleRangingCallback> callbackSptr = nearlinkSleRangingSptr->pimpl->callback_.lock();
        NL_CHECK_RETURN(callbackSptr, "callbackSptr is nullptr.");
        RangingResult rangingResult(addr.GetAddress(), ret, hdamRangingResult.disSmoothed);
        HILOGI("sle ranging dis:%{public}f, ret:%{public}d, hadmId:%{public}u", hdamRangingResult.disSmoothed,
            ret, nearlinkSleRangingSptr->pimpl->hadmId_);
        callbackSptr->OnSleRangingResult(rangingResult);
    }

    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) override
    {
        std::shared_ptr<NearlinkSleRanging> nearlinkSleRangingSptr = nearlinkSleRanging_.lock();
        NL_CHECK_RETURN(nearlinkSleRangingSptr, "nearlinkSleRangingSptr is nullptr.");
        std::shared_ptr<SleRangingCallback> callbackSptr = nearlinkSleRangingSptr->pimpl->callback_.lock();
        NL_CHECK_RETURN(callbackSptr, "callbackSptr is nullptr.");
        RangingState rangingState(addr.GetAddress(), newState, errorCode);
        HILOGI("sle ranging errorCode %{public}d, hadmId:%{public}u, newState:%{public}d",
            errorCode, nearlinkSleRangingSptr->pimpl->hadmId_, newState);
        callbackSptr->OnSleRangingStateChange(rangingState);
    }

private:
    std::weak_ptr<NearlinkSleRanging> nearlinkSleRanging_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(HadmClientCallbackStubImpl);
};

NearlinkSleRanging::impl::impl(std::shared_ptr<SleRangingCallback> callback) : callback_(callback)
{}

NearlinkSleRanging::impl::~impl()
{
    HILOGI("sle ranging impl id=%{public}d destructor enter", hadmId_);
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    if (callbackStubImp_ != nullptr) {
        sptr<INearlinkHadmClient> proxy = GetProxy<INearlinkHadmClient>(NEARLINK_HADM_CLIENT_SERVER);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        proxy->DeregisterNearlinkHadmClientCallback(hadmId_, callbackStubImp_);
    }
}

NearlinkSleRanging::NearlinkSleRanging(std::shared_ptr<SleRangingCallback> callback) : pimpl(nullptr)
{
    if (pimpl == nullptr) {
        pimpl = std::make_unique<impl>(callback);
        NL_CHECK_RETURN(pimpl, "pimpl is nullptr");
    }
}

NearlinkSleRanging::~NearlinkSleRanging()
{
}

void NearlinkSleRanging::impl::Init(std::weak_ptr<NearlinkSleRanging> nearlinkSleRanging)
{
    callbackStubImp_ = new (std::nothrow) HadmClientCallbackStubImpl(nearlinkSleRanging);
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_HADM_CLIENT_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkHadmClient> proxy = iface_cast<INearlinkHadmClient>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        NL_CHECK_RETURN(callbackStubImp_, "callbackStubImp_ is nullptr.");
        proxy->RegisterNearlinkHadmClientCallback(hadmId_, callbackStubImp_);
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        hadmId_ = SLE_HADM_INVALID_ID;
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}


std::shared_ptr<NearlinkSleRanging> NearlinkSleRanging::CreateNearlinkSleRanging(
    std::shared_ptr<SleRangingCallback> callback)
{
    std::shared_ptr<NearlinkSleRanging> nearlinkSleRanging = std::make_shared<NearlinkSleRanging>(Pattern(), callback);
    NL_CHECK_RETURN_RET(nearlinkSleRanging, nullptr, "Create NearlinkSleRanging failed.");
    nearlinkSleRanging->pimpl->Init(nearlinkSleRanging);
    return nearlinkSleRanging;
}

NlErrCode NearlinkSleRanging::StartSleRanging(const NearlinkRemoteDevice &device, const RangingConfig &config)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "invalid device");
    NL_CHECK_RETURN_RET(pimpl->hadmId_ != SLE_HADM_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid hadmId");
    HILOGI("sle ranging start, device:%{public}s, hadmId:%{public}u ",
        GetEncryptAddr((device).GetDeviceAddr()).c_str(), pimpl->hadmId_);
    sptr<INearlinkHadmClient> proxy = GetProxy<INearlinkHadmClient>(NEARLINK_HADM_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NL_CHECK_RETURN_RET(HadmRangingAdapter::GetInstance().InitHadmAlgo(), NL_ERR_INTERNAL_ERROR, "init Hadm failed.");
    NearlinkRawAddress rawAddr(device.GetDeviceAddr());
    return proxy->StartSounding(pimpl->hadmId_, rawAddr);
}

NlErrCode NearlinkSleRanging::StopSleRanging(const NearlinkRemoteDevice &device)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "invalid device");
    NL_CHECK_RETURN_RET(pimpl->hadmId_ != SLE_HADM_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid hadmId");
    HILOGI("sle ranging stop, device:%{public}s, hadmId:%{public}u ",
        GetEncryptAddr((device).GetDeviceAddr()).c_str(), pimpl->hadmId_);
    sptr<INearlinkHadmClient> proxy = GetProxy<INearlinkHadmClient>(NEARLINK_HADM_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    HadmRangingAdapter::GetInstance().CleanUp();
    NearlinkRawAddress rawAddr(device.GetDeviceAddr());
    return proxy->StopSounding(pimpl->hadmId_, rawAddr);
}

NlErrCode NearlinkSleRanging::GetRangingSupportedCapability(uint8_t &capability)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHadmClient> proxy = GetProxy<INearlinkHadmClient>(NEARLINK_HADM_CLIENT_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NlErrCode result = proxy->GetHadmFeature(capability);
    HILOGI("sle ranging capability:%{public}u", capability);
    return result;
}

RangingConfig::RangingConfig()
{}

RangingConfig::~RangingConfig()
{}

RangingConfig::RangingConfig(uint8_t refreshRate) : refreshRate_(refreshRate)
{}

void RangingConfig::SetRefreshRate(const uint8_t refreshRate)
{
    refreshRate_ = refreshRate;
}

uint8_t RangingConfig::GetRefreshRate() const
{
    return refreshRate_;
}


RangingResult::RangingResult()
{}

RangingResult::~RangingResult()
{}

RangingResult::RangingResult(const std::string &address, const int result, const float distance)
    : address_(address), result_(result), distance_(distance)
{}

void RangingResult::SetAddress(const std::string &address)
{
    address_ = address;
}

std::string RangingResult::GetAddress() const
{
    return address_;
}

void RangingResult::SetResult(const int &result)
{
    result_ = result;
}

int RangingResult::GetResult() const
{
    return result_;
}

void RangingResult::SetDistance(const float &distance)
{
    distance_ = distance;
}

float RangingResult::GetDistance() const
{
    return distance_;
}

void RangingResult::SetProb(const float &prob)
{
    prob_ = prob;
}

float RangingResult::GetProb() const
{
    return prob_;
}

void RangingResult::SetRssi(const float &rssi)
{
    rssi_ = rssi;
}

float RangingResult::GetRssi() const
{
    return rssi_;
}

RangingState::RangingState()
{}

RangingState::~RangingState()
{}

RangingState::RangingState(const std::string &address, const int newState,
    const int errorCode) : address_(address), newState_(newState), errorCode_(errorCode)
{}

void RangingState::SetAddress(const std::string &address)
{
    address_ = address;
}

std::string RangingState::GetAddress() const
{
    return address_;
}
 
 
void RangingState::SetNewState(const int &newState)
{
    newState_ = newState;
}
 
int RangingState::GetNewState() const
{
    return newState_;
}

void RangingState::SetErrorCode(const int &errorCode)
{
    errorCode_ = errorCode;
}

int RangingState::GetErrorCode() const
{
    return errorCode_;
}
} // Nearlink
} // OHOS