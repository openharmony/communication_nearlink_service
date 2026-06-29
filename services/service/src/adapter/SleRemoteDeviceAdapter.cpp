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

#include <algorithm>

#include "SleRemoteDeviceAdapter.h"
#include "ManufacturerAbilityLoader.h"
#include "parameters.h"
#include "SleServiceManager.h"
#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceProfileTws.h"
#include "SleInterfaceProfile.h"
#include "SleInterfaceProfileManager.h"
#include "interface_cloud_pair_service.h"
#include "interface_scan_service.h"
#include "SleProperties.h"
#include "nearlink_permission_manager.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_utils.h"
#include "cm.h"
#include "cm_api.h"
#include "slem.h"
#include "cm_errno.h"
#include "SleRemoteDeviceManager.h"
#include "SleUtils.h"
#include "TwsDefines.h"
#include "CdsmDefines.h"
#include "nlstk_sm_api.h"
#include <future>
#include <ThreadUtil.h>
#include "ServiceManagerPluginLoader.h"
#include "SleReconnectManager.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr const char* INVALID_NAME = "";
constexpr const char* NEARLINK_RECONN_DEVICE_BT_ADDR = "persist.nearlink.reconn_device_bluetooth_address";
constexpr uint32_t BG_CONN_MAX_NUMBER = 255;

enum class DeviceTypeForService : uint32_t {
    DEVICE_TYPE_OTHER = 0,    // 其他设备设备
    DEVICE_TYPE_AUDIO = 1,    // 音频设备
    DEVICE_TYPE_KEYBOARD = 2,    // 键盘
    DEVICE_TYPE_MOUSE = 3,    // 鼠标
    DEVICE_TYPE_PEN = 4,    // 手写笔
    DEVICE_TYPE_HANDLE = 5,    // 游戏手柄
    DEVICE_TYPE_WATCH = 6,    // 手表
};

}

SleRemoteDeviceAdapter::SleRemoteDeviceAdapter()
{}

SleRemoteDeviceAdapter::~SleRemoteDeviceAdapter()
{}

SleRemoteDeviceAdapter* SleRemoteDeviceAdapter::GetInstance(void)
{
    static SleRemoteDeviceAdapter instance;
    return &instance;
}

std::vector<RawAddress> SleRemoteDeviceAdapter::GetPairedDevices()
{
    std::vector<RawAddress> pairedList = SleRemoteDeviceManager::GetInstance()->GetPairedDevices();
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    std::vector<RawAddress> resultList;
    for (const auto& rawAddr : pairedList) {
        if (cdsmService == nullptr || !cdsmService->CdsmCheckIsCooperationDevice(rawAddr)) {
            resultList.push_back(rawAddr);
            continue;
        }

        RawAddress reportAddr;
        cdsmService->CdsmGetReportAddr(rawAddr, reportAddr);
        auto it = std::find(resultList.begin(), resultList.end(), reportAddr);
        if (it != resultList.end()) {
            HILOGD("[SleRemoteDeviceAdapter]:report addr:%{public}s already in paired list.",
                GetEncryptAddr(reportAddr.GetAddress()).c_str());
            continue;
        }
        resultList.push_back(reportAddr);
    }
    InterfaceCloudPairService::GetInstance().AddCloudPairDevices(resultList);
    return resultList;
}

std::vector<RawAddress> SleRemoteDeviceAdapter::GetConnectedDevices()
{
    std::promise<std::vector<RawAddress>> promise;
    std::future<std::vector<RawAddress>> future = promise.get_future();
    DoInDeviceAdapterThread([&promise]() -> void {
        std::vector<RawAddress> connectedList = SleRemoteDeviceManager::GetInstance()->GetConnectedDevices();
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<RawAddress> resultList;
        for (const auto& rawAddr : connectedList) {
            if (cdsmService == nullptr || !cdsmService->CdsmCheckIsCooperationDevice(rawAddr)) {
                resultList.push_back(rawAddr);
                continue;
            }
            RawAddress reportAddr;
            cdsmService->CdsmGetReportAddr(rawAddr, reportAddr);
            auto it = std::find(resultList.begin(), resultList.end(), reportAddr);
            if (it != resultList.end()) {
                HILOGD("[SleRemoteDeviceAdapter]:report addr:%{public}s already in connected list.",
                    GetEncryptAddr(reportAddr.GetAddress()).c_str());
                continue;
            }
            resultList.push_back(reportAddr);
        }
        promise.set_value(resultList);
    });
    return future.get();
}

std::string SleRemoteDeviceAdapter::GetDeviceName(const RawAddress &device)
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        std::string remoteName = InterfaceScanService::GetInstance().GetDeviceName(realAddr.GetAddress());
        if (!remoteName.empty()) {
            promise.set_value(remoteName);
            return;
        }
        std::string result = SleRemoteDeviceManager::GetInstance()->GetDeviceName(realAddr);
        promise.set_value(result);
    });
    return future.get();
}

int SleRemoteDeviceAdapter::GetPairState(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdmsInfo = {};
        if (cdsmService == nullptr || cdsmService->CdsmGetAllMemberInfo(device, cdmsInfo) != NL_NO_ERROR) {
            int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(device);
            LOG_INFO("addr: %{public}s, state:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), pairState);
            promise.set_value(pairState);
            return;
        }
        int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
        for (auto &member : cdmsInfo) {
            int memberPairState = SleRemoteDeviceManager::GetInstance()->GetPairState(member.addr_);
            if (memberPairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
                pairState = memberPairState;
            }
        }
        HILOGD("[SleRemoteDeviceAdapter]:get pair state status,dev:%{public}s.cdsm return:%{public}u",
            GET_ENCRYPT_ADDR(device), pairState);
        promise.set_value(pairState);
    });
    return future.get();
}

std::string SleRemoteDeviceAdapter::GetAliasName(const RawAddress &device)
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        std::string name = SleRemoteDeviceManager::GetInstance()->GetAliasName(realAddr);
        if (name == INVALID_NAME) {
            name = InterfaceCloudPairService::GetInstance().GetCloudDeviceAliasName(realAddr);
        }
        if (name == INVALID_NAME) {
            name = InterfaceScanService::GetInstance().GetDeviceName(realAddr.GetAddress());
        }
        HILOGD("[SleRemoteDeviceAdapter] addr:%{public}s, %{public}zu", GET_ENCRYPT_ADDR(device), name.size());
        promise.set_value(name);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetAliasName(const RawAddress &device, const std::string &name)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, name, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        bool result = false;
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService != nullptr && cdsmService->CdsmCheckIsCooperationDevice(device) &&
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR) {
            for (const auto &member : cdsmInfo) {
                realAddr = member.addr_;
                result = SleRemoteDeviceManager::GetInstance()->SetAliasName(realAddr, name);
            }
            promise.set_value(result);
            return;
        }
        result = SleRemoteDeviceManager::GetInstance()->SetAliasName(realAddr, name);
        promise.set_value(result);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetName(const RawAddress &device, const std::string &name)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, name, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        bool result = false;
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService != nullptr && cdsmService->CdsmCheckIsCooperationDevice(device) &&
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR) {
            for (const auto &member : cdsmInfo) {
                realAddr = member.addr_;
                result = SleRemoteDeviceManager::GetInstance()->SetName(realAddr, name);
            }
            promise.set_value(result);
            return;
        }
        result = SleRemoteDeviceManager::GetInstance()->SetName(realAddr, name);
        promise.set_value(result);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetAppearance(const RawAddress &device, const int appearance)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, appearance, &promise]() -> void {
        LOG_DEBUG("[SleRemoteDeviceAdapter]:%{public}d", appearance);
        RawAddress realAddr = GetRealAddressInner(device);
        bool result = false;
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService != nullptr && cdsmService->CdsmCheckIsCooperationDevice(device) &&
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR) {
            for (const auto &member : cdsmInfo) {
                realAddr = member.addr_;
                result = SleRemoteDeviceManager::GetInstance()->SetAppearance(realAddr, appearance);
            }
            promise.set_value(result);
            return;
        }
        result = SleRemoteDeviceManager::GetInstance()->SetAppearance(realAddr, appearance);
        promise.set_value(result);
    });
    return future.get();
}

int SleRemoteDeviceAdapter::GetDeviceAppearance(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        int appearance = GetDeviceAppearanceInner(device);
        promise.set_value(appearance);
    });
    return future.get();
}

