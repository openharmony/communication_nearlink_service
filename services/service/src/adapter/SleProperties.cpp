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
#include "SleProperties.h"

#include "SleDefs.h"

#include "sle_uuid.h"
#include "log.h"
#include "securec.h"
#include "nearlink_datashare_helper.h"
#include "ThreadUtil.h"
#include "SleNameChangeManager.h"
#include "nlstk_cfgdb_api.h"

namespace OHOS {
namespace Nearlink {
struct SleProperties::impl {
    ffrt::mutex deviceNameMtx;
    ffrt::mutex macAddrMtx;

    std::string deviceName_ = "";
    class SleNameChangeCallback;
    std::shared_ptr<SleNameChangeCallback> sleNameChangeCallback_{ nullptr };
    BaseObserverList<IAdapterSleObserver> observer_;
    std::atomic<int32_t> ioCapability_ = SLE_DEFAULT_IO;
    std::atomic<int32_t> bondableMode_ = static_cast<int>(BondableMode::BONDABLE_MODE_OFF);
    std::string macAddr_{SLE_INVALID_MAC_ADDRESS};
    std::atomic<SLE_Addr_S> sleAddr_;
};

class SleProperties::impl::SleNameChangeCallback : public ISleNameChangeListener {
public:
    explicit SleNameChangeCallback(std::weak_ptr<impl> implPtr) : pimpl_(implPtr) {};
    ~SleNameChangeCallback() override = default;

