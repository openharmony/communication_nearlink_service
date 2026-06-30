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
#include <future>
#include "DisService.h"

#include "CdsmService.h"
#include "parameters.h"
#include "param_wrapper.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_dft_ue.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_device_data.h"
#include "ThreadUtil.h"
#include "DisClientStackAdapter.h"
#include "DisServerStackAdapter.h"

#include "nearlink_dft_device_data.h"
#include "nearlink_dft_exception.h"
#include "nearlink_common_event_helper.h"
#include "SleNameChangeManager.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t DIS_DEVICE_APPEARANCE_LEN = 3;
constexpr size_t DIS_DEVICE_APPEARANCE_SIZE = 3; // 星闪协议规定外观信息的长度为3
std::vector<uint8_t> UintToAppearanceVector(uint32_t value)
{
    HILOGI("[DisServer] enter");
    std::vector<uint8_t> vec(DIS_DEVICE_APPEARANCE_SIZE);
    for (size_t i = 0; i < DIS_DEVICE_APPEARANCE_SIZE; i++) {
        vec[i] = (value >> (8 * i)) & 0xFF; // 依次取低8位,中间8位,高8位
    }
    return vec;
}

enum class DisServerInfoType : uint8_t {
    DIS_MANUFACTURER_ID = 0X00,
    DIS_DEVICE_MODEL,
    DIS_DEVICE_SERIAL_NUMBER,
    DIS_HARDWARE_VERSION,
    DIS_FIRMWARE_VERSION,
    DIS_SOFTWARE_VERSION,
    DIS_DEVICE_NAME,
    DIS_DEVICE_APPEARANCE,
};
}

struct DisService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    class SleNameChangeCallback;
    std::shared_ptr<SleNameChangeCallback> sleNameChangeCallback_{ nullptr };
    // client
    DisClientStackAdapter clientStackAdapter_;
    BaseObserverList<DisObserver> disObservers_ {};
    // server
    DisServerStackAdapter serverStackAdapter_;
    DisServerInfo disInfo_;
};

class DisService::impl::SleNameChangeCallback : public ISleNameChangeListener {
public:
    explicit SleNameChangeCallback(std::weak_ptr<impl> implPtr) : pimpl_(implPtr) {};
    ~SleNameChangeCallback() override = default;

    void OnLocalNameChanged(const std::string &newLocalName) override
    {
        // 更新DIS服务端name信息
        auto ptr = pimpl_.lock();
        NL_CHECK_RETURN(ptr, "DisService impl is destroyed.");
        DoInDisThread([ptr, newLocalName]() -> void {
            HILOGI("[DIS Service] size(%{public}zu)", newLocalName.size());
            ptr->serverStackAdapter_.UpdateLocalDeviceName(newLocalName);
            ptr->disInfo_.nameInfo_ = newLocalName;
        });
    }

private:
    std::weak_ptr<impl> pimpl_;
};

DisService::DisService()
    : utility::Context(PROFILE_NAME_DIS, "1.0.0"), pimpl(std::make_shared<DisService::impl>())
{
    HILOGI("[DIS Service]%{public}s:%{public}s Create", PROFILE_NAME_DIS.c_str(), Name().c_str());
    // 客户端向协议栈注册回调
    if (pimpl->clientStackAdapter_.RegisteCallBackToStack() != DIS_SUCCESS) {
        HILOGE("[DIS Service] register cb to stack failed.");
    }
    pimpl->sleNameChangeCallback_ = std::make_shared<DisService::impl::SleNameChangeCallback>(pimpl);
    SleNameChangeManager::GetInstance().RegisterLocalNameObserver(pimpl->sleNameChangeCallback_);
}

DisService::~DisService()
{
    HILOGI("[DIS Service]%{public}s:%{public}s Destroy", PROFILE_NAME_DIS.c_str(), Name().c_str());
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(pimpl->sleNameChangeCallback_);
}

utility::Context *DisService::GetContext()
{
    return this;
}

DisService *DisService::GetDisService()
{
    return static_cast<DisService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_DIS));
}

void DisService::RegisterObserver(DisObserver &disObserver)
{
    HILOGI("[DIS Service] Enter");
    pimpl->disObservers_.Register(disObserver);
}

void DisService::DeregisterObserver(DisObserver &disObserver)
{
    HILOGD("[DIS Service] Enter");
    pimpl->disObservers_.Deregister(disObserver);
}