int SleRemoteDeviceAdapter::GetDeviceAppearanceInner(const RawAddress &device)
{
    RawAddress realAddr = GetRealAddressInner(device);
    int appearance = SleRemoteDeviceManager::GetInstance()->GetDeviceAppearance(realAddr);
    if (appearance == INVALID_APPEARANCE) {
        appearance = InterfaceScanService::GetInstance().GetDeviceAppearance(realAddr.GetAddress());
    }
    if (appearance == INVALID_APPEARANCE && InterfaceCloudPairService::GetInstance().IsCloudDevice(realAddr)) {
        appearance = static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE);
    }
    HILOGD("[SleRemoteDeviceAdapter] addr:%{public}s appearance:%{public}d", GET_ENCRYPT_ADDR(device), appearance);
    return (appearance == 0 || appearance == INVALID_APPEARANCE) ? INVALID_APPEARANCE : appearance;
}

std::vector<Uuid> SleRemoteDeviceAdapter::GetDeviceUuids(const RawAddress &device)
{
    std::promise<std::vector<Uuid>> promise;
    std::future<std::vector<Uuid>> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        std::vector<Uuid> uuids = SleRemoteDeviceManager::GetInstance()->GetDeviceUuids(realAddr);
        promise.set_value(uuids);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::IsBondedFromLocal(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        bool result = SleRemoteDeviceManager::GetInstance()->IsBondedFromLocal(device);
        promise.set_value(result);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::IsAcbConnected(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdmsInfo = {};
        if (cdsmService == nullptr || cdsmService->CdsmGetAllMemberInfo(device, cdmsInfo) != NL_NO_ERROR) {
            bool result = SleRemoteDeviceManager::GetInstance()->IsAcbConnected(device);
            promise.set_value(result);
            return;
        }
        bool isCdsmConnected = false;
        for (auto &member : cdmsInfo) {
            if (SleRemoteDeviceManager::GetInstance()->IsAcbConnected(member.addr_)) {
                isCdsmConnected = true;
            }
        }
        HILOGD("[SleRemoteDeviceAdapter]:get acb connected status,dev:%{public}s.cdsm connected:%{public}u",
            GET_ENCRYPT_ADDR(device), isCdsmConnected);
        promise.set_value(isCdsmConnected);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::IsAcbEncrypted(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdmsInfo = {};
        if (cdsmService == nullptr || cdsmService->CdsmGetAllMemberInfo(device, cdmsInfo) != NL_NO_ERROR) {
            bool result = SleRemoteDeviceManager::GetInstance()->IsAcbEncrypted(device);
            promise.set_value(result);
            return;
        }
        bool isCdsmEncrypted = false;
        for (auto &member : cdmsInfo) {
            if (SleRemoteDeviceManager::GetInstance()->IsAcbEncrypted(member.addr_)) {
                isCdsmEncrypted = true;
            }
        }
        HILOGI("[SleRemoteDeviceAdapter]:get acb encrypted status,dev:%{public}s,cdsm return:%{public}u",
            GET_ENCRYPT_ADDR(device), isCdsmEncrypted);
        promise.set_value(isCdsmEncrypted);
    });
    return future.get();
}

uint8_t SleRemoteDeviceAdapter::GetLinkRole(const RawAddress &device)
{
    std::promise<uint8_t> promise;
    std::future<uint8_t> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(device);
        uint8_t role = SleRemoteDeviceManager::GetInstance()->GetLinkRole(realAddr);
        promise.set_value(role);
    });
    return future.get();
}

uint16_t SleRemoteDeviceAdapter::GetLcidByAddress(const RawAddress &device)
{
    std::promise<uint16_t> promise;
    std::future<uint16_t> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        uint16_t lcid = SleRemoteDeviceManager::GetInstance()->GetLcidByAddress(device);
        promise.set_value(lcid);
    });
    return future.get();
}

std::string SleRemoteDeviceAdapter::GetAddressByLcid(uint16_t lcid)
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();
    DoInDeviceAdapterThread([lcid, &promise]() -> void {
        std::string addr = SleRemoteDeviceManager::GetInstance()->GetAddressByLcid(lcid);
        promise.set_value(addr);
    });
    return future.get();
}

int SleRemoteDeviceAdapter::GetAcbState(const std::string &address)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([address, &promise]() -> void {
        int state = SleRemoteDeviceManager::GetInstance()->GetAcbState(address);
        promise.set_value(state);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetAcbState(const std::string &address, int connectState)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([address, connectState, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetAcbState(address, connectState);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetLcid(const std::string &address, uint16_t lcid)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([address, lcid, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetLcid(address, lcid);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetPairAlgoInfo(
    const RawAddress &addr, uint8_t &cryptoAlgo, uint8_t &keyDerivAlgo, uint8_t &integrChkInd)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([addr, &cryptoAlgo, &keyDerivAlgo, &integrChkInd, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->GetPairAlgoInfo(addr, cryptoAlgo, keyDerivAlgo, integrChkInd);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetPairAlgoInfo(
    const RawAddress &addr, uint8_t cryptoAlgo, uint8_t keyDerivAlgo, uint8_t integrChkInd)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([addr, cryptoAlgo, keyDerivAlgo, integrChkInd, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetPairAlgoInfo(addr, cryptoAlgo, keyDerivAlgo, integrChkInd);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetGroupAndGiv(const RawAddress &addr, std::string &encryptGroupKeyStr, uint64_t &giv)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([addr, &encryptGroupKeyStr, &giv, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->GetGroupAndGiv(addr, encryptGroupKeyStr, giv);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetGroupAndGiv(const RawAddress &addr, std::string encryptGroupKeyStr, uint64_t giv)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([addr, encryptGroupKeyStr, giv, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetGroupAndGiv(addr, encryptGroupKeyStr, giv);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::HasConnectedDevice()
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([&promise]() -> void {
        bool result = SleRemoteDeviceManager::GetInstance()->HasConnectedDevice();
        promise.set_value(result);
    });
    return future.get();
}

int SleRemoteDeviceAdapter::GetConnDirect(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        int connDirect = SleRemoteDeviceManager::GetInstance()->GetConnDirect(device);
        promise.set_value(connDirect);
    });
    return future.get();
}

/* 获取广播中的业务类型 */
int SleRemoteDeviceAdapter::GetManufacturerBusinessType(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        int businessType = SleRemoteDeviceManager::GetInstance()->GetManufacturerBusinessType(device);
        promise.set_value(businessType);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::SetConnDirect(const RawAddress &device, int connDirect)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, connDirect, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->SetConnDirect(device, connDirect);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SetConnDirectActive(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->SetConnDirectActive(device);
        promise.set_value();
    });
    future.get();
}

int SleRemoteDeviceAdapter::GetPairStatus(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(device);
        promise.set_value(pairState);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetPairStatus(const RawAddress &device, int pairStatus)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, pairStatus, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetPairStatus(device, pairStatus);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetPrePairStatus(const RawAddress &device, int pairStatus)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, pairStatus, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetPrePairStatus(device, pairStatus);
        promise.set_value(ret);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::SetDeviceIsAvailable(const RawAddress &device, bool isAvailable)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, isAvailable, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->SetDeviceIsAvailable(device, isAvailable);
        promise.set_value();
    });
    future.get();
}

int SleRemoteDeviceAdapter::GetConnectedCnt()
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([&promise]() -> void {
        int cnt = SleRemoteDeviceManager::GetInstance()->GetConnectedCnt();
        promise.set_value(cnt);
    });
    return future.get();
}

/* 配对状态瞬态计数 */
int SleRemoteDeviceAdapter::GetNotPairNoneCnt(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        int notPairNoneNum = 0;
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService == nullptr ||
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
            promise.set_value(notPairNoneNum);
            return;
        }

        for (auto &member : cdsmInfo) {
            int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(member.addr_);
            if (pairState != static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
                notPairNoneNum++;
            }
        }
        promise.set_value(notPairNoneNum);
    });
    return future.get();
}

/* 配对状态稳态计数 */
int SleRemoteDeviceAdapter::GetPairedCnt(const RawAddress &device)
{
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        int pairedNum = 0;
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService == nullptr ||
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
            promise.set_value(pairedNum);
            return;
        }

        for (auto &member : cdsmInfo) {
            int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(member.addr_);
            if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
                pairedNum++;
            }
        }
        promise.set_value(pairedNum);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::GetBgList(NearlinkSafeList<RawAddress> &bgList,
                                       const std::vector<std::string> &reconnectList)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([&bgList, &reconnectList, &promise]() -> void {
        std::vector<RawAddress> pairedDevices = SleRemoteDeviceManager::GetInstance()->GetPairedDevices();
        for (const auto &device : pairedDevices) {
            int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(device);
            if (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
                continue;
            }
            // 被动配对不回连，车钥匙场景不回连，路由器不回连，不可用设备不回连
            int pairDirection = SleRemoteDeviceManager::GetInstance()->GetPairDirection(device);
            int appearance = SleRemoteDeviceManager::GetInstance()->GetDeviceAppearance(device);
            bool isDeviceAvailable = SleRemoteDeviceManager::GetInstance()->GetIsDeviceAvailable(device);
            if (pairDirection == static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE) ||
                appearance == static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK) ||
                appearance == static_cast<int>(DeviceClassForService::DEVICE_NETWORKING) ||
                !isDeviceAvailable) {
                continue;
            }
            // 开启星闪且不回连被用户主动断连的设备的情况
            bool isUserDisconnected = SleRemoteDeviceManager::GetInstance()->GetIsUserDisconnected(device);
            if (isUserDisconnected &&
                !SleServiceManager::GetInstance()->IsEnableAutoConnectUserDisconnectedDevices()) {
                continue;
            }

            // 不在背景回连队列里的设备不回连
            if (!reconnectList.empty() &&
                std::find(reconnectList.begin(), reconnectList.end(), device.GetAddress()) == reconnectList.end()) {
                continue;
            }
            bgList.EnsureInsert(device);
        }
        promise.set_value();
    });
    future.get();
}

