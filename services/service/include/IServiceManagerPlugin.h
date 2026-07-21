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
#ifndef I_SERVICE_MANAGER_PLUGIN_H
#define I_SERVICE_MANAGER_PLUGIN_H

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include "raw_address.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "HidHostDefines.h"
#include "nearlink_def.h"
#include "nearlink_safe_list.h"
#include "sdf_struct.h" // SLE_Addr_S

namespace OHOS {
namespace Nearlink {
/**
 * @brief 提供给插件使用的方法封装.
 */
class SleInterfaceAdapterWrapper {

public:
    virtual ~SleInterfaceAdapterWrapper() = default;

    /**
     * @brief Get connected devices.
     *
     * @return Returns connected devices vector.
     * @since 6
     */
    virtual std::vector<RawAddress> GetConnectedDevices() const = 0;
    virtual uint16_t GetLcidByAddress(const RawAddress &device) = 0;
    virtual void AddBgConnDevice(const std::string &address)  = 0;
    virtual bool IsAudioDevice(const std::string &address) = 0;
    virtual bool GetCdsmOtherAddr(const RawAddress &member, RawAddress &other) = 0;
    virtual void SendBgConnList(NearlinkSafeList<RawAddress> &bgList) = 0;
    virtual void SendDirectConnList(NearlinkSafeList<RawAddress> &directList) = 0;
    virtual std::string GetAliasName(const std::string &address) const = 0;
    virtual bool WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) = 0;
    virtual uint8_t GetPeerDeviceAddrType(const RawAddress& address) const = 0;
    virtual void GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType) const = 0;
    virtual int HidSendData(const HidReportInfo &reportInfo) const = 0;
    virtual int GetDeviceAppearance(const RawAddress &device) const = 0;
    virtual bool DisconnectAllProfile(const RawAddress &device) = 0;
    virtual bool IsProxyConnectExisted(std::string &devAddress) = 0;
};

/**
 * @brief 函数调用枚举。
 * 通过枚举值区分不同的函数调用，避免相似函数调用的函数重载。
 */

enum class CollaborationProcType {
    COLLABORATION_PROC_INIT = 0,
    COLLABORATION_PROC_DEINIT = 1,
    COLLABORATION_PROC_TYPE_MAX,
}; // enum CollaborationProcType end

// adapter模块相关函数 start
enum class PeerDeviceTypeProcType {
    PEERDEVICETYPE_PROC_SET = 0,
    PEERDEVICETYPE_PROC_REMOVE = 1,
    PEERDEVICETYPE_PROC_TYPE_MAX,
};

enum class SetPowerModeProcType {
    POWERMODE_PROC_ENABLE = 0,
    POWERMODE_PROC_DISENABLE = 1,
    POWERMODE_PROC_TYPE_MAX,
};

enum class RegisterCallbackModule {
    REGISTER_CALLBACK_MODULE_CM = 0,
    REGISTER_CALLBACK_MODULE_NBC = 1,
    REGISTER_CALLBACK_MODULE_MAX,
};
// adapter模块相关函数 end

enum class BundleNameType {
    BUNDLE_NAME_SETTINGS = 0,
    BUNDLE_NAME_AIBASE = 1,
    BUNDLE_NAME_TYPE_MAX,
};

// 函数调用枚举结束

struct AcbSubrateParam_S {
    uint16_t subrate;
    uint16_t subrateMax;
};

class ServiceManagerPluginInterface {
public:
    using GetSleAdapterFunc = std::function<SleInterfaceAdapterWrapper *()>;
    using SetAdapterStateObserverFunc = std::function<void(IAdapterStateObserver& observer)>;
    using SetPeripheralCallbackFunc = std::function<void(ISlePeripheralCallback& callback)>;

    virtual ~ServiceManagerPluginInterface() = default;

    /**
     * @brief Get adapter manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static ServiceManagerPluginInterface* GetInstance();

    bool IsLibraryLoaded(void);

    virtual void SetSleAdapterFunc(GetSleAdapterFunc func) = 0;
    virtual void SetAdapterStateObserver(SetAdapterStateObserverFunc func) = 0;
    virtual void SetPeripheralCallback(SetPeripheralCallbackFunc func) = 0;

    virtual void Init() = 0;
    virtual void DeInit() = 0;

    virtual void PowerMgrProc() = 0;
    virtual void HighPowerProc(uint16_t lcid) = 0;
    virtual void SleTvMgrProc(const std::string &address) = 0;
    virtual void HidDataStatisticsProc(const std::string &address) = 0;
    virtual void RssiChangedCbkProc(const std::string &address, int8_t rssi) = 0;
    virtual void UpdateSleFreqBandAbility(const std::string &address) = 0;
    virtual void CollaborationProc(CollaborationProcType type) = 0;
    virtual void SvcCmdProc(std::string cmd, int32_t fd, const std::vector<std::u16string> &args, int32_t &svcResult,
        std::string &info) = 0;
    virtual void SleReconnectProc(bool &isNeedReconn, int acbConnState, const RawAddress &peerAddr, int reason,
        std::set<int> reasonList) = 0;
    // adapter模块相关函数 start
    virtual void PeerDeviceTypeProc(PeerDeviceTypeProcType proctype, const RawAddress &device) = 0;
    virtual void SetPowerModeProc(SetPowerModeProcType proctype) = 0;
    virtual void RegisterCallbackExt(RegisterCallbackModule module, int32_t &result) = 0;
    virtual void GetLocalVocieCallFrameFourAbility(bool &isSupport) = 0;
    // adapter模块相关函数 end
    // chiputil模块 start
    virtual void SetAcbSubrate(bool &ret, const RawAddress &device, const SleAcbSubrateParam &subrateParam) = 0;
    virtual void SetConnFrameType4Subrate(const RawAddress &device) = 0;
    // chiputil模块 end

    virtual std::string GetBundleName(BundleNameType type) = 0;
    // plugin 模块函数抽象接口声明 end
};

}  // namespace Nearlink
}  // namespace OHOS
#endif