    void OnLocalNameChanged(const std::string &newLocalName) override
    {
        auto ptr = pimpl_.lock();
        NL_CHECK_RETURN(ptr, "SleProperties impl is destroyed.");
        std::lock_guard<ffrt::mutex> lock(ptr->deviceNameMtx);
        HILOGI("enter");
        ptr->deviceName_ = newLocalName;
        return;
    }

private:
    std::weak_ptr<impl> pimpl_;
};

SleProperties &SleProperties::GetInstance()
{
    static SleProperties instance;
    return instance;
}

SleProperties::SleProperties() : pimpl(std::make_shared<SleProperties::impl>())
{
    pimpl->sleNameChangeCallback_ = std::make_shared<SleProperties::impl::SleNameChangeCallback>(pimpl);
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(pimpl->sleNameChangeCallback_);
}

SleProperties::~SleProperties()
{
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(pimpl->sleNameChangeCallback_);
}

std::string SleProperties::GetLocalName() const
{
    std::lock_guard<ffrt::mutex> lock(pimpl->deviceNameMtx);
    HILOGI("enter");
    if (pimpl->deviceName_.empty()) {
        HILOGI("retry read LocalName");
        std::string deviceName = NearlinkDataShareHelper::GetLocalDeviceName();
        NL_CHECK_RETURN_RET(!deviceName.empty(), SLE_DEFAULT_DEVICE_NAME, "GetLocalName failed");
        pimpl->deviceName_ = deviceName;
    }
    return pimpl->deviceName_;
}

bool SleProperties::SetLocalName(const std::string &name) const
{
    HILOGI("enter");
    return true;
}

std::string SleProperties::GetLocalAddress() const
{
    std::lock_guard<ffrt::mutex> lock(pimpl->macAddrMtx);
    HILOGD("enter");
    return pimpl->macAddr_;
}

SLE_Addr_S SleProperties::GetLocalSleAddress() const
{
    HILOGI("enter");
    return pimpl->sleAddr_.load();
}

int SleProperties::GetBondableMode() const
{
    HILOGI("enter");
    return pimpl->bondableMode_.load();
}

int SleProperties::SetBondableMode(const int mode) const
{
    HILOGI("enter");
    pimpl->bondableMode_.store(mode);
    return NLSTK_ERRCODE_SUCCESS;
}

int SleProperties::GetIoCapability() const
{
    HILOGI("enter");
    return pimpl->ioCapability_.load();
}

bool SleProperties::SetIoCapability(const int ioCapability) const
{
    HILOGI("enter");
    pimpl->ioCapability_.store(ioCapability);
    return true;
}

bool SleProperties::GetAddrFromController() const
{
    HILOGI("enter");

    SLE_Addr_S btAddr;
    (void)memset_s(&btAddr, sizeof(btAddr), 0x00, sizeof(btAddr));
    NLSTK_CfgdbGetPublicAddress(&btAddr);
    RawAddress addr = RawAddress::ConvertToString(btAddr.addr);
    {
        std::lock_guard<ffrt::mutex> lock(pimpl->macAddrMtx);
        pimpl->macAddr_ = addr.GetAddress();
    }
    pimpl->sleAddr_.store(btAddr);
    return UpdateConfig(static_cast<int>(SLE_CONFIG::SLE_CONFIG_LOCAL_ADDRESS));
}

bool SleProperties::UpdateConfig(int type) const
{
    HILOGI("Type = %{public}d", type);
    bool ret = NLSTK_ERRCODE_STATUS_ERR;
    switch (type) {
        case static_cast<int>(SLE_CONFIG::SLE_CONFIG_LOCAL_NAME): {
                std::string deviceName;
                {
                    std::lock_guard<ffrt::mutex> lock(pimpl->deviceNameMtx);
                    ret = SleConfig::GetInstance().SetLocalName(pimpl->deviceName_);
                    deviceName = pimpl->deviceName_;
#ifdef TV_STANDARD
                if (SleConfig::GetInstance().GetLocalName() != pimpl->deviceName_) {
                    ret &= SleConfig::GetInstance().Save();
                }
#endif
                }
                pimpl->observer_.ForEach(
                    [deviceName](IAdapterSleObserver &observer) { observer.OnDeviceNameChanged(deviceName); });
            }
            break;
        case static_cast<int>(SLE_CONFIG::SLE_CONFIG_LOCAL_ADDRESS): {
                std::string macAddr;
                {
                    std::lock_guard<ffrt::mutex> lock(pimpl->macAddrMtx);
                    SleConfig::GetInstance().SetLocalAddress(pimpl->macAddr_);
                    ret = SleConfig::GetInstance().SetSleLocalAddrType(
                        static_cast<int>(SLE_ADDR_TYPE::SLE_PUBLIC_ADDRESS_TYPE));
                    macAddr = pimpl->macAddr_;
#ifdef TV_STANDARD
                if (SleConfig::GetInstance().GetLocalAddress() != pimpl->macAddr_) {
                    ret &= SleConfig::GetInstance().Save();
                }
#endif
                }
                pimpl->observer_.ForEach(
                    [macAddr](IAdapterSleObserver &observer) { observer.OnDeviceAddrChanged(macAddr); });
            }
            break;
        default:
            break;
    }

#ifndef TV_STANDARD
    ret &= SleConfig::GetInstance().Save();
#endif
    return ret;
}

bool SleProperties::LoadSleConfigInfo() const
{
    HILOGI("enter");

    bool ret = SleConfig::GetInstance().LoadConfigInfo();
    if (!ret) {
        HILOGE("Load device config file failed");
    }
    ReadSleHostInfo();
    return ret;
}

void SleProperties::ReadSleHostInfo() const
{
    HILOGI("enter");
    int ioCapability = SleConfig::GetInstance().GetIoCapability();
    pimpl->ioCapability_.store(ioCapability);
}

bool SleProperties::SaveDefaultValues() const
{
    HILOGI("enter");
    bool ret = SetIoCapability(SLE_DEFAULT_IO);
    ret &= SleConfig::GetInstance().Save();
    return ret;
}

void SleProperties::RegisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    HILOGI("enter");
    pimpl->observer_.Register(observer);
}

void SleProperties::DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    HILOGI("enter");
    pimpl->observer_.Deregister(observer);
}
}  // namespace Nearlink
}  // namespace OHOS