std::string SleRemoteDeviceAdapter::GetReconnDevice() const
{
    // 待回连音频设备：后激活/后连接，激活优先级高于连接
    std::string lastASCActiveDevice = SleConfig::GetInstance().GetLastASCActiveDevice();
    std::string lastASCConnectedDevice = SleConfig::GetInstance().GetLastASCConnectedDevice();
    if (IsValidAddress(lastASCActiveDevice)) {
        HILOGI("DirectConn last active device reportAddr : %{public}s", GetEncryptAddr(lastASCActiveDevice).c_str());
        return lastASCActiveDevice;
    } else if (IsValidAddress(lastASCConnectedDevice)) {
        HILOGI("DirectConn last active device reportAddr : %{public}s", GetEncryptAddr(lastASCConnectedDevice).c_str());
        return lastASCConnectedDevice;
    }
    return "";
}

void SleRemoteDeviceAdapter::SetReconnDeviceParam()
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &promise]() -> void {
        HILOGI("Enter");
        std::string sleAddr = GetReconnDevice();
        if (sleAddr.empty()) {
            OHOS::system::SetParameter(NEARLINK_RECONN_DEVICE_BT_ADDR, SLE_INVALID_MAC_ADDRESS);
            HILOGI("No Sle Reconn Device");
        } else {
            std::string btAddr = "";
            if (SleRemoteDeviceManager::GetInstance()->GetBtAddrBySleAddr(sleAddr, btAddr)) {
                OHOS::system::SetParameter(NEARLINK_RECONN_DEVICE_BT_ADDR, btAddr);
                HILOGI("Save Sle Reconn Param : %{public}s", GetEncryptAddr(btAddr).c_str());
            }
        }
        promise.set_value();
    });
    future.get();
}


bool SleRemoteDeviceAdapter::IsNeedToIgnore(std::string reconnDevAddr) const
{
    bool enableAutoConnAudioDevice = SleServiceManager::GetInstance()->IsEnableAutoConnectAudioDevices();
    if (!enableAutoConnAudioDevice) {
        return true;
    }
    std::string addr = GetReconnDevice();
    RawAddress reportAddr(addr);
    RawAddress memberAddr;

    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    if (cdsmService != nullptr && !reportAddr.GetAddress().empty()) {
        cdsmService->CdsmGetOtherAddr(reportAddr, memberAddr);
        if (reportAddr.GetAddress() != reconnDevAddr && memberAddr.GetAddress() != reconnDevAddr) {
            HILOGI("report addr : %{public}s, member addr : %{public}s, cur addr : %{public}s",
                GET_ENCRYPT_ADDR(reportAddr), GET_ENCRYPT_ADDR(memberAddr), GetEncryptAddr(reconnDevAddr).c_str());
            return true;
        }
    }
    return false;
}