void DisService::Enable()
{
    DoInDisThread([this]() -> void {
        HILOGI("[DIS Service] start enable dis service");
        if (pimpl->isEnabled_) {
            GetContext()->OnEnable(PROFILE_NAME_DIS, true);
            HILOGW("[DIS Service] DisService has already been enabled before.");
            return;
        }
        GetContext()->OnEnable(PROFILE_NAME_DIS, true);
        // 服务端添加设备信息服务
        LoadDeviceInfo();
        pimpl->serverStackAdapter_.CreateServerInfo(pimpl->disInfo_);
        pimpl->isEnabled_ = true;
        HILOGI("[DIS Service] dis service enabled");
    });
}

void DisService::Disable()
{
    DoInDisThread([this]() -> void {
        HILOGI("[DIS Service] start disable dis service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_DIS, true);
            HILOGW("[DIS Service] DisService has already been disabled before.");
            return;
        }
        GetContext()->OnDisable(PROFILE_NAME_DIS, true);

        // 服务端销毁设备信息服务
        pimpl->serverStackAdapter_.DestroyServerInfo();
        pimpl->isEnabled_ = false;
        HILOGI("[DIS Service] dis service disabled");
    });
}

int DisService::Connect(const RawAddress &device)
{
    DftCacheDisStart(device.GetAddress());
    DoInDisThread([this, device]() -> void {
        HILOGD("[DIS Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.Connect(device);
    });
    return DIS_SUCCESS;
}

int DisService::Disconnect(const RawAddress &device)
{
    DoInDisThread([this, device]() -> void {
        HILOGI("[DIS Service] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.Disconnect(device);
    });
    return DIS_SUCCESS;
}

void DisService::NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState)
{
    DoInDisThread([this, device, curState, preState]() -> void {
        HILOGD("[DIS Service], curState(%{public}d), preState(%{public}d)", curState, preState);
        if (curState == SleConnectState::CONNECTED && preState == SleConnectState::CONNECTING) {
            DftCacheDisFinish(device.GetAddress());
            SaveAppearanceInfo(device);
            SaveNameInfo(device);
            pimpl->clientStackAdapter_.GetDeviceVendorId(device);
        }

        pimpl->disObservers_.ForEach([device, curState, preState](DisObserver &observer) {
            observer.OnConnectionStateChanged(device, static_cast<int>(curState), static_cast<int>(preState));
        });
    });
}

std::list<RawAddress> DisService::GetConnectDevices()
{
    return std::list<RawAddress>();
}

DeviceInformation DisService::GetDeviceInformation(const RawAddress &device) const
{
    HILOGD("enter");
    std::promise<DeviceInformation> promise;
    DoInDisThread([this, device, &promise]() -> void {
        CdsmService *cdsmService = CdsmService::GetService();
        RawAddress realConnectDevice(device);
        if (cdsmService != nullptr) {
            cdsmService->GetRealConnectAddress(device, realConnectDevice);
        }
        DeviceInformation info;
        pimpl->clientStackAdapter_.GetDeviceInformation(realConnectDevice, info);
        promise.set_value(info);
    });
    return promise.get_future().get();
}

int DisService::GetConnectState()
{
    return static_cast<int>(SleConnectState::DISCONNECTED);
}

int DisService::GetDeviceVendorId(const RawAddress &device)
{
    std::promise<int> promise;
    DoInDisThread([this, device, &promise]() -> void {
        int vendorId = pimpl->clientStackAdapter_.GetDeviceVendorId(device);
        HILOGI("[DIS Service] addr(%{public}s), vendorId(%{public}d)", GET_ENCRYPT_ADDR(device), vendorId);
        promise.set_value(vendorId);
    });
    return promise.get_future().get();
}

int DisService::GetDeviceProductId(const RawAddress &device)
{
    std::promise<int> promise;
    DoInDisThread([this, device, &promise]() -> void {
        int productId = pimpl->clientStackAdapter_.GetDeviceProductId(device);
        HILOGI("[DIS Service] addr(%{public}s), productId(%{public}d)", GET_ENCRYPT_ADDR(device), productId);
        promise.set_value(productId);
    });
    return promise.get_future().get();
}

int DisService::GetDeviceVersion(const RawAddress &device)
{
    std::promise<int> promise;
    DoInDisThread([this, device, &promise]() -> void {
        int ver = pimpl->clientStackAdapter_.GetDeviceVersion(device);
        HILOGI("[DIS Service] addr(%{public}s), ver(%{public}d)", GET_ENCRYPT_ADDR(device), ver);
        promise.set_value(ver);
    });
    return promise.get_future().get();
}

void DisService::SaveAppearanceInfo(const RawAddress &device)
{
    uint32_t appearance = pimpl->clientStackAdapter_.GetAppearanceInfo(device);
    NL_CHECK_RETURN(appearance != DIS_INVALID_APPEARANCE_INFO, "appearance is invalid");
    HILOGI("[DIS Service] save addr(%{public}s), appearance(%{public}d)", GET_ENCRYPT_ADDR(device), appearance);
    SleRemoteDeviceAdapter::GetInstance()->SetAppearance(device, appearance);
    DftCacheAppearance(device.GetAddress(), appearance);
    DftDeviceManager::GetInstance().UpdateAppearance(device, appearance);
}

void DisService::SaveNameInfo(const RawAddress &device)
{
    std::string name = pimpl->clientStackAdapter_.GetNameInfo(device);
    SleRemoteDeviceAdapter::GetInstance()->SetName(device, name);
    DftCacheName(device.GetAddress(), name);
    DftDeviceManager::GetInstance().UpdateName(device, name);
}

// DIS服务端
void DisService::LoadDeviceInfo()
{
    // load versionInfo_
    std::string versionInfo;
    int ret = OHOS::system::GetStringParameter("const.nearlink.dis.version", versionInfo, "1");
    HILOGI("[DIS Service] versionInfo(%{public}s), ret(%{public}d)", versionInfo.c_str(), ret);
    pimpl->disInfo_.versionInfo_ = versionInfo;

    // load manufactureInfo_
    std::string manufactureInfo;
    ret = OHOS::system::GetStringParameter("const.nearlink.dis.manufacture_id", manufactureInfo, "0");
    HILOGI("[DIS Service] manufactureInfo(%{public}s), ret(%{public}d)", manufactureInfo.c_str(), ret);
    pimpl->disInfo_.manufactureInfo_ = manufactureInfo;

    // load appearanceId_
    pimpl->disInfo_.appearanceId_ =
        OHOS::system::GetUintParameter<uint32_t>("const.nearlink.dis.appearance_id", DIS_DEVICE_SMARTPHONE);
    HILOGI("[DIS Service] appearanceId(%{public}d)", pimpl->disInfo_.appearanceId_);

    // load nameInfo_
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(sleService, "sleService is null.");
    pimpl->disInfo_.nameInfo_ = sleService->GetLocalName();
    HILOGI("[DIS Service] name size(%{public}zu)", pimpl->disInfo_.nameInfo_.size());
    NearlinkDftUe::GetInstance().WriteCommonUe(
        DFT_DIS_EXPOSED_UE, LOCALNAME, LOAD, pimpl->disInfo_.nameInfo_);
}

void DisService::GetDisServiceData(std::string &serviceData)
{
    std::promise<void> promise;
    DoInDisThread([this, &serviceData, &promise]() -> void {
        // 获取本端设备信息
        char cAppearanceData[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
        cAppearanceData[0] = static_cast<char>(DisServerInfoType::DIS_DEVICE_APPEARANCE);
        cAppearanceData[1] = DIS_DEVICE_APPEARANCE_LEN;
        std::string appearance;
        HILOGI("[DIS Service] appearanceId(%{public}d)", pimpl->disInfo_.appearanceId_);
        std::vector<uint8_t> vAppearance = UintToAppearanceVector(pimpl->disInfo_.appearanceId_);
        appearance.assign(vAppearance.begin(), vAppearance.end());
        serviceData.append(std::string(cAppearanceData, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) + appearance);

        char cDeviceNameData[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
        cDeviceNameData[0] = static_cast<char>(DisServerInfoType::DIS_DEVICE_NAME);
        cDeviceNameData[1] = pimpl->disInfo_.nameInfo_.size();
        HILOGI("[DIS Service] size(%{public}zu)", pimpl->disInfo_.nameInfo_.size());
        serviceData.append(std::string(cDeviceNameData, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) + pimpl->disInfo_.nameInfo_);
        promise.set_value();
    });
    promise.get_future().get();
}

int DisService::GetDisServiceAppearance()
{
    std::promise<int> promise;
    DoInDisThread([this, &promise]() -> void {
        int appearance = static_cast<int>(pimpl->disInfo_.appearanceId_);
        HILOGI("[DIS Service] appearanceId(%{public}d)", appearance);
        promise.set_value(appearance);
    });
    return promise.get_future().get();
}

REGISTER_CLASS_CREATOR(DisService);

} // namespace Nearlink
} // namespace OHOS