void SleRemoteDeviceAdapter::GetDirectConnList(NearlinkSafeList<RawAddress> &bgList)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &bgList, &promise]() -> void {
        std::vector<RawAddress> directDevices =
            SleRemoteDeviceManager::GetInstance()->GetDirectConnDevices();
        for (const auto &device : directDevices) {
            if (SleReconnectManager::GetInstance().IsNeedToIgnore(device.GetAddress())) {
                continue;
            }
            SleRemoteDeviceManager::GetInstance()->SetAcbState(
                device.GetAddress(), static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
            DftCachePairConnType(device.GetAddress(), NO_PAIR, BG_CONN);
            bgList.Insert(device);
        }
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::AddPeripheralDevice(const std::string &address,
    std::shared_ptr<SlePeripheralDevice> &peerDevice)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([address, &peerDevice, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(address, peerDevice);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::RemovePeripheralDevice(const std::string &address)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([address, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->RemovePeripheralDevice(address);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::RemoveAllPeripheralDevices()
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([&promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->RemoveAllPeripheralDevices();
        promise.set_value();
    });
    future.get();
}

std::shared_ptr<const SlePeripheralDevice> SleRemoteDeviceAdapter::GetRemoteDevice(const RawAddress &device)
{
    std::promise<std::shared_ptr<const SlePeripheralDevice>> promise;
    std::future<std::shared_ptr<const SlePeripheralDevice>> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        auto dev = SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(device);
        promise.set_value(dev);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::SaveDeviceModelInfo(const std::string &address, const DeviceModel &model,
    const std::string &newModelId)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([address, model, newModelId, &promise]() -> void {
        HILOGI("[SleRemoteDeviceAdapter] SaveDeviceModelInfo: device:%{public}s, modelId:%{public}s,\
        newModelId:%{public}s, iconId:%{public}s, devType:%{public}s", GetEncryptAddr(address).c_str(),
            model.GetModelId().c_str(), newModelId.c_str(), model.GetIconId().c_str(),
            model.GetDevType().c_str());
        SleRemoteDeviceManager::GetInstance()->SaveDeviceModelInfo(address, model, newModelId);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::FindDeviceModelInfoInCache(const RawAddress &reportAddr, DeviceModel &model,
    std::string &newModelId)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([&reportAddr, &model, &newModelId, &promise]() -> void {
        if (SleRemoteDeviceManager::GetInstance()->GetDeviceModelInfo(reportAddr, model, newModelId)) {
            promise.set_value();
            return;
        }

        newModelId = InterfaceScanService::GetInstance().GetNewModelId(reportAddr.GetAddress());
        model.SetModelId(InterfaceScanService::GetInstance().GetModelId(reportAddr.GetAddress()));
        model.SetSubModelId(InterfaceScanService::GetInstance().GetSubModelId(reportAddr.GetAddress()));
        model.SetIconId(InterfaceScanService::GetInstance().GetIconId(reportAddr.GetAddress()));
        if (model.GetIconId().empty()) {
            model.SetIconId(InterfaceCloudPairService::GetInstance().GetCloudDeviceIcondId(reportAddr));
        }
        if (model.GetSubModelId().empty()) {
            model.SetSubModelId(InterfaceCloudPairService::GetInstance().GetCloudDeviceSubModelId(reportAddr));
        }
        promise.set_value();
    });
    future.get();
}

/* 获取真实地址 */
RawAddress SleRemoteDeviceAdapter::GetRealAddress(const RawAddress &reportAddr)
{
    std::promise<RawAddress> promise;
    std::future<RawAddress> future = promise.get_future();
    DoInDeviceAdapterThread([this, &reportAddr, &promise]() -> void {
        RawAddress realAddr = GetRealAddressInner(reportAddr);
        promise.set_value(realAddr);
    });
    return future.get();
}

RawAddress SleRemoteDeviceAdapter::GetRealAddressInner(const RawAddress &reportAddr)
{
    if (SleRemoteDeviceManager::GetInstance()->IsAcbConnected(reportAddr)) {
        return reportAddr;
    }

    RawAddress otherAddr;
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    if (cdsmService != nullptr && cdsmService->CdsmGetOtherAddr(reportAddr, otherAddr)) {
        if (SleRemoteDeviceManager::GetInstance()->IsAcbConnected(otherAddr)) {
            HILOGD("[SleRemoteDeviceAdapter]:address mapped, realAdress:%{public}s map to %{public}s",
                GET_ENCRYPT_ADDR(reportAddr), GET_ENCRYPT_ADDR(otherAddr));
            return otherAddr;
        }
    }

    HILOGD("[SleRemoteDeviceAdapter]:can not get real address,addr:%{public}s", GET_ENCRYPT_ADDR(reportAddr));
    return reportAddr;
}

void SleRemoteDeviceAdapter::UpdateManufacturerAbilityFromAdvData(const RawAddress &reportAddr,
    std::shared_ptr<SlePeripheralDevice> value) const
{
    HILOGI("enter");
    auto manuAbility = InterfaceScanService::GetInstance().GetDeviceManufacturerAbility(reportAddr.GetAddress());
    if (manuAbility != INVALID_MANUFACTURER_ABLITITY) {
        ManufacturerAbilityLoader::GetInstance().FilterAbility(manuAbility);
        value->SetManufacturerAbility(manuAbility);
    }
}

void SleRemoteDeviceAdapter::UpdateSlePeripheralDeviceInfo(const RawAddress &device,
    std::shared_ptr<SlePeripheralDevice> value)
{
    HILOGI("enter");
    RawAddress reportAddr(device);
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(device, reportAddr);
    }
    std::string name = InterfaceScanService::GetInstance().GetDeviceName(reportAddr.GetAddress());
    std::string aliasName = InterfaceCloudPairService::GetInstance().GetCloudDeviceAliasName(reportAddr);
    int appearance = InterfaceScanService::GetInstance().GetDeviceAppearance(reportAddr.GetAddress());
    if (appearance == INVALID_APPEARANCE && InterfaceCloudPairService::GetInstance().IsCloudDevice(reportAddr)) {
        appearance = static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE);
    }
    uint8_t type = InterfaceScanService::GetInstance().GetDeviceAddrType(reportAddr.GetAddress());
    bool isDeviceAvailable = SleConfig::GetInstance().GetAvailableControl(reportAddr.GetAddress());

    UpdateManufacturerAbilityFromAdvData(reportAddr, value);

    value->SetConnDirect(static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    value->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    if (name != INVALID_NAME) {
        value->SetName(name);
    }
    if (aliasName != INVALID_NAME) {
        value->SetAliasName(aliasName);
    }
    if (appearance != INVALID_APPEARANCE) {
        HILOGI("[SleRemoteDeviceAdapter] new device SetAppearance:%{public}d", appearance);
        value->SetAppearance(appearance);
    }
    if (type != static_cast<uint8_t>(SLE_ADDR_TYPE::SLE_ADDR_TYPE_END)) {
        value->SetAddressType(type);
    }
    value->SetIsDeviceAvailable(isDeviceAvailable);
    UpdateSlePeripheralDeviceHiLinkInfo(reportAddr, value);
}

void SleRemoteDeviceAdapter::UpdateSlePeripheralDeviceHiLinkInfo(const RawAddress &reportAddr,
    std::shared_ptr<SlePeripheralDevice> value) const
{
    HILOGI("enter");
    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(reportAddr.GetAddress());
    businessType = businessType == 0 ?
        InterfaceCloudPairService::GetInstance().GetCloudDeviceManufacturerBusinessType(reportAddr) : businessType;
    // 设备广播中HiLink信息
    std::string modeId = InterfaceScanService::GetInstance().GetModelId(reportAddr.GetAddress());
    std::string newModeId = InterfaceScanService::GetInstance().GetNewModelId(reportAddr.GetAddress());
    std::string subModeId = InterfaceScanService::GetInstance().GetSubModelId(reportAddr.GetAddress());
    std::string iconId = InterfaceScanService::GetInstance().GetIconId(reportAddr.GetAddress());
    iconId = iconId.empty() ? InterfaceCloudPairService::GetInstance().GetCloudDeviceIcondId(reportAddr) : iconId;
    subModeId = subModeId.empty() ? 
        InterfaceCloudPairService::GetInstance().GetCloudDeviceSubModelId(reportAddr) : subModeId;

    bool isAudioDevice = InterfaceScanService::GetInstance().IsAudioDevice(reportAddr.GetAddress());
    isAudioDevice = isAudioDevice == false ?
        InterfaceCloudPairService::GetInstance().IsCloudDevice(reportAddr) : isAudioDevice;
    auto manuAbility = InterfaceScanService::GetInstance().GetDeviceManufacturerAbility(reportAddr.GetAddress());
    value->SetManufacturerAbility(manuAbility);
    if (businessType != 0) {
        value->SetManufacturerBusiness(businessType);
    }
    if (!modeId.empty()) {
        value->SetModelId(modeId);
    }
    if (!newModeId.empty()) {
        value->SetNewModelId(newModeId);
    }
    if (!subModeId.empty()) {
        value->SetSubModelId(subModeId);
    }
    if (!iconId.empty()) {
        value->SetIconId(iconId);
    }
    // 对应设备已经是音频设备，不需要再通过扫描数据更新音频设备类型
    if (!value->GetIsAudioDeviceFlag()) {
        value->SetIsAudioDeviceFlag(isAudioDevice);
    }
}


uint8_t SleRemoteDeviceAdapter::GetPeerDeviceAddrType(const RawAddress &device)
{
    std::promise<uint8_t> promise;
    std::future<uint8_t> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        HILOGD("[SleRemoteDeviceAdapter] enter");
        uint8_t type = GetPeerDeviceAddrTypeInner(device);
        promise.set_value(type);
    });
    return future.get();
}

uint8_t SleRemoteDeviceAdapter::GetPeerDeviceAddrTypeInner(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter] enter");
    uint8_t type = InterfaceScanService::GetInstance().GetDeviceAddrType(device.GetAddress());
    if (type == static_cast<uint8_t>(SLE_ADDR_TYPE::SLE_ADDR_TYPE_END)) {
        type = SleRemoteDeviceManager::GetInstance()->GetPeerDeviceAddrType(device);
    }
    return type;
}

void SleRemoteDeviceAdapter::SleAddPeerList(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, device, &promise]() -> void {
        HILOGI("enter");
        std::shared_ptr<SlePeripheralDevice> existDevice =
            SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(device);
        if (existDevice != nullptr) {
            UpdateSlePeripheralDeviceInfo(device, existDevice);
            SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(device.GetAddress(), existDevice);
            promise.set_value();
            return;
        }

        std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
        HILOGI("[SleRemoteDeviceAdapter] add new device:%{public}s", GET_ENCRYPT_ADDR(device));
        peerDevice->SetAddress(device);
        uint8_t peerAddrType = GetPeerDeviceAddrTypeInner(device);
        peerDevice->SetAddressType(peerAddrType);
        peerDevice->SetRoles(SLE_CONNECTION_ROLE_PRIMARY);
        UpdateSlePeripheralDeviceInfo(device, peerDevice);
        SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
        promise.set_value();
    });
    future.get();
}

bool SleRemoteDeviceAdapter::IsVendorDevice(const RawAddress &memberAddr)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([memberAddr, &promise]() -> void {
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        if (cdsmService == nullptr || cdsmService->CdsmGetAllMemberInfo(memberAddr, cdsmInfo) != NL_NO_ERROR) {
            HILOGE("[SleRemoteDeviceAdapter]:get cdsm info error!");
            promise.set_value(false);
            return;
        }
        bool isVendorDevice = false;
        for (const auto &member : cdsmInfo) {
            int businessType = SleRemoteDeviceManager::GetInstance()->GetManufacturerBusinessType(member.addr_);
            if (businessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
                isVendorDevice = true;
            }
        }
        promise.set_value(isVendorDevice);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetManufacturerAbility(const RawAddress &rawAddr, uint8_t ability)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([rawAddr, ability, &promise]() -> void {
        bool isSupport = ManufacturerAbilityLoader::GetInstance().GetAbilityValue(rawAddr, ability);
        promise.set_value(isSupport);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::UpdateDeviceManufacturerAbility(
    const RawAddress &rawAddr, std::array<uint8_t, SLE_MANU_ABILITY_LEN> &deviceManuAbility)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([rawAddr, &deviceManuAbility, &promise]() -> void {
        HILOGI("UpdateDeviceManufacturerAbility, deviceManuAbility[0]=%{public}d", deviceManuAbility[0]);
        SleRemoteDeviceManager::GetInstance()->SetManufacturerAbility(rawAddr, deviceManuAbility);
        promise.set_value();
    });
    future.get();
}

bool SleRemoteDeviceAdapter::SavePeerDeviceInfoToConf()
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([&promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SavePeerDeviceInfoToConf();
        promise.set_value(ret);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::SavePeerDevices2Smp()
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([&promise, this]() -> void {
        LOG_DEBUG("[SleRemoteDeviceAdapter]");
        std::vector<NLSTK_SmRecoverKeyParam_S> storeDevices = CollectPairedDevicesForSmp();
        if (!storeDevices.empty()) {
            LOG_INFO("[SleRemoteDeviceAdapter]SavePeerDevices2Smp deviceSize = %{public}zu", storeDevices.size());
            NLSTK_SmRecoverKey(&storeDevices[0], storeDevices.size());
            for (size_t i = 0; i < storeDevices.size(); i++) {
                (void)memset_s(&storeDevices[i], sizeof(storeDevices[i]), 0x00, sizeof(storeDevices[i]));
            }
            storeDevices.clear();
        }
        promise.set_value();
    });
    future.get();
}

std::vector<NLSTK_SmRecoverKeyParam_S> SleRemoteDeviceAdapter::CollectPairedDevicesForSmp()
{
    LOG_DEBUG("[SleRemoteDeviceAdapter]");
    std::vector<NLSTK_SmRecoverKeyParam_S> storeDevices;
    std::vector<RawAddress> devices = SleRemoteDeviceManager::GetInstance()->GetConnectedDevices();
    if (devices.empty()) {
        return storeDevices;
    }
    for (const auto& device : devices) {
        auto deviceInfo = SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(device);
        if (deviceInfo == nullptr) {
            LOG_WARN("[SleRemoteDeviceAdapter] device info is null, addr: %{public}s", GET_ENCRYPT_ADDR(device));
            continue;
        }
        std::string linkKeyStr = SleConfig::GetInstance().GetLinkKey(device.GetAddress());
        if ((INVALID_MAC_ADDRESS.compare(device.GetAddress()) == 0) ||
            (device.GetAddress().empty()) ||
            (linkKeyStr.empty())) {
            LOG_INFO("[SleRemoteDeviceAdapter] invalid data, addr: %{public}s", GET_ENCRYPT_ADDR(device));
            continue;
        }
        NLSTK_SmRecoverKeyParam_S pairedDevice;
        (void)memset_s(&pairedDevice, sizeof(pairedDevice), 0x00, sizeof(pairedDevice));

        SLE_Addr_S btAddr;
        (void)memset_s(&btAddr, sizeof(btAddr), 0x00, sizeof(btAddr));
        device.ConvertToUint8(btAddr.addr);
        btAddr.type = deviceInfo->GetAddressType();
        pairedDevice.addr = btAddr;
        // lk
        LinkKey sleLinkkey;
        if (SleLinkKeyDecrypt(linkKeyStr, sleLinkkey)) {
            if (memcpy_s(&(pairedDevice.linkKey), SM_LINK_KEY_LEN, &sleLinkkey[0], OCTET16_LEN) != EOK) {
                LOG_ERROR("[SleRemoteDeviceAdapter] memcpy_s failed!");
                (void)memset_s(&sleLinkkey, sizeof(sleLinkkey), 0x00, sizeof(sleLinkkey));
                continue;
            }
        }
        (void)memset_s(&sleLinkkey, sizeof(sleLinkkey), 0x00, sizeof(sleLinkkey));
        pairedDevice.cryptoAlgo = deviceInfo->GetCryptoAlgo();
        pairedDevice.keyDerivAlgo = deviceInfo->GetKeyDerivAlgo();
        pairedDevice.intgChkInd = deviceInfo->GetIntegrChkInd();
        storeDevices.push_back(pairedDevice);
        (void)memset_s(&pairedDevice, sizeof(pairedDevice), 0x00, sizeof(pairedDevice));
    }
    return storeDevices;
}

bool SleRemoteDeviceAdapter::SleLinkKeyDecrypt(const std::string &linkKeyStr, LinkKey &sleLinkkey)
{
    HILOGD("enter");
    if (linkKeyStr.empty()) {
        return false;
    }
    EncryptedLinkKey sleEncryptedLinkkey;
    if (SleUtils::ConvertHexStringToInt(linkKeyStr, sleEncryptedLinkkey.data(), sleEncryptedLinkkey.size())) {
        if (SleHksTool::GetInstance().SleLinkKeyDecrypt(sleEncryptedLinkkey, sleLinkkey) == HKS_SUCCESS) {
            return true;
        }
    }
    return false;
}

bool SleRemoteDeviceAdapter::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([sleAddr, btAddr, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetBtAddrBySleAddr(sleAddr, btAddr);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetBtAddrBySleAddrTask(const std::string &sleAddr, std::string &btAddr)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([sleAddr, &btAddr, &promise]() -> void {
        btAddr = InterfaceCloudPairService::GetInstance().GetBtAddrByReportAddr(sleAddr);
        if (!btAddr.empty()) {
            promise.set_value(true);
            return;
        }
        bool ret = SleRemoteDeviceManager::GetInstance()->GetBtAddrBySleAddr(sleAddr, btAddr);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetSleAddrByBtAddrTask(const std::string &btAddr, std::string &sleAddr)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([btAddr, &sleAddr, &promise]() -> void {
        sleAddr = InterfaceCloudPairService::GetInstance().GetReportAddrByBtAddr(btAddr);
        if (!sleAddr.empty()) {
            promise.set_value(true);
            return;
        }
        bool ret = SleRemoteDeviceManager::GetInstance()->GetSleAddrByBtAddr(btAddr, sleAddr);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::SetCdsmAddrType(const RawAddress &device, int addrType)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, addrType, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->SetCdsmAddrType(device, addrType);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::IsAudioDevice(const std::string &address)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([address, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->IsAudioDevice(address);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::IsServiceSupportedConn(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        bool ret = SleRemoteDeviceManager::GetInstance()->IsServiceSupportedConn(device);
        promise.set_value(ret);
    });
    return future.get();
}

/* 合作集report连接完成后保存数据 */
void SleRemoteDeviceAdapter::CdsmSaveData(const RawAddress &memberAddr)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, memberAddr, &promise]() -> void {
        CdsmSaveDataTask(memberAddr);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::CdsmSaveDataTask(const RawAddress &memberAddr)
{
    RawAddress reportAddr(memberAddr);
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService != nullptr && cdsmService->CdsmGetReportAddr(memberAddr, reportAddr) &&
        cdsmService->CdsmGetAllMemberInfo(memberAddr, cdsmInfo) == NL_NO_ERROR,
        "[SleRemoteDeviceAdapter]:save cdsm data fail,get cdsm info error!");

    std::vector<std::string> cdsmDevList = {};
    for (const auto &member : cdsmInfo) {
        cdsmDevList.push_back(member.addr_.GetAddress());
    }

    bool isPrivate = false;
    for (const auto &member : cdsmInfo) {
        std::shared_ptr<SlePeripheralDevice> device =
            SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(member.addr_);
        if (device == nullptr) {
            HILOGI("device is nullptr");
            continue;
        }
        int cdsmAddrType = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER);
        if (member.addr_ == reportAddr) {
            cdsmAddrType = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_REPORT);
        }
        device->SetCdsmAddrType(cdsmAddrType);
        SleRemoteDeviceManager::GetInstance()->SaveCdsmDeviceList(member.addr_, cdsmDevList);
        if (device->GetManufacturerBusiness() == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
            isPrivate = true;
        }
        SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(member.addr_.GetAddress(), device);
    }

    RawAddress otherAddr(memberAddr);
    cdsmService->CdsmGetOtherAddr(memberAddr, otherAddr);
    DftCacheCdsmInfo(memberAddr.GetAddress(), reportAddr.GetAddress(), otherAddr.GetAddress());

    SleRemoteDeviceManager::GetInstance()->SaveCdsmInfo(reportAddr, isPrivate, cdsmDevList);
}

static bool CheckCdsmDeviceIsPrivateAudioDevice(const RawAddress &reportAddr)
{
    int businessType = SleRemoteDeviceManager::GetInstance()->GetManufacturerBusinessType(reportAddr);
    // 双耳互换恢厂的场景中，会先把两只耳的缓存都从peerConnDeviceSafeList中删除，因此这里如果取不到，在从广播中取一下
    if (businessType != Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(reportAddr.GetAddress());
    }
    if (businessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        return true;
    }
    return false;
}

static bool CheckCdsmReportDeviceState(const RawAddress &reportAddr)
{
    /* 主设备配对状态检查 */
    int reportPairState = SleRemoteDeviceManager::GetInstance()->GetPairState(reportAddr);
    bool isReportAcbConnected = SleRemoteDeviceManager::GetInstance()->IsAcbConnected(reportAddr);
    int reportAcbState = SleRemoteDeviceManager::GetInstance()->GetAcbState(reportAddr.GetAddress());
    // 走到配对请求逻辑中，那么肯定是首次配对或者KeyMissing后重新触发了配对
    // 此时如果report地址未连接（可能左右互换恢厂）或者report地址未配对，都需要重新拉起一下配对弹窗
    if (!isReportAcbConnected) {
        HILOGI("[SleRemoteDeviceAdapter]cdsm report addr:%{public}s acb connect state:%{public}d invalid.",
            GET_ENCRYPT_ADDR(reportAddr), reportAcbState);
        if (reportAcbState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING)) {
            auto sleAdapter = SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE);
            NL_CHECK_RETURN_RET(sleAdapter, false, "sleAdapter is null");
            sleAdapter->DisconnectAcb(reportAddr,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        }
        return false;
    }
    if (reportPairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        HILOGI("[SleRemoteDeviceAdapter]:cdsm report addr:%{public}s pair state:%{public}d invalid.",
            GET_ENCRYPT_ADDR(reportAddr), reportPairState);
        return false;
    }
    return true;
}

static bool CheckIsCdsmMemberPair(const RawAddress &device, const RawAddress &reportAddr)
{
    if (CheckCdsmDeviceIsPrivateAudioDevice(reportAddr)) {
        return false;
    }
    if (reportAddr == device) {
        return false;
    }
    if (!CheckCdsmReportDeviceState(reportAddr)) {
        return false;
    }
    HILOGI("[SleRemoteDeviceAdapter]:cdsm member:%{public}s pair confirm ok.", GET_ENCRYPT_ADDR(device));
    return true;
}

/*
 * 是否合作集成员地址配对
 *
 * 逻辑说明：
 * 1. 如果是私有音频业务设备，直接返回false（私有耳机从管家弹窗拉起时，不需要弹框；副耳共LinkKey，也不需要弹窗）
 * 2. 如果report地址与device地址相同，返回false（说明当前为report地址的配对，需要弹框）
 * 3. 检查report设备状态，如果状态无效返回false（可能为首次配对或左右互换KeyMissing场景，需要重新拉起配对弹框）
 * 4. 所有条件都满足时返回true（说明此时为非私有耳机的副耳配对，不需要弹框）
 */
bool SleRemoteDeviceAdapter::IsCdsmMemberPair(const RawAddress &device, const RawAddress &reportAddr)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, reportAddr, &promise]() -> void {
        promise.set_value(CheckIsCdsmMemberPair(device, reportAddr));
    });
    return future.get();
}

void SleRemoteDeviceAdapter::SetAudioDeviceFlag(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        SleRemoteDeviceManager::GetInstance()->SetIsAudioDeviceFlag(device);
        HILOGI("SetAudioDeviceFlag done for device: %{public}s", GET_ENCRYPT_ADDR(device));
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SaveDeviceManufacturerAbility(const RawAddress &rawAddr)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, rawAddr, &promise]() -> void {
        SaveDeviceManufacturerAbilityInner(rawAddr);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SaveDeviceManufacturerAbilityInner(const RawAddress &rawAddr)
{
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manfacturerAbility =
        SleRemoteDeviceManager::GetInstance()->GetManufacturerAbility(rawAddr);
    ProfileTws *twsService = static_cast<ProfileTws *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    NL_CHECK_RETURN(twsService != nullptr, "[SleRemoteDeviceAdapter]tws service instance invalid");
    ManufacturerAbilityLoader::GetInstance().FilterAbility(manfacturerAbility);
    twsService->SetDeviceManufacturerAbility(rawAddr, manfacturerAbility);
}

void SleRemoteDeviceAdapter::SavePairDirect(int connDirect, const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, connDirect, device, &promise]() -> void {
        SavePairDirectInner(connDirect, device);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SavePairDirectInner(int connDirect, const RawAddress &device)
{
    if (connDirect == static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE)) {
        SleConfig::GetInstance().SetPairDirect(device.GetAddress(),
            static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
        SleRemoteDeviceManager::GetInstance()->SetPairDirection(device,
            static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    } else if (connDirect == static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE)) {
        SleConfig::GetInstance().SetPairDirect(device.GetAddress(),
            static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
        SleRemoteDeviceManager::GetInstance()->SetPairDirection(device,
            static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
    }
}

/* 私有设备设置默认角色 */
void SleRemoteDeviceAdapter::UpdateDefaultRole(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService == nullptr ||
            cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
            promise.set_value();
            return;
        }

        std::vector<RawAddress> memberAddrs;
        for (auto &member : cdsmInfo) {
            memberAddrs.push_back(member.addr_);
        }
        int encryptedCnt = SleRemoteDeviceManager::GetInstance()->GetEncryptedDevicesCount(memberAddrs);

        if (encryptedCnt != 1) {
            promise.set_value();
            return;
        }

        ProfileTws *twsService = static_cast<ProfileTws *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
        if (twsService == nullptr) {
            promise.set_value();
            return;
        }

        HILOGI("[SleRemoteDeviceAdapter]:peer addr:%{public}s,update default role to Primary.",
            GET_ENCRYPT_ADDR(device));
        twsService->TwsUpdateDeviceDefaultRole(device, static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
        promise.set_value();
    });
    future.get();
}

bool SleRemoteDeviceAdapter::ConnectionCompleteHelper(const RawAddress &device, uint16_t lcid, uint8_t role,
    uint8_t addrType)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([device, lcid, role, addrType, &promise]() -> void {
        int pairState = SleRemoteDeviceManager::GetInstance()->GetPairState(device);
        if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED) &&
            SleRemoteDeviceManager::GetInstance()->GetDeviceAppearance(device)
            == static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK)) {
            ServiceManagerPluginLoader::GetInstance()->HighPowerProc(lcid);
        }
        bool ret = SleRemoteDeviceManager::GetInstance()->SetConnectionInfo(device, lcid, role, addrType);
        promise.set_value(ret);
    });
    return future.get();
}

void SleRemoteDeviceAdapter::HandleCdsmMemberFirstPairing(const RawAddress &member)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, member, &promise]() -> void {
        HandleCdsmMemberFirstPairingTask(member);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::HandleCdsmMemberFirstPairingTask(const RawAddress &member)
{
    std::string name = "";
    int appearance = GetDeviceAppearanceInner(member);
    RawAddress reportDevice;
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    cdsmService->CdsmGetReportAddr(member, reportDevice);
    std::shared_ptr<const SlePeripheralDevice> reportDevicePtr =
        SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(reportDevice);
    if (reportDevicePtr != nullptr) {
        if (appearance == INVALID_APPEARANCE) {
            appearance = reportDevicePtr->GetAppearance();
        }
        name = reportDevicePtr->GetName();
    }
    HILOGI("[SleRemoteDeviceAdapter]find report device:%{public}s, appearance: %{public}d",
        GET_ENCRYPT_ADDR(member), appearance);
    std::shared_ptr<const SlePeripheralDevice> memberDevice =
        SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(member);
    if (memberDevice != nullptr) {
        SleRemoteDeviceManager::GetInstance()->SetAppearance(member, appearance);
        SleRemoteDeviceManager::GetInstance()->SetName(member, name);
        SleRemoteDeviceManager::GetInstance()->SetCdsmAddrType(member,
            static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER));
        SleRemoteDeviceManager::GetInstance()->SetConnDirect(member,
            static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
        HILOGI("[SleRemoteDeviceAdapter]update member device:%{public}s appearance: %{public}d",
            GET_ENCRYPT_ADDR(member), appearance);
    }
}

void SleRemoteDeviceAdapter::SetOtherDeviceInfo(std::shared_ptr<const SlePeripheralDevice> &srcDev,
    std::shared_ptr<SlePeripheralDevice> &otherDev) const
{
    otherDev->SetRoles(srcDev->GetLinkRole());
    otherDev->SetAddressType(srcDev->GetAddressType());
    otherDev->SetAppearance(srcDev->GetAppearance());
    otherDev->SetName(srcDev->GetName());
    otherDev->SetManufacturerBusiness(srcDev->GetManufacturerBusiness());
    otherDev->SetModelId(srcDev->GetModelId());
    otherDev->SetNewModelId(srcDev->GetNewModelId());
    otherDev->SetSubModelId(srcDev->GetSubModelId());
    otherDev->SetIconId(srcDev->GetIconId());
    otherDev->SetCryptoAlgo(srcDev->GetCryptoAlgo());
    otherDev->SetKeyDerivAlgo(srcDev->GetKeyDerivAlgo());
    otherDev->SetIntegrChkInd(srcDev->GetIntegrChkInd());
    otherDev->SetEncryptGroupKeyStr(srcDev->GetEncryptGroupKeyStr());
    otherDev->SetGiv(srcDev->GetGiv());
    otherDev->SetManufacturerAbility(srcDev->GetManufacturerAbility());
}

uint32_t SleRemoteDeviceAdapter::GetDeviceTypeFromAppearance(const RawAddress &device, int deviceAppearance)
{
    uint32_t deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_OTHER);
    switch (deviceAppearance) {
        case static_cast<int>(DeviceClassForService::DEVICE_SMART_WATCH):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_WATCH);
            break;
        case static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_KEYBOARD);
            break;
        case static_cast<int>(DeviceClassForService::DEVICE_MOUSE):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_MOUSE);
            break;
        case static_cast<int>(DeviceClassForService::DEVICE_STYLUS):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_PEN);
            break;
        case static_cast<int>(DeviceClassForService::DEVICE_HANDLE):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_HANDLE);
            break;
        case static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE):
            deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_AUDIO);
            break;
        default:
            HILOGD("[SleRemoteDeviceAdapter]:appearance:%{public}#x,device type set default.", deviceAppearance);
            break;
    }
    if (SleRemoteDeviceManager::GetInstance()->IsAudioDevice(device.GetAddress())) {
        deviceType = static_cast<uint32_t>(DeviceTypeForService::DEVICE_TYPE_AUDIO);
    }
    return deviceType;
}

uint32_t SleRemoteDeviceAdapter::GetAudioDeviceGroupId(const RawAddress &device)
{
    uint32_t groupId = CDSM_SERVICE_INVALID_GROUP_ID;
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN_RET(cdsmService, groupId, "cdsmService is nullptr.");
    if (SleRemoteDeviceManager::GetInstance()->IsAudioDevice(device.GetAddress())) {
        bool isVendorDevice = false;
        int businessType = SleRemoteDeviceManager::GetInstance()->GetManufacturerBusinessType(device);
        if (businessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
            isVendorDevice = true;
        }
        bool hasGid = cdsmService->CdsmGetGroupId(groupId, device.GetAddress());
        if (!hasGid && !isVendorDevice) {
            std::vector<RawAddress> devList;
            devList.push_back(device);
            groupId = cdsmService->CdsmCreateGroup(device, devList);
        }
        if (groupId == CDSM_SERVICE_INVALID_GROUP_ID) {
            HILOGI("[SleRemoteDeviceAdapter]get cdsm group id fail.");
        }
    }
    return groupId;
}

void SleRemoteDeviceAdapter::GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType)
{
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    RawAddress report(device);
    if (cdsmService != nullptr && !cdsmService->CdsmGetReportAddr(device, report)) {
        HILOGW("[SleRemoteDeviceAdapter]get cdsm report fail.");
    }
    /* 地址转换 */
    addrInfo.type = GetPeerDeviceAddrTypeInner(report);  // 成员地址的信息，这个时候还取不到，转换为取report的信息
    device.ConvertToUint8(addrInfo.addr, SLE_ADDR_LEN);

    int deviceAppearance = GetDeviceAppearanceInner(device);
    uint32_t deviceType = GetDeviceTypeFromAppearance(report, deviceAppearance);
    uint32_t groupId = GetAudioDeviceGroupId(report);

    devType = ((groupId << 16) & 0xFFFF0000) | (deviceType & 0x0000FFFF); // 16 is shift left by 16 bits
    HILOGI("[SleRemoteDeviceAdapter]get peer(%{public}s->%{public}s) devType:%{public}#x, appearance:%{public}#x, "
           "cdsm group:%{public}#x", GET_ENCRYPT_ADDR(device), GET_ENCRYPT_ADDR(report), devType, deviceAppearance,
           groupId);
}

// 下发对端设备类别给芯片, 参考DeviceTypeForService
void SleRemoteDeviceAdapter::SetPeerDeviceTypeToController(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        ServiceManagerPluginLoader::GetInstance()->PeerDeviceTypeProc(
                PeerDeviceTypeProcType::PEERDEVICETYPE_PROC_SET, device);
        promise.set_value();
    });
    future.get();
}

// 下发对端设备类别给芯片, 参考DeviceTypeForService
void SleRemoteDeviceAdapter::RemovePeerDeviceTypeToController(const RawAddress &device)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([device, &promise]() -> void {
        ServiceManagerPluginLoader::GetInstance()->PeerDeviceTypeProc(
                PeerDeviceTypeProcType::PEERDEVICETYPE_PROC_REMOVE, device);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::AddBgConnDevice(const std::string &address)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, address, &promise]() -> void {
        AddBgConnDeviceInner(address);
        promise.set_value();
    });
    future.get();
}

bool SleRemoteDeviceAdapter::GetCdsmOtherAddr(const RawAddress &member, RawAddress &other)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInDeviceAdapterThread([this, member, &other, &promise]() -> void {
        bool result = GetCdsmOtherAddrInner(member, other);
        promise.set_value(result);
    });
    return future.get();
}

bool SleRemoteDeviceAdapter::GetCdsmOtherAddrInner(const RawAddress &device, RawAddress &other)
{
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    return cdsmService != nullptr && cdsmService->CdsmGetOtherAddr(device, other);
}

void SleRemoteDeviceAdapter::SendBgConnList(NearlinkSafeList <RawAddress> &bgList)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &bgList, &promise]() -> void {
        SendBgConnListInner(bgList);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SendDirectConnList(NearlinkSafeList <RawAddress> &directList)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &directList, &promise]() -> void {
        SendDirectConnListInner(directList);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::SendBgConnListInner(NearlinkSafeList<RawAddress> &bgList)
{
    uint32_t bgNum = static_cast<uint32_t>(bgList.Size());
    NL_CHECK_RETURN(bgNum > 0 && bgNum <= BG_CONN_MAX_NUMBER, "bgNum is invalid.");
    HILOG_COMM_INFO("[%{public}s:%{public}d]bgNum:%{public}d", __FUNCTION__, __LINE__, bgNum);
    std::vector<CM_BgConnAddrParam_S> stackAddr;
    stackAddr.reserve(bgNum);
    bgList.Iterate([this, &stackAddr](RawAddress device) -> void {
        ServiceManagerPluginLoader::GetInstance()->PeerDeviceTypeProc(
                PeerDeviceTypeProcType::PEERDEVICETYPE_PROC_SET, device);
        CM_BgConnAddrParam_S addrParam = {};
        addrParam.isBypass = SleRemoteDeviceManager::GetInstance()->GetIsAudioDeviceFlag(device);
        device.ConvertToUint8(addrParam.addr.addr);
        addrParam.addr.type = SleRemoteDeviceManager::GetInstance()->GetPeerDeviceAddrType(device);
        HILOGI("add bg list, isBypass=%{public}d, addr=%{public}s, addrType=%{public}d", addrParam.isBypass,
               GET_ENCRYPT_ADDR(device), addrParam.addr.type);
        stackAddr.push_back(addrParam);
        DftCacheBgStartConn(device.GetAddress(), GetDeviceAppearanceInner(device));
    });
    uint32_t ret = CM_BackgroundConnectAdd(CM_MODULE_ADPT, static_cast<uint8_t>(stackAddr.size()), stackAddr.data());
    NL_CHECK_RETURN(ret == CM_SUCCESS, "ret=%{public}d", ret);
}

void SleRemoteDeviceAdapter::SendDirectConnListInner(NearlinkSafeList<RawAddress> &directList)
{
    size_t directNum = directList.Size();
    NL_CHECK_RETURN(directNum > 0 && directNum <= BG_CONN_MAX_NUMBER, "directNum is invalid.");
    HILOG_COMM_INFO("[%{public}s:%{public}d]directNum:%{public}zu", __FUNCTION__, __LINE__, directNum);
    directList.Iterate([this](RawAddress device) -> void {
        ServiceManagerPluginLoader::GetInstance()->PeerDeviceTypeProc(
                PeerDeviceTypeProcType::PEERDEVICETYPE_PROC_SET, device);
        CM_DirectConnAddrParam_S param = {};
        device.ConvertToUint8(param.addr.addr);
        param.addr.type = SleRemoteDeviceManager::GetInstance()->GetPeerDeviceAddrType(device);
        param.frameType = CM_CONN_PARAM_FRAME_TYPE_1;
        uint32_t ret = CM_DirectConnectAdd(CM_MODULE_ADPT, &param);
        if (ret != CM_SUCCESS) {
            HILOGE("CM_DirectConnectAdd:ret=%{public}d", ret);
        }
    });
}

void SleRemoteDeviceAdapter::AddBgConnDeviceInner(const std::string &address)
{
    HILOGD("[SleRemoteDeviceAdapter] enter");
    bool bgConnFlag = true;
    bool isNeedBypass = false;
    RawAddress rawAddr(address);
    int pairDirection = SleRemoteDeviceManager::GetInstance()->GetPairDirection(rawAddr);
    int appearance = SleRemoteDeviceManager::GetInstance()->GetDeviceAppearance(rawAddr);
    bool isDeviceAvailable = SleRemoteDeviceManager::GetInstance()->GetIsDeviceAvailable(rawAddr);
    isNeedBypass = SleRemoteDeviceManager::GetInstance()->GetIsAudioDeviceFlag(rawAddr);
    if (pairDirection == static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE) ||
        appearance == static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK) ||
        appearance == static_cast<int>(DeviceClassForService::DEVICE_NETWORKING) ||
        !isDeviceAvailable) {
        bgConnFlag = false;
    }
    if (!bgConnFlag) {
        HILOGI("not find address(%{public}s) or passive pair or DEVICE_CLASS_VEHICLE, bgConnFlag:%{public}d",
            GetEncryptAddr(address).c_str(), bgConnFlag);
    } else {
        CM_BgConnAddrParam_S stackAddr = {};
        rawAddr.ConvertToUint8(stackAddr.addr.addr);
        stackAddr.addr.type =  SleRemoteDeviceManager::GetInstance()->GetPeerDeviceAddrType(rawAddr);
        stackAddr.isBypass = isNeedBypass;
        DftCacheBgStartConn(address, GetDeviceAppearanceInner(rawAddr));
        HILOG_COMM_INFO("[%{public}s:%{public}d]add bg list, isBypass=%{public}d, addr=%{public}s, addrType=%{public}d",
            __FUNCTION__, __LINE__, stackAddr.isBypass, GET_ENCRYPT_ADDR(rawAddr), stackAddr.addr.type);
        ServiceManagerPluginLoader::GetInstance()->PeerDeviceTypeProc(
                PeerDeviceTypeProcType::PEERDEVICETYPE_PROC_SET, rawAddr);
        uint32_t ret = CM_BackgroundConnectAdd(CM_MODULE_ADPT, 1, &stackAddr);
        NL_CHECK_RETURN(ret == CM_SUCCESS, "ret=%{public}d", ret);
    }
}

/* 共用link_key特性 */
void SleRemoteDeviceAdapter::CdsmAddOtherRecord(const RawAddress &srcAddr, const RawAddress &otherAddr)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, srcAddr, otherAddr, &promise]() -> void {
        CdsmAddOtherRecordTask(srcAddr, otherAddr);
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::CdsmAddOtherRecordTask(const RawAddress &srcAddr, const RawAddress &otherAddr)
{
    int acbConnState = SleRemoteDeviceManager::GetInstance()->GetAcbState(otherAddr.GetAddress());
    std::shared_ptr<SlePeripheralDevice> otherDev = std::make_shared<SlePeripheralDevice>();
    std::shared_ptr<const SlePeripheralDevice> srcDevice =
        SleRemoteDeviceManager::GetInstance()->GetRemoteDevice(srcAddr);
    NL_CHECK_RETURN(srcDevice != nullptr,
        "update addr %{public}s pair record failed", GetEncryptAddr(srcAddr.GetAddress()).c_str());
    SetOtherDeviceInfo(srcDevice, otherDev);
    otherDev->SetLcid(INVALID_LCID);
    otherDev->SetAddress(otherAddr);
    otherDev->SetAcbConnectState(acbConnState);
    otherDev->SetPrePairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    otherDev->SetConnDirect(static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));
    otherDev->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    SavePairDirectInner(static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE), otherAddr);
    otherDev->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)); /* 共link_key，已配对 */

    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "ProfileCdsm is null.");
    int cdsmAddrType = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE);
    if (cdsmService->CdsmCheckIsCooperationReport(otherAddr)) {
        cdsmAddrType = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_REPORT);
    } else if (cdsmService->CdsmCheckIsCooperationMember(otherAddr)) {
        cdsmAddrType = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER);
    }
    otherDev->SetCdsmAddrType(cdsmAddrType);
    SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice(otherAddr.GetAddress(), otherDev);
    DftDeviceManager::GetInstance().AddDevice(otherAddr);
    SleRemoteDeviceManager::GetInstance()->SetIsAudioDeviceFlag(otherAddr);
    AddBgConnDeviceInner(otherAddr.GetAddress());
    SleRemoteDeviceManager::GetInstance()->SavePeerDeviceInfoToConf();
    SaveDeviceManufacturerAbilityInner(otherAddr);
    RawAddress reportAddr(otherAddr);
    cdsmService->CdsmGetReportAddr(otherAddr, reportAddr);
    std::string btAddr = InterfaceScanService::GetInstance().GetBtAddr(reportAddr.GetAddress());
    if (btAddr.empty()) {
        btAddr = InterfaceCloudPairService::GetInstance().GetBtAddrByReportAddr(reportAddr.GetAddress());
    }
    bool ret = SleRemoteDeviceManager::GetInstance()->SetBtAddrBySleAddr(reportAddr.GetAddress(), btAddr);
    HILOGI("sle:%{public}s, bt:%{public}s,  ret=%{public}d", GET_ENCRYPT_ADDR(reportAddr),
        GetEncryptAddr(btAddr).c_str(), ret);
}

void SleRemoteDeviceAdapter::CancelAllConnection()
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &promise]() -> void {
        auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        if (sleAdapter == nullptr) {
            promise.set_value();
            return;
        }
        std::vector<RawAddress> connectingDevices = SleRemoteDeviceManager::GetInstance()->GetConnectingDevices();
        for (const auto &device : connectingDevices) {
            std::string addr = device.GetAddress();
            SleRemoteDeviceManager::GetInstance()->SetAcbState(
                addr, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING));
            DftCacheAcbDisConn(addr, NearLinkPermissionManager::GetCallingName());
            sleAdapter->DisconnectAcb(device,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        }
        sleAdapter->ClearBgConnDevice();
        promise.set_value();
    });
    future.get();
}

bool SleRemoteDeviceAdapter::DisconnectAcbAction(const RawAddress &device, uint8_t discReason)
{
    SleRemoteDeviceManager::GetInstance()->SetAcbState(
        device.GetAddress(), static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING));
    HILOGI("[SleRemoteDeviceAdapter] DisconnectAcb: %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    HILOGI("[SleRemoteDeviceAdapter] addr: %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    DftCacheAcbDisConn(device.GetAddress(), NearLinkPermissionManager::GetCallingName());
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleAdapter, false, "sleAdapter is null");
    return sleAdapter->DisconnectAcb(device, discReason);
}

void SleRemoteDeviceAdapter::RemoveAllPairsProcess(std::vector<RawAddress> &removeDevices)
{
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInDeviceAdapterThread([this, &removeDevices, &promise]() -> void {
        std::vector<RawAddress> pairedDevices = SleRemoteDeviceManager::GetInstance()->GetPairedDevices();
        for (const auto &device : pairedDevices) {
            std::string addr = device.GetAddress();
            bool ret = SleConfig::GetInstance().RemovePairedDevice(addr);
            DftDealUnPairLinkKeyInfo(addr, ret);
            LOG_INFO("[SleRemoteDeviceAdapter] RemovePairedDevice %{public}s", GetEncryptAddr(addr).c_str());
            removeDevices.push_back(device);
            if (!DisconnectAcbAction(device,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED))) {
                HILOGE("[SleRemoteDeviceAdapter] Disconnect failed!");
                continue;
            }
            SleRemoteDeviceManager::GetInstance()->RemovePeripheralDevice(addr);
        }
        promise.set_value();
    });
    future.get();
}

void SleRemoteDeviceAdapter::ClearPeerDeviceGroupId()
{
    HILOGI("[SleRemoteDeviceAdapter] enter");

    std::promise<std::vector<RawAddress>> promise;
    std::future<std::vector<RawAddress>> future = promise.get_future();

    DoInDeviceAdapterThread([&promise]() -> void {
        std::vector<RawAddress> pairedList = SleRemoteDeviceManager::GetInstance()->GetPairedDevices();
        promise.set_value(pairedList);
    });

    std::vector<RawAddress> pairedList = future.get();
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter != nullptr, "[SleRemoteDeviceAdapter] sleAdapter is nullptr");
    for (const auto &device : pairedList) {
        RemovePeerDeviceTypeToController(device);
    }
}

} // namespace Nearlink
} // namespace OHOSG