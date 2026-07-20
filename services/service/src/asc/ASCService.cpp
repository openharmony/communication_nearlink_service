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
#include "ASCService.h"
#include <iostream>
#include <sstream>
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "SleUtils.h"
#include "SleFeature.h"
#include "ASCUtils.h"
#include "ThreadUtil.h"
#include "parameters.h"
#include "SleASC.h"
#include "CdsmService.h"
#include "SleInterfaceProfileTws.h"
#include "VcpService.h"
#include "TwsService.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include <future>
#include "VasService.h"
#include "McpServerService.h"
#include "CcpService.h"
#include "nearlink_dft_ue.h"
#include "nearlink_dft_exception.h"
#include "cm_api.h"
#include "SleHuksTool.h"
#include "DeviceBatteryManager.h"
#include "cm_def.h"
#include "ASCDefines.h"
#include "MicService.h"
#include "SleConfig.h"
#include "SleReconnectManager.h"
#include "nearlink_common_event_helper.h"
#include "ServiceManagerPluginLoader.h"
#include "SleRemoteDeviceAdapter.h"
#include "ManufacturerAbilityLoader.h"
#include "nearlink_verification_manager.h"
#include "verification_type.h"
#include "SleAudioFrameworkAdapter.h"
#include "nearlink_datashare_helper.h"
#include "nearlink_system_config.h"
#include "nlstk_api_type_ext.h"
#include "nlstk_public_define_ext.h"

constexpr const uint16_t TIMER_FOR_RESTORE_VOLUME_IF_PAUSED_MS = 1000;

namespace OHOS {
namespace Nearlink {
namespace {
// treat nullptr as empty for safe read-only check
inline bool IsQueueEmpty(const std::queue<AudioStreamType>* q)
{
    return (q == nullptr) || q->empty();
}
} // namespace
ASCService::ASCService() : utility::Context(PROFILE_NAME_ASC, "1.0.0")
{
    HILOGD("[ASCService]%{public}s Create", PROFILE_NAME_ASC.c_str());
    Init();
    audioObserver_ = nullptr;
}

ASCService::~ASCService()
{
    HILOGD("[ASCService]%{public}s Destroy", PROFILE_NAME_ASC.c_str());
}

utility::Context *ASCService::GetContext()
{
    return this;
}

ASCService *ASCService::GetService()
{
    return static_cast<ASCService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
}

std::list<RawAddress> ASCService::GetConnectDevices()
{
    HILOGD("[ASCService] GetConnectDevices Enter");

    std::list<RawAddress> list {};
    connectedDev_.Iterate(
        [&list](std::string key, ASCConnectedDev value)-> void {
        list.emplace_back(value.dev);
    });

    return list;
}

void ASCService::AddConnectDevices(const RawAddress &device)
{
    HILOGD("[ASCService] AddConnectDevices Enter");
    ASCConnectedDev connDev {};
    connDev.dev = device;
    // 当前时间作为连接时间
    connDev.time = time(nullptr);
    connectedDev_.EnsureInsert(device.GetAddress(), connDev);

    return;
}

bool ASCService::IsLeftEarDevice(const RawAddress &device)
{
    if (SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        TwsService* twsService = TwsService::GetService();
        if (twsService == nullptr) {
            return true;
        }

        uint8_t nature = twsService->TwsGetDeviceNature(device);
        HILOGI("[ASCService] device=%{public}s, nature=%{public}d", GET_ENCRYPT_ADDR(device), nature);
        return nature == static_cast<uint8_t>(TwsNatureType::TWS_LEFT);
    } else {
        bool isLeft = false;
        ASCConnectedDev connDev {};
        if (connectedDev_.GetValue(device.GetAddress(), connDev)) {
            isLeft = connDev.isLeft;
        }
        HILOGI("[ASCService][Common]IsLeftEar %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            isLeft);
        return isLeft;
    }
}

void ASCService::DeleteConnectDevices(const RawAddress &device)
{
    HILOGD("[ASCService] DeleteConnectDevices Enter");
    connectedDev_.FindAndRmv(
        [this, &device](std::string key, ASCConnectedDev value)->bool {
        if (value.dev == device) {
            return true;
        }
        return false;
    });

    return;
}

void ASCService::DeleteEarliestDevice()
{
    HILOGD("[ASCService] DeleteConnectDevices Enter");
    time_t timeEarliest = time(nullptr);
    RawAddress earliestDev {};
    connectedDev_.Iterate(
        [this, &earliestDev, &timeEarliest](std::string key, ASCConnectedDev value)-> void {
        bool isEarlier = (value.time < timeEarliest);
        ASCState state = GetASCStatus(value.dev);
        HILOGI("[ASCService] DeleteConnectDevices %{public}s earlier %{public}d state %{public}d sinkDev %{public}s",
            GetEncryptAddr(value.dev.GetAddress()).c_str(), isEarlier, state,
            GetEncryptAddr(activeSinkDevice_.GetAddress()).c_str());
        if (isEarlier && !IsStarted(state) && !(value.dev == activeSinkDevice_)) {
            earliestDev = value.dev;
            timeEarliest = value.time;
            HILOGI("[ASCService] DeleteConnectDevices earlier choose %{public}s",
                GetEncryptAddr(earliestDev.GetAddress()).c_str());
        }
    });

    HILOGI("[ASCService] DeleteConnectDevices earliest %{public}s", GetEncryptAddr(earliestDev.GetAddress()).c_str());
    // 合作集设备也删除
    RawAddress coSetDevice {};
    bool isCoExist = IsCoSetDeviceExist(earliestDev, coSetDevice);
    if (isCoExist) {
        DisconnectAcb(coSetDevice);
    }

    DisconnectAcb(earliestDev);
}

void ASCService::RegisterObserver(ASCObserver &observer)
{
    HILOGD("[ASCService] Enter");
    audioObserver_ = &observer;
}

void ASCService::DeregisterObserver(ASCObserver &observer)
{
    HILOGD("[ASCService] Enter");
    audioObserver_ = nullptr;
}

void ASCService::Enable()
{
    HILOGD("[ASCService] Enable Enter");

    ASCMessage event(ASC_SERVICE_STARTUP_EVT);
    PostEvent(event);
}

void ASCService::Disable()
{
    HILOGD("[ASCService] Disable Enter");

    ASCMessage event(ASC_SERVICE_SHUTDOWN_EVT);
    PostEvent(event);
}

void ASCService::StartUp()
{
    HILOGD("[ASCService]:==========<start>==========");
    if (isStarted_.load()) {
        HILOGW("[ASCService]:ASCService has already been started before.");
    }

    InitMicStateObserver();
    GetContext()->OnEnable(PROFILE_NAME_ASC, true);
    isStarted_.store(true);
    HILOGD("[ASCService]:ASCService started");
}

void ASCService::ShutDown()
{
    HILOGD("[ASCService]:==========<shutdown>==========");
    if (!isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_ASC, true);
        HILOGW("[ASCService]:ASCService has already been shutdown before.");
        return;
    }

    DeInitMicStateObserver();
    // 断开所有已连接设备
    connectedDev_.Iterate(
        [this](std::string key, ASCConnectedDev value)-> void {
        /* 上报虚拟连接设备下线 */
        RawAddress addr = GetReportAddr(value.dev);
        UpdateSleVirtualDevice(UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_DELETE, addr);
    });

    GetContext()->OnDisable(PROFILE_NAME_ASC, true);
    isStarted_.store(false);
    UnregisterAudioPreferredOutPutDeviceChangeListener();
    HILOGD("[ASCService]:ASCService shutdown");
}

int ASCService::Connect(const RawAddress &device)
{
    HILOGD("[ASCService] Connect Enter %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    ASCMessage event(ASC_CONNECT_START_EVT);
    event.dev_ = device.GetAddress();
    PostEvent(event);
    return NL_NO_ERROR;
}

void ASCService::ProcessConnectEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    // 音频属性获取
    int ret = GetAudioProperty(device);
    if (ret != NL_NO_ERROR) {
        // 上报状态: 连接,失败
        ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_CONN, NL_SLE_ASC_RESULT_FAIL, ret);
    }
    SetAutoConnectDevice(device, false);
    return;
}

int ASCService::Disconnect(const RawAddress &device)
{
    HILOGD("[ASCService Service] Disconnect Enter");
    ASCMessage event(ASC_DISCONNECT_START_EVT);
    event.dev_ = device.GetAddress();
    PostEvent(event);
    return NL_NO_ERROR;
}

void ASCService::ProcessDisConnectEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);

    ASCState state = GetASCStatus(device);
    if (state >= NL_SLE_ASC_BUTT) {
        HILOGE("[ASCService], state error (%{public}d)", state);
        return;
    }

    // 根据状态查跳转表处理
    ASCDisconnectStateProcFunc func = disconnStateProcTable_[state];
    int ret = (this->*func)(device, state);

    if (ret != NL_NO_ERROR) {
        // 上报状态: 断连接,失败
        ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_SLE_ASC_RESULT_FAIL, ret);
    }

    return;
}

void ASCService::InitDisconnProcTable()
{
    disconnStateProcTable_ =  {
        /*     状态                             处理函数                   */
        {NL_SLE_ASC_DISABLED,          &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_DISABLING,         &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_ENABLED,           &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_ENABLING,          &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_CONNECTED,         &ASCService::DisconnProcAction},
        {NL_SLE_ASC_CONNECTING,        &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_DISCONNECTED,      &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_DISCONNECTING,     &ASCService::DisconnProcStatusError},
        {NL_SLE_ASC_CONFIGED,          &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_CONFIGURING,       &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_OPENED,            &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_OPENING,           &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_STARTED,           &ASCService::DisconnProcStopPlaying},
        {NL_SLE_ASC_STARTING,          &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_STOPPED,           &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_STOPPING,          &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RELEASED,          &ASCService::DisconnProcAction},
        {NL_SLE_ASC_RELEASING,         &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RECONFIG_STOPPING, &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RECONFIG_STOPPED,  &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RECONFIGED,        &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RECONFIGURING,     &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_CREATED,           &ASCService::DisconnProcAction},
        {NL_SLE_ASC_CREATING,          &ASCService::DisconnProcAction},
        {NL_SLE_ASC_CONFIG_SUBRATE_CHANGED,   &ASCService::DisconnProcSetFlag},
        {NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED, &ASCService::DisconnProcSetFlag},
    };

    HILOGD("[ASCService]InitDisconnProcTable OK");
}

void ASCService::InitMicStateObserver()
{
    HILOGD("[ASCService]Enter");
    ProfileMic *micService = static_cast<ProfileMic *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_MIC));
    NL_CHECK_RETURN(micService, "Can't find mic profile service");
    micService->RegisterMicStateObserver(micStateObserver_);
}

void ASCService::DeInitMicStateObserver()
{
    HILOGD("[ASCService]Enter");
    ProfileMic *micService = static_cast<ProfileMic *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_MIC));
    NL_CHECK_RETURN(micService, "Can't find mic profile service");
    micService->DeregisterMicStateObserver(micStateObserver_);
}

void ASCService::AscMicStateObserver::OnMicStateChanged(
    const RawAddress &device, uint8_t micState)
{
    HILOGD("[ASCService]Enter %{public}s micState %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), micState);
    DoInAscThread([device, micState]() {
        ASCService *ascService = ASCService::GetService();
        NL_CHECK_RETURN(ascService, "ascService is null");
        ascService->SleAudioDeviceActionChanged(NearlinkRawAddress(),
            static_cast<int>(UpdateOutputStackAction::ACTION_ROLE_TYPE_CHANGE));
        ascService->UpdateDeviceRole(device, micState);
    });
}

int ASCService::DisconnProcAction(const RawAddress &device, ASCState state)
{
    HILOGD("[ASCService]DisconnProcSetFlag %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), state);
    int ret = SleASC::Disconnect(device);
    return ret;
}

int ASCService::DisconnProcSetFlag(const RawAddress &device, ASCState state)
{
    HILOGD("[ASCService]DisconnProcSetFlag %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), state);
    // 设置需要disconnect标记
    SetNeedDisconnect(device, true);
    return NL_NO_ERROR;
}

int ASCService::DisconnProcStatusError(const RawAddress &device, ASCState state)
{
    HILOGW("[ASCService]Disconnect not need %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), state);
    return NL_NO_ERROR;
}

int ASCService::DisconnProcStopPlaying(const RawAddress &device, ASCState state)
{
    AudioStreamType streamType = GetProcessingStreamType(device);
    HILOGI("[ASCService]Disconnect StopPlaying %{public}s state %{public}d streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), state, streamType);
    // 有音频流的场景，先停止+释放流
    if (streamType != AUDIO_STREAM_NONE) {
        // 清除延迟释放标记
        CancelStopDelay(device);

        // 设置需要disconnect标记
        SetNeedDisconnect(device, true);
        StopPlayingExcute(device, streamType);
    }
    return NL_NO_ERROR;
}

int ASCService::DisconnProcSetStatus(const RawAddress &device, ASCState state)
{
    // 设置状态:disconnected
    SetASCStatus(device, NL_SLE_ASC_DISCONNECTED);

    // 上报状态: 断连接,成功
    ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
    return NL_NO_ERROR;
}

int ASCService::GetConnectState()
{
    HILOGD("[ASCService Service] GetConnectState Enter");
    return !(connectedDev_.IsEmpty());
}

bool ASCService::UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device)
{
    HILOGI("[ASCService Service] UpdateSleVirtualDevice Enter cmd: %{public}d", cmd);
    ASCMessage event(ASC_UPDATE_VIRTUAL_DEVICE_EVT, cmd);
    event.dev_ = device.GetAddress();
    PostEvent(event);
    return true;
}

void ASCService::ProcessUpdateVirtualDeviceEvent(const ASCMessage &event)
{
    HILOGI("[ASCService], event(%{public}d), cmd(%{public}d)", event.whatM, event.arg1M);
    int cmd = event.arg1M;
    RawAddress device(event.dev_);
    // 上报虚拟连接设备增删操作给音频框架
    NL_CHECK_RETURN(callback_, "[ASCService] The callback is nullptr.");

    switch (cmd) {
        case UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_ADD: {
            AudioStreamType streamType = GetProcessingStreamType(device);
            HILOGI("[ASCService]add Sle Virtual AudioDevic address: %{public}s, streamType: %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType);
            callback_->OnAddSleVirtualAudioDevice(NearlinkRawAddress(device), streamType);
            NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(device, ADD_VIRTUAL_DEVICE_SCENE, streamType,
                UPDATE_VOICE_STACK_REASON_INVALID);
            connectedVirtualDev_.Insert(device.GetAddress());
            break;
        }
        case UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_DELETE: {
            callback_->OnDeleteSleVirtualAudioDevice(NearlinkRawAddress(device));
            NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(device, DELETE_VIRTUAL_DEVICE_SCENE,
                AUDIO_STREAM_NONE, UPDATE_VOICE_STACK_REASON_INVALID);
            if (connectedVirtualDev_.Count(device.GetAddress()) > 0) {
                connectedVirtualDev_.Erase(device.GetAddress());
            }
            break;
        }
        default:
            HILOGI("[ASCService Service] Unknown cmd");
            break;
    }
}

int ASCService::RegisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback)
{
    HILOGI("RegisterApplication");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "Invalid params: callback must not be null.");
    DoInAscThread([this, callback] {
        callback_ = callback;
    });
    return NL_NO_ERROR;
}

int ASCService::DeregisterApplication()
{
    HILOGD("DeregisterApplication");
    DoInAscThread([this] {
        callback_ = nullptr;
    });
    return NL_NO_ERROR;
}

int ASCService::DeregisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback)
{
    HILOGD("DeregisterApplication");
    DeregisterApplication();
    return NL_NO_ERROR;
}

std::queue<AudioStreamType>& ASCService::GetStartBuff(const RawAddress& device)
{
    HILOGD("[ASCService Service] GetStartBuff Enter");
    return startBuff_[device.GetAddress()];
}

std::list<AudioStreamType>& ASCService::GetStartedStreamList(const RawAddress& device)
{
    HILOGD("[ASCService Service] GetStartedStreamList Enter");
    return startedStreamList[device.GetAddress()];
}

std::queue<AudioStreamType>& ASCService::GetStopBuff(const RawAddress& device)
{
    HILOGD("[ASCService Service] GetStopBuff Enter");
    return stopBuff_[device.GetAddress()];
}

// safe read-only access, returns nullptr when key not found
const std::queue<AudioStreamType>* ASCService::FindStartBuff(const RawAddress& device)
{
    auto it = startBuff_.find(device.GetAddress());
    return (it != startBuff_.end()) ? &(it->second) : nullptr;
}

const std::queue<AudioStreamType>* ASCService::FindStopBuff(const RawAddress& device)
{
    auto it = stopBuff_.find(device.GetAddress());
    return (it != stopBuff_.end()) ? &(it->second) : nullptr;
}

void ASCService::ClearStartBuff(const RawAddress& device)
{
    auto it = startBuff_.find(device.GetAddress());
    if (it != startBuff_.end()) {
        while (!it->second.empty()) {
            it->second.pop();
        }
        startBuff_.erase(it);
    }
}

void ASCService::ClearStopBuff(const RawAddress& device)
{
    auto it = stopBuff_.find(device.GetAddress());
    if (it != stopBuff_.end()) {
        while (!it->second.empty()) {
            it->second.pop();
        }
        stopBuff_.erase(it);
    }
}

bool ASCService::IsCoSetDeviceExist(const RawAddress& device, RawAddress& coSetDevice)
{
    CdsmService* csdm = CdsmService::GetService();
    if (csdm == nullptr) {
        return false;
    }

    bool isExist = false;
    connectedDev_.Iterate(
        [&device, &coSetDevice, &csdm, &isExist](const std::string key, const ASCConnectedDev value)-> void {
        const RawAddress& dev = value.dev;
        if (dev == device) {
            HILOGD("[ASCService] IsCoSetDeviceExist self %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
            return;
        }

        std::vector<RawAddress> devAddrList = {device, dev};
        bool ret = csdm->CdsmCheckIsSameCooperation(devAddrList);
        if (ret) {
            HILOGD("[ASCService] IsCoSetDeviceExist co dev %{public}s", GetEncryptAddr(dev.GetAddress()).c_str());
            coSetDevice = dev;
            isExist = true;
            return;
        }
    });

    return isExist;
}

void ASCService::GetAllReportAddr(std::map<std::string, RawAddress>& reportAddrMap)
{
    connectedDev_.Iterate(
        [this, &reportAddrMap](std::string key, ASCConnectedDev value)-> void {
        RawAddress addr = GetReportAddr(value.dev);
        reportAddrMap[addr.GetAddress()] = addr;
    });
}

void ASCService::GetAllVirtualAudioAddr(std::set<std::string>& virtualAddrSet)
{
    auto audioDeviceList = [&virtualAddrSet](const std::string& device) {
        virtualAddrSet.insert(device);
    };
    connectedVirtualDev_.ForEach(audioDeviceList);
}

int ASCService::GetCoSetNum()
{
    // 判断有多少个report地址
    std::map<std::string, RawAddress> reportAddrMap {};
    GetAllReportAddr(reportAddrMap);
    return reportAddrMap.size();
}

RawAddress ASCService::GetReportAddr(const RawAddress& device)
{
    RawAddress reportAddr {};
    CdsmService* cdsm = CdsmService::GetService();
    if (cdsm != nullptr) {
        if (cdsm->CdsmGetReportAddr(device, reportAddr)) {
            HILOGD("[ASCService]GetReportAddr %{public}s reportAddr %{public}s",
                GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(reportAddr.GetAddress()).c_str());
            return reportAddr;
        }
    }

    reportAddr = device;
    HILOGI("[ASCService]GetReportAddr %{public}s reportAddr %{public}s not in cdsm.",
        GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(reportAddr.GetAddress()).c_str());
    return reportAddr;
}

bool ASCService::IsNeedReportAudioDevice(const RawAddress& device, int cmd)
{
    bool isNeedReport = true;
    int connectedCnt = GetConnectedCnt(device);
    if (cmd == NL_SLE_ASC_CONN_CMD_CONN) {
        isNeedReport = (connectedCnt == 1);
    } else {
        isNeedReport = (connectedCnt == 0);
    }

    HILOGD("[ASCService]IsNeedReportAudioDevice %{public}s cmd %{public}d connectedCnt %{public}d "
        "isNeedReport %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), cmd, connectedCnt, isNeedReport);
    return isNeedReport;
}

/**
 * @brief 通知音频设备连接状态变化
 * @param isConnect true表示设备连接，false表示设备断开
 */
void ASCService::NotifyConnectAudioDevice(bool isConnect)
{
    if (isConnect) {
        if (connectAudioDevice_ < UINT8_MAX) {
            connectAudioDevice_++;
        }
    } else if (connectAudioDevice_ >= 1) {
        connectAudioDevice_--;
    }

    int32_t state = (connectAudioDevice_ > 0) ?
        static_cast<int32_t>(NL_SLE_ASC_STATE_CONNECTED) :
        static_cast<int32_t>(NL_SLE_ASC_STATE_NOT_CONNECTED);

    int32_t lastState = lastAudioConnectionState_.load(std::memory_order_acquire);
    HILOGI("[ASCService] isConnect %{public}d, state %{public}d, lastState %{public}d",
        isConnect, state, lastState);

    if (state != lastState) {
        if (lastAudioConnectionState_.compare_exchange_strong(lastState, state,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
            NearlinkHelper::NearlinkCommonEventHelper::PublishAudioConnectionStateEvent(state);
        }
    }
}

void ASCService::OnAddDeleteAudioDevice(const RawAddress& device, int cmd)
{
    NL_CHECK_RETURN_LOGD(IsNeedReportAudioDevice(device, cmd), "do not need report conn state");
    RawAddress reportAddr = GetReportAddr(device);
    // 上报设备增删状态给音频框架
    if (cmd == NL_SLE_ASC_CONN_CMD_CONN) {
        RegisterAudioPreferredOutPutDeviceChangeListener();
        uint32_t supportStreamType = AUDIO_STREAM_NONE;
        (void)GetSupportStreamType(NearlinkRawAddress(device), supportStreamType);
        HILOGD("[ASCService] reportAddr %{public}s supportStreamType %{public}d",
            GetEncryptAddr(reportAddr.GetAddress()).c_str(), supportStreamType);
        VcpService* vcpService = VcpService::GetService();
        NL_CHECK_RETURN(vcpService, "cant find vcp service");
        int mediaVolume = vcpService->GetDeviceMediaVolume(device);
        int callVolume = vcpService->GetDeviceCallVolume(device);

        if (callback_ != nullptr) {
            callback_->OnAddSleAudioDevice(NearlinkRawAddress(reportAddr), supportStreamType, mediaVolume, callVolume);
        }
        SendPlayOrPauseIfNeed(reportAddr);
        NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(reportAddr, ADD_DEVICE_SCENE, supportStreamType,
            UPDATE_VOICE_STACK_REASON_INVALID);
        NotifyConnectAudioDevice(true);
    } else {
        if (callback_ != nullptr) {
            callback_->OnDeleteSleAudioDevice(NearlinkRawAddress(reportAddr));
            playOrPauseDevice_.Erase(reportAddr.GetAddress());
        }
        NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(reportAddr, DELETE_DEVICE_SCENE, AUDIO_STREAM_NONE,
            UPDATE_VOICE_STACK_REASON_INVALID);
        NotifyConnectAudioDevice(false);
    }
}

void ASCService::OnConnectionStateChanged(const RawAddress& device, int cmd, int result, int reason)
{
    // 上报状态: 连接/断连接,成功/失败
    HILOGD("[ASCService]OnConnectionStateChanged %{public}s cmd %{public}d result %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), cmd, result);

    if (result != NL_NO_ERROR) {
        // 连接失败，状态机回退
        if (cmd == NL_SLE_ASC_CONN_CMD_CONN) {
            SetASCStatus(device, NL_SLE_ASC_DISCONNECTED);
        }
        playOrPauseDevice_.Erase(GetReportAddr(device).GetAddress());
    }

    // step1: 上报连接状态给profile连接管理
    int state;
    int oldState;
    if (cmd == NL_SLE_ASC_CONN_CMD_CONN) {
        state = (result == NL_SLE_ASC_RESULT_SUCC) ? NL_SLE_ASC_STATE_CONNECTED : NL_SLE_ASC_STATE_NOT_CONNECTED;
        oldState = NL_SLE_ASC_STATE_NOT_CONNECTED;
    } else {
        state = (result == NL_SLE_ASC_RESULT_SUCC) ? NL_SLE_ASC_STATE_DISCONNECTED : NL_SLE_ASC_STATE_CONNECTED;
        oldState = NL_SLE_ASC_STATE_CONNECTED;
    }
    NL_CHECK_RETURN(audioObserver_ != nullptr, "[ASCService] audioObserver_ is null");
    audioObserver_->OnConnectionStateChanged(device, state, oldState);
}

void ASCService::ReportConnectStateChanged(const RawAddress& device, int cmd, int result, int reason)
{
    // step1: 上报连接状态给profile连接管理
    OnConnectionStateChanged(device, cmd, result, reason);

    // step2: 上报设备增删给音频框架
    OnAddDeleteAudioDevice(device, cmd);

    // step3: 上报dft大数据打点维测
    ASCState state = (cmd == NL_SLE_ASC_CONN_CMD_CONN ? NL_SLE_ASC_CONNECTED : NL_SLE_ASC_DISCONNECTED);
    DftReportStreamASC(device.GetAddress(), state, result, reason);
}

int ASCService::GetConnectedCnt(const RawAddress& device)
{
    int connectedCnt = 0;
    CdsmService* cdsm = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsm, connectedCnt, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsm->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, connectedCnt, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        // 对于合作集设备，给组内所有成员（包括当前设备）发起播
        if (info.state_ == static_cast<uint8_t>(CdsmConnectState::CONNECTED) && IsConnected(info.addr_)) {
            ASCState state = GetASCStatus(info.addr_);
            if (IsConnected(state) || IsCreating(state) || IsCreated(state) || IsInStartProcess(state) ||
                IsStarted(state) || IsInStopProcess(state) || IsReleased(state)) {
                connectedCnt++;
            }
        }
    }

    return connectedCnt;
}

bool ASCService::IsCoSetDeviceStarted(const RawAddress& device, RawAddress& coSetDevice)
{
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        if (IsStarted(GetASCStatus(coSetDevice))) {
            return true;
        }
    }

    return false;
}

// ASC映射DSP版本
uint8_t ASCService::GetDspL2hcVersion(const AscQosmInfo& info)
{
    AscCodecIdKey codec = {
        .codecId = info.codecId,
        .companyId = info.companyId,
        .vendorId = info.vendorId,
        .version = info.version,
    };
    if (IsEqualCodec(codec, ASC_L2HC_CODEC) && codec.version == ASC_L2HC_VERSION_STANDARD) {
        return DSP_L2HC_STD_VERSION;
    } else if (IsEqualCodec(codec, ASC_L2HC_5_0_CODEC) && codec.version == ASC_L2HC_5_VERSION) {
        return DSP_L2HC_5_0_VERSION;
    } else if (IsEqualCodec(codec, ASC_L2HC_4_0_CODEC) && codec.version == ASC_L2HC_4_0_HISI_VERSION) {
        return DSP_L2HC_4_0_VERSION;
    } else if (IsEqualCodec(codec, ASC_L2HC_VOICE_CODEC) && codec.version == ASC_L2HC_VERSION_STANDARD) {
        return DSP_L2HC_VOICE_VERSION;
    }
    return DSP_L2HC_STD_VERSION;
}

void ASCService::UpdateASCToDSPInfo(const RawAddress& device, const AscQosmInfo& info, ASCToDSPInfo &ascToDspInfo)
{
    ascToDspInfo.encodeDspVersion = GetDspL2hcVersion(info);
    ascToDspInfo.encodeBitDepth = info.bitSamp;
    ascToDspInfo.encodeSampleRate = info.rate;
    ascToDspInfo.encodeBps = info.bps;
    ascToDspInfo.encodeFrame = info.frame;
    ascToDspInfo.sduInterval = info.sduInterval;
    ascToDspInfo.frameNumPerSdu = static_cast<uint8_t>((info.frame != 0) ? (info.sduInterval / info.frame) : 0);
    ascToDspInfo.twsNum = ASCUtils::GetCosetDeviceNum(device);
    ascToDspInfo.maxSdu = info.maxSdu;
    ascToDspInfo.buffNum = info.bufNum;
    ascToDspInfo.encodeCodecClan = ASCUtils::GetEncodeCodecClan(info.codecId);
    ascToDspInfo.encodeChannelNum = ASCUtils::GetEncodeSoundChannelNum();
    Qos cos = QosM::GetInstance().GetCOS(device);
    uint8_t pointType = GetPointType(device, cos);
    ascToDspInfo.downChannelNum = ASCUtils::GetCustomizeDownChannelNum(pointType, info.codecId);
    ascToDspInfo.decodeCodecClan = ASCUtils::GetDecodeCodecClan(device, ascToDspInfo, cos);
    ascToDspInfo.decodeChannelNum = ASCUtils::GetDecodeSoundChannelNum();
    ascToDspInfo.upChannelNum = ASCUtils::GetCustomizeUpChannelNum(pointType, cos);
    ascToDspInfo.ft = info.ft;
    ascToDspInfo.bn = info.bn;
    ascToDspInfo.decodeBitDepth = ASCUtils::GetDecodeBitDepth(ascToDspInfo, cos);
    ascToDspInfo.decodeSampleRate = ASCUtils::GetDecodeSampleRate(ascToDspInfo, cos);
}

/**
 * music sample: 2,24,96000,160,10,10,1,2,2877,16,2,0,0,0,0,0,24,96000,15,2
 * call voice sample: 4,16,32000,64,10,20,2,2,161,16,2,1,0,1,1,0,16,32000,3,1
 * dual record sample: 2,24,96000,96,10,20,2,2,246,16,2,0,0,2,1,0,16,48000,4,1
 */
std::string ASCService::ASCToDSPInfoToString(const ASCToDSPInfo& ascToDspInfo)
{
    std::stringstream ss;
    // 将每个成员变量转换成字符串并拼接
    ss << static_cast<int>(ascToDspInfo.encodeDspVersion) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeBitDepth) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeSampleRate) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeBps) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeFrame) << ",";
    ss << static_cast<int>(ascToDspInfo.sduInterval) << ",";
    ss << static_cast<int>(ascToDspInfo.frameNumPerSdu) << ",";
    ss << static_cast<int>(ascToDspInfo.twsNum) << ",";
    ss << static_cast<int>(ascToDspInfo.maxSdu) << ",";
    ss << static_cast<int>(ascToDspInfo.buffNum) << ",";
    ss << static_cast<int>(ascToDspInfo.downChannelNum) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeCodecClan) << ",";
    ss << static_cast<int>(ascToDspInfo.encodeChannelNum) << ",";
    ss << static_cast<int>(ascToDspInfo.upChannelNum) << ",";
    ss << static_cast<int>(ascToDspInfo.decodeCodecClan) << ",";
    ss << static_cast<int>(ascToDspInfo.decodeChannelNum) << ",";
    ss << static_cast<int>(ascToDspInfo.decodeBitDepth) << ",";
    ss << static_cast<int>(ascToDspInfo.decodeSampleRate) << ",";
    ss << static_cast<int>(ascToDspInfo.ft) << ",";
    ss << static_cast<int>(ascToDspInfo.bn);

    return ss.str();
}

std::string ASCService::GenerateCodecPara(const RawAddress& device, AudioStreamType streamType)
{
    AscQosmInfo info {};
    if (!GetStreamQosmInfo(device, info)) {
        HILOGD("[ASCService] NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return "";
    }

    // 没有重建同步链路不用更新参数
    if (!IsQosmInfoUpdated(device)) {
        HILOGI("[ASCService] NO Need %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return "";
    }
    ASCToDSPInfo ascToDspInfo;
    (void)memset_s(&ascToDspInfo, sizeof(ASCToDSPInfo), 0x0, sizeof(ASCToDSPInfo));
    UpdateASCToDSPInfo(device, info, ascToDspInfo);

    std::string codecParaStr = "0," + ASCToDSPInfoToString(ascToDspInfo);
    HILOGI("[ASCService] codecParaStr:%{public}s", codecParaStr.c_str());
    return codecParaStr;
}

bool ASCService::IsRolePrimary(const RawAddress& device)
{
    bool isRolePrimary = true;
    if (SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        TwsService* twsService = TwsService::GetService();
        if (twsService == nullptr) {
            return true;
        }

        uint8_t devRole = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
        twsService->TwsGetDeviceRole(device, devRole);
        isRolePrimary = (devRole == static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));

        HILOGI("[ASCService][Tws]IsRolePrimary %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            isRolePrimary);
    } else {
        MicService* micService = MicService::GetService();
        if (micService == nullptr) {
            return true;
        }
        isRolePrimary = micService->IsMicOpen(device);
        HILOGI("[ASCService][Common]IsRolePrimary %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            isRolePrimary);
    }
    return isRolePrimary;
}

void ASCService::AddCoSetChannelInfo(const RawAddress& coSetDevice, std::string& channelInfoStr)
{
    AscQosmInfo info {};
    if (!GetStreamQosmInfo(coSetDevice, info)) {
        HILOGD("[ASCService]AddCoSetChannelInfo NO Item %{public}s", GetEncryptAddr(coSetDevice.GetAddress()).c_str());
        return;
    }

    bool left = IsLeftEarDevice(coSetDevice);
    bool isPrimary = IsRolePrimary(coSetDevice);
    channelInfoStr = channelInfoStr + "," + std::to_string(info.connHandle) + "," +
        std::to_string(left) + "," + std::to_string(isPrimary) + "," + std::to_string(info.bufNum);
}

std::string ASCService::GenerateChannelInfo(const RawAddress& device, int cmd)
{
    HILOGD("[ASCService]GenerateChannelInfo");
    std::string channelInfoStr;
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        AscQosmInfo info {};
        if (!GetStreamQosmInfo(device, info)) {
            HILOGD("[ASCService]GenerateChannelInfo NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
            return "";
        }

        // 没有重建同步链路不用更新参数
        if (!IsQosmInfoUpdated(device)) {
            HILOGI("[ASCService]GenerateChannelInfo NO Need %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
            return "";
        }

        RawAddress coSetDevice {};
        uint8_t connectionCnt = IsCoSetDeviceStarted(device, coSetDevice) ? ASC_ICB_NUM_TWO : ASC_ICB_NUM_ONE;
        // 场景：媒体0 通话1
        uint8_t mcast = IsL2HCVoice(info.codecId) ? ASC_COMM_TYPE_MULTICAST : ASC_COMM_TYPE_UNICAST;

        bool left = IsLeftEarDevice(device);
        bool isPrimary = IsRolePrimary(device);
        channelInfoStr = "1," + std::to_string(mcast) + ",1," + std::to_string(info.gHandle) + "," +
            std::to_string(connectionCnt) + "," + std::to_string(info.connHandle) + "," + std::to_string(left) + "," +
            std::to_string(isPrimary) + "," + std::to_string(info.bufNum);

        if (connectionCnt == ASC_ICB_NUM_TWO) {
            AddCoSetChannelInfo(coSetDevice, channelInfoStr);
            if (IsQosmInfoUpdated(device)) {
                SetQosmInfoUpdateFlag(device, false);
            }
            if (IsQosmInfoUpdated(coSetDevice)) {
                SetQosmInfoUpdateFlag(coSetDevice, false);
            }
        } else {
            // 没有合作集，单耳上报完链路信息后就清除标记位
            RawAddress coSetDevice {};
            bool isCoSetDeviceExist = IsCoSetDeviceExist(device, coSetDevice);
            if (!isCoSetDeviceExist && IsQosmInfoUpdated(device)) {
                SetQosmInfoUpdateFlag(device, false);
            }
        }
    } else {
        AscQosmInfo info {};
        if (!GetStreamQosmInfo(device, info)) {
            HILOGD("[ASCService]GenerateChannelInfo NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
            return "1,0,0,0,0";
        }

        // 场景：媒体0 通话1
        uint8_t mcast = (info.codecId == ASC_CODEC_ID_L2HC_VOICE) ? ASC_COMM_TYPE_MULTICAST : ASC_COMM_TYPE_UNICAST;
        channelInfoStr = "1," + std::to_string(mcast) + ",0,0,0";
    }

    HILOGI("[ASCService]GenerateChannelInfo device=%{public}s, %{public}s",
        GET_ENCRYPT_ADDR(device), channelInfoStr.c_str());
    return channelInfoStr;
}

void ASCService::SetAudioDisconnInfo(const RawAddress& device, uint16_t connHandle)
{
    std::string disconnInfoStr = "3," + std::to_string(connHandle);
    HILOGI("[ASCService]SetAudioDisconnInfo device=%{public}s, %{public}s",
        GET_ENCRYPT_ADDR(device), disconnInfoStr.c_str());

    std::vector<std::pair<std::string, std::string>> pairsInfo;
    pairsInfo.push_back(std::make_pair("nearlink_audio_info", disconnInfoStr));
    OHOS::AudioStandard::AudioSystemClientEngineManager::GetInstance().SetExtraParameters("nearlink_extra", pairsInfo);
}

void ASCService::ClearStreamResult(const RawAddress& device)
{
    uint32_t num = streamResultMap_.erase(device.GetAddress());
    if (num == 0) {
        HILOGD("[ASCService]ClearStreamResult %{public}s NO Item", GetEncryptAddr(device.GetAddress()).c_str());
    }
}

bool ASCService::GetStreamResult(const RawAddress& device, AudioStreamType streamType, StreamResult& streamResult)
{
    // 按照设备存储
    std::map<std::string, std::map<AudioStreamType, StreamResult>>::const_iterator itDevice =
        streamResultMap_.find(device.GetAddress());
    if (itDevice == streamResultMap_.end()) {
        HILOGD("[ASCService]GetStreamResult %{public}s NO Item", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    // 按照流存储
    const std::map<AudioStreamType, StreamResult>& resultMap = itDevice->second;
    std::map<AudioStreamType, StreamResult>::const_iterator itStream = resultMap.find(streamType);
    if (itStream == resultMap.end()) {
        HILOGI("[ASCService]GetStreamResult %{public}s NO stream Item %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
        return false;
    }

    streamResult = itStream->second;
    HILOGI("[ASCService]GetStreamResult %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    return true;
}

void ASCService::CheckAndAddPlayingResultInfo(const RawAddress& device, AudioStreamType streamType)
{
    // 按照设备存储
    std::map<std::string, std::map<AudioStreamType, StreamResult>>::iterator itDevice =
        streamResultMap_.find(device.GetAddress());
    if (itDevice == streamResultMap_.end()) {
        // 插入
        StreamResult result {};
        std::map<AudioStreamType, StreamResult> mapTmp;
        mapTmp[streamType] = result;
        streamResultMap_[device.GetAddress()] = mapTmp;
        HILOGI("[ASCService]CheckAndAddPlayingResultInfo %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
    }

    // 按照流存储
    std::map<AudioStreamType, StreamResult>& resultMap = streamResultMap_[device.GetAddress()];
    std::map<AudioStreamType, StreamResult>::iterator itStream = resultMap.find(streamType);
    if (itStream == resultMap.end()) {
        // 插入
        StreamResult result {};
        resultMap[streamType] = result;
        HILOGI("[ASCService]CheckAndAddPlayingResultInfo %{public}s streamType %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    }

    return;
}

void ASCService::SetStartPlayingResult(const RawAddress& device, AudioStreamType streamType, bool isFinished,
    int result)
{
    HILOGI("[ASCService]SetStartPlayingResult %{public}s streamType %{public}d isFinished %{public}d "
        "result %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), streamType, isFinished, result);

    CheckAndAddPlayingResultInfo(device, streamType);

    std::map<AudioStreamType, StreamResult>& resultMap = streamResultMap_[device.GetAddress()];
    StreamResult& streamResult = resultMap[streamType];
    streamResult.isStartFinished = isFinished;
    streamResult.startPlayingResult = result;
}

void ASCService::SetStopPlayingResult(const RawAddress& device, AudioStreamType streamType, bool isFinished,
    int result)
{
    HILOGI("[ASCService]SetStopPlayingResult %{public}s streamType %{public}d isFinished %{public}d "
        "result %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), streamType, isFinished, result);

    CheckAndAddPlayingResultInfo(device, streamType);

    std::map<AudioStreamType, StreamResult>& resultMap = streamResultMap_[device.GetAddress()];
    StreamResult& streamResult = resultMap[streamType];
    streamResult.isStopFinished = isFinished;
    streamResult.stopPlayingResult = result;

    return;
}

bool ASCService::IsStartPlayingSucc(const StreamResult& streamResult)
{
    return streamResult.isStartFinished && (streamResult.startPlayingResult == NL_NO_ERROR);
}

bool ASCService::IsStartPlayingFail(const StreamResult& streamResult)
{
    return streamResult.isStartFinished && (streamResult.startPlayingResult != NL_NO_ERROR);
}

bool ASCService::IsStartPlayingDoing(const StreamResult& streamResult)
{
    return !streamResult.isStartFinished;
}

bool ASCService::IsStopPlayingSucc(const StreamResult& streamResult)
{
    return streamResult.isStopFinished && (streamResult.stopPlayingResult == NL_NO_ERROR);
}

bool ASCService::IsStopPlayingFail(const StreamResult& streamResult)
{
    return streamResult.isStopFinished && (streamResult.stopPlayingResult != NL_NO_ERROR);
}

bool ASCService::IsStopPlayingDoing(const StreamResult& streamResult)
{
    return !streamResult.isStopFinished;
}

bool ASCService::IsNeedReportAudioControlFailed(int cmd, StreamResult& resultCoSet, const RawAddress& coSetDevice,
    ASCState coSetState)
{
    bool isNeedReport = true;
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        // 如果合作集设备正常起播，或者还在起播中不上报
        bool isStartPlayingSuccOrDoing = (IsStartPlayingSucc(resultCoSet) ||
            (IsInStartProcess(coSetState) && IsStartPlayingDoing(resultCoSet)));
        if (isStartPlayingSuccOrDoing) {
            isNeedReport = false;
        } else if (IsStartPlayingFail(resultCoSet)) {
            // 如果合作集设备已失败，上报
            isNeedReport = true;
        } else {
            // 合作集设备处在其余状态，上报
            HILOGI("[ASCService]IsNeedReportAudioControlComplete %{public}s coSetState %{public}d "
                "isStartFinished %{public}d startResult %{public}d",
                GetEncryptAddr(coSetDevice.GetAddress()).c_str(), coSetState,
                resultCoSet.isStartFinished, resultCoSet.startPlayingResult);
            isNeedReport = true;
        }
    } else {
        // 停播时第一个成功的设备上报结果
        isNeedReport = IsStopPlayingSucc(resultCoSet) ? false : true;
        // 如果合作集设备正常停播，不上报
        bool isStopPlayingSuccOrDoing = (IsStopPlayingSucc(resultCoSet) ||
            (IsInStopProcess(coSetState) && IsStopPlayingDoing(resultCoSet)));
        if (isStopPlayingSuccOrDoing) {
            isNeedReport = false;
        } else if (IsStopPlayingFail(resultCoSet)) {
            // 如果合作集设备已失败，上报
            isNeedReport = true;
        } else {
            // 合作集设备处在其余状态，上报
            HILOGI("[ASCService]IsNeedReportAudioControlComplete %{public}s coSetState %{public}d "
                "isStopFinished %{public}d stopResult %{public}d",
                GetEncryptAddr(coSetDevice.GetAddress()).c_str(), coSetState,
                resultCoSet.isStopFinished, resultCoSet.stopPlayingResult);
            isNeedReport = true;
        }
    }
    return isNeedReport;
}

bool ASCService::IsNeedReportAudioControlComplete(const RawAddress& device, int cmd, AudioStreamType streamType,
    int result)
{
    bool isNeedReport = true;
    // 有合作集
    RawAddress coSetDevice {};
    bool isCoSetConnected = (IsCoSetDeviceExist(device, coSetDevice) && !IsDisconnected(GetASCStatus(coSetDevice)));
    if (!isCoSetConnected) {
        HILOGI("[ASCService]IsNeedReportAudioControlComplete %{public}s cmd %{public}d result %{public}d "
            "isNeedReport %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), cmd, result, isNeedReport);
        return isNeedReport;
    }

    ASCState coSetState = GetASCStatus(coSetDevice);
    StreamResult resultCoSet {};
    GetStreamResult(coSetDevice, streamType, resultCoSet);

    // 成功
    if (result == NL_NO_ERROR) {
        // 起播时第一个成功的设备上报结果
        if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
            isNeedReport = IsStartPlayingSucc(resultCoSet) ? false : true;
        } else {
            // 停播时第一个成功的设备上报结果
            isNeedReport = IsStopPlayingSucc(resultCoSet) ? false : true;
        }
    } else {
        // 失败
        // 起播失败时
        isNeedReport = IsNeedReportAudioControlFailed(cmd, resultCoSet, coSetDevice, coSetState);
    }

    HILOGI("[ASCService]IsNeedReportAudioControlComplete %{public}s cmd %{public}d result %{public}d "
        "coSetState %{public}d isNeedReport %{public}d isStartFinished %{public}d startResult %{public}d "
        "isStopFinished %{public}d stopResult %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), cmd,
        result, coSetState, isNeedReport, resultCoSet.isStartFinished, resultCoSet.startPlayingResult,
        resultCoSet.isStopFinished, resultCoSet.stopPlayingResult);

    return isNeedReport;
}

int ASCService::GetReportResultCode(int result, int reason)
{
    if (reason == ASC_REJECT_REASON_ONCE) {
        return static_cast<int>(NL_SLE_ASC_ERROR_REJECT_ONCE);
    }

    return result;
}

void ASCService::ReportAudioControlComplete(const RawAddress& device, AudioStreamType streamType, int cmd,
    int result, int reason)
{
    bool isSpatialConfiguring = IsSpatialConfiguring(device);
    // 上报状态: 打开音频流/关闭音频流,成功/失败
    HILOGI("[ASCService]OnAudioControlComplete %{public}s streamType %{public}d cmd %{public}d result %{public}d "
        "isSpatialConfiguring %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), streamType, cmd, result,
        isSpatialConfiguring);

    // 是否在拆同步链路
    bool isStopDoing = IsStopDoing(device);
    // 下发编解码参数、链路参数给DSP
    if (result == NL_NO_ERROR) {
        if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
            std::string codecPara = GenerateCodecPara(device, streamType);
            std::string channelInfo = GenerateChannelInfo(device, cmd);
            SetAudioParameter(cmd, codecPara, channelInfo);
        } else {
            // 拆同步链路后给DSP上报链路参数
            if (isStopDoing) {
                std::string channelInfo = GenerateChannelInfo(device, cmd);
                SetAudioParameter(cmd, "", channelInfo);
            }
        }
    }

    // 停止StartPlaying定时器
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        DisableStartPlayingTimer(device);
        // 清除单切双同步标记
        SetSyncFlag(device, false);
    } else {
        if (isStopDoing) {
            // 停止StopPlaying定时器
            DisableStopPlayingTimer(device);
            // 清除标记位
            SetStopDoingFlag(device, false);
        }
    }

    ReportAudioControlCompleteSub(device, streamType, cmd, result, reason);

    // 如果是开/关空间音频改配同步链路，不需要上报结果给音频框架
    if (isSpatialConfiguring) {
        SetSpatialConfiguringFlag(device, false);
        return;
    }

    // 记录结果
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        SetStartPlayingResult(device, streamType, true, result);
    } else {
        SetStopPlayingResult(device, streamType, true, result);
    }

    RawAddress reportAddr = GetReportAddr(device);
    if (IsNeedReportAudioControlComplete(device, cmd, streamType, result)) {
        NearlinkASCAudioControlResult controlResult {};
        controlResult.SetStreamType(streamType);
        controlResult.SetCmd(cmd);
        controlResult.SetResult(GetReportResultCode(result, reason));
        if (callback_ != nullptr) {
            callback_->OnAudioControlComplete(NearlinkRawAddress(reportAddr), controlResult);
        }
    }
}

void ASCService::ReportAudioControlCompleteSub(const RawAddress& device, AudioStreamType streamType, int cmd,
    int result, int reason)
{
    if (result != NL_NO_ERROR) {
        // 打开音频流失败，状态机回退
        if ((cmd == NL_SLE_ASC_CONTROL_CMD_START) && IsInStartProcess(GetASCStatus(device))) {
            HILOGI("[ASCService] start fail cbk CREATED state dev %{public}s",
                GetEncryptAddr(device.GetAddress()).c_str());
            SetASCStatus(device, NL_SLE_ASC_CREATED);

            // 起播失败qos删除
            QosM& qosM = QosM::GetInstance();
            qosM.ClearQosIfStartFailed(device, GetStreamQos(device, streamType));

            // 取出缓存任务处理
            ProcBuff(device, NL_SLE_ASC_CREATED);
        }

        // 关闭音频流失败，状态机回退
        if ((cmd == NL_SLE_ASC_CONTROL_CMD_STOP) && IsInStopProcess(GetASCStatus(device))) {
            SetASCStatus(device, NL_SLE_ASC_CREATED);
        }
    }
    // 音频流启停成功/失败，上报DFT
    DftReportStreamASC(device.GetAddress(), GetASCStatus(device), result, reason);
}

void ASCService::SetAutorateParameter(const AscBitrateChange& ascBitrate)
{
    std::stringstream ss;
    ss << "2,";
    ss << static_cast<int>(ascBitrate.downBitrate) << ",";
    ss << static_cast<int>(ascBitrate.groupId) << ",";
    ss << static_cast<int>(ascBitrate.labelId) << ",";
    ss << static_cast<int>(ascBitrate.qosIndex) << ",";
    ss << static_cast<int>(ascBitrate.qosLevel) << ",";
    ss << static_cast<int>(ascBitrate.dutyCycle) << ",";
    ss << static_cast<int>(ascBitrate.availableBitratesCnt);
    for (uint8_t i = 0; i < ascBitrate.availableBitratesCnt && i < ASC_AVAILABLE_BITRATE_MAX; i++) {
        ss << "," << static_cast<int>(ascBitrate.availableBitrates[i]);
    }
    std::string autoratePara = ss.str();
    HILOGI("[SleAudioStream] SetAutorateParameter autorate %{public}s", autoratePara.c_str());
    std::vector<std::pair<std::string, std::string>> pairsAutorate;
    pairsAutorate.push_back(std::make_pair("nearlink_audio_info", autoratePara));
    OHOS::AudioStandard::AudioSystemClientEngineManager::GetInstance().SetExtraParameters("nearlink_extra",
        pairsAutorate);
}

void ASCService::SetAudioParameter(int cmd, const std::string& codecPara, const std::string& channelInfo)
{
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        // 设置codec编解码参数
        if (codecPara != "") {
            HILOGI("[ASCService] SetAudioParameter codec");
            std::vector<std::pair<std::string, std::string>> pairsCodec;
            pairsCodec.push_back(std::make_pair("nearlink_audio_info", codecPara));
            OHOS::AudioStandard::AudioSystemClientEngineManager::GetInstance().SetExtraParameters("nearlink_extra",
                pairsCodec);
        }

        // 通知同步链路建立
        if (channelInfo != "") {
            HILOGI("[ASCService] SetAudioParameter channel info");
            std::vector<std::pair<std::string, std::string>> pairsChan;
            pairsChan.push_back(std::make_pair("nearlink_audio_info", channelInfo));
            OHOS::AudioStandard::AudioSystemClientEngineManager::GetInstance().SetExtraParameters("nearlink_extra",
                pairsChan);
        }
    } else if (cmd == NL_SLE_ASC_CONTROL_CMD_STOP) {
        // 通知同步链路断开
        if (channelInfo != "") {
            HILOGI("[ASCService] SetAudioParameter channel info");
            std::vector<std::pair<std::string, std::string>> pairsChan;
            pairsChan.push_back(std::make_pair("nearlink_audio_info", channelInfo));
            OHOS::AudioStandard::AudioSystemClientEngineManager::GetInstance().SetExtraParameters("nearlink_extra",
                pairsChan);
        }
    } else {
        HILOGE("cmd Error %{public}d", cmd);
    }
}

int ASCService::GetAudioProperty(const RawAddress& device)
{
    HILOGD("[ASCService]GetAudioProperty in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    int ret = SleASC::GetAudioProperty(device);
    if (ret != NL_NO_ERROR) {
        HILOGD("[ASCService]GetAudioProperty ret %{public}d", ret);
        return ret;
    }

    // 状态：连接中
    SetASCStatus(device, NL_SLE_ASC_CONNECTING);
    HILOGD("[ASCService]GetAudioProperty Start");
    return NL_NO_ERROR;
}

bool ASCService::FindBps(const AscProp& prop, uint8_t codecId, uint64_t& bps)
{
    uint8_t codecNum = std::min(ASC_CODEC_NUM_MAX, prop.ability.codecNum);
    for (uint8_t i = 0; i < codecNum; i++) {
        const AscCodecConfig* codec = &(prop.ability.codec[i]);
        if (codec->codecId == codecId) {
            bps = codec->param.l2hcParam.bps;
            return true;
        }
    }

    return false;
}

uint64_t ASCService::GetSupportBps(const RawAddress &device, NLSTK_ActmPointType_E pointType, uint8_t codecId)
{
    // 从保存的音频属性读取
    const std::vector<AscProp>& properties = GetPropertyList(device);
    for (const AscProp& prop : properties) {
        uint64_t bps = 0;
        bool isFound = FindBps(prop, codecId, bps);
        if (isFound) {
            return bps;
        }
    }

    HILOGE("[ASCService]GetSupportBps NO Item %{public}s pointType %{public}d codecId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), pointType, codecId);
    return NL_NO_ERROR;
}

uint8_t ASCService::GetBpsBitIndex(const RawAddress &device, uint64_t resultBps, uint8_t codecId, Qos qos)
{
    // 测试用码率参数
    int param = OHOS::system::GetIntParameter("persist.config.nearlink.audiobps", -1);
    if ((param >= 0) && IsL2HC(codecId) && ((resultBps & (1 << param)) != 0)) {
        HILOGI("[ASCService]GetBpsBitIndex param %{public}d resultBps %{public}d codecId %{public}d",
            param, resultBps, codecId);

        return param;
    }

    // 单切双场景同步码率，查看合作集设备状态，进行同步
    RawAddress coSetDevice;
    if (IsSync(device) && IsCoSetDeviceExist(device, coSetDevice)) {
        ASCState coStatus = GetASCStatus(coSetDevice);
        if (IsStarted(coStatus)) {
            uint16_t autoRateBps = 0;
            if (GetAutoRateBps(coSetDevice, autoRateBps)) {
                SetAutoRateBps(device, autoRateBps);
                return ASCUtils::GetBpsBitIndexByBps(autoRateBps);
            }
        }
    }

    if (IsL2HCVoice(codecId)) {
        // 通话固定64k bit5：64kbps (必选参数)
        return L2HC_VOICE_BPS_S_64_BIT;
    }

    if (IsL2HC(codecId) && IsSameAsFormer(formerPlayRecord_, device, qos, codecId) &&
            IsStartAtLowBps(formerPlayRecord_, qos)) {
        // 保护时间内，若上一次停流时为160k，本次起播按160k，若为96k/48k，本次按96k起播 bit4：单声道96kbps
        if (formerPlayRecord_.autoRateBpsBit <= L2HC_BPS_S_96_BIT){
            return L2HC_BPS_S_96_BIT;
        } else {
            return L2HC_BPS_S_160_BIT;
        }
    }

    if (IsL2HC(codecId) && ((resultBps & L2HC_BPS_S_320) != 0)) {
        // 媒体固定320k bit8：单声道320kbps
        return L2HC_BPS_S_320_BIT;
    }

    if (IsL2HC(codecId) && ((resultBps & L2HC_BPS_S_160) != 0)) {
        // 媒体固定160k bit6：单声道160kbps
        return L2HC_BPS_S_160_BIT;
    }

    // 非通话取位图的最高bit索引
    uint8_t startIndex = BPS_DOUBLE_HIGH_BIT;
    uint64_t tmp = 0x01LL << startIndex;

    for (int i = startIndex; i >= 0; i--) {
        if (resultBps & tmp) {
            return i;
        }
        tmp = tmp >> 1;
    }

    HILOGE("[ASCService]GetBpsBitIndex NO invalid ItemresultBps %{public}d codecId %{public}d", resultBps, codecId);
    return 0;
}

bool ASCService::IsVidoeExist(const RawAddress &device, AudioStreamType streamType)
{
    if (streamType == AUDIO_STREAM_VIDEO) {
        return true;
    }

    std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);
    for (const AudioStreamType& stream : startedStreamList) {
        if (stream == AUDIO_STREAM_VIDEO) {
            return true;
        }
    }

    return false;
}

uint64_t ASCService::ChooseBps(const RawAddress &device, JudgeL2HCParamStru paramIn, uint64_t& bpsRange)
{
    // 耳机侧支持的编码速率
    uint64_t supportBps = GetSupportBps(device, paramIn.pointType, paramIn.codecId);

    // qos也要协商
    // 协议定义的Qos支持码率
    uint64_t defineBps = GetDefineBps(device, paramIn);
    bpsRange = (defineBps & supportBps);

    // 起播编码速率索引，根据位图获取对应速率bit的索引
    uint64_t resultBpsIndex = GetBpsBitIndex(device, bpsRange, paramIn.codecId, paramIn.qos);

    // 记录本次起播的信息，供下次判断是否需要低码率起播
    SetPlayRecord(GetReportAddr(device), paramIn.qos, paramIn.codecId, resultBpsIndex);

    HILOGI("[ASCService]ChooseBps %{public}s qos%{public}d define%{public}d support%{public}d resultBpsIndex%{public}d"
        " pointType%{public}d bpsRange%{public}ld", GetEncryptAddr(device.GetAddress()).c_str(), paramIn.qos,
        defineBps, supportBps, resultBpsIndex, paramIn.pointType, bpsRange);
    return resultBpsIndex;
}

uint64_t ASCService::GetVoiceCallDefineBps(const RawAddress &device, NLSTK_ActmPointType_E pointType)
{
    uint64_t defineBps = (pointType == NLSTK_ACTM_SOURCE_POINT) ? BPS_QOS3_UP : BPS_QOS3_DOWN;
    if (GetLongRangeVoiceCallAbility(device)) {
        defineBps = (pointType == NLSTK_ACTM_SOURCE_POINT) ? BPS_QOS3_AUTORATE_UP : BPS_QOS3_AUTORATE_DOWN;
    }
    return defineBps;
}

uint64_t ASCService::GetDefineBps(const RawAddress &device, const JudgeL2HCParamStru &paramIn)
{
    uint64_t defineBps = 0;
    bool isVendorDevice = SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device);

    switch (paramIn.qos) {
        case NL_SLE_QOS_1:
            // 同时连接两副星闪耳机，不进2.3M/4.6M
            defineBps = (GetCoSetNum() > 1) ? BPS_QOS1_NO_2300 : BPS_QOS1;
            // 非vendor设备，不支持4.6Mbps
            if (defineBps == BPS_QOS1 && !isVendorDevice) {
                defineBps = BPS_QOS1_NO_4600;
            }
            break;
        case NL_SLE_QOS_2:
            defineBps = BPS_QOS2;
            break;
        case NL_SLE_QOS_3:
            defineBps = GetVoiceCallDefineBps(device, paramIn.pointType);
            break;
        case NL_SLE_QOS_4:
            defineBps = BPS_QOS4;
            break;
        case NL_SLE_QOS_5:
            defineBps = BPS_QOS5;
            break;
        case NL_SLE_QOS_6:
            defineBps = BPS_QOS6;
            break;
        case NL_SLE_QOS_7:
            defineBps = IsVidoeExist(device, paramIn.streamType) ? BPS_QOS7_VIDEO : BPS_QOS7;
            break;
        case NL_SLE_QOS_8:
            // 同qos1：同时连接两副星闪耳机，不进2.3M/4.6M
            defineBps = (GetCoSetNum() > 1) ? BPS_QOS1_NO_2300 : BPS_QOS1;
            // 非vendor设备，不支持4.6Mbps
            if (defineBps == BPS_QOS1 && !isVendorDevice) {
                defineBps = BPS_QOS1_NO_4600;
            }
            break;
        case NL_SLE_QOS_9:
            defineBps = BPS_QOS2;
            break;
        case NL_SLE_QOS_10:
            defineBps = (GetCoSetNum() > 1) ? BPS_QOS1_NO_2300 : BPS_QOS1;
            break;
        default:
            defineBps = 0;
            HILOGE("[ASCService]ChooseBps Qos error %{public}s qos %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), paramIn.qos);
            break;
    }
    HILOGI("[ASCService]GetDefineBps device: %{public}s, isVendor: %{public}d, qos: %{public}d, bps: %{public}ld",
        GetEncryptAddr(device.GetAddress()).c_str(), isVendorDevice, paramIn.qos, defineBps);
    return defineBps;
}

bool ASCService::IsEqualCodec(const AscCodecIdKey &codecA, const AscCodecIdKey &codecB)
{
    return (codecA.codecId == codecB.codecId) && (codecA.companyId == codecB.companyId) &&
        (codecA.vendorId == codecB.vendorId);
}

// 处理编解码器版本的向前兼容性
void ASCService::ProcessCodecFCVersion(AscCodecIdKey &codec)
{
    if (IsEqualCodec(codec, ASC_L2HC_5_0_CODEC)) {
        codec.version = std::min(codec.version, ASC_L2HC_5_0_CODEC.version);
    } else if (IsEqualCodec(codec, ASC_L2HC_4_0_CODEC)) {
        codec.version = std::min(codec.version, ASC_L2HC_4_0_CODEC.version);
    } else if (IsEqualCodec(codec, ASC_L2HC_CODEC)) {
        codec.version = std::min(codec.version, ASC_L2HC_CODEC.version);
    }
}

AscCodecIdKey ASCService::SelectMediaCodecIdPolicy(const AscProp& prop)
{
    // 支持默认标准l2hc
    AscCodecIdKey codecRes = ASC_L2HC_CODEC;
    uint8_t codecNum = std::min(ASC_CODEC_NUM_MAX, prop.ability.codecNum);
    for (uint8_t i = 0; i < codecNum; i++) {
        const AscCodecConfig* codec = &(prop.ability.codec[i]);
        if (codec->codecId == ASC_CODEC_ID_L2HC_VOICE) {
            continue;
        }
        codecRes.codecId = codec->codecId;
        codecRes.companyId = codec->companyId;
        codecRes.vendorId = codec->vendorId;
        codecRes.version = codec->param.l2hcParam.version;
        HILOGI("[ASCService] codecId%{public}d companyId%{public}d vendorId%{public}d version%{public}d",
            codecRes.codecId, codecRes.companyId, codecRes.vendorId, codecRes.version);
        if (IsEqualCodec(codecRes, ASC_L2HC_5_0_CODEC)) {
            // 优选L2HC_5_0
            break;
        } else if (IsEqualCodec(codecRes, ASC_L2HC_4_0_CODEC)) {
            // 次选L2HC_4_0
            break;
        }
    }
    return codecRes;
}

bool ASCService::IsL2HCVoice(uint8_t codecId)
{
    return (codecId == ASC_CODEC_ID_L2HC_VOICE);
}

bool ASCService::IsL2HC(uint8_t codecId)
{
    return ((codecId == ASC_CODEC_ID_L2HC_PRI) || (codecId == ASC_CODEC_ID_L2HC));
}

bool ASCService::IsSameAsFormer(const ASCPlayRecord &formerPlayRecord, const RawAddress &device,
    Qos qos, uint8_t codecId)
{
    return (GetReportAddr(device) == formerPlayRecord_.addr &&
            qos == formerPlayRecord_.qos &&
            codecId == formerPlayRecord_.codecId);
}

bool ASCService::IsStartAtLowBps(const ASCPlayRecord &formerPlayRecord, Qos qos)
{
    time_t timeStamp = time(nullptr);
    bool musicQos = (qos == NL_SLE_QOS_1 || qos == NL_SLE_QOS_4 || qos == NL_SLE_QOS_7 || qos == NL_SLE_QOS_8 ||
                    qos == NL_SLE_QOS_10);
    return (musicQos && (timeStamp >= formerPlayRecord_.timeStamp) &&
            (timeStamp <= formerPlayRecord_.timeStamp + ASC_FORMER_RECORD_VALID_SECOND) &&
            (formerPlayRecord_.autoRateBpsBit <= L2HC_BPS_S_160_BIT));
}

AscCodecIdKey ASCService::SelectPeerCodecIdInMedia(const RawAddress &device, uint8_t comm)
{
    // 支持默认标准l2hc
    AscCodecIdKey codec = ASC_L2HC_CODEC;
    // 从保存的音频属性读取
    const std::vector<AscProp>& properties = GetPropertyList(device);
    for (const AscProp& prop : properties) {
        if ((prop.ability.comm & (1 << comm)) != 0) {
            codec = SelectMediaCodecIdPolicy(prop);
            ProcessCodecFCVersion(codec);
            break;
        }
    }
    HILOGI("[ASCService]dev:%{public}s,comm:%{public}d,codecId:%{public}d,version:%{public}d, companyId:%{public}d,"
        "vendorId:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        comm, codec.codecId, codec.version, codec.companyId, codec.vendorId);
    return codec;
}

AscCodecIdKey ASCService::SelectPeerCodecId(const RawAddress &device, Qos qos, uint8_t comm)
{
    AscCodecIdKey codec = ASC_L2HC_CODEC;
    bool ret = OHOS::system::GetBoolParameter("const.nearlink.l2hcstandard.test", false);
    // 测试使用
    if (ret) {
        HILOGI("[ASCService] enter l2hcstandard");
        codec = ASC_L2HC_CODEC;
        return codec;
    }
    // 实际业务
    if (IsMediaService(qos)) {
        codec = SelectPeerCodecIdInMedia(device, comm);
        HILOGI("[ASCService] codecId:%{public}d, version:%{public}d, companyId:%{public}d, vendorId:%{public}d",
            codec.codecId, codec.version, codec.companyId, codec.vendorId);
    } else if (IsVoiceCallService(qos)) {
        codec = ASC_L2HC_VOICE_CODEC;
    } else {
        HILOGE("[ASCService] qos illegeal %{public}d", qos);
    }

    return codec;
}

void ASCService::GetCommType(Qos qos, uint8_t &commType)
{
    if ((qos == NL_SLE_QOS_3) || (qos == NL_SLE_QOS_6)) {
        commType = ASC_COMM_TYPE_MULTICAST;
    } else {
        commType = ASC_COMM_TYPE_UNICAST;
    }
}

uint16_t ASCService::GetLocalCodecSampleRateCap(AscCodecIdKey codec)
{
    uint16_t sampleRate = 0;
    if (IsEqualCodec(codec, ASC_L2HC_5_0_CODEC)) {
        sampleRate = ASC_L2HC_5_0_SAMPLE_RATE_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_4_0_CODEC)) {
        sampleRate = ASC_L2HC_4_0_SAMPLE_RATE_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_CODEC)) {
        sampleRate = ASC_L2HC_SAMPLE_RATE_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_VOICE_CODEC)) {
        sampleRate = ASC_L2HC_VOICE_SAMPLE_RATE_CAP;
    } else {
        HILOGE("[ASCService] not find local match sampleRate");
    }
    return sampleRate;
}

uint8_t ASCService::GetLocalCodecBitDepthCap(AscCodecIdKey codec)
{
    uint8_t bitDepth = 0;
    if (IsEqualCodec(codec, ASC_L2HC_5_0_CODEC)) {
        bitDepth = ASC_L2HC_5_0_BIT_DEPTH_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_4_0_CODEC)) {
        bitDepth = ASC_L2HC_4_0_BIT_DEPTH_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_CODEC)) {
        bitDepth = ASC_L2HC_BIT_DEPTH_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_VOICE_CODEC)) {
        bitDepth = ASC_L2HC_VOICE_BIT_DEPTH_CAP;
    } else {
        HILOGE("[ASCService] not find local match bitDepth");
    }
    return bitDepth;
}

uint8_t ASCService::GetLocalCodecChannelCap(AscCodecIdKey codec)
{
    uint8_t channel = 0;
    if (IsEqualCodec(codec, ASC_L2HC_5_0_CODEC)) {
        channel = ASC_L2HC_5_0_CHANNEL_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_4_0_CODEC)) {
        channel = ASC_L2HC_4_0_CHANNEL_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_CODEC)) {
        channel = ASC_L2HC_CHANNEL_CAP;
    } else if (IsEqualCodec(codec, ASC_L2HC_VOICE_CODEC)) {
        channel = ASC_L2HC_VOICE_CHANNEL_CAP;
    } else {
        HILOGE("[ASCService] not find local match channel");
    }
    return channel;
}

uint16_t ASCService::GetPeerCodecSampleRateCap(const RawAddress &device, AscCodecIdKey inCodec)
{
    uint16_t sampleRate = 0;
    // 从保存的音频属性读取
    const std::vector<AscProp>& properties = GetPropertyList(device);
    for (const AscProp& prop : properties) {
        uint8_t codecNum = std::min(ASC_CODEC_NUM_MAX, prop.ability.codecNum);
        for (uint8_t i = 0; i < codecNum; i++) {
            const AscCodecConfig* codec = &(prop.ability.codec[i]);
            AscCodecIdKey peerCodec = {
                .codecId = codec->codecId,
                .companyId = codec->companyId,
                .vendorId = codec->vendorId,
            };
            if (IsEqualCodec(peerCodec, inCodec)) {
                sampleRate = static_cast<uint16_t>(codec->param.l2hcParam.rate);
                HILOGI("[ASCService] match peer sampleRate: %{public}d", sampleRate);
                return sampleRate;
            }
        }
    }
    HILOGE("[ASCService] not find peer match sampleRate: %{public}d", sampleRate);
    return sampleRate;
}

uint8_t ASCService::GetPeerCodecBitDepthCap(const RawAddress &device, AscCodecIdKey inCodec)
{
    uint8_t bitDepth = 0;
    // 从保存的音频属性读取
    const std::vector<AscProp>& properties = GetPropertyList(device);
    for (const AscProp& prop : properties) {
        uint8_t codecNum = std::min(ASC_CODEC_NUM_MAX, prop.ability.codecNum);
        for (uint8_t i = 0; i < codecNum; i++) {
            const AscCodecConfig* codec = &(prop.ability.codec[i]);
            AscCodecIdKey peerCodec = {
                .codecId = codec->codecId,
                .companyId = codec->companyId,
                .vendorId = codec->vendorId,
            };
            if (IsEqualCodec(peerCodec, inCodec)) {
                bitDepth = codec->param.l2hcParam.depth;
                HILOGI("[ASCService] match peer bitDepth: %{public}d", bitDepth);
                return bitDepth;
            }
        }
    }
    HILOGE("[ASCService] not find peer match bitDepth: %{public}d", bitDepth);
    return bitDepth;
}

uint8_t ASCService::GetPeerCodecChannelCap(const RawAddress &device, AscCodecIdKey inCodec)
{
    uint8_t channel = 0;
    // 从保存的音频属性读取
    const std::vector<AscProp>& properties = GetPropertyList(device);
    for (const AscProp& prop : properties) {
        uint8_t codecNum = std::min(ASC_CODEC_NUM_MAX, prop.ability.codecNum);
        for (uint8_t i = 0; i < codecNum; i++) {
            const AscCodecConfig* codec = &(prop.ability.codec[i]);
            AscCodecIdKey peerCodec = {
                .codecId = codec->codecId,
                .companyId = codec->companyId,
                .vendorId = codec->vendorId,
            };
            if (IsEqualCodec(peerCodec, inCodec)) {
                channel = codec->param.l2hcParam.channel;
                HILOGI("[ASCService] match peer channel: %{public}d", channel);
                return channel;
            }
        }
    }
    HILOGE("[ASCService] not find peer match channel: %{public}d", channel);
    return channel;
}

uint8_t ASCService::SelectCodecSampleRatePolicy(const RawAddress &device, AscCodecIdKey codec,
    JudgeL2HCParamStru paramIn)
{
    Qos qos = paramIn.qos;
    // 测试用码率参数
    int param = OHOS::system::GetIntParameter("persist.config.nearlink.audiosample", -1);
    if ((param >= 0) && IsMediaService(qos)) {
        HILOGI("[ASCService] param %{public}d qos %{public}d", param, qos);
        return param;
    }
    // 对端采样率能力
    uint16_t peerCap = GetPeerCodecSampleRateCap(device, codec);
    // 本端采样率能力
    uint16_t localCap = GetLocalCodecSampleRateCap(codec);
    // 取交集
    uint16_t sampleRate = (peerCap & localCap);
    HILOGI("[ASCService] %{public}s qos%{public}d peerCap%{public}d localCap%{public}d samRate%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), qos, peerCap, localCap, sampleRate);
    // 双耳录音下行96khz
    if ((qos == NL_SLE_QOS_5) && ((sampleRate & (1 << ASC_SAMPLE_RATE_96KHZ)) != 0)) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, DUAL_REC_START, DUAL_REC_NOTIFY_PEER);
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_96KHZ);
    }
    // codec_1 L2HC_VOICE 采样率（优选32，必选16） 帧4通话仅支持16
    if (IsVoiceCallService(qos)) {
        if (GetLongRangeVoiceCallAbility(device) && (sampleRate & (1 << ASC_SAMPLE_RATE_16KHZ)) != 0) {
            return static_cast<uint8_t>(ASC_SAMPLE_RATE_16KHZ);
        }
        if ((sampleRate & (1 << ASC_SAMPLE_RATE_32KHZ)) != 0) {
            return static_cast<uint8_t>(ASC_SAMPLE_RATE_32KHZ);
        }
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_16KHZ);
    }
    // 游戏和空间音频场景/移动全景音 采样率 48KHZ
    if (((qos == NL_SLE_QOS_2) || (qos == NL_SLE_QOS_4) || (qos == NL_SLE_QOS_8) ||
        qos == NL_SLE_QOS_10) && ((sampleRate & (1 << ASC_SAMPLE_RATE_48KHZ)) != 0)) {
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_48KHZ);
    }
    // codec_2 L2HC
    // 音乐（优选96，必选48）
    if (sampleRate & (1 << ASC_SAMPLE_RATE_96KHZ)) {
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_96KHZ);
    }
    if (sampleRate & (1 << ASC_SAMPLE_RATE_48KHZ)) {
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_48KHZ);
    }
    return static_cast<uint8_t>(ASC_SAMPLE_RATE_48KHZ);
}

uint8_t ASCService::SelectCodecBitDepthPolicy(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn)
{
    // 测试用码率参数
    int param = OHOS::system::GetIntParameter("persist.config.nearlink.audiodepth", -1);
    if ((param >= 0) && IsMediaService(paramIn.qos)) {
        HILOGI("[ASCService] param %{public}d qos %{public}d", param, paramIn.qos);
        return param;
    }

    // codec_1 L2HC_VOICE 限定采样深度 16bit(协议规定必选支持能力)
    if (IsVoiceCallService(paramIn.qos)) {
        return ASC_SAMPLE_DEPTH_16BIT;
    }
    uint8_t peerDepth = GetPeerCodecBitDepthCap(device, codec);
    uint8_t localDepth = GetLocalCodecBitDepthCap(codec);
    // 取交集
    uint8_t depth = (peerDepth & localDepth);

    HILOGI("[ASCService] %{public}s, peerDepth:%{public}d, localDepth:%{public}d, depth:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), peerDepth, localDepth, depth);

    // 优选24bit
    if ((depth & (1 << ASC_SAMPLE_DEPTH_24BIT)) != 0) {
        return ASC_SAMPLE_DEPTH_24BIT;
    }

    // 24bit不支持就选16bit(协议规定必选支持能力)
    return ASC_SAMPLE_DEPTH_16BIT;
}

uint8_t ASCService::SelectCodecChannelPolicy(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn)
{
    uint8_t peerChannel = GetPeerCodecChannelCap(device, codec);
    uint8_t localChannel = GetLocalCodecChannelCap(codec);
    uint8_t channel = (peerChannel & localChannel);
    HILOGI("[ASCService] dev%{public}s, peerCh:%{public}d, localCh:%{public}d, ch:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), peerChannel, localChannel, channel);
    // 当前默认优先单声道
    if ((channel & (ASC_CHANNEL_SINGLE)) != 0) {
        return static_cast<uint8_t>(ASC_CHANNEL_SINGLE);
    }
    return static_cast<uint8_t>(ASC_CHANNEL_DOUBLE);
}

uint8_t ASCService::SelectCodecSampleRatePolicyInDualRec(AscCodecIdKey codec, uint16_t peerSampleRate)
{
    // 对端采样率能力
    uint16_t peerCap = peerSampleRate;
    // 本端采样率能力
    uint16_t localCap = GetLocalCodecSampleRateCap(codec);
    // 取交集
    uint16_t sampleRate = (peerCap & localCap);
    // 双耳录音上行48khz
    if (((sampleRate & (1 << ASC_SAMPLE_RATE_48KHZ)) != 0)) {
        return static_cast<uint8_t>(ASC_SAMPLE_RATE_48KHZ);
    }
    HILOGE("[ASCService]default peerCap%{public}d,localCap%{public}d,samRate%{public}d", peerCap, localCap, sampleRate);
    return static_cast<uint8_t>(ASC_SAMPLE_RATE_48KHZ);
}

uint8_t ASCService::SelectCodecBitDepthPolicyInDualRec(AscCodecIdKey codec, uint8_t inDepth)
{
    uint8_t peerDepth = inDepth;
    uint8_t localDepth = GetLocalCodecBitDepthCap(codec);
    // 取交集
    uint8_t depth = (peerDepth & localDepth);
    // 优选24bit
    if ((depth & (1 << ASC_SAMPLE_DEPTH_24BIT)) != 0) {
        return ASC_SAMPLE_DEPTH_24BIT;
    }
    // 24bit不支持就选16bit(协议规定必选支持能力)
    return ASC_SAMPLE_DEPTH_16BIT;
}

uint8_t ASCService::SelectCodecChannelPolicyInDualRec(AscCodecIdKey codec, uint16_t peerChannel)
{
    uint8_t peerCh = peerChannel;
    uint8_t localCh = GetLocalCodecChannelCap(codec);
    uint8_t channel = (peerCh & localCh);
    // 当前默认优先单声道
    if ((channel & (ASC_CHANNEL_SINGLE)) != 0) {
        return static_cast<uint8_t>(ASC_CHANNEL_SINGLE);
    }
    HILOGE("[ASCService]err peerCh:%{public}d,localCh:%{public}d,channel%{public}d", peerCh, localCh, channel);
    return static_cast<uint8_t>(ASC_CHANNEL_SINGLE);
}

// 本端和对端设备协商后选择的编解码具体参数值并通知对端\默认使用L2HC标准版
void ASCService::SetPeerCodecParamConfig(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn,
    NLSTK_L2HCConfig_S &l2hcParam)
{
    l2hcParam.version = codec.version;
    l2hcParam.rateConf = SelectCodecSampleRatePolicy(device, codec, paramIn);
    l2hcParam.depthConf = SelectCodecBitDepthPolicy(device, codec, paramIn);
    l2hcParam.channelConf = SelectCodecChannelPolicy(device, codec, paramIn);
    // 帧长模式
    l2hcParam.frameConf = ASC_L2HC_FRAME_10MS;
    // 起播码率/码率范围
    l2hcParam.bpsConf = ChooseBps(device, paramIn, l2hcParam.bpsRange);
    HILOGI("[ASCService]dev:%{public}s,codecId:%{public}d,companyId:%{public}d, version:%{public}d"
        "vendorId:%{public}d,rate:%{public}d,depth:%{public}d,channel:%{public}d,frame:%{public}d,bps:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), codec.codecId, codec.companyId, codec.vendorId, codec.version,
        l2hcParam.rateConf, l2hcParam.depthConf, l2hcParam.channelConf, l2hcParam.frameConf, l2hcParam.bpsConf);
}

Qos ASCService::ConvertQos(Qos qos)
{
    // service层定义的qos和底层qos映射(qos8、qos10 --> qos1)
    return (qos == NL_SLE_QOS_8 || qos == NL_SLE_QOS_10) ? NL_SLE_QOS_1 : qos;
}

void ASCService::UpdateL2HCParamByTrans(const RawAddress &device, NLSTK_ActmConfig_S *para)
{
    if (para == nullptr) {
        return;
    }
    bool isSupport = false;
    int qosIdConfigIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(MANU_ABILITY_QOSID_CONFIG);
    if (qosIdConfigIndex >= 0) {
        isSupport = SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(device,
            static_cast<uint8_t>(qosIdConfigIndex));
    }
    if (isSupport) {
        para->channel.trans = ASC_TRANSPORT_TYPE_SLE_QOSID;
    }
}

void ASCService::SetStreamPara(const RawAddress &device, AudioStreamType streamType, Qos qos,
    NLSTK_ActmPointType_E pointType, NLSTK_ActmConfig_S *para)
{
    HILOGD("[ASCService]SetStreamPara in %{public}s streamType %{public}d qos %{public}d pointType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, qos, pointType);

    para->pointType = pointType;
    // 通信方式, 0x0: 单播, 0x1: 数据组播
    GetCommType(qos, para->channel.comm);
    // 编解码器参数
    // 编解码器标识, 0x0: PCM(暂不支持), 0x1: L2HC, 0x1: L2HC_Voice
    // 厂商标识 当编解码器类型为标准类型时，该值为0x0000
    // 厂商编解码器标识 当编解码器类型为标准类型时，该值为0x0000
    AscCodecIdKey codec = SelectPeerCodecId(device, qos, para->channel.comm);
    para->codec.codecId = codec.codecId;
    para->codec.companyId = codec.companyId;
    para->codec.vendorId = codec.vendorId;
    // 编解码参数协商
    JudgeL2HCParamStru paraIn {
        .qos = qos,
        .pointType = pointType,
        .codecId = codec.codecId,
        .streamType = streamType,
    };
    // 更新l2hc的参数
    NLSTK_L2HCConfig_S l2hcParam {};
    SetPeerCodecParamConfig(device, codec, paraIn, l2hcParam);
    para->codec.l2hc = l2hcParam;
    // 透传方式, 0x0: 不透传, 0x2: SLE透传, 0x3: 扩展Qosid
    para->channel.trans = ASC_TRANSPORT_TYPE_SLE_TRANSPARENT;
    UpdateL2HCParamByTrans(device, para);
    // qos参数索引
    para->channel.qosId = ConvertQos(qos);
    // 音频流类型
    para->streamType = streamType;
    // 音频流持续时长
    para->duration = ASC_DURATION_LONG;
    // 源端口号
    para->src = 0;
    // 目标端口号
    para->dst = 0;

    HILOGD("[ASCService]StreamPara %{public}s pointType %{public}d codecId %{public}d companyId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), para->pointType, para->codec.codecId, para->codec.companyId);
    HILOGD("vendorId %{public}d rate %{public}d depth %{public}d channel %{public}d frame %{public}d bps %{public}d",
        para->codec.vendorId, l2hcParam.rateConf, l2hcParam.depthConf, l2hcParam.channelConf, l2hcParam.frameConf,
        l2hcParam.bpsConf);
    HILOGD("channel.comm %{public}d trans %{public}d qosId %{public}d streamType %{public}d duration %{public}d",
        para->channel.comm, para->channel.trans, para->channel.qosId, para->streamType, para->duration);
    HILOGD("src %{public}d dst %{public}d bpsRange %{public}d, version %{public}d",
        para->src, para->dst, l2hcParam.bpsRange, l2hcParam.version);
}

int ASCService::FillActmImgEncpParam(
    const RawAddress &device, Qos cos, NLSTK_ActmConfigParam_S &cfgPara)
{
    if (cos != NL_SLE_QOS_3 && cos != NL_SLE_QOS_6) {
        return NL_NO_ERROR;
    }
    // 单播业务不走组播加密。QOS_3-通话，QOS_5-录音，QOS_6-语音助手走组播加密

    // 通话场景下发组加密参数
    HILOGI("[ASCService]IsCallType set encp para");

    std::string encryptGroupKeyStr = "";
    uint64_t giv = 0;
    uint8_t cryptoAlgo = 0;

    int ret = GetCdsmActmImgEncpParam(device, encryptGroupKeyStr, giv, cryptoAlgo);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, NL_SLE_ASC_ERROR_CFG_QOS_ERROR, "GetGroupAndGiv or GetPairAlgoInfo fail");
    LinkKey sleGroupkey;
    EncryptedLinkKey sleEncryptedGroupkey;
    bool result = SleUtils::ConvertHexStringToInt(
        encryptGroupKeyStr, sleEncryptedGroupkey.data(), sleEncryptedGroupkey.size());
    if (!result) {
        HILOGE("ConvertHexStringToInt fail");
        DftReportAudioError(device.GetAddress(), ERROR_ASC, SUB_ERRCODE_CASE1);
        return NL_SLE_ASC_ERROR_CFG_QOS_ERROR;
    }
    int32_t hksret = SleHksTool::GetInstance().SleLinkKeyDecrypt(sleEncryptedGroupkey, sleGroupkey);
    NL_CHECK_RETURN_RET(hksret == HKS_SUCCESS, NL_SLE_ASC_ERROR_CFG_QOS_ERROR, "SleLinkKeyDecrypt fail");
    cfgPara.isImg = true;
    cfgPara.encp.cryptAlgo = cryptoAlgo;
    cfgPara.encp.giv = giv;
    if (memcpy_s(&(cfgPara.encp.groupKey), OCTET16_LEN, &sleGroupkey[0], OCTET16_LEN) != EOK) {
        LOG_ERROR("memcpy_s failed!");
        return NL_SLE_ASC_ERROR_CFG_QOS_ERROR;
    }
    (void)memset_s(&sleGroupkey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
    return NL_NO_ERROR;
}

int ASCService::GetCdsmActmImgEncpParam(const RawAddress &device, std::string &encryptGroupKeyStr,
    uint64_t &giv, uint8_t &cryptoAlgo)
{
    uint8_t keyDerivAlgo = 0;
    uint8_t integrChkInd = 0;

    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN_RET(cdsmService, NL_SLE_ASC_ERROR_CFG_QOS_ERROR, "cdsmService is null");
    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    // 1. 先从reportAddr获取groupKey
    bool ret = SleRemoteDeviceAdapter::GetInstance()->GetGroupAndGiv(reportAddr, encryptGroupKeyStr, giv);
    ret = ret && SleRemoteDeviceAdapter::GetInstance()->GetPairAlgoInfo(reportAddr, cryptoAlgo,
        keyDerivAlgo, integrChkInd);
    // 2. reportAddr获取失败，从otherAddr获取groupKey
    if (!ret || encryptGroupKeyStr.empty() || giv == 0) {
        RawAddress otherAddr(device);
        cdsmService->CdsmGetOtherAddr(reportAddr, otherAddr);
        ret = SleRemoteDeviceAdapter::GetInstance()->GetGroupAndGiv(otherAddr, encryptGroupKeyStr, giv);
        ret = ret && SleRemoteDeviceAdapter::GetInstance()->GetPairAlgoInfo(otherAddr, cryptoAlgo,
            keyDerivAlgo, integrChkInd);
    }
    NL_CHECK_RETURN_RET(ret && !encryptGroupKeyStr.empty() && giv != 0,
        NL_SLE_ASC_ERROR_CFG_QOS_ERROR, "encryptGroupKeyStr is empty or giv=0");
    return NL_NO_ERROR;
}

int ASCService::ConfigStreamCustomize(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara)
{
    int ret = NL_NO_ERROR;
    if (SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        ret = SleASC::ConfigStreamCustomize(device, cfgPara);
    } else {
        ret = SleASC::ConfigStream(device, cfgPara);
    }

    return ret;
}

int ASCService::ConfigStreamToActm(const RawAddress &device, AudioStreamType streamType)
{
    QosM& qosM = QosM::GetInstance();
    Qos cos = qosM.GetCOS(device);
    NLSTK_ActmPointType_E pointType = GetPointType(device, cos);
    NLSTK_ActmConfigParam_S cfgPara {};
    const AscStreamInfo& streamInfo = GetStreamInfo(device, pointType);
    cfgPara.streamId = streamInfo.streamId;
    cfgPara.groupId = 0;
    CdsmService* cdsm = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsm, NL_ERR_INTERNAL_ERROR, "cdsmService nullptr.");
    cdsm->CdsmGetGroupId(cfgPara.groupId, device.GetAddress());
    NLSTK_ActmConfig_S para[2];
    if (pointType == NLSTK_ACTM_ALL_POINT) {
        // 有上下行，配置音源、音宿访问点属性
        SetStreamPara(device, streamType, cos, NLSTK_ACTM_SOURCE_POINT, &(para[0]));
        SetStreamPara(device, streamType, cos, NLSTK_ACTM_SINK_POINT, &(para[1]));
        cfgPara.srcConfig = &(para[0]);
        cfgPara.sinkConfig = &(para[1]);
    } else if (pointType == NLSTK_ACTM_SINK_POINT) {
        // 单下行，配置音宿访问点属性
        SetStreamPara(device, streamType, cos, NLSTK_ACTM_SINK_POINT, &(para[0]));
        cfgPara.srcConfig = nullptr;
        cfgPara.sinkConfig = &(para[0]);
    } else if (pointType == NLSTK_ACTM_SOURCE_POINT) {
        if (IsRolePrimary(device)) {
            // 录音双上行暂不支持，主耳配置音源访问点属性
            SetStreamPara(device, streamType, cos, NLSTK_ACTM_SOURCE_POINT, &(para[0]));
            cfgPara.srcConfig = &(para[0]);
            cfgPara.sinkConfig = nullptr;
        }
    } else {
        HILOGE("[ASCService]ConfigStream qos illegeal %{public}d", cos);
        return NL_SLE_ASC_ERROR_CFG_QOS_ERROR;
    }

    int ret = NL_NO_ERROR;
    ret = FillActmImgEncpParam(device, cos, cfgPara);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, NL_SLE_ASC_ERROR_CFG_QOS_ERROR, "FillActmImgEncpParam fail.");
    ret = ConfigStreamCustomize(device, cfgPara);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]ConfigStream ret %{public}d", ret);
        return ret;
    }
    // 保存码率
    if ((cos == NL_SLE_QOS_1) || (cos == NL_SLE_QOS_8) || (cos == NL_SLE_QOS_10)) {
        SetBpsRange(device, para[0].codec.l2hc.bpsRange);
    }
    return NL_NO_ERROR;
}

int ASCService::ConfigStream(const RawAddress &device, AudioStreamType streamType, ASCState state)
{
    HILOGI("[ASCService]enter %{public}s, state:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
    // 如果是通话类型的流建立同步链路，将calling标记位置位，避免因监听的OnAudioSceneChange时序不准确导致标记位不准确
    SetIsCallingFlag(IsCallType(streamType));

    QosM& qosM = QosM::GetInstance();
    // 添加Qos（重配置场景在外层进行判决时已经添加过）
    if ((state == NL_SLE_ASC_CONFIGURING) && !IsSync(device)) {
        qosM.AddQos(device, GetStreamQos(device, streamType));
        // 空间音频打开再起播的场景，添加Qos4/8
        (void)AddSpatialQos(device);
    }
    bool isVoice = IsVoiceCallService(qosM.GetCOS(device));
    (void)AudioStandard::AudioSystemClientPolicyManager::GetInstance().SetSleVoiceStatusFlag(isVoice);

    // 设置subrate 60ms
    auto subrateStatus = GetASCSubRateStatus(device);
    switch (subrateStatus) {
        case NL_SLE_ASC_INIT:
        case NL_SLE_ASC_SETTED:
            SetSubratePreConfigStream(device);
            break;
        case NL_SLE_ASC_SETTING:
            HILOGW("[ASCService] %{public}s Self-healing ready", GetEncryptAddr(device.GetAddress()).c_str());
            SetASCStartStreamChangeSubrateFlag(device, true);
            break;
        default:
            break;
    }

    // 状态：音频流属性配置中/重配置中
    SetASCStatus(device, state);
    // 设置处理中的流类型
    SetProcessingStreamType(device, streamType);
    HILOGI("[ASCService]ConfigStream Start in %{public}s, streamType:%{public}d, state:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
    return NL_NO_ERROR;
}

Qos ASCService::GetStreamQos(const RawAddress &device, AudioStreamType streamType)
{
    std::map<AudioStreamType, Qos> streamQosMapping = {
        {AUDIO_STREAM_NONE,             NL_SLE_QOS_1},
        {AUDIO_STREAM_UNDEFINED,        NL_SLE_QOS_1},
        {AUDIO_STREAM_MUSIC,            NL_SLE_QOS_1},
        {AUDIO_STREAM_VOICE_CALL,       NL_SLE_QOS_3},
        // 规则八：QOS6废掉与QOS3归一
        {AUDIO_STREAM_VOICE_ASSISTANT,  NL_SLE_QOS_3},
        {AUDIO_STREAM_RING,             NL_SLE_QOS_3},
        {AUDIO_STREAM_VOIP,             NL_SLE_QOS_3},
        {AUDIO_STREAM_GAME,             NL_SLE_QOS_2},
        {AUDIO_STREAM_RECORD,           NL_SLE_QOS_3},
        {AUDIO_STREAM_ALERT,            NL_SLE_QOS_7},
        {AUDIO_STREAM_VIDEO,            NL_SLE_QOS_7},
        {AUDIO_STREAM_GUID,             NL_SLE_QOS_7},
        {AUDIO_STREAM_ALARM,            NL_SLE_QOS_7},
        {AUDIO_STREAM_SING,             NL_SLE_QOS_1},
    };
    if (GetDualRecordAbility(device)) {
        streamQosMapping[AUDIO_STREAM_RECORD] = NL_SLE_QOS_5;
    }
    // 全景音打开，音频和视频都需要锁定QOS 10
    if ((streamType == AUDIO_STREAM_VIDEO) && IsNeedTransferForColAudio(streamType)) {
        return NL_SLE_QOS_10;
    }
    if (GetKaraokeAbility(device)) {
        streamQosMapping[AUDIO_STREAM_SING] = NL_SLE_QOS_9;
    }
    if (streamType > AUDIO_STREAM_SING) {
        HILOGE("[ASCService] streamType %{public}d illegeal", streamType);
        return NL_SLE_QOS_NONE;
    }
    return streamQosMapping[streamType];
}

bool ASCService::AddSpatialQos(const RawAddress &device)
{
    bool isNeedReconfig = false;
    QosM& qosM = QosM::GetInstance();
    // 1. 自适应开关关闭，头动打开 2. 自适应开关打开，头动打开，音源类型支持（非立体声）-> 使能空间音频头动
    bool isHeadTracking = (!IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioHeadTrackingEnabled());
    isHeadTracking = isHeadTracking || (IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioSourceTypeSupported() &&
        IsSpatialAudioHeadTrackingEnabled());
    // 空间音频头动已打开再起播的场景，添加Qos4
    if (isHeadTracking && !(qosM.IsQos4Exist(device))) {
        isNeedReconfig |= qosM.AddQos(device, NL_SLE_QOS_4);
    } else {
        // 1. 自适应开关关闭，固定模式打开 2. 自适应开关打开，固定模式打开，音源类型支持（非立体声）-> 使能空间音频固定
        bool isSpatial = (!IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioModeEnabled());
        isSpatial = isSpatial || (IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioSourceTypeSupported() &&
            IsSpatialAudioModeEnabled());
        // 空间音频固定已打开再起播的场景，添加Qos8
        if (isSpatial && !(qosM.IsQosExist(device, NL_SLE_QOS_8))) {
            isNeedReconfig |= qosM.AddQos(device, NL_SLE_QOS_8);
        }
    }

    return isNeedReconfig;
}

bool ASCService::IsNeedTransferForColAudio(AudioStreamType streamType)
{
    std::map<AudioStreamType, bool> streamTransferMapping = {
        {AUDIO_STREAM_NONE,             true},
        {AUDIO_STREAM_UNDEFINED,        true},
        {AUDIO_STREAM_MUSIC,            true},
        {AUDIO_STREAM_VOICE_CALL,       false},
        {AUDIO_STREAM_RECORD,           false},
        {AUDIO_STREAM_VOICE_ASSISTANT,  false},
        {AUDIO_STREAM_RING,             false},
        {AUDIO_STREAM_VOIP,             false},
        {AUDIO_STREAM_GAME,             true},
        {AUDIO_STREAM_ALERT,            true},
        {AUDIO_STREAM_VIDEO,            true},
        {AUDIO_STREAM_GUID,             true},
        {AUDIO_STREAM_ALARM,            true},
    };
    auto it = streamTransferMapping.find(streamType);
    if (it == streamTransferMapping.end()) {
        return false;
    }
    return isColAudioEnabled_ && streamTransferMapping[streamType];
}

bool ASCService::IsColAudioStreamExist(const RawAddress &device)
{
    bool ret = false;
    QosM& qosM = QosM::GetInstance();
    if (IsStreamExists(device, AUDIO_STREAM_VIDEO) && !qosM.IsQosExist(device, NL_SLE_QOS_7)) {
        ret = true;
    }
 
    return ret;
}

int ASCService::JudgeReConfig(const RawAddress &device, AudioStreamType streamType, ASCState state, bool& isGoOn)
{
    HILOGI("[ASCService]JudgeReConfig in %{public}s streamType %{public}d state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);

    QosM& qosM = QosM::GetInstance();
    Qos cos = qosM.GetCOS(device);

    isGoOn = false;
    // 添加Qos，进行重配置判决
    bool isNeedReconfig = qosM.AddQos(device, GetStreamQos(device, streamType));
    // 空间音频打开再起播的场景，添加Qos4/8
    bool isSpatialNeedReconfig = AddSpatialQos(device);
    isNeedReconfig |= isSpatialNeedReconfig;

    // 需要重配置
    if (isNeedReconfig) {
        int ret = NL_NO_ERROR;
        if (IsStarted(state)) {
            // 8.2.9.3 音频流传输时的重配置: 开始传输音频数据后，不应进行音频流重配置流程，应将音频流暂停或停止后，再进行音频流重配置
            // 先停止流，在停止流的回调里面继续状态机处理
            HILOGI("[ASCService]JudgeReConfig Need Reconfig %{public}s streamType %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType);
            ret = ReconfigStreamInStartedState(device, streamType, cos);

            // 更改参数更新标记位保证可以通知DSP
            SetQosmInfoUpdateFlag(device, true);
            RawAddress coSetDevice;
            if (IsCoSetDeviceExist(device, coSetDevice)) {
                SetQosmInfoUpdateFlag(coSetDevice, true);
            }
        } else if (IsConfiged(state) || IsOpened(state)) {
            // 8.2.9.1 音频流配置后的重配置: 音频访问点已配置但音频流传输通道未开启时，音频客户端可直接发送音频流配置请求对已配置过的音频访问点进行重配置
            // 8.2.9.2 音频流数据通道开启后的重配置: 音频流传输通道已经建立完成但音频流未传输时，音频客户端可直接发送音频流配置请求对已配置的音频访问点进行重配置
            // 先停止流，在停止流的回调里面继续状态机处理
            HILOGI("[ASCService]JudgeReConfig Need Reconfig %{public}s streamType %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType);
            // 重配置
            ret = ConfigStream(device, streamType, NL_SLE_ASC_RECONFIGURING);
        } else {
            HILOGE("[ASCService]JudgeReConfig state illegeal %{public}s streamType %{public}d state %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
            ret = NL_SLE_ASC_ERROR_JUDGE_RECFG_STATE_ERROR;
        }

        return ret;
    } else {
        // 不需要重配置，直接返回，继续使用已打开的音频流
        HILOGI("[ASCService]JudgeReConfig NOT Need Reconfig %{public}s streamType %{public}d COS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, cos);

        // 添加到已打开流列表
        std::list<AudioStreamType>& list = GetStartedStreamList(device);
        list.emplace_back(streamType);

        // 上报状态: 音频流打开,成功
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);

        // 继续处理既有音频流
        isGoOn = true;
        return NL_NO_ERROR;
    }
}

int ASCService::ReconfigStreamInStartedState(const RawAddress &device, AudioStreamType streamType, Qos cos)
{
    // 重配置-停止播放的流
    std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);

    // 保存重配置的流类型
    SetReconfigStream(device, streamType);
    AudioStreamType streamToStop;
    if (!(startedStreamList.empty())) {
        // 存在started的流, 需要先释放
        streamToStop = startedStreamList.front();
    } else {
        // 先删除残留的编解码参数
        ClearStreamQosmInfo(device);

        // 不存在started的流, 默认为AUDIO_STREAM_NONE
        streamToStop = AUDIO_STREAM_NONE;
        HILOGI("[ASCService]Self-healing Solution: started stream size is 0, cos %{public}d", cos);
    }
    return DeleteCommLink(device, streamToStop, cos, NL_SLE_ASC_RECONFIG_STOPPING);
}

bool ASCService::IsInStopProcess(ASCState state) const
{
    return ((state == NL_SLE_ASC_RELEASING) || (state == NL_SLE_ASC_STOPPED) || (state == NL_SLE_ASC_STOPPING));
}

bool ASCService::IsStartingOrStopingMemberExist(const RawAddress &device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        ASCState state = GetASCStatus(info.addr_);
        if (IsInStartProcess(state) || IsInStopProcess(state)) {
            HILOGI("[ASCService]IsStartingOrStopingMemberExist true %{public}s state %{public}d",
                GetEncryptAddr(info.addr_.GetAddress()).c_str(), state);
            return true;
        }
    }

    return false;
}

int ASCService::StartPlaying(const RawAddress &device, AudioStreamType streamType, bool isSync)
{
    HILOGI("[ASCService]StartPlaying in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    if (IsNeedTransferForColAudio(streamType)) {
        streamType = AUDIO_STREAM_VIDEO;
        HILOGI("[ASCService]StartPlaying in %{public}s, transfer to streamType %{public}d ",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    }
    // 如果之前的出声设备还在停播中，新出声设备的起播先缓存等待
    if (!(formerSinkDevice_.GetAddress().empty())) {
        if (IsStartingOrStopingMemberExist(formerSinkDevice_) || IsStopDelaying(formerSinkDevice_)) {
            HILOGI("[ASCService]StartPlaying Need Cache %{public}s streamType %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType);
            // 缓存本次配置，在进行中的流程结束后，再触发起播/重配置判决
            GetStartBuff(device).push(streamType);
            return NL_NO_ERROR;
        }
    }

    int ret = NL_NO_ERROR;
    // 音频流配置中/音频流打开中/音频流重配置中/停止中状态
    ASCState state = GetASCStatus(device);
    if (IsInStartProcess(state) || IsInStopProcess(state)) {
        HILOGI("[ASCService]StartPlaying Need Cache %{public}s streamType %{public}d state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
        // 缓存本次配置，在进行中的流程结束后，再触发起播/重配置判决
        GetStartBuff(device).push(streamType);

        ret = NL_NO_ERROR;
    } else if (IsCreating(state)) {
        HILOGI("[ASCService]StartPlaying Need Cache %{public}s streamType %{public}d state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
        // 缓存本次配置，在进行中的流程结束后，再触发起播/重配置判决
        GetStartBuff(device).push(streamType);
        SetSyncFlag(device, isSync);
        ret = NL_NO_ERROR;
    } else if (IsStarted(state)) {
        // 音频流已开始传输状态
        // 进行重配置判决
        bool isGoOn = false;
        ret = JudgeReConfig(device, streamType, NL_SLE_ASC_STARTED, isGoOn);
    } else if (IsCreated(state) || IsReleased(state)) {
        // 正常配置本音频流
        HILOGI("[ASCService]StartPlaying config %{public}s streamType %{public}d state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
        SetSyncFlag(device, isSync);
        ret = ConfigStream(device, streamType, NL_SLE_ASC_CONFIGURING);
    } else {
        HILOGE("[ASCService]StartPlaying state error %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        ret = NL_SLE_ASC_ERROR_START_PLAY_STATE_ERROR;
    }

    return ret;
}

NLSTK_ActmPointType_E ASCService::GetPointType(const RawAddress &device, Qos qos)
{
    NLSTK_ActmPointType_E pointType = NLSTK_ACTM_ALL_POINT;
    if ((qos == NL_SLE_QOS_3) || (qos == NL_SLE_QOS_6)) {
        // 有上下行，配置音源、音宿访问点属性
        pointType = NLSTK_ACTM_ALL_POINT;
    } else if ((qos == NL_SLE_QOS_1) || (qos == NL_SLE_QOS_2) || (qos == NL_SLE_QOS_4) ||
        (qos == NL_SLE_QOS_7) || (qos == NL_SLE_QOS_8) || (qos == NL_SLE_QOS_9) ||
        (qos == NL_SLE_QOS_10)) {
        // 单下行，配置音宿访问点属性
        pointType = NLSTK_ACTM_SINK_POINT;
    } else if (qos == NL_SLE_QOS_5) {
        // 上行+静音流下行，配置音宿访问点属性
        pointType = NLSTK_ACTM_SINK_POINT;
    } else if (qos == NL_SLE_QOS_NONE) {
        pointType = NLSTK_ACTM_ALL_POINT;
    } else {
        HILOGE("[ASCService]GetPointType qos illegeal %{public}d", qos);
        pointType = NLSTK_ACTM_ALL_POINT;
    }

    HILOGD("[ASCService]GetPointType %{public}s pointType %{public}d qos %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), pointType, qos);
    return pointType;
}

int ASCService::OpenStream(const RawAddress &device, AudioStreamType streamType)
{
    HILOGD("[ASCService]OpenStream in %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);

    // 状态检查: 在配置/配置回调中已检查

    Qos cos = QosM::GetInstance().GetCOS(device);
    uint8_t pointType = GetPointType(device, cos);
    const AscStreamInfo& streamInfo = GetStreamInfo(device, pointType);
    int ret = SleASC::OpenStream(device, streamInfo.streamId);
    if (ret != NL_NO_ERROR) {
        HILOGE("[ASCService]OpenStream ret %{public}d", ret);
        return ret;
    }

    // 状态：打开音频流中
    SetASCStatus(device, NL_SLE_ASC_OPENING);
    // 设置处理中的流类型
    SetProcessingStreamType(device, streamType);
    HILOGI("[ASCService]OpenStream Start");
    return NL_NO_ERROR;
}

void ASCService::CreateCommLink(const RawAddress &device, AudioStreamType streamType)
{
    if (SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        int ret = StartStream(device, streamType);
        if (ret != NL_NO_ERROR) {
            // 上报状态: 音频流打开,失败
            ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
                NL_SLE_ASC_RESULT_FAIL, ret);
        }

        return;
    } else {
        // 打开音频流
        int ret = OpenStream(device, streamType);
        if (ret != NL_NO_ERROR) {
            // 上报状态: 音频流打开,失败
            ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
                NL_SLE_ASC_RESULT_FAIL, ret);
        }

        return;
    }
}

int ASCService::StartStream(const RawAddress &device, AudioStreamType streamType)
{
    HILOGD("[ASCService]StartStream in %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);

    Qos cos = QosM::GetInstance().GetCOS(device);
    uint8_t pointType = GetPointType(device, cos);
    const AscStreamInfo& streamInfo = GetStreamInfo(device, pointType);

    int ret = SleASC::StartStream(device, streamInfo.streamId);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]StartStream ret %{public}d", ret);
        return ret;
    }

    // 状态：开始传输音频流中(ing)
    SetASCStatus(device, NL_SLE_ASC_STARTING);
    // 设置处理中的流类型
    SetProcessingStreamType(device, streamType);
    HILOGI("[ASCService]StartStream Start");
    return NL_NO_ERROR;
}

AudioStreamType ASCService::GetReconfigStream(const RawAddress &device) const
{
    // 按照设备存储
    AudioStreamType streamType = AUDIO_STREAM_NONE;
    std::map<std::string, AudioStreamType>::const_iterator it = reconfigStreamMap_.find(device.GetAddress());
    if (it != reconfigStreamMap_.end()) {
        streamType = it->second;
    }

    HILOGD("[ASCService]GetReconfigStream %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    return streamType;
}

void ASCService::SetReconfigStream(const RawAddress &device, AudioStreamType streamType)
{
    HILOGD("[ASCService]SetReconfigStream in %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    // 按照设备存储
    reconfigStreamMap_[device.GetAddress()] = streamType;
}

int ASCService::DeleteCommLink(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState)
{
    // 改配时在Stop之前禁止UART进低功耗下电
    if (IsReconfigStopping(targetState)) {
        ServiceManagerPluginLoader::GetInstance()->SetPowerModeProc(SetPowerModeProcType::POWERMODE_PROC_DISENABLE);
    }

    return ReleaseStream(device, streamType, qos, targetState);
}

int ASCService::StopStream(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState)
{
    HILOGD("[ASCService]StopStream in %{public}s streamType %{public}d qos %{public}d targetState %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, qos, targetState);

    uint8_t pointType = GetPointType(device, qos);
    const AscStreamInfo& streamInfo = GetStreamInfo(device, pointType);
    int ret = SleASC::StopStream(device, streamInfo.streamId);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]StopStream ret %{public}d", ret);
        return ret;
    }

    // 状态：停止音频流中/重配置-停止音频流中
    SetASCStatus(device, targetState);
    // 设置处理中的流类型
    if (IsStopping(targetState)) {
        SetProcessingStreamType(device, streamType);
    }

    HILOGI("[ASCService]StopStream Start");
    return NL_NO_ERROR;
}

int ASCService::StopPlaying(const RawAddress &device, AudioStreamType streamType)
{
    HILOGI("[ASCService]StopPlaying in %{public}s streamType %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        streamType);
    if (device == formerPlayRecord_.addr) {
        formerPlayRecord_.timeStamp = time(nullptr);
        HILOGD("[ASCService]StopPlaying at %{public}ld", formerPlayRecord_.timeStamp);
    }
    // 状态检查: 须是OPENED/STARTED/CREATED状态
    ASCState state = GetASCStatus(device);
    Qos cos = QosM::GetInstance().GetCOS(device);
    if ((state != NL_SLE_ASC_OPENED) && (state != NL_SLE_ASC_STARTED) && (state != NL_SLE_ASC_CREATED)) {
        HILOGE("[ASCService]StopPlaying state error %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        DeleteCommLink(device, streamType, cos, NL_SLE_ASC_RELEASING);
        return NL_SLE_ASC_ERROR_STOP_PLAY_STATE_ERROR;
    }

    DftSetAirplaneMode(NearlinkDataShareHelper::GetInstance().GetAirplaneModeState());
    PrintStartedStreamList(device);
    int ret = DeleteCommLink(device, streamType, cos, NL_SLE_ASC_RELEASING);
    return ret;
}

ASCState ASCService::GetASCStatus(const RawAddress &device) const
{
    // 如果还没有记录状态，视为星闪开关刚打开的状态
    ASCState state = NL_SLE_ASC_ENABLED;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        state = it->second.state;
    }

    HILOGD("[ASCService]GetASCStatus %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
    return state;
}

bool ASCService::IsStopDelaying(const RawAddress &device) const
{
    bool isStopDelaying = false;
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        isStopDelaying = it->second.isStopDelaying;
    }

    HILOGD("[ASCService]IsStopDelaying %{public}s isStopDelaying %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isStopDelaying);
    return isStopDelaying;
}

void ASCService::SetStopDelayingFlag(const RawAddress &device, bool isStopDelaying)
{
    HILOGD("[ASCService]SetStopDelayingFlag in %{public}s isStopDelaying %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isStopDelaying);
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        ASCStatus stateSt;
        (void)memset_s(&stateSt, sizeof(ASCStatus), 0x0, sizeof(ASCStatus));
        stateSt.state = NL_SLE_ASC_ENABLED;
        stateSt.streamType = AUDIO_STREAM_NONE;
        stateSt.isStopDelaying = isStopDelaying;
        ascStatusMap_[device.GetAddress()] = stateSt;
    } else {
        it->second.isStopDelaying = isStopDelaying;
    }
}

bool ASCService::IsStopDoing(const RawAddress &device) const
{
    bool isStopDoing = false;
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        isStopDoing = it->second.isStopDoing;
    }

    HILOGI("[ASCService]IsStopDoing %{public}s isStopDoing %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isStopDoing);
    return isStopDoing;
}

void ASCService::SetSpatialConfiguringFlag(const RawAddress &device, bool isSpatialConfiguring)
{
    HILOGD("[ASCService]SetSpatialConfiguringFlag in %{public}s isSpatialConfiguring %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isSpatialConfiguring);
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetSpatialConfiguringFlag NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.isSpatialConfiguring = isSpatialConfiguring;
}

bool ASCService::IsSpatialConfiguring(const RawAddress &device) const
{
    bool isSpatialConfiguring = false;
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        isSpatialConfiguring = it->second.isSpatialConfiguring;
    }

    HILOGI("[ASCService]IsSpatialConfiguring %{public}s isSpatialConfiguring %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isSpatialConfiguring);
    return isSpatialConfiguring;
}

void ASCService::SetStopDoingFlag(const RawAddress &device, bool isStopDoing)
{
    HILOGD("[ASCService]SetStopDoingFlag in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetStopDoingFlag NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.isStopDoing = isStopDoing;
}

bool ASCService::GetAutoRateBps(const RawAddress &device, uint16_t& autoRateBps) const
{
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        if (it->second.autoRateBps != 0) {
            autoRateBps = it->second.autoRateBps;
            HILOGI("[ASCService]GetAutoRateBps %{public}s autoRateBps %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), autoRateBps);
            return true;
        }
    }

    HILOGD("[ASCService]GetAutoRateBps NO item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    return false;
}

void ASCService::SetAutoRateBps(const RawAddress &device, uint16_t autoRateBps)
{
    HILOGI("[ASCService]SetAutoRateBps in %{public}s autoRateBps %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), autoRateBps);
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetAutoRateBps NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    if (device == formerPlayRecord_.addr && autoRateBps > 0){
        formerPlayRecord_.autoRateBpsBit = ASCUtils::GetBpsBitIndexByBps(autoRateBps);
        HILOGI("[ASCService]SetAutoRateBps to formerPlayRecord");
    }

    it->second.autoRateBps = autoRateBps;
}

void ASCService::SetPlayRecord(const RawAddress &device, Qos qos, uint8_t codecId, uint8_t autoRateBpsBit)
{
    formerPlayRecord_.addr = device;
    formerPlayRecord_.qos = qos;
    formerPlayRecord_.codecId = codecId;
    formerPlayRecord_.autoRateBpsBit = autoRateBpsBit;
    HILOGI("[ASCService]SetPlayRecord %{public}s qos%{public}d codecId%{public}d autoRateBpsBit%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), qos, codecId, autoRateBpsBit);
}

AudioStreamType ASCService::GetProcessingStreamType(const RawAddress &device) const
{
    AudioStreamType streamType = AUDIO_STREAM_NONE;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        streamType = it->second.streamType;
    }

    HILOGD("[ASCService]GetProcessingStreamType %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    return streamType;
}

void ASCService::SetProcessingStreamType(const RawAddress &device, AudioStreamType streamType)
{
    HILOGD("[ASCService]SetProcessingStreamType in %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetProcessingStreamType NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.streamType = streamType;
}

uint64_t ASCService::GetBpsRange(const RawAddress &device) const
{
    uint64_t bpsRange = false;
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        bpsRange = it->second.bpsRange;
    }

    HILOGI("[ASCService]GetBpsRange %{public}s bpsRange %{public}ld",
        GetEncryptAddr(device.GetAddress()).c_str(), bpsRange);
    return bpsRange;
}

void ASCService::SetBpsRange(const RawAddress &device, uint64_t bpsRange)
{
    HILOGD("[ASCService]SetBpsRange in %{public}s bpsRange %{public}ld",
        GetEncryptAddr(device.GetAddress()).c_str(), bpsRange);
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetBpsRange NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.bpsRange = bpsRange;
}

void ASCService::SetASCStatus(const RawAddress &device, ASCState state)
{
    HILOGI("[ASCService]SetASCStatus in %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        state);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        ASCStatus stateSt;
        stateSt.state = state;
        stateSt.streamType = AUDIO_STREAM_NONE;
        ascStatusMap_[device.GetAddress()] = stateSt;
    } else {
        it->second.state = state;
    }
    // DFT缓存对应时间戳
    ASCToDftCacheTime(device, state);

    return;
}

void ASCService::ASCToDftCacheTime(const RawAddress &device, ASCState state)
{
    DftAudioStreamInfo info;
    info.addr = device.GetAddress();
    info.configparam = GenerateCodecPara(device, AUDIO_STREAM_NONE);
    info.reportaddr = GetReportAddr(device).GetAddress();
    info.runningType = IsSync(device) ? NL_SLE_ASC_RUNNING_TYPE_SINGLE : NL_SLE_ASC_RUNNING_TYPE_DOUBLE;
    info.pointType = GetPointType(device, QosM::GetInstance().GetCOS(device));
    info.state = state;
    std::list<AudioStreamType>& streamlist = GetStartedStreamList(device);
    for (const auto& streamtype : streamlist) {
        if (!info.startBuff.empty()) {
            info.startBuff += ';';
        }
        info.startBuff += std::to_string(streamtype);
    }
    DftCacheStreamASC(info);
}

void ASCService::SaveStreamInfo(const RawAddress &device, AscStreamInfo streamInfo)
{
    HILOGD("[ASCService]SaveStreamInfo in %{public}s streamId %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        streamInfo.streamId);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SaveStreamInfo NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
    } else {
        if (streamInfo.pointType == NLSTK_ACTM_SINK_POINT) {
            it->second.unicastStream = streamInfo;
        } else {
            it->second.multicastStream = streamInfo;
        }
    }

    return;
}

AscStreamInfo ASCService::GetStreamInfo(const RawAddress &device, uint8_t pointType) const
{
    AscStreamInfo info {};
    info.streamId = ASC_INVALID_STREAM_ID;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        if (pointType == NLSTK_ACTM_SINK_POINT) {
            info = it->second.unicastStream;
        } else {
            info = it->second.multicastStream;
        }
    }

    HILOGD("[ASCService]GetStreamInfo %{public}s pointType %{public}d streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), pointType, info.streamId);
    return info;
}

bool ASCService::IsStreamIdValid(const RawAddress &device, uint8_t pointType)
{
    const AscStreamInfo& info = GetStreamInfo(device, pointType);
    if (info.streamId != ASC_INVALID_STREAM_ID) {
        return true;
    }

    return false;
}

bool ASCService::IsNeedDisconnect(const RawAddress &device) const
{
    bool ret = false;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        ret = it->second.isNeedDisconnect;
    }

    HILOGD("[ASCService]isNeedDisconnect %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), ret);
    return ret;
}

void ASCService::SetNeedDisconnect(const RawAddress &device, bool isNeedDisconnect)
{
    HILOGD("[ASCService]SetNeedDisconnect in %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        isNeedDisconnect);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetNeedDisconnect NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
    } else {
        it->second.isNeedDisconnect = isNeedDisconnect;
    }

    return;
}

bool ASCService::IsSync(const RawAddress &device) const
{
    bool isSync = false;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        isSync = it->second.isSync;
    }

    HILOGD("[ASCService]IsSync %{public}s isSync %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isSync);
    return isSync;
}

void ASCService::SetSyncFlag(const RawAddress &device, bool isSync)
{
    HILOGD("[ASCService]SetSyncFlag in %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isSync);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetSyncFlag NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.isSync = isSync;
}

int ASCService::ReleaseStream(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState)
{
    HILOGD("[ASCService]ReleaseStream in %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);

    uint8_t pointType = GetPointType(device, qos);
    const AscStreamInfo& streamInfo = GetStreamInfo(device, pointType);

    int ret = SleASC::ReleaseStream(device, streamInfo.streamId);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]ReleaseStream ret %{public}d", ret);
        return ret;
    }

    // 状态：释放音频流中
    SetASCStatus(device, targetState);
    // 设置处理中的流类型
    SetProcessingStreamType(device, streamType);
    HILOGI("[ASCService]ReleaseStream Start");
    return NL_NO_ERROR;
}

std::vector<AscProp>& ASCService::GetPropertyList(const RawAddress& device)
{
    // 这里map里面不存在时会默认插入一个list
    return devicePropList_[device.GetAddress()];
}

void ASCService::SaveProperty(const RawAddress& device, const std::vector<AscProp>& properties)
{
    HILOGD("[ASCService]SaveProperty in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    // 这里map会默认插入构造一个list
    std::vector<AscProp>& devProp = GetPropertyList(device);
    devProp.clear();
    for (const AscProp& prop : properties) {
        devProp.emplace_back(prop);
    }
}

void ASCService::SetConnRptDelayingFlag(const RawAddress &device, bool isDelaying)
{
    HILOGD("[ASCService]SetConnRptDelayingFlag in %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isDelaying);
    // 按照设备存储
    std::map<std::string, ASCStatus>::iterator it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetConnRptDelayingFlag NO item %{public}s in deviceAscStatusMap",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.isConnRptDelaying = isDelaying;
}

bool ASCService::IsConnRptDelaying(const RawAddress &device) const
{
    bool isConnRptDelaying = false;
    // 按照设备存储
    std::map<std::string, ASCStatus>::const_iterator it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        isConnRptDelaying = it->second.isConnRptDelaying;
    }

    HILOGD("[ASCService]IsConnRptDelaying %{public}s isConnRptDelaying %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isConnRptDelaying);
    return isConnRptDelaying;
}

void ASCService::DoDeviceConnectedReprot(const RawAddress& device)
{
    // 状态：已连接
    SetASCStatus(device, NL_SLE_ASC_CONNECTED);
    // 上报设备增删给音频框架
    OnAddDeleteAudioDevice(device, NL_SLE_ASC_CONN_CMD_CONN);
    // 创建单播流、组播流
    CreateStream(device);
}

void ASCService::DeviceReprotDelayCheck(const RawAddress& device)
{
    HILOGD("[ASCService]DeviceReprotDelayCheck %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    RawAddress coSetDevice;
    for (const auto& info : cdsmList) {
        if (!(device == info.addr_)) {
            coSetDevice = info.addr_;
            HILOGD("[ASCService]DeviceReprotDelayCheck coSetDevice %{public}s",
                GetEncryptAddr(coSetDevice.GetAddress()).c_str());
        }
    }

    if (cdsmList.size() > 1) {
        if (!IsConnected(coSetDevice)) {
            StartConnRptDelayTimer(device);
            SetConnRptDelayingFlag(device, true);
            return;
        }

        if (IsConnRptDelaying(coSetDevice)) {
            StopConnRptDelayTimer(coSetDevice);
            SetConnRptDelayingFlag(coSetDevice, false);
            // 状态：已连接 上报设备增删给音频框架 创建单播流、组播流
            DoDeviceConnectedReprot(coSetDevice);
        }

        // 状态：已连接 上报设备增删给音频框架 创建单播流、组播流
        DoDeviceConnectedReprot(device);
    } else {
        // 状态：已连接 上报设备增删给音频框架 创建单播流、组播流
        DoDeviceConnectedReprot(device);
    }
}

void ASCService::PrintStartedStreamList(const RawAddress& device)
{
    std::string buff;
    const std::list<AudioStreamType>& streamList = GetStartedStreamList(device);
    for (const AudioStreamType& stream : streamList) {
        buff = buff + std::to_string(stream) + " ";
    }
    HILOGI("[ASCService]StartedStreamList %{public}s %{public}s", GetEncryptAddr(device.GetAddress()).c_str(),
        buff.c_str());
}

void ASCService::SyncWithCoSetDevice(const RawAddress& device, const RawAddress& coSetDevice,
    AudioStreamType procStream)
{
    HILOGI("[ASCService]SyncWithCoSetDevice %{public}s coSetDevice %{public}s",
        GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(coSetDevice.GetAddress()).c_str());
    // 1. 同步已打开流列表
    std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);
    const std::list<AudioStreamType>& coStartedStreamList = GetStartedStreamList(coSetDevice);
    bool isMatched = false;
    for (const AudioStreamType& coStream : coStartedStreamList) {
        if (coStream == procStream) {
            if (!isMatched) {
                isMatched = true;
                continue;
            }
        }

        startedStreamList.emplace_back(coStream);
    }

    PrintStartedStreamList(coSetDevice);
    PrintStartedStreamList(device);

    // 2. 同步qos
    QosM::GetInstance().SyncQos(device, coSetDevice);
    // 3. 帧4通话业务，同步acb帧格式
    SyncAcbPhyStatusWithCoSetDevice(device, coSetDevice);
}

void ASCService::SyncAcbPhyStatusWithCoSetDevice(const RawAddress& device, const RawAddress& coSetDevice)
{
    uint8_t coMulticastFrameType = 0;
    uint8_t coMulticastPhyType = 0;
    Qos cos = QosM::GetInstance().GetCOS(device);
    if (IsVoiceCallService(cos) && GetLongRangeVoiceCallAbility(device) &&
        GetVoiceCallAcbStatus(coSetDevice, coMulticastFrameType, coMulticastPhyType)) {
        SetVoiceCallAcbStatus(device, coMulticastFrameType, coMulticastPhyType);
        HILOGI("Sync FrameType %{public}d, phyType %{public}d to %{public}s",
            coMulticastFrameType, coMulticastPhyType, GET_ENCRYPT_ADDR(device));
        auto adapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(adapter, "[ASCService]sleAdapter is null.");
        adapter->SetPhy(device, coMulticastFrameType, coMulticastPhyType);
    }
}

void ASCService::ChangeBpsRange()
{
    const RawAddress& device = activeSinkDevice_;
    HILOGI("[ASCService]ChangeBpsRange in %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), GetASCStatus(activeSinkDevice_));

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    for (const auto& info : cdsmList) {
        if (IsStarted(GetASCStatus(info.addr_))) {
            Qos cos = QosM::GetInstance().GetCOS(info.addr_);
            if ((cos != NL_SLE_QOS_1) && (cos != NL_SLE_QOS_8) && (cos != NL_SLE_QOS_10)) {
                HILOGI("[ASCService]ChangeBpsRange cos %{public}d not 1", cos);
                return;
            }

            // 通信方式, 0x0: 单播, 0x1: 数据组播
            AscCodecIdKey codec = SelectPeerCodecId(info.addr_, cos, ASC_COMM_TYPE_UNICAST);
            // 编解码参数
            NLSTK_L2HCConfig_S l2hcParam {};
            JudgeL2HCParamStru paraIn {};
            paraIn.qos = cos;
            paraIn.pointType = NLSTK_ACTM_SINK_POINT;
            paraIn.streamType = GetProcessingStreamType(info.addr_);
            paraIn.codecId = codec.codecId;
            SetPeerCodecParamConfig(info.addr_, codec, paraIn, l2hcParam);

            uint64_t bpsRange = GetBpsRange(info.addr_);
            if (bpsRange == l2hcParam.bpsRange) {
                HILOGI("[ASCService]ChangeBpsRange bpsRange %{public}ld equal", l2hcParam.bpsRange);
                continue;
            }

            const AscStreamInfo& streamInfo = GetStreamInfo(device, NLSTK_ACTM_SINK_POINT);
            int ret = SleASC::UpdateBitRate(info.addr_, l2hcParam.bpsRange, streamInfo.streamId);
            if (ret != NL_NO_ERROR) {
                HILOGI("[ASCService]ChangeBpsRange ret %{public}d", ret);
                continue;
            }

            SetBpsRange(info.addr_, l2hcParam.bpsRange);
        }
    }
}

void ASCService::RegisterListener()
{
    if (!isListenerRegistered_) {
        RegisterSpatialAudioListener();
        RegisterAudioSceneListener();
        SleAudioFrameworkAdapter::GetInstance().RegisterCollaborativeAudioListener();
        isListenerRegistered_ = true;
    }
}

void ASCService::CbkGetProperty(const RawAddress& device, const std::vector<AscProp>& properties)
{
    ASCState state = GetASCStatus(device);
    if (!(IsConnecting(state))) {
        HILOGE("[ASCService]CbkGetProperty state error %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        // 上报状态: 连接,失败
        ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_CONN, NL_SLE_ASC_RESULT_FAIL,
            NL_SLE_ASC_ERROR_CBK_GET_PROP_STATE_ERROR);
        return;
    }
    // 设备连接，处理媒体和通话相关的业务
    ProcessMcpInit(device);
    ProcessCcpInit(device);
    // 设备连接，注册监听接口
    RegisterListener();

    // 保存音频属性
    SaveProperty(device, properties);

    // 添加已连接设备
    AddConnectDevices(device);

    // 上报连接状态给profile连接管理
    OnConnectionStateChanged(device, NL_SLE_ASC_CONN_CMD_CONN, NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
    // 上报状态/等待
    DeviceReprotDelayCheck(device);

    // 新连接的设备，需判断集合数门限 <= 2，超过门限时，下线最老的未在播放的设备
    int coSetNum = GetCoSetNum();
    if (coSetNum > ASC_COSET_MAX_NUM) {
        DeleteEarliestDevice();
    }

    if (coSetNum > 1) {
        // 同时连接两副星闪耳机，不进4.6M
        ChangeBpsRange();
    }
}

int ASCService::CreateStream(const RawAddress &device)
{
    HILOGI("[ASCService]dev:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    // 创建单播流
    AscStreamInfo infoSink {};
    infoSink.pointType = NLSTK_ACTM_SINK_POINT;
    infoSink.streamId = ASC_INVALID_STREAM_ID;
    SaveStreamInfo(device, infoSink);
    int ret = SleASC::SleASC::CreateStream(device, NLSTK_ACTM_SINK_POINT, ASC_COMM_TYPE_UNICAST);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]CreateStream ret %{public}d", ret);
        return ret;
    }

    // 创建组播流
    AscStreamInfo infoAll {};
    infoAll.pointType = NLSTK_ACTM_ALL_POINT;
    infoAll.streamId = ASC_INVALID_STREAM_ID;
    SaveStreamInfo(device, infoAll);
    ret = SleASC::SleASC::CreateStream(device, NLSTK_ACTM_ALL_POINT, ASC_COMM_TYPE_MULTICAST);
    if (ret != NL_NO_ERROR) {
        HILOGI("[ASCService]CreateStream ret %{public}d", ret);
        return ret;
    }

    // 状态：创建中
    SetASCStatus(device, NL_SLE_ASC_CREATING);

    return NL_NO_ERROR;
}

void ASCService::SyncWhenConnect(const RawAddress& device)
{
    // 查看合作集设备状态，进行同步
    RawAddress coSetDevice;
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        ASCState coStatus = GetASCStatus(coSetDevice);
        const auto* startBuffCo = FindStartBuff(coSetDevice);
        const auto* stopBuffCo = FindStopBuff(coSetDevice);
        bool isCoBuffEmpty = IsQueueEmpty(startBuffCo) && IsQueueEmpty(stopBuffCo);
        std::list<AudioStreamType>& startedList = GetStartedStreamList(coSetDevice);
        if (IsStarted(coStatus) && !IsStopDelaying(activeSinkDevice_) && isCoBuffEmpty && !startedList.empty() &&
            IsConnected(device)) {
            // 取出合作集设备处理中的流类型
            AudioStreamType coStreamType = GetProcessingStreamType(coSetDevice);

            HILOGI("[ASCService]CbkGetProperty StartPlaying device %{public}s coDevice %{public}s "
                "coStreamType %{public}d coStatus %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
                GetEncryptAddr(coSetDevice.GetAddress()).c_str(), coStreamType, coStatus);

            // 初始化结果记录
            SetStartPlayingResult(device, coStreamType, false, NL_NO_ERROR);
            // 启动定时器
            EnableStartPlayingTimer(device);
            // 同步流列表、qos管理信息
            SyncWithCoSetDevice(device, coSetDevice, coStreamType);
            // 打开音频流，当前运行类型：单切双
            int ret = StartPlaying(device, coStreamType, true);
            if (ret != NL_NO_ERROR) {
                // 上报状态: 音频流打开,失败
                ReportAudioControlComplete(device, coStreamType, NL_SLE_ASC_CONTROL_CMD_START,
                    NL_SLE_ASC_RESULT_FAIL, ret);
            }
        }
    }
}

void ASCService::SyncWhenDisconnect(const RawAddress& device, const ASCState state)
{
    // 查看合作集设备状态，进行同步
    RawAddress coSetDevice;
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        ASCState coState = GetASCStatus(coSetDevice);
        bool isSync = IsSync(device);
        // A业务启动中，B Profile连接完成，A断连，都断（直接），FWK设备下线
        if (IsInStartProcess(state) && isSync && IsConnected(coState)) {
            HILOGE("[ASCService]CbkDisconnect device %{public}s coDevice %{public}s state %{public}d "
                "coState %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
                GetEncryptAddr(coSetDevice.GetAddress()).c_str(), state, coState);
        }
    }
}

void ASCService::ProcStartBuff(const RawAddress& device, ASCState state, bool& isGoOn)
{
    std::queue<AudioStreamType>& startBuff = GetStartBuff(device);
    if (!(startBuff.empty())) {
        HILOGI("[ASCService] dev:%{public}s, state:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
        AudioStreamType streamToStart = startBuff.front();
        startBuff.pop();
        // 初始化起播结果
        SetStartPlayingResult(device, streamToStart, false, NL_NO_ERROR);
        RawAddress reportAddr = GetReportAddr(device);
        CancelStopDelay(reportAddr);

        if (IsConfiged(state) || IsStarted(state)) {
            HILOGI("[ASCService]ProcStartBuff JudgeReConfig %{public}s streamType %{public}d state %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamToStart, state);
            // 进行重配置判决
            int ret = JudgeReConfig(device, streamToStart, state, isGoOn);
            if (ret != NL_NO_ERROR) {
                // 上报状态: 音频流打开,失败
                ReportAudioControlComplete(device, streamToStart, NL_SLE_ASC_CONTROL_CMD_START,
                    NL_SLE_ASC_RESULT_FAIL, ret);
            }

            // 继续处理其它音频流
            if (isGoOn) {
                ProcStartBuff(device, state, isGoOn);
            }
        }

        if (IsCreated(state) || IsReleased(state)) {
            // 正常配置本音频流
            HILOGI("[ASCService]ProcStartBuff config %{public}s streamType %{public}d state %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), streamToStart, state);
            int ret = ConfigStream(device, streamToStart, NL_SLE_ASC_CONFIGURING);
            if (ret != NL_NO_ERROR) {
                // 上报状态: 音频流打开,失败
                ReportAudioControlComplete(device, streamToStart, NL_SLE_ASC_CONTROL_CMD_START,
                    NL_SLE_ASC_RESULT_FAIL, ret);
            }
        }
    }
}

void ASCService::ProcStopBuff(const RawAddress& device, ASCState state)
{
    std::queue<AudioStreamType>& stopBuff = GetStopBuff(device);
    if (!(stopBuff.empty())) {
        HILOGI("[ASCService] dev:%{public}s, state:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
        if (IsStarted(state)) {
            AudioStreamType streamType = stopBuff.front();
            stopBuff.pop();

            // 初始化停播结果
            SetStopPlayingResult(device, streamType, false, NL_NO_ERROR);

            bool isDelayStop = false;
            bool isStopStream = false;
            bool isNeedReconfig = false;
            JudgeQosWhenStopPlaying(device, streamType, isDelayStop, isStopStream, isNeedReconfig);
            if (isDelayStop) {
                DelayStop(device, streamType);
                return;
            }

            if (!isStopStream && !isNeedReconfig) {
                ProcStopBuff(device, state);
            }
        }

        if (IsCreated(state) || IsReleased(state)) {
            std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);
            while (!(stopBuff.empty())) {
                AudioStreamType streamToStop = stopBuff.front();
                // 从已打开列表删除该流类型
                startedStreamList.remove(streamToStop);
                stopBuff.pop();
                HILOGI("[ASCService]ProcStopBuff dev %{public}s streamType %{public}d state %{public}d",
                    GetEncryptAddr(device.GetAddress()).c_str(), streamToStop, state);
                bool isStopStream = false;
                bool isStopImmediately = false;
                QosM::GetInstance().DeleteQos(device, GetStreamQos(device, streamToStop),
                    isStopStream, isStopImmediately);
                // 上报状态: 音频流关闭,成功
                ReportAudioControlComplete(device, streamToStop, NL_SLE_ASC_CONTROL_CMD_STOP,
                    NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
            }
        }
    }
}

void ASCService::CbkCreateStream(const RawAddress& device, uint8_t result, AscStreamInfo streamInfo)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);
    if (result != NL_NO_ERROR) {
        streamInfo.streamId = ASC_INVALID_STREAM_ID;
    }

    SaveStreamInfo(device, streamInfo);

    // 组播单播流都已创建时切状态
    if (IsStreamIdValid(device, NLSTK_ACTM_SINK_POINT) && IsStreamIdValid(device, NLSTK_ACTM_ALL_POINT)) {
        // 状态：已创建
        VcpService* vcpService = VcpService::GetService();
        if (vcpService != nullptr) {
            vcpService->SetAllStreamVolume(device);
        }
        SetASCStatus(device, NL_SLE_ASC_CREATED);
        // 取出缓存的打开流任务进行处理
        bool isGoOn = false;
        ProcStartBuff(device, NL_SLE_ASC_CREATED, isGoOn);

        SyncWhenConnect(device);
    }
}

void ASCService::CbkConfigStream(const RawAddress& device, uint8_t result)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);

    ASCState state = GetASCStatus(device);
    // 取出处理中的流类型
    AudioStreamType streamType = GetProcessingStreamType(device);
    if (!IsSubrateChanged(state)) {
        HILOGE("[ASCService]CbkConfigStream state illegeal %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        return;
    }

    // 结果检查
    if (result != NL_NO_ERROR) {
        HILOGE("[ASCService]CbkConfigStream callback result %{public}s %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), result);
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, result);
        return;
    }

    ASCState stateNew = (state == NL_SLE_ASC_RECONFIGURING) ? NL_SLE_ASC_RECONFIGED : NL_SLE_ASC_CONFIGED;
    // 状态：已配置/已重配置
    SetASCStatus(device, stateNew);

    // 创建通信链路
    CreateCommLink(device, streamType);
}

void ASCService::CbkReadPropFail(const RawAddress& device, uint8_t result)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);

    ASCState state = GetASCStatus(device);
    if (!(IsConnecting(state))) {
        HILOGE("[ASCService]CbkReadPropFail state error %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        // 上报状态: 连接,失败
        ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_CONN, NL_SLE_ASC_RESULT_FAIL,
            NL_SLE_ASC_ERROR_CBK_GET_PROP_STATE_ERROR);
        return;
    }

    HILOGE("[ASCService]CbkReadPropFail callback result %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), result);
    // 结果检查
    if (result != NL_NO_ERROR) {
        // 上报状态: 连接,失败
        ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_CONN, NL_SLE_ASC_RESULT_FAIL, result);
        return;
    }
}

void ASCService::SaveStreamQosmInfo(const RawAddress &device, const AscQosmInfo &info)
{
    streamQosmInfo_[device.GetAddress()] = info;
    // 已更新
    streamQosmInfo_[device.GetAddress()].isUpdated = true;
    HILOGD("[ASCService]SaveStreamQosmInfo updated %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
}

void ASCService::SetQosmInfoUpdateFlag(const RawAddress &device, bool isUpdated)
{
    std::map<std::string, AscQosmInfo>::iterator it =
        streamQosmInfo_.find(device.GetAddress());
    if (it == streamQosmInfo_.end()) {
        HILOGD("[ASCService]SetQosmInfoUpdateFlag NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    it->second.isUpdated = isUpdated;
}

bool ASCService::IsQosmInfoUpdated(const RawAddress &device)
{
    std::map<std::string, AscQosmInfo>::iterator it =
        streamQosmInfo_.find(device.GetAddress());
    if (it == streamQosmInfo_.end()) {
        HILOGD("[ASCService]IsQosmInfoUpdated NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    return it->second.isUpdated;
}

bool ASCService::GetStreamQosmInfo(const RawAddress &device, AscQosmInfo &info)
{
    std::map<std::string, AscQosmInfo>::iterator it =
        streamQosmInfo_.find(device.GetAddress());
    if (it == streamQosmInfo_.end()) {
        HILOGD("[ASCService]GetStreamQosmInfo NO Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    info = it->second;
    return true;
}

void ASCService::ClearStreamQosmInfo(const RawAddress &device)
{
    uint32_t num = streamQosmInfo_.erase(device.GetAddress());
    if (num == 0) {
        HILOGD("[ASCService]ClearStreamQosmInfo %{public}s NO Item", GetEncryptAddr(device.GetAddress()).c_str());
    }
}

bool ASCService::GetAscQosmInfo(const RawAddress& device, AscQosmInfo& qosmInfo)
{
    HILOGD("[ASCService]GetQosmInfo");

    std::promise<bool> promise;
    DoInAscThread([this, &device, &qosmInfo, &promise] {
        if (GetStreamQosmInfo(device, qosmInfo)) {
            HILOGD("[ASCService]GetQosmInfo get Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
            promise.set_value(true);
            return;
        }

        promise.set_value(false);
    });

    return promise.get_future().get();
}

void ASCService::ProcWhenIOBCreated(const RawAddress& device, const AscQosmInfo& qosmInfo)
{
    // 同步链路建立OK后解禁UART进低功耗
    ServiceManagerPluginLoader::GetInstance()->SetPowerModeProc(SetPowerModeProcType::POWERMODE_PROC_ENABLE);
    // 删除编解码参数
    ClearStreamQosmInfo(device);
    // 保存新的编解码参数
    SaveStreamQosmInfo(device, qosmInfo);
    // 帧4通话 & 通话 autorate 能力通知Actm
    NotifyVoiceCallAutorateAblityToActm(device, qosmInfo);
    // 单切双场景，因为重建了新链路要发送链路信息给DSP，将合作集链路标记位置位
    if (IsSync(device)) {
        RawAddress coSetDevice;
        if (IsCoSetDeviceExist(device, coSetDevice)) {
            SetQosmInfoUpdateFlag(coSetDevice, true);
        }
    }
}

void ASCService::CbkOpenStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);

    ASCState state = GetASCStatus(device);
    // 取出处理中的流类型
    AudioStreamType streamType = GetProcessingStreamType(device);
    if (!(IsOpening(state))) {
        HILOGE("[ASCService]CbkOpenStream state error %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        return;
    }

    // 结果检查
    if (result != NL_NO_ERROR) {
        HILOGE("[ASCService]CbkOpenStream callback result %{public}s %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), result);
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, result);
        // 取出缓存任务处理
        ProcBuff(device, NL_SLE_ASC_CREATED);
        return;
    }

    // 状态：已打开音频流
    SetASCStatus(device, NL_SLE_ASC_OPENED);
    // 同步链路建立OK后的处理
    ProcWhenIOBCreated(device, qosmInfo);

    int ret = StartStream(device, streamType);
    if (ret != NL_NO_ERROR) {
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, ret);
    }
}

void ASCService::SetDeviceRole(const RawAddress& device)
{
    Qos cos = QosM::GetInstance().GetCOS(device);
    HILOGI("[ASCService]SetDeviceRole %{public}s COS %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), cos);

    uint8_t direction = NLSTK_ACTM_DIRECTION_BOTH;
    bool isRolePrimary = false;
    if (SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        TwsService* twsService = TwsService::GetService();
        if (twsService == nullptr) {
            return;
        }
        uint8_t devRole = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
        twsService->TwsGetDeviceRole(device, devRole);
        isRolePrimary = devRole == static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
        HILOGI("[ASCService][Tws]SetDeviceRole %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            isRolePrimary);
    } else {
        MicService* micService = MicService::GetService();
        if (micService == nullptr) {
            return;
        }
        isRolePrimary = micService->IsMicOpen(device);
        HILOGI("[ASCService][Common]SetDeviceRole %{public}s %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            isRolePrimary);
    }
    direction = ASCUtils::GetDirection(isRolePrimary, cos);

    int ret = SleASC::SetDirection(device, direction);
    if (ret != NL_NO_ERROR) {
        HILOGE("[ASCService]SetDeviceRole ret %{public}d", ret);
    }
}

void ASCService::ProcSpatialIfNeed(const RawAddress& device, AudioStreamType streamType)
{
    QosM& qosM = QosM::GetInstance();
    bool isNeedReconfig = IsNeedReconfigSpatialAudio(device, streamType, qosM);
    if (!isNeedReconfig) {
        return;
    }

    // 存在延迟释放流需要先停定时器并清延迟断链路标记位
    RawAddress reportAddr = GetReportAddr(device);
    CancelStopDelay(reportAddr);

    // 设置标记位:开空间音频改配同步链路执行中
    SetSpatialConfiguringFlag(device, true);
    Qos cos = qosM.GetCOS(device);
    ReconfigStream(device, streamType, streamType, cos);
}

void ASCService::CancelStopDelay(const RawAddress &device)
{
    if (!IsStopDelaying(device)) {
        return;
    }

    // 重置等待断同步链路标记
    DisableStopDelayTimer(device);
    SetStopDelayingFlag(device, false);
}

bool ASCService::IsNeedReconfigSpatialAudio(const RawAddress& device, AudioStreamType streamType, QosM& qosM)
{
    // 1. 自适应开关关闭，头动打开 2. 自适应开关打开，头动打开，音源类型支持（非立体声）-> 使能空间音频头动
    bool isHeadTracking = (!IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioHeadTrackingEnabled());
    isHeadTracking = isHeadTracking || (IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioSourceTypeSupported() &&
        IsSpatialAudioHeadTrackingEnabled());

    // 1. 自适应开关关闭，固定模式打开 2. 自适应开关打开，固定模式打开，音源类型支持（非立体声）-> 使能空间音频固定模式
    bool isSpatial = (!IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioModeEnabled());
    isSpatial = isSpatial || (IsSpatialAudioAdaptiveSwitchEnabled() && IsSpatialAudioSourceTypeSupported() &&
        IsSpatialAudioModeEnabled());
    bool isQos4Exist = qosM.IsQos4Exist(device);
    bool isQos8Exist = qosM.IsQosExist(device, NL_SLE_QOS_8);

    bool isStopStream = false;
    bool isStopImmediately = false;
    bool isNeedReconfig = false;
    if ((isHeadTracking || isSpatial) && IsColAudioStreamExist(device)) {
        isNeedReconfig = qosM.DeleteQos(device, NL_SLE_QOS_10, isStopStream, isStopImmediately);
    }
    if (isHeadTracking) {
        // 固定模式关闭，如果qos8存在则删除
        if (isQos8Exist) {
            isNeedReconfig = qosM.DeleteQos(device, NL_SLE_QOS_8, isStopStream, isStopImmediately);
        }

        // 空间音频头动模式，添加Qos4
        if (!isQos4Exist) {
            isNeedReconfig = qosM.AddQos(device, NL_SLE_QOS_4);
        }
    } else {
        // 头动模式关闭，如果qos4存在则删除
        if (isQos4Exist) {
            // 关闭空间音频，删除Qos4
            isNeedReconfig = qosM.DeleteQos(device, NL_SLE_QOS_4, isStopStream, isStopImmediately);
        }

        // 空间音频固定模式，添加Qos8
        if (isSpatial) {
            if (!isQos8Exist) {
                isNeedReconfig = qosM.AddQos(device, NL_SLE_QOS_8);
            }
        } else {
            // 固定模式关闭，如果qos8存在则删除
            if (isQos8Exist) {
                isNeedReconfig = qosM.DeleteQos(device, NL_SLE_QOS_8, isStopStream, isStopImmediately);
            }
        }
    }
    HILOGI("[ASCService]ProcSpatialIfNeed %{public}s streamType %{public}d isNeedReconfig %{public}d isSpatial "
        "%{public}d isHeadTracking %{public}d isQos4Exist %{public}d isQos8Exist %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, isNeedReconfig, isSpatial, isHeadTracking,
        isQos4Exist, isQos8Exist);
    return isNeedReconfig;
}

void ASCService::ProcColAudioIfNeed(const RawAddress& device, AudioStreamType streamType)
{
    QosM& qosM = QosM::GetInstance();

    bool isQos10Exist = qosM.IsQosExist(device, NL_SLE_QOS_10);
    Qos cos = qosM.GetCOS(device);
    bool isStopStream = false;
    bool isStopImmediately = false;
    bool isNeedReconfig = false;
    AudioStreamType reconfigStreamType = streamType;
    if (isColAudioEnabled_) {
        // 移动全景音开启，添加Qos10,流类型设置锁定为视频
        if (!isQos10Exist) {
            isNeedReconfig = qosM.AddQos(device, NL_SLE_QOS_10);
        }
        reconfigStreamType = AUDIO_STREAM_VIDEO;
    }

    HILOGI("[ASCService]device %{public}s streamType %{public}d reconfigStreamType %{public}d "
        "isNeedReconfig %{public}d isColAudioEnabled_ %{public}d isQos10Exist %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, reconfigStreamType, isNeedReconfig,
        isColAudioEnabled_.load(), isQos10Exist);
    if (!isNeedReconfig) {
        return;
    }

    ReconfigStream(device, streamType, reconfigStreamType, cos);
}

void ASCService::SyncWhenStartStream(const RawAddress& device, AudioStreamType streamType)
{
    // 查看合作集设备状态，进行同步
    RawAddress coSetDevice;
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        ASCState coStatus = GetASCStatus(coSetDevice);
        ASCState status = GetASCStatus(device);
        const auto* startBuff = FindStartBuff(device);
        const auto* stopBuff = FindStopBuff(device);
        bool isBuffEmpty = IsQueueEmpty(startBuff) && IsQueueEmpty(stopBuff);
        if ((IsCreated(coStatus) || IsReleased(coStatus)) && IsStarted(status) && isBuffEmpty) {
            HILOGI("[ASCService]CbkStartStream StartPlaying device %{public}s coDevice %{public}s "
                "coStreamType %{public}d coStatus %{public}d status %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(coSetDevice.GetAddress()).c_str(),
                streamType, coStatus, status);

            // 初始化结果记录
            SetStartPlayingResult(coSetDevice, streamType, false, NL_NO_ERROR);
            // 启动定时器
            EnableStartPlayingTimer(coSetDevice);
            // 同步流列表、qos管理信息
            SyncWithCoSetDevice(coSetDevice, device, streamType);
            // 打开音频流
            int ret = StartPlaying(coSetDevice, streamType, true);
            if (ret != NL_NO_ERROR) {
                // 上报状态: 音频流打开,失败
                ReportAudioControlComplete(coSetDevice, streamType, NL_SLE_ASC_CONTROL_CMD_START,
                    NL_SLE_ASC_RESULT_FAIL, ret);
            }
        }
    }
}

bool ASCService::IsStreamExists(const RawAddress& device, AudioStreamType streamType)
{
    std::list<AudioStreamType>& list = GetStartedStreamList(device);
    if (list.empty()) {
        return false;
    }

    auto it = std::find(list.begin(), list.end(), streamType);
    if (it != list.end()) {
        return true;
    }

    return false;
}

void ASCService::ProcBuff(const RawAddress& device, ASCState state)
{
    HILOGI("[ASCService]enter %{public}s, state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
    // 取出缓存的打开流任务进行重配置判决
    bool isGoOn = false;
    ProcStartBuff(device, state, isGoOn);

    // 取出缓存的关闭流任务进行处理
    ProcStopBuff(device, state);
}

void ASCService::CbkStartStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo)
{
    HILOGI("[ASCService]CbkStartStream in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    // 取出处理中的流类型
    AudioStreamType streamType = GetProcessingStreamType(device);
    NL_CHECK_RETURN(CheckStartStreamCondition(device, result, streamType), "start stream failed");
    // 同步链路建立OK后的处理
    ProcWhenIOBCreated(device, qosmInfo);
    ProcessCachedSubrate();
    // 主副切换
    SetDeviceRole(device);
    // 状态：已开始音频流传输
    SetASCStatus(device, NL_SLE_ASC_STARTED);
    // 添加到已打开列表
    std::list<AudioStreamType>& list = GetStartedStreamList(device);
    RawAddress coSetDevice;
    if (IsSync(device) && IsCoSetDeviceExist(device, coSetDevice)) {
        if (IsStreamExists(coSetDevice, streamType)) {
            list.emplace_back(streamType);
        }
    } else {
        list.emplace_back(streamType);
    }
    RawAddress reportAddr = GetReportAddr(device);
    CancelStopDelay(reportAddr);

    // 上报状态: 音频流打开,成功
    ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
        NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
    // 判断是否需要起语音助手
    OpenVoiceAssistant(device, streamType);
    if (IsNeedDisconnect(device)) {
        StopPlayingExcute(device, streamType);
        return;
    }
    // 空间音频开关判断
    ProcSpatialIfNeed(device, streamType);
    // 取出缓存任务处理
    ProcBuff(device, NL_SLE_ASC_STARTED);
    // 查看合作集设备状态，进行同步
    SyncWhenStartStream(device, streamType);
}

bool ASCService::CheckStartStreamCondition(const RawAddress& device, uint8_t result, AudioStreamType streamType)
{
    ASCState state = GetASCStatus(device);
    int startPlayMergeIndex =
        ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(MANU_ABILITY_ASC_START_PLAYING_MERGE);
    bool isStartPlayMerge = (startPlayMergeIndex >= 0) &&
        SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(
        device, static_cast<uint8_t>(startPlayMergeIndex));
    bool isConfig = (state == NL_SLE_ASC_CONFIG_SUBRATE_CHANGED) || (state == NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED);
    if ((!isStartPlayMerge && !IsStarting(state)) || (isStartPlayMerge && !isConfig)) {
        HILOGE("[ASCService]CbkStartStream state error %{public}s state %{public}d isStartPlayMerge %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state, isStartPlayMerge);
        return false;
    }
    // 结果检查
    if (result != NL_NO_ERROR) {
        HILOGE("[ASCService]CbkStartStream callback result %{public}s %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), result);
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, result);
        // 取出缓存任务处理
        ProcBuff(device, NL_SLE_ASC_CREATED);
        return false;
    }
    return true;
}

void ASCService::ClearWhenDisconnect(const RawAddress& device)
{
    ASCState state = GetASCStatus(device);
    HILOGI("[ASCService]ClearWhenDisconnect %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        state);

    // 删除Qos配置信息
    QosM::GetInstance().ClearQosM(device);
    // 清除已打开流列表
    std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);
    startedStreamList.clear();

    // 清除待打开流队列
    ClearStartBuff(device);
    // 清除待关闭流队列
    ClearStopBuff(device);

    // 清除StopDoing标记位
    SetStopDoingFlag(device, false);
    // 清除空间音频配置标记位
    SetSpatialConfiguringFlag(device, false);

    // 清除结果记录
    ClearStreamResult(device);
    // 清除同步链路参数
    ClearStreamQosmInfo(device);
    ClearASCSubrateInfo(device);
    // 清除码率
    SetAutoRateBps(device, 0);
    // 设置状态:connected
    SetASCStatus(device, NL_SLE_ASC_CREATED);
    // 重置等待断同步链路标记
    CheckAndSetStopDelayingFlag(device);
    return;
}

void ASCService::CbkStopStream(const RawAddress& device, uint8_t result, uint16_t connHandle)
{
    HILOGD("[ASCService]CbkStopStream in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
}

bool ASCService::EnableStartPlayingTimer(const RawAddress& device)
{
    HILOGI("Enable StartStream timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::map<std::string, std::shared_ptr<NearlinkTimer>>::iterator it =
        startPlayingTimer_.find(device.GetAddress());
    if (it == startPlayingTimer_.end()) {
        std::shared_ptr<NearlinkTimer> timer = std::make_shared<NearlinkTimer>([this, device]() -> void {
            DoInAscThread([this, device] {
                this->StartPlayingTimeout(device);
            });
        });
        NL_CHECK_RETURN_RET(timer, false, "StartStream timer is nullptr.");

        startPlayingTimer_[device.GetAddress()] = timer;
    }

    std::shared_ptr<NearlinkTimer> timer = startPlayingTimer_[device.GetAddress()];
    return timer->Start(START_PLAYING_TIMEOUT_MS);
}

bool ASCService::DisableStartPlayingTimer(const RawAddress& device)
{
    std::shared_ptr<NearlinkTimer> timer = startPlayingTimer_[device.GetAddress()];
    if (timer != nullptr) {
        HILOGI("Disable StartStream timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return timer->Stop();
    }

    return false;
}

void ASCService::StartPlayingTimeout(const RawAddress& device)
{
    HILOGI("StartPlayingTimeout! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    // 有可能切换了出声设备(地址发生变化), 需要从外层传入
    ASCState state = GetASCStatus(device);
    if (IsInStartProcess(state)) {
        Qos cos = QosM::GetInstance().GetCOS(device);
        AudioStreamType streamType = GetProcessingStreamType(device);
        DeleteCommLink(device, streamType, cos, NL_SLE_ASC_RELEASING);
    }
}

bool ASCService::EnableStopPlayingTimer(const RawAddress& device)
{
    HILOGI("Enable StopStream timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::map<std::string, std::shared_ptr<NearlinkTimer>>::iterator it =
        stopPlayingTimer_.find(device.GetAddress());
    if (it == stopPlayingTimer_.end()) {
        std::shared_ptr<NearlinkTimer> timer = std::make_shared<NearlinkTimer>([this]() -> void {
            this->StopPlayingTimeout();
        });
        NL_CHECK_RETURN_RET(timer, false, "StopStream timer is nullptr.");

        stopPlayingTimer_[device.GetAddress()] = timer;
    }

    std::shared_ptr<NearlinkTimer> timer = stopPlayingTimer_[device.GetAddress()];
    return timer->Start(STOP_PLAYING_TIMEOUT_MS);
}

bool ASCService::DisableStopPlayingTimer(const RawAddress& device)
{
    std::shared_ptr<NearlinkTimer> timer = stopPlayingTimer_[device.GetAddress()];
    if (timer != nullptr) {
        HILOGI("Disable StopStream timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return timer->Stop();
    }

    return false;
}

void ASCService::StopPlayingTimeout()
{
    HILOGI("StopPlayingTimeout!");
}

bool ASCService::EnableStopDelayTimer(const RawAddress& device)
{
    HILOGI("Enable StopDelay timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::map<std::string, std::shared_ptr<NearlinkTimer>>::iterator it =
        stopDelayTimer_.find(device.GetAddress());
    if (it == stopDelayTimer_.end()) {
        std::shared_ptr<NearlinkTimer> timer = std::make_shared<NearlinkTimer>([this]() -> void {
            this->StopDelayTimeout();
        });
        NL_CHECK_RETURN_RET(timer, false, "StopDelay timer is nullptr.");

        stopDelayTimer_[device.GetAddress()] = timer;
    }

    std::shared_ptr<NearlinkTimer> timer = stopDelayTimer_[device.GetAddress()];
    return timer->Start(STOP_DELAY_TIMEOUT_MS);
}

bool ASCService::DisableStopDelayTimer(const RawAddress& device)
{
    std::shared_ptr<NearlinkTimer> timer = stopDelayTimer_[device.GetAddress()];
    if (timer != nullptr) {
        HILOGI("Disable StopDelay timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return timer->Stop();
    }

    return false;
}

void ASCService::StopDelayTimeout()
{
    HILOGI("StopDelayTimeout!");
    ASCMessage event(ASC_STOP_DELAY_TIMEOUT_EVT);
    PostEvent(event);
}

bool ASCService::StartConnRptDelayTimer(const RawAddress& device)
{
    HILOGD("Start ConnRptDelay timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::map<std::string, std::shared_ptr<NearlinkTimer>>::iterator it =
        connRptDelayTimer_.find(device.GetAddress());
    if (it == connRptDelayTimer_.end()) {
        std::shared_ptr<NearlinkTimer> timer = std::make_shared<NearlinkTimer>([this]() -> void {
            this->ConnRptDelayTimeout();
        });
        NL_CHECK_RETURN_RET(timer, false, "ConnRptDelay timer is nullptr.");

        connRptDelayTimer_[device.GetAddress()] = timer;
    }

    std::shared_ptr<NearlinkTimer> timer = connRptDelayTimer_[device.GetAddress()];
    return timer->Start(CONN_RPT_DELAY_TIMEOUT_MS);
}

bool ASCService::StopConnRptDelayTimer(const RawAddress& device)
{
    if (!IsConnRptDelaying(device)) {
        HILOGI("Stop ConnRptDelay No timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    std::shared_ptr<NearlinkTimer> timer = connRptDelayTimer_[device.GetAddress()];
    if (timer != nullptr) {
        HILOGD("Stop ConnRptDelay timer! %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return timer->Stop();
    }

    return false;
}

void ASCService::ConnRptDelayTimeout()
{
    HILOGI("ConnRptDelayTimeout!");
    ASCMessage event(ASC_CONN_RPT_DELAY_TIMEOUT_EVT);
    PostEvent(event);
}

AudioStreamType ASCService::GetStopStreamType(const RawAddress &device)
{
    AudioStreamType streamType = AUDIO_STREAM_NONE;
    const std::list<AudioStreamType>& list = GetStartedStreamList(device);
    if (!(list.empty())) {
        streamType = list.front();
    } else {
        Qos cos = QosM::GetInstance().GetCOS(device);
        if (cos != NL_SLE_QOS_NONE) {
            streamType = GetProcessingStreamType(device);
        }
    }

    HILOGI("[ASCService]GetStopStreamType %{public}s streamType %{public}d streamList size %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, list.size());
    return streamType;
}

void ASCService::ExcuteDelayStop(const RawAddress &device)
{
    HILOGI("[ASCService] enter device %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    // 重置等待断同步链路标记
    SetStopDelayingFlag(device, false);

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    // 初始化结果记录状态信息
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            std::list<AudioStreamType>& list = GetStartedStreamList(info.addr_);
            if ((list.size() != 1)) {
                continue;
            }
            SetStopPlayingResult(info.addr_, list.front(), false, NL_NO_ERROR);
        }
    }

    for (const auto& info : cdsmList) {
        // 对于合作集设备，给组内所有成员（包括当前设备）发停播
        if (IsConnected(info.addr_)) {
            AudioStreamType streamType = GetStopStreamType(info.addr_);
            if (streamType == AUDIO_STREAM_NONE) {
                continue;
            }
            StopPlayingExcute(info.addr_, streamType);
        }
    }
    return;
}

void ASCService::ProcessStopDelayTimeOutEvent(const ASCMessage &event)
{
    // 检查切出声栈前的设备是否在延迟释放
    const RawAddress& formerDevice = formerSinkDevice_;
    if (!(formerDevice.GetAddress().empty())) {
        if (IsStopDelaying(formerDevice)) {
            HILOGI("[ASCService]ProcessStopDelayTimeOutEvent formerSinkDevice %{public}s StopDelaying",
                GetEncryptAddr(formerDevice.GetAddress()).c_str());
            ExcuteDelayStop(formerDevice);
        }
    }

    const RawAddress& device = activeSinkDevice_;
    if (!IsStopDelaying(device)) {
        HILOGW("[ASCService]ProcessStopDelayTimeOutEvent %{public}s NOT StopDelaying",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    ExcuteDelayStop(device);
    return;
}

void ASCService::ProcessConnRptDelayTimeOutEvent(const ASCMessage &event)
{
    for (const auto& pair : ascStatusMap_) {
        if (pair.second.isConnRptDelaying) {
            RawAddress dev = RawAddress(pair.first);
            HILOGI("[ASCService]ProcessConnRptDelayTimeOutEvent %{public}s isConnRptDelaying",
                GetEncryptAddr(dev.GetAddress()).c_str());
            if (IsConnected(dev)) {
                // 状态：已连接 上报设备增删给音频框架 创建单播流、组播流
                DoDeviceConnectedReprot(dev);
                // 清除延迟上线等待标记
                SetConnRptDelayingFlag(dev, false);
            }

            return;
        }
    }

    HILOGE("[ASCService]ProcessConnRptDelayTimeOutEvent NO Delaying Item");
    return;
}

void ASCService::CbkReconfigStopping(const RawAddress& device, uint8_t result, AudioStreamType streamType)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d, streamT:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), result, streamType);
    // 结果检查
    if (result != NL_NO_ERROR) {
        HILOGE("[ASCService]CbkReleaseStream callback result %{public}s %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), result);
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, result);
        return;
    }

    // 状态：重配置流程-已关闭音频流
    SetASCStatus(device, NL_SLE_ASC_RECONFIG_STOPPED);

    // 通知DSP链路断开
    std::string channelInfo = GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_STOP);
    SetAudioParameter(NL_SLE_ASC_CONTROL_CMD_STOP, "", channelInfo);
    // 清除码率
    SetAutoRateBps(device, 0);

    // 重配置音频流
    AudioStreamType reconfigStreamType = GetReconfigStream(device);
    int ret = ConfigStream(device, reconfigStreamType, NL_SLE_ASC_RECONFIGURING);
    if (ret != NL_NO_ERROR) {
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, ret);
    }
    return;
}

bool ASCService::IsInStartProcess(ASCState state) const
{
    // 音频流配置中/音频流打开中/音频流重配置中状态
    return ((state == NL_SLE_ASC_CONFIGURING) || (state == NL_SLE_ASC_CONFIGED) || (state == NL_SLE_ASC_OPENING) ||
        (state == NL_SLE_ASC_OPENED) || (state == NL_SLE_ASC_STARTING) || (state == NL_SLE_ASC_RECONFIGURING) ||
        (state == NL_SLE_ASC_RECONFIGED) || (state == NL_SLE_ASC_RECONFIG_STOPPING) ||
        (state == NL_SLE_ASC_RECONFIG_STOPPED) || (state == NL_SLE_ASC_CONFIG_SUBRATE_CHANGED) ||
        (state == NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED));
}

bool ASCService::IsInConnectedState(ASCState state) const
{
    return (IsConnected(state) || IsInStartProcess(state) || IsStarted(state) ||
        IsInStopProcess(state) || IsReleased(state));
}

inline bool ASCService::IsCallType(AudioStreamType streamType)
{
    return ((streamType == AUDIO_STREAM_VOICE_CALL) || (streamType == AUDIO_STREAM_RING) ||
        (streamType == AUDIO_STREAM_VOIP) || (streamType == AUDIO_STREAM_VOICE_ASSISTANT));
}

void ASCService::CbkReleaseStream(const RawAddress& device, uint8_t result, uint16_t connHandle)
{
    HILOGI("[ASCService]dev:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);

    ASCState state = GetASCStatus(device);
    AudioStreamType streamType = GetProcessingStreamType(device);
    if (IsReconfigStopping(state)) {
        CbkReconfigStopping(device, result, streamType);
        return;
    }

    if (IsNeedDisconnect(device)) {
        int ret = DisconnProcAction(device, state);
        if (ret != NL_NO_ERROR) {
             // 上报状态: 断连接,失败
            ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_SLE_ASC_RESULT_FAIL, ret);
        }
        SetNeedDisconnect(device, false);
    }

    // 通知音频框架星闪链路是否是通话链路
    SetSleVoiceStatusFlag(device);

    if (!IsReleasing(state)) {
        HILOGE("[ASCService]CbkReleaseStream from stack icb %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        // 通知DSP断链的connhandle
        SetAudioDisconnInfo(device, connHandle);
        ClearWhenDisconnect(device);
        // 断连接场景会上报deleteDevice，音频框架会清空状态，不用上报流状态
        return;
    }

    // 结果检查
    if (result != NL_NO_ERROR) {
        HILOGE("[ASCService]CbkReleaseStream callback result %{public}s %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), result);
        // 上报状态: 音频流关闭,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP,
            NL_SLE_ASC_RESULT_FAIL, result);
        return;
    }
    HILOGI("[ASCService] dev:%{public}s, streamT:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    // 状态：已释放音频流
    SetASCStatus(device, NL_SLE_ASC_RELEASED);
    // 从已打开列表删除该流类型
    RemoveStartedStream(device, streamType);

    // 上报状态: 音频流关闭,成功
    ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP,
        NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
    // 清除AutoRate码率
    SetAutoRateBps(device, 0);
    // 删除Qos配置信息
    QosM::GetInstance().ClearQosM(device);
    // 通话 Autorate 还原帧格式为帧1
    RecoverFrameTypeWhenReleaseStream(device);
    // 取出缓存的关闭流任务进行处理
    ProcStopBuff(device, NL_SLE_ASC_RELEASED);

    // 如果有缓存的待打开流，取出缓存任务进行音频流打开处理
    bool isGoOn = false;
    ProcStartBuff(device, NL_SLE_ASC_RELEASED, isGoOn);

    // 如果是之前的出声停播结束，处理新出声设备的起播缓存
    RawAddress reportAddr = GetReportAddr(device);
    if (!(reportAddr == activeSinkDevice_) && (reportAddr == formerSinkDevice_)) {
        ProcAllMemberStartBuff(activeSinkDevice_);
    }
}

void ASCService::SetSleVoiceStatusFlag(const RawAddress& device)
{
    if (IsSleVoiceExisted(device)) {
        HILOGI("[ASCService]Sle voice device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    (void)AudioStandard::AudioSystemClientPolicyManager::GetInstance().SetSleVoiceStatusFlag(false);
}

bool ASCService::IsSleVoiceExisted(const RawAddress& device)
{
    if (!IsTwsVoiceExistedWithCos(device)) {
        // 通话同步链路断连, 通话链路不在星闪
        HILOGD("[ASCService] sle voice link not exist, dev:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }
    if (!GetIsCallingFlag()) {
        // 通话状态标记位为false, 通话链路不在星闪
        HILOGW("[ASCService] calling flag is false, dev:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }
    return true;
}

bool ASCService::IsTwsVoiceExistedWithCos(const RawAddress& device)
{
    Qos cos = QosM::GetInstance().GetCOS(device);
    bool isVoice = IsVoiceCallService(cos);

    // 合作集设备的qos值
    RawAddress coSetDevice {};
    bool isCoExist = IsCoSetDeviceExist(device, coSetDevice);
    if (isCoExist) {
        // 合作集设备存在：两只耳机都为通话Qos才为星闪链路通话
        Qos coCos = QosM::GetInstance().GetCOS(coSetDevice);
        bool isCoVoice = IsVoiceCallService(coCos);
        return isCoVoice && isVoice;
    }
    return isVoice;
}

void ASCService::ProcAllMemberStartBuff(const RawAddress& device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            bool isGoOn = false;
            HILOGI("[ASCService]dev %{public}s", GetEncryptAddr(info.addr_.GetAddress()).c_str());
            ProcStartBuff(info.addr_, GetASCStatus(info.addr_), isGoOn);
        }
    }

    return;
}

void ASCService::CheckAndSetStopDelayingFlag(const RawAddress& device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    bool isStopDelaying = false;
    bool isStartedMemberExist = false;
    const RawAddress* delayingDevice = nullptr;
    for (const auto& info : cdsmList) {
        if (IsStopDelaying(info.addr_)) {
            isStopDelaying = true;
            delayingDevice = &(info.addr_);
        }

        if (IsStarted(GetASCStatus(info.addr_))) {
            isStartedMemberExist = true;
        }
    }

    HILOGI("[ASCService]CheckAndSetStopDelayingFlag %{public}s StopDelaying %{public}d StartedMemberExist %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isStopDelaying, isStartedMemberExist);
    if (isStopDelaying && !isStartedMemberExist) {
        if (delayingDevice != nullptr) {
            // 重置等待断同步链路标记
            DisableStopDelayTimer(*delayingDevice);
            SetStopDelayingFlag(*delayingDevice, false);
        }
    }
}

void ASCService::CbkDisconnect(const RawAddress& device, uint8_t result)
{
    ASCState state = GetASCStatus(device);
    HILOGI("[ASCService]CbkDisconnect in %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        state);

    // 删除Qos配置信息
    QosM::GetInstance().ClearQosM(device);
    // 清除已打开流列表
    std::list<AudioStreamType>& startedStreamList = GetStartedStreamList(device);
    startedStreamList.clear();

    // 清除待打开流队列
    ClearStartBuff(device);
    // 清除待关闭流队列
    ClearStopBuff(device);

    // 清除延迟上线等待标记
    if (IsConnRptDelaying(device)) {
        StopConnRptDelayTimer(device);
        SetConnRptDelayingFlag(device, false);
    }

    // 清除StopDoing标记位
    SetStopDoingFlag(device, false);
    // 清除空间音频配置标记位
    SetSpatialConfiguringFlag(device, false);

    // 从已连接设备列表删除
    DeleteConnectDevices(device);
    // 清除结果记录
    ClearStreamResult(device);
    // 清除同步链路参数
    ClearStreamQosmInfo(device);
    ClearASCSubrateInfo(device);
    // 清除码率
    SetAutoRateBps(device, 0);
    // 清除主动断链的标记位
    SetNeedDisconnect(device, false);

    // 上报状态: 断连接,成功
    // 设置状态:disconnected
    SetASCStatus(device, NL_SLE_ASC_DISCONNECTED);
    ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN,
        NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
    // 重置等待断同步链路标记
    CheckAndSetStopDelayingFlag(device);

    if (GetCoSetNum() == 1) {
        // 同时连接两副星闪耳机，不进4.6M
        ChangeBpsRange();
    }
    SyncWhenDisconnect(device, state);
    RawAddress reportAddr = GetReportAddr(device);
    if (reportAddr == activeSinkDevice_ && !IsConnectedMemberExist(activeSinkDevice_)) {
        RemoveAutorateGroupByAddr(activeSinkDevice_);
        activeSinkDevice_.SetAddress("");
        // 双耳都断联，才清除播放记录
        SetPlayRecord({}, Qos::NL_SLE_QOS_NONE, 0, 0);
    }
}

void ASCService::DisconnectAcb(const RawAddress& device)
{
    HILOGI("[ASCService]DisconnectAcb device %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(sleService, "sleService invalid.");
    sleService->DisconnectAction(device, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
}

static void StackPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop)
{
    const RawAddress& device = RawAddress::ConvertToString(addr->addr);
    HILOGD("[ASCService]StackCallback in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    ASCMessage event(ASC_STACK_PROP_CBK_EVT);
    event.dev_ = device.GetAddress();

    ASCService *service = ASCService::GetService();
    if (service == nullptr) {
        HILOGE("[ASCService]StackPropCbk nullptr %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    ASCUtils::TransferProperty(device, num, prop, event.properties_);

    service->PostEvent(event);
}

static void StackBitrateCbk(NLSTK_ActmBitrateChange_S *bitrate)
{
    if (bitrate == nullptr) {
        HILOGE("[ASCService]StackBitrateCbk bitrate nullptr");
        return;
    }

    ASCMessage event(ASC_BITRATE_CBK_EVT);
    event.ascBitrate_.groupId = bitrate->groupId;
    event.ascBitrate_.direction = bitrate->direction;
    event.ascBitrate_.labelId = bitrate->labelId;
    event.ascBitrate_.downBitrate = bitrate->downBitrate;
    event.ascBitrate_.upBitrate = bitrate->upBitrate;
    event.ascBitrate_.qosIndex = bitrate->qosIndex;
    event.ascBitrate_.qosLevel = bitrate->qosLevel;
    event.ascBitrate_.dutyCycle = bitrate->dutyCycle;
    event.ascBitrate_.availableBitratesCnt = bitrate->availableBitratesCnt;
    for (uint8_t i = 0; i < bitrate->availableBitratesCnt && i < ASC_AVAILABLE_BITRATE_MAX; i++) {
        event.ascBitrate_.availableBitrates[i] = bitrate->availableBitrates[i];
    }

    ASCService *service = ASCService::GetService();
    if (service == nullptr) {
        HILOGE("[ASCService]StackBitrateCbk service nullptr");
        return;
    }

    service->PostEvent(event);
}

static void StackAutoRateMsgCbk(NLSTK_ActmAutoRateSendMsg_S *recvMsg)
{
    if (recvMsg == nullptr || recvMsg->linkCnt == 0) {
        HILOGE("[ASCService]StackAutoRateMsgCbk param nullptr or link unavaliable");
        return;
    }
    HILOGI("[ASCService]recv stack auto rate msg, msgType = %{public}d, direction = %{public}d, result = %{public}d",
        recvMsg->msgType, recvMsg->direction, recvMsg->result);
    ASCMessage event(ASC_STACK_AUTORATE_MSG_CBK);
    event.ascBitrate_.groupId = recvMsg->qosId;
    event.ascBitrate_.direction = recvMsg->direction;
    event.ascBitrate_.labelId = recvMsg->labelId;
    event.ascBitrate_.upBitrate = recvMsg->upwardBitrate;
    event.ascBitrate_.downBitrate = recvMsg->downwardBitrate;
    event.ascBitrate_.qosIndex = recvMsg->qosIndex;

    event.eventType_ = recvMsg->msgType;
    event.result_ = recvMsg->result;
    ASCService *service = ASCService::GetService();
    NL_CHECK_RETURN(service, "[ASCService]StackAutoRateMsgCbk service nullptr");
    service->PostEvent(event);
}

void ASCService::ProcessVoiceCallAutoRateEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    if (event.ascBitrate_.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP) {
        NL_CHECK_RETURN(event.ascBitrate_.downBitrate == BPS_64, "upwardBitrate error.");
    } else if (event.ascBitrate_.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_DOWN) {
        NL_CHECK_RETURN(event.ascBitrate_.downBitrate == BPS_32, "downwardBitrate error.");
    } else {
        HILOGE("bitrate change direction error.");
        return;
    }
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    uint32_t groupId = 0;
    cdsmService->CdsmGetGroupId(groupId, activeSinkDevice_.GetAddress());
    NL_CHECK_RETURN(groupId == event.ascBitrate_.groupId, "group id not match.");

    uint8_t result = event.result_;
    bool ret = UpdateAutorateGroupInfo(event.ascBitrate_);

    NL_CHECK_RETURN(ret, "[ASCService] activeSink device not support long range voice call.");
    switch (event.eventType_) {
        case ASC_CHANGE_FRAME_TYPE_REQ:
            HandleChangeFrameType();
            break;
        case ASC_CHANGE_PEER_BITRATE_REQ:
            HandleChangeVoiceCallBitrate(result);
            break;
        case ASC_CHANGE_LABEL_ID_RSP:
            HandleChangeLabelIdRsp(event.ascBitrate_.direction, result);
            break;
        default:
            break;
    }
}

void ASCService::HandleChangeLabelIdRsp(uint8_t direction, uint8_t result)
{
    HILOGI("actm change label result = %{public}d, direciton = %{public}d", result, direction);

    if (direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP) {
        if (result != NL_SLE_ASC_RESULT_SUCC) { // 通话升码率, 底层同步链路切label id失败, 流程中止，还原帧格式
            HILOGI("level up fail, roll back frame type");
            RollBackPhyParam();
        }
        return;
    }
    // 通话降码率, 无论是否成功，都重置
    RemoveAutorateGroupByAddr(activeSinkDevice_);
}

void ASCService::HandleChangeVoiceCallBitrate(uint8_t result)
{
    uint32_t groupId = GetGroupIdByAddress(activeSinkDevice_);
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN(it != ascAutorateMap_.end(), "[ASCService] ascAutorateMap_ not find group id %{public}d", groupId);
    bool isLevelUp = (it->second.ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP);
    // 通话码率回滚触发的升码率, 底层切label id成功后即流程结束，不需要通知dsp切码率
    if (isLevelUp && result != NL_SLE_ASC_RESULT_SUCC) {
        HILOGI("actm roll back, no need change dsp bitrate");
        RemoveAutorateGroupByAddr(activeSinkDevice_);
    }
    for (const AscPhyStatus& item : it->second.phyStatusList) {
        RawAddress dev = RawAddress(item.addr);
        if (!IsStarted(GetASCStatus(dev)) || !IsConnected(dev)) {
            HILOGI("device %{public}s not started or disconnected, ignore.", GET_ENCRYPT_ADDR(dev));
            continue;
        }
        uint64_t targetBpsRange = ASCUtils::GetAutorateTargetBpsRange(it->second.ascBitrate.downBitrate);
        const AscStreamInfo& streamInfo = GetStreamInfo(dev, NLSTK_ACTM_ALL_POINT);
        int ret = SleASC::UpdateBitRate(dev, targetBpsRange, streamInfo.streamId);
        if (ret != NL_NO_ERROR) {
            HILOGE("[ASCService]ChangeBpsRange error, ret %{public}d", ret);
            continue;
        }
        HILOGI("device:%{public}s in group %{public}d isLevelUp:%{public}d targetBpsRange: %{public}d",
            GET_ENCRYPT_ADDR(dev), groupId, isLevelUp, targetBpsRange);
    }
}

uint8_t ASCService::GetExistPhyStauts(uint32_t groupId, std::string addr)
{
    auto it = ascAutorateMap_.find(groupId);
    if (it == ascAutorateMap_.end()) {
        return NL_SLE_ASC_INIT;
    }
    for (const AscPhyStatus& item : it->second.phyStatusList) {
        if (item.addr == addr) {
            return item.frameChangeState;
        }
    }
    return NL_SLE_ASC_INIT;
}

bool ASCService::UpdateAutorateGroupInfo(const AscBitrateChange &ascBitrate)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(activeSinkDevice_, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");

    AscAutorateInfo autorateItem;
    bool isExist = (ascAutorateMap_.find(ascBitrate.groupId) != ascAutorateMap_.end());
    for (const auto& info : cdsmList) {
        RawAddress dev = RawAddress(info.addr_);
        if (!IsVoiceCallService(static_cast<Qos>(ascBitrate.qosIndex)) || !GetLongRangeVoiceCallAbility(dev)) {
            HILOGE("device %{public}s in group %{public}d, qos = %{public}d error", GET_ENCRYPT_ADDR(dev),
                ascBitrate.groupId, ascBitrate.qosIndex);
            RemoveAutorateGroupByAddr(dev);
            return false;
        }
        AscPhyStatus phyStatus;
        phyStatus.addr = info.addr_.GetAddress();
        phyStatus.frameChangeState = GetExistPhyStauts(ascBitrate.groupId, info.addr_.GetAddress());
        autorateItem.phyStatusList.emplace_back(phyStatus);
    }
    autorateItem.changeDSPBitState = isExist ? ascAutorateMap_[ascBitrate.groupId].changeDSPBitState : 0;
    autorateItem.isGroupFrameChangeSucc = isExist ? ascAutorateMap_[ascBitrate.groupId].isGroupFrameChangeSucc : false;
    autorateItem.ascBitrate = ascBitrate;
    ascAutorateMap_[ascBitrate.groupId] = autorateItem;
    HILOGI("%{public}s group id %{public}d", isExist ? "Update" : "Add", ascBitrate.groupId);
    return true;
}

uint32_t ASCService::GetGroupIdByAddress(const RawAddress &device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, CDSM_SERVICE_INVALID_GROUP_ID, "cdsmService nullptr.");
    uint32_t groupId = CDSM_SERVICE_INVALID_GROUP_ID;
    cdsmService->CdsmGetGroupId(groupId, device.GetAddress());
    return groupId;
}

void ASCService::RemoveAutorateGroupByAddr(const RawAddress &device)
{
    uint32_t groupId = GetGroupIdByAddress(device);
    if (ascAutorateMap_.find(groupId) != ascAutorateMap_.end()) {
        ascAutorateMap_.erase(groupId);
        HILOGI("Delete group id %{public}d", groupId);
    }
}

bool ASCService::GetVoiceCallAcbStatus(const RawAddress &device, uint8_t& frameType, uint8_t& phyType)
{
    auto it = ascStatusMap_.find(device.GetAddress());
    if (it != ascStatusMap_.end()) {
        frameType = it->second.multicastFrameType;
        phyType = it->second.multicastPhyType;
        HILOGI("[ASCService]GetVoiceCallAcbStatus %{public}s frameType %{public}d, phyType %{public}d",
            GET_ENCRYPT_ADDR(device), frameType, phyType);
        return true;
    }
    HILOGD("[ASCService]GetVoiceCallAcbStatus NO item %{public}s", GET_ENCRYPT_ADDR(device));
    return false;
}

void ASCService::SetVoiceCallAcbStatus(const RawAddress &device, uint8_t frameType, uint8_t phyType)
{
    HILOGI("[ASCService]SetVoiceCallAcbStatus in %{public}s frameType %{public}d, phyType %{public}d",
        GET_ENCRYPT_ADDR(device), frameType, phyType);
    auto it = ascStatusMap_.find(device.GetAddress());
    if (it == ascStatusMap_.end()) {
        HILOGE("[ASCService]SetVoiceCallAcbStatus NO item %{public}s in deviceAscStatusMap", GET_ENCRYPT_ADDR(device));
        return;
    }
    it->second.multicastFrameType = frameType;
    it->second.multicastPhyType = phyType;
}

void ASCService::RecoverFrameTypeWhenReleaseStream(const RawAddress &device)
{
    auto it = ascStatusMap_.find(device.GetAddress());
    NL_CHECK_RETURN(it != ascStatusMap_.end() && GetLongRangeVoiceCallAbility(device),
        "[ASCService] NO item %{public}s in AscStatusMap or not support frame4 voice call", GET_ENCRYPT_ADDR(device));
    uint8_t multicastFrameType = 0;
    uint8_t multicastPhyType = 0;
    if (GetVoiceCallAcbStatus(device, multicastFrameType, multicastPhyType) &&
        multicastFrameType == CM_RADIO_FRAME_TYPE_4_M0 && multicastPhyType == CM_PHY_TYPE_2M) {
        HILOGI("%{public}s voice call acb in frame4, switch to frame1", GET_ENCRYPT_ADDR(device));
        auto adapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(adapter, "[ASCService]sleAdapter is null.");
        adapter->SetPhy(device, CM_RADIO_FRAME_TYPE_1, CM_PHY_TYPE_1M);
    }
}

void ASCService::HandleChangeFrameType()
{
    auto adapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(adapter, "[ASCService]sleAdapter is null.");
    uint32_t groupId = GetGroupIdByAddress(activeSinkDevice_);
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN(it != ascAutorateMap_.end(), "[ASCService] not find group id %{public}d", groupId);

    bool isLevelUp = (it->second.ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP);
    for (AscPhyStatus& item : it->second.phyStatusList) {
        RawAddress dev = RawAddress(item.addr);
        if (!IsStarted(GetASCStatus(dev)) || !IsConnected(dev)) {
            HILOGI("device %{public}s not started or disconnected, ignore.", GET_ENCRYPT_ADDR(dev));
            continue;
        }
        if (item.frameChangeState == NL_SLE_ASC_INIT) {
            uint8_t targetFrameType = ASCUtils::GetAutorateTargetFrameType(isLevelUp);
            uint8_t targetPhyType = ASCUtils::GetAutorateTargetPhyType(isLevelUp);
            HILOGI("device %{public}s set frameType %{public}d, phyType %{public}d",
                GET_ENCRYPT_ADDR(dev), targetFrameType, targetPhyType);
            SetVoiceCallAcbStatus(dev, targetFrameType, targetPhyType);
            adapter->SetPhy(dev, targetFrameType, targetPhyType);
            item.frameChangeState = NL_SLE_ASC_SETTING;
            return;
        }
    }
}

void ASCService::RollBackPhyParam()
{
    auto adapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(adapter, "[ASCService]sleAdapter is null.");
    uint32_t groupId = GetGroupIdByAddress(activeSinkDevice_);
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN(it != ascAutorateMap_.end(), "[ASCService] not find group id %{public}d", groupId);

    bool isLevelUp = (it->second.ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP);
    for (const AscPhyStatus& item : it->second.phyStatusList) {
        RawAddress dev = RawAddress(item.addr);
        if (item.frameChangeState != NL_SLE_ASC_SETTED) {
            continue;
        }
        uint8_t targetFrameType = ASCUtils::GetAutorateTargetFrameType(!isLevelUp);
        uint8_t targetPhyType = ASCUtils::GetAutorateTargetPhyType(!isLevelUp);
        HILOGI("device %{public}s roll back frameType %{public}d, phyType %{public}d",
            GET_ENCRYPT_ADDR(dev), targetFrameType, targetPhyType);
        adapter->SetPhy(dev, targetFrameType, targetPhyType);
    }
}

void ASCService::PhyChanged(RawAddress device, uint8_t frameType, uint8_t phyType, uint8_t status)
{
    HILOGD("PhyChanged %{public}s status: %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), status);
    ASCMessage event(ASC_FRAME_CHANGED_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = status;
    event.frameType_ = frameType;
    event.phyType_ = phyType;
    PostEvent(event);
}

bool ASCService::IsAllCosetFrameTypeChangedSucc(uint32_t groupId)
{
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN_RET(it != ascAutorateMap_.end(), false, "[ASCService] not find group id %{public}d", groupId);

    for (const AscPhyStatus& item : it->second.phyStatusList) {
        RawAddress dev = RawAddress(item.addr);
        if (!IsStarted(GetASCStatus(dev)) || !IsConnected(dev)) {
            continue;
        }
        if (item.frameChangeState != NL_SLE_ASC_SETTED) {
            HILOGI("groupId %{public}d has not changed all member frameType", groupId);
            return false;
        }
    }
    HILOGI("groupId %{public}d has changed all member frameType", groupId);
    return true;
}

void ASCService::SetAutoRateRecvMsg(AscBitrateChange bitchange, NLSTK_ActmAutoRateRecvMsg_S& msg)
{
    msg.qosId = bitchange.groupId;
    msg.labelId = bitchange.labelId;
    msg.qosIndex = bitchange.qosIndex;
}

void ASCService::ProcessFrameTypeChangedEvent(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    uint32_t groupId = GetGroupIdByAddress(device);
    auto it = ascAutorateMap_.find(groupId);
    if (it == ascAutorateMap_.end()) {
        HILOGI("group id %{public}d not in ascAutorateMap_, ignore", groupId);
        return;
    }
    NL_CHECK_RETURN(!it->second.isGroupFrameChangeSucc, "group id %{public}d change frame done, ignore", groupId);

    uint8_t result = event.result_;
    uint8_t frameType = event.frameType_;
    uint8_t phyType = event.phyType_;

    NLSTK_ActmAutoRateRecvMsg_S msg;
    SetAutoRateRecvMsg(it->second.ascBitrate, msg);

    uint8_t direction = it->second.ascBitrate.direction;
    if (result != NL_SLE_ASC_RESULT_SUCC) {
        msg.msgType = ASC_CHANGE_FRAME_TYPE_RSP;
        msg.result = NL_SLE_ASC_RESULT_FAIL;
        RollBackPhyParam();
        RemoveAutorateGroupByAddr(device);
        int ret = SleASC::SendVoiceCallAutoRateMsg(device, msg);
        NL_CHECK_RETURN(ret == NL_NO_ERROR, "[ASCService]SendVoiceCallAutoRateMsg err, ret %{public}d", ret);
        HILOGI("device: %{public}s SetPhy fail", GET_ENCRYPT_ADDR(RawAddress(event.dev_)));
        return;
    }

    bool isLevelUp = (it->second.ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP);
    UpdatePhyStatus(it->second.phyStatusList, isLevelUp, event.dev_, frameType, phyType);
    it->second.isGroupFrameChangeSucc = IsAllCosetFrameTypeChangedSucc(groupId);
    if (!it->second.isGroupFrameChangeSucc) {
        HILOGI("device: %{public}s in group %{public}d continue SetPhy", GetEncryptAddr(event.dev_).c_str(), groupId);
        HandleChangeFrameType();
        return;
    }
    // 双耳异步链路成功切帧格式
    if (isLevelUp) {  // 通话升码率: 通知Actm切同步链路 lable id
        msg.msgType = ASC_CHANGE_FRAME_TYPE_RSP;
        msg.result = NL_SLE_ASC_RESULT_SUCC;
        HILOGI("device: %{public}s in group %{public}d all SetPhy Succ, notify actm change label id (level up)",
            GetEncryptAddr(event.dev_).c_str(), groupId);
        int ret = SleASC::SendVoiceCallAutoRateMsg(device, msg);
        NL_CHECK_RETURN(ret == NL_NO_ERROR, "[ASCService]SendVoiceCallAutoRateMsg err, ret %{public}d", ret);
    } else if (IsExcuteChangeLocalBitrateNow(groupId)) { // 通话降码率: 满足条件后通知手机dsp降码率
        HILOGI("device: %{public}s in group %{public}d all SetPhy Succ, notify local dsp change bit (level down)",
            GetEncryptAddr(event.dev_).c_str(), groupId);
        UpdateLocalDspBitrate(it->second.ascBitrate);
    }
}

void ASCService::UpdatePhyStatus(std::vector<AscPhyStatus>& phyStatusList, bool isLevelUp, std::string dev,
    uint8_t retFrameType, uint8_t retPhyType)
{
    for (AscPhyStatus& item : phyStatusList) {
        if (item.addr != dev) {
            continue;
        }
        if (item.frameChangeState == NL_SLE_ASC_SETTING &&
            ASCUtils::GetAutorateTargetFrameType(isLevelUp) == retFrameType &&
            ASCUtils::GetAutorateTargetPhyType(isLevelUp) == retPhyType) {
            item.frameChangeState = NL_SLE_ASC_SETTED;
            HILOGI("device %{public}s SetPhy Succ", GetEncryptAddr(dev).c_str());
        }
    }
}

void ASCService::CbkBitrateChanged(const RawAddress &device, uint8_t result)
{
    uint32_t groupId = GetGroupIdByAddress(device);
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN(it != ascAutorateMap_.end(), "[ASCService] not find group id %{public}d", groupId);
    HILOGI("[ASCService] device %{public}s bitrate changed reuslt %{public}d", GET_ENCRYPT_ADDR(device), result);

    if (!IsRolePrimary(device)) {
        HILOGI("device %{public}s is not isPrimary role, ignore", GET_ENCRYPT_ADDR(device));
        return;
    }
    NLSTK_ActmAutoRateRecvMsg_S msg;
    SetAutoRateRecvMsg(it->second.ascBitrate, msg);
    msg.msgType = ASC_CHANGE_PEER_BITRATE_RSP;
    msg.result = result == NL_SLE_ASC_RESULT_SUCC ? NL_SLE_ASC_RESULT_SUCC : NL_SLE_ASC_RESULT_FAIL;
    int ret = SleASC::SendVoiceCallAutoRateMsg(device, msg);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "[ASCService]SendVoiceCallAutoRateMsg err, ret %{public}d", ret);
    if (result != NL_SLE_ASC_RESULT_SUCC) { // 对端切码率失败
        RemoveAutorateGroupByAddr(device);
        return;
    }
    it->second.changeDSPBitState |= NL_SLE_ASC_VOICE_CALL_PEER_BITRATE_CHANED;
    // 对端降码率完成，本端切acb帧格式；升码率，本端acb帧格式已切成功
    if (!it->second.isGroupFrameChangeSucc) {
        HandleChangeFrameType();
        return;
    }
    bool isExcuteNow = IsExcuteChangeLocalBitrateNow(groupId);
    if (isExcuteNow) {
        HILOGI("group %{public}d notify local dsp change bit now", groupId);
        UpdateLocalDspBitrate(it->second.ascBitrate);
    }
}

void ASCService::ProcessChangeBitrateEvent(const ASCMessage &event)
{
    HILOGD("[ASCService] event(%{public}d)", event.whatM);
    AscBitrateChange ascBitrate = event.ascBitrate_;
    uint32_t groupId = GetGroupIdByAddress(activeSinkDevice_);
    auto it = ascAutorateMap_.find(groupId);

    bool isExcuteNow = true;
    if (it != ascAutorateMap_.end() && ascBitrate.labelId != ASC_INVALID_LABLE_ID) {
        it->second.ascBitrate = ascBitrate;
        it->second.changeDSPBitState |= NL_SLE_ASC_VOICE_CALL_LOCAL_BITRATE_NTF;
        isExcuteNow = IsExcuteChangeLocalBitrateNow(groupId);
    }
    if (isExcuteNow) {
        UpdateLocalDspBitrate(ascBitrate);
    }
}

bool ASCService::IsExcuteChangeLocalBitrateNow(uint32_t groupId)
{
    auto it = ascAutorateMap_.find(groupId);
    NL_CHECK_RETURN_RET(it != ascAutorateMap_.end(), true, "[ASCService] not find group id %{public}d", groupId);

    bool isLevelUp = (it->second.ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP);
    bool canExcuteNow = false;
    if (isLevelUp) {
        canExcuteNow = it->second.changeDSPBitState == (NL_SLE_ASC_VOICE_CALL_LOCAL_BITRATE_NTF |
            NL_SLE_ASC_VOICE_CALL_PEER_BITRATE_CHANED);
        HILOGI("group %{public}d level up can excute now : %{public}d", groupId, canExcuteNow);
    } else {
        bool isAllSetPhyDone = it->second.isGroupFrameChangeSucc;
        canExcuteNow = (it->second.changeDSPBitState == (NL_SLE_ASC_VOICE_CALL_LOCAL_BITRATE_NTF |
            NL_SLE_ASC_VOICE_CALL_PEER_BITRATE_CHANED)) && isAllSetPhyDone;
        HILOGI("group %{public}d level down can excute now : %{public}d", groupId, canExcuteNow);
    }
    return canExcuteNow;
}

void ASCService::UpdateLocalDspBitrate(const AscBitrateChange& ascBitrate)
{

    // 配置DSP AutoRate参数;
    SetAutorateParameter(ascBitrate);
    // 保存码率，用于单切双起播码率同步
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(activeSinkDevice_, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        if (info.state_ == static_cast<uint8_t>(CdsmConnectState::CONNECTED) && IsStarted(GetASCStatus(info.addr_))) {
            SetAutoRateBps(info.addr_, ascBitrate.downBitrate);
        }
    }
    // 通话升码率，通知dsp则流程结束
    QosM& qosM = QosM::GetInstance();
    Qos qos = qosM.GetCOS(activeSinkDevice_);
    if (IsVoiceCallService(qos) && ascBitrate.direction == NL_SLE_ASC_VOICE_CALL_BITRATE_UP) {
        ascAutorateMap_.erase(ascBitrate.groupId);
        HILOGI("level up done, remove group %{public}d from ascAutorateMap_", ascBitrate.groupId);
    }
}

void ASCService::ProcessStackCbkProp(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    CbkGetProperty(device, event.properties_);
    return;
}

static void StackEventCbk(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void* para)
{
    const RawAddress& device = RawAddress::ConvertToString(addr->addr);
    HILOGD("[ASCService]StackCallback in %{public}s result %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), result);

    ASCMessage event(ASC_STACK_EVENT_CBK_EVT);
    event.dev_ = device.GetAddress();
    event.eventType_ = eventType;
    event.result_ = result;

    if (para != nullptr) {
        if ((eventType == ASC_STACK_CBK_OPEN_STREAM) || (eventType == ASC_STACK_CBK_START_STREAM)) {
            NLSTK_ActmQosmInfo_S *info = static_cast<NLSTK_ActmQosmInfo_S *>(para);
            ASCUtils::SetQosmInfo(*info, event.qosmInfo_);
        }

        if ((eventType == ASC_STACK_CBK_STOP_STREAM) || (eventType == ASC_STACK_CBK_RELEASE_STREAM)) {
            if (result == NL_NO_ERROR) {
                event.connHandle_ = *(static_cast<uint16_t *>(para));
            }
        }

        if (eventType == ASC_STACK_CBK_CREATE_STREAM) {
            if (result == NL_NO_ERROR) {
                NLSTK_ActmStreamInfo_S *info = static_cast<NLSTK_ActmStreamInfo_S *>(para);
                event.ascStreamInfo_.streamId = info->streamId;
                event.ascStreamInfo_.pointType = info->pointType;
                event.ascStreamInfo_.commType = info->commType;
                ServiceManagerPluginLoader::GetInstance()->UpdateSleFreqBandAbility(device.GetAddress());
            }
        }
    }

    ASCService *service = ASCService::GetService();
    if (service == nullptr) {
        HILOGE("[ASCService]StackEventCbk nullptr %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    service->PostEvent(event);
    return;
}

void ASCService::ProcessStackCbkEvent(const ASCMessage &event)
{
    HILOGI("[ASCService], event(%{public}d), Type(%{public}d)", event.whatM, event.eventType_);
    RawAddress device(event.dev_);
    uint8_t result = event.result_;
    switch (event.eventType_) {
        case ASC_STACK_CBK_READ_PROP_FAIL:
            CbkReadPropFail(device, result);
            break;
        case ASC_STACK_CBK_OPEN_STREAM:
            CbkOpenStream(device, result, event.qosmInfo_);
            break;
        case ASC_STACK_CBK_CFG_STREAM:
            CbkConfigStream(device, result);
            break;
        case ASC_STACK_CBK_START_STREAM:
            CbkStartStream(device, result, event.qosmInfo_);
            break;
        case ASC_STACK_CBK_STOP_STREAM:
            CbkStopStream(device, result, event.connHandle_);
            break;
        case ASC_STACK_CBK_RELEASE_STREAM:
            CbkReleaseStream(device, result, event.connHandle_);
            break;
        case ASC_STACK_CBK_DISCONNECT:
            CbkDisconnect(device, result);
            break;
        case ASC_STACK_CBK_BITRATE_CHANGED:
            CbkBitrateChanged(device, result);
            break;
        case ASC_STACK_CBK_CREATE_STREAM:
            CbkCreateStream(device, result, event.ascStreamInfo_);
            break;
        default:
            break;
    }
    return;
}

static void StackLocationChangeCbk(SLE_Addr_S *addr, bool isLeft)
{
    const RawAddress& device = RawAddress::ConvertToString(addr->addr);
    HILOGD("[ASCService]StackLocationChangeCbk in %{public}s isLeft: %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isLeft);

    ASCService *service = ASCService::GetService();
    if (service == nullptr) {
        HILOGE("[ASCService]StackLocationChangeCbk nullptr %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    ASCMessage event(ASC_STACK_LOCATION_CBK_EVT);
    event.dev_ = device.GetAddress();
    event.isLeft_ = isLeft;
    service->PostEvent(event);
}

static void StackStreamTypeChangedCbk(SLE_Addr_S *stackAddr, uint32_t availableStreamType)
{
    NL_CHECK_RETURN(stackAddr != nullptr, "[ASCService]StackStreamTypeChangedCbk stackAddr is null.");
    const RawAddress& device = RawAddress::ConvertToString(stackAddr->addr);

    ASCService *service = ASCService::GetService();
    NL_CHECK_RETURN(service != nullptr, "[ASCService]StackStreamTypeChangedCbk nullptr %{public}s",
        GetEncryptAddr(device.GetAddress()).c_str());

    ASCMessage event(ASC_STACK_STREAM_TYPE_CBK_EVT);
    event.dev_ = device.GetAddress();
    event.availableStreamType_ = availableStreamType;
    service->PostEvent(event);
}

void ASCService::Init()
{
    InitDisconnProcTable();
    // 向stack注册回调
    NLSTK_ActmCbk_S cbk = {StackEventCbk, StackPropCbk, StackBitrateCbk, StackLocationChangeCbk,
        StackStreamTypeChangedCbk, StackAutoRateMsgCbk};
    uint32_t ret = NLSTK_ActmRegisterCallback(&cbk);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGD("[ASCService]Init ret %{public}d", ret);
        return;
    }
    HILOGI("[ASCService]Init OK");
}

int ASCService::AudioControl(const RawAddress &device, AudioStreamType streamType, int cmd)
{
    HILOGI("[ASCService]AudioControl in %{public}s streamType %{public}d cmd %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, cmd);

    if (streamType > AUDIO_STREAM_SING) {
        HILOGE("[ASCService]AudioControl streamType error %{public}s streamType %{public}d cmd %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, cmd);
        return NL_SLE_ASC_ERROR_STREAMTYPE_ERROR;
    }
    CheckStreamIsNeedNotifyCcp(device, streamType, cmd);

    switch (cmd) {
        case NL_SLE_ASC_CONTROL_CMD_START:
            {
                ASCMessage event(ASC_START_PLAYING_EVT);
                event.dev_ = device.GetAddress();
                event.streamType_ = streamType;
                PostEvent(event);
                break;
            }
        case NL_SLE_ASC_CONTROL_CMD_STOP:
            {
                ASCMessage event(ASC_STOP_PLAYING_EVT);
                event.dev_ = device.GetAddress();
                event.streamType_ = streamType;
                PostEvent(event);
                break;
            }
        default:
            break;
    }

    HILOGD("[ASCService]AudioControl Start");
    return NL_NO_ERROR;
}

void ASCService::RemoveStartedStream(const RawAddress &device, AudioStreamType streamType)
{
    std::list<AudioStreamType>& listStream = GetStartedStreamList(device);
    for (auto it = listStream.begin(); it != listStream.end();) {
        if (*it == streamType) {
            it = listStream.erase(it);
            break;
        } else {
            ++it;
        }
    }

    HILOGI("[ASCService]RemoveStartedStream %{public}s remove streamType %{public}d size %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType, listStream.size());
}

void ASCService::SendPlayOrPauseByWearDetection(const RawAddress &devAddr, uint8_t playOrPauseKey)
{
    HILOGI("[ASCService Service] %{public}s playOrPauseKey: %{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), playOrPauseKey);
    ASCMessage event(ASC_SEND_PLAY_OR_PAUSE_EVT);
    event.dev_ = devAddr.GetAddress();
    event.result_ = playOrPauseKey;
    PostEvent(event);
}

void ASCService::SendEventByWearDetection(const RawAddress &device, uint8_t playOrPauseKey)
{
    HILOGI("[ASCService] send play or pause, playOrPauseKey(%{public}d)", playOrPauseKey);
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->SendKeyEventByWearDetection(device, playOrPauseKey);
}

void ASCService::SendPlayOrPauseIfNeed(const RawAddress &device)
{
    uint8_t playOrPauseKey = 0;
    if (!playOrPauseDevice_.GetValue(device.GetAddress(), playOrPauseKey)) {
        HILOGD("[ASCService] not need send play or pause");
        return;
    }

    if (playOrPauseKey == 0) {  // 0 is not need send play or pause
        HILOGD("[ASCService] not need send play or pause");
        return;
    }
    SendEventByWearDetection(device, playOrPauseKey);
    playOrPauseDevice_.Erase(device.GetAddress());
}

void ASCService::ProcessSendPlayOrPauseEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    uint8_t playOrPauseKey = event.result_;

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    for (const auto& info : cdsmList) {
        if (IsInConnectedState(GetASCStatus(info.addr_))) {
            SendEventByWearDetection(info.addr_, playOrPauseKey);
            return;
        }
    }
    HILOGI("[ASCService] %{public}s set playOrPauseKey(%{public}d)",
        GetEncryptAddr(device.GetAddress()).c_str(), playOrPauseKey);
    playOrPauseDevice_.EnsureInsert(device.GetAddress(), playOrPauseKey);
}

void ASCService::SyncWhenStartPlaying(bool isSync, const RawAddress* startedDevice, const RawAddress* idleDevice)
{
    if (isSync && (startedDevice != nullptr) && (idleDevice != nullptr)) {
        if (IsStarted(GetASCStatus(*startedDevice))) {
            SyncWhenConnect(*idleDevice);
        }
    }
}

void ASCService::ProcessStartPlayingEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    AudioStreamType streamType = static_cast<AudioStreamType>(event.streamType_);

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    bool isStopDelaying = false;
    AudioStreamType stopDelayingStreamType = AUDIO_STREAM_NONE;
    // 初始化结果记录状态信息
    bool isStartedMemberExist = false;
    bool isIdleMemberExist = false;
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            SetStartPlayingResult(info.addr_, streamType, false, NL_NO_ERROR);
        }

        RawAddress reportAddr = GetReportAddr(info.addr_);
        if (IsStopDelaying(reportAddr)) {
            isStopDelaying = true;
            std::list<AudioStreamType>& listStarted = GetStartedStreamList(info.addr_);
            if (!(listStarted.empty())) {
                stopDelayingStreamType = listStarted.front();
            }
        }

        ASCState state = GetASCStatus(info.addr_);
        isStartedMemberExist = isStartedMemberExist || IsStarted(state);
        isIdleMemberExist = isIdleMemberExist || (IsCreated(state) || IsReleased(state));
    }

    bool isSync = isStartedMemberExist && isIdleMemberExist;
    const RawAddress* startedDevice = nullptr;
    const RawAddress* idleDevice = nullptr;
    for (const auto& info : cdsmList) {
        // 对于合作集设备，给组内所有成员（包括当前设备）发起播
        if (IsConnected(info.addr_)) {
            ASCState state = GetASCStatus(info.addr_);
            // 双耳一个播放中一个空闲的场景，空闲耳机等播放中耳机完成本次起播后走单切双
            if (isSync && (IsCreated(state) || IsReleased(state))) {
                idleDevice = &(info.addr_);
                continue;
            }

            if (isSync && IsStarted(state)) {
                startedDevice = &(info.addr_);
            }

            StartPlayingExcute(info.addr_, streamType, isStopDelaying, stopDelayingStreamType);
        }
    }

    // 双耳一个播放中一个空闲的场景，播放中耳机本次起播不重配置的话，空闲耳机单切双
    SyncWhenStartPlaying(isSync, startedDevice, idleDevice);
    return;
}

void ASCService::StartPlayingExcute(const RawAddress& device, AudioStreamType streamType, bool isStopDelaying,
    AudioStreamType stopDelayingStreamType)
{
    // 启动定时器
    EnableStartPlayingTimer(device);

    int ret = StartPlaying(device, streamType, false);
    if (ret != NL_NO_ERROR) {
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, ret);
    } else {
        // 如果是起新流,两个设备都需要删除等待延迟断链路的流
        if (isStopDelaying) {
            RemoveStartedStream(device, stopDelayingStreamType);
        }

        // 主设备停定时器并清延迟断链路标记位
        RawAddress reportAddr = GetReportAddr(device);
        CancelStopDelay(reportAddr);
    }
}

void ASCService::StopPlayingWhenOtherStreamExist(const RawAddress& device, AudioStreamType streamType,
    bool isNeedReconfig, Qos cos)
{
    HILOGI("[ASCService]StopPlaying other stream exist %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);

    RemoveStartedStream(device, streamType);
    // 上报状态: 音频流关闭,成功
    ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP,
        NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);

    // Qos回退
    std::list<AudioStreamType>& listStream = GetStartedStreamList(device);
    if (!listStream.empty() && isNeedReconfig) {
        AudioStreamType streamTypeToReconfig = listStream.front();
        HILOGI("[ASCService]StopPlaying Need Reconfig %{public}s streamType to Reconfig %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamTypeToReconfig);

        // 多流场景在流释放过程中: 为保证反复进退和起停游戏的体验(不卡顿), 不能重切qos
        ReleaseStartedStream(device, streamType, streamTypeToReconfig, cos);
    }
}

void ASCService::ReleaseStartedStream(const RawAddress& device, AudioStreamType streamType,
    AudioStreamType streamTypeToReconfig, Qos cos)
{
    // 此处反复修改过几次, 以该实现为准, 多流释放过程不重新切qos
    // 保存待重配置的流类型
    SetReconfigStream(device, streamTypeToReconfig);
    // 从已打开流列表删除，重建链路成功后在startStream回调里面会再添加进来
    RemoveStartedStream(device, streamTypeToReconfig);
    DeleteCommLink(device, streamType, cos, NL_SLE_ASC_RECONFIG_STOPPING);
}

void ASCService::ReconfigStream(const RawAddress& device, AudioStreamType streamType,
    AudioStreamType streamTypeToReconfig, Qos cos)
{
    ReleaseStartedStream(device, streamType, streamTypeToReconfig, cos);

    // 更新QOS的值：从gosList里面取出优先级最高的cos设置下去
    Qos nos = QosM::GetInstance().CalculateNos(device);
    QosM::GetInstance().SetCos(device, nos);
    HILOGI("[ASCService]Reconfig %{public}s streamType %{public}d COS %{public}d to NOS %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamTypeToReconfig, cos, nos);
}

void ASCService::StopPlayingExcute(const RawAddress& device, AudioStreamType streamType)
{
    HILOGI("[ASCService]StopPlayingExcute %{public}s streamType %{public}d stop ok.",
        GetEncryptAddr(device.GetAddress()).c_str(), streamType);

    // 启动定时器
    EnableStopPlayingTimer(device);
    // 设置标记位
    SetStopDoingFlag(device, true);
    int ret = StopPlaying(device, streamType);
    if (ret != NL_NO_ERROR) {
        // 上报状态: 音频流关闭,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP,
            NL_SLE_ASC_RESULT_FAIL, ret);
    }
}

void ASCService::JudgeQosWhenStopPlaying(const RawAddress& device, AudioStreamType streamType,
    bool& isDelayStop, bool& isStopStream, bool& isNeedReconfig)
{
    ASCState state = GetASCStatus(device);
    if (IsStarted(state) && !IsStreamExists(device, streamType) && !IsColAudioStreamExist(device)) {
        HILOGI("[ASCService]StopPlaying %{public}s streamType %{public}d Not start %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType, state);
        return;
    }

    Qos cos = QosM::GetInstance().GetCOS(device);
    // 删除Qos
    bool isStopImmediately = false;
    isNeedReconfig = QosM::GetInstance().DeleteQos(device, GetStreamQos(device, streamType), isStopStream,
        isStopImmediately);

    HILOGI("[ASCService]StopPlaying %{public}s isStopStream %{public}d isStopImmediately %{public}d "
        "isNeedReconfig %{public}d streamType:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), isStopStream,
        isStopImmediately, isNeedReconfig, streamType);

    // 检查已打开流列表，还有其他流的话不停止流，只是从列表删除本流
    if ((GetStartedStreamList(device).size() > 1) || !isStopStream) {
        StopPlayingWhenOtherStreamExist(device, streamType, isNeedReconfig, cos);
    }

    // 前出声设备的流立即停止
    RawAddress reportAddr = GetReportAddr(device);
    if (isStopStream && (!(reportAddr == activeSinkDevice_))) {
        isStopImmediately = true;
    }

    // 断同步链路
    if (isStopStream && isStopImmediately) {
        StopPlayingExcute(device, streamType);
    }

    // 是否延迟释放
    if (isStopStream && !isStopImmediately) {
        isDelayStop = true;
    }
}

void ASCService::DelayStop(const RawAddress& device, AudioStreamType streamType)
{
    // 启动定时器，延迟断同步链路
    RawAddress reportAddr = GetReportAddr(device);
    if (!IsStopDelaying(reportAddr)) {
        EnableStopDelayTimer(reportAddr);
        SetStopDelayingFlag(reportAddr, true);
    }
    // 上报状态: 音频流关闭,成功
    ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP, NL_SLE_ASC_RESULT_SUCC, NL_NO_ERROR);
}

void ASCService::ProcessStopPlayingEvent(const ASCMessage &event)
{
    HILOGI("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    AudioStreamType streamType = static_cast<AudioStreamType>(event.streamType_);

    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    // 判断是否需要停语音助手
    CloseVoiceAssistant(device, streamType);

    // 初始化结果记录状态信息
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            SetStopPlayingResult(info.addr_, streamType, false, NL_NO_ERROR);
        }
    }

    bool isDelayStop = false;
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            // 音频流停止中/释放中状态，缓存任务
            ASCState state = GetASCStatus(info.addr_);
            if (IsConnected(state) || IsCreated(state)) {
                HILOGI("[ASCService]StopPlaying Not Playing %{public}s streamType %{public}d state %{public}d",
                    GetEncryptAddr(info.addr_.GetAddress()).c_str(), streamType, state);
                continue;
            }

            if (IsInStopProcess(state) || IsInStartProcess(state)) {
                HILOGI("[ASCService]StopPlaying Need Cache %{public}s streamType %{public}d state %{public}d",
                    GetEncryptAddr(info.addr_.GetAddress()).c_str(), streamType, state);
                // 缓存本次配置，在进行中的音频流停止/释放/流配置/流打开流程结束后，再触发更改状态或者触发停止/释放流程
                GetStopBuff(info.addr_).push(streamType);
                continue;
            }

            bool isStopStream = false;
            bool isNeedReconfig = false;
            JudgeQosWhenStopPlaying(info.addr_, streamType, isDelayStop, isStopStream, isNeedReconfig);
        }
    }

    if (isDelayStop) {
        DelayStop(device, streamType);
    }
}

int ASCService::GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices)
{
    std::map<std::string, RawAddress> reportAddrMap {};
    GetAllReportAddr(reportAddrMap);
    for (const auto& pair : reportAddrMap) {
        HILOGI("GetAudioDeviceList %{public}s", GetEncryptAddr(pair.second.GetAddress()).c_str());
        devices.emplace_back(NearlinkRawAddress(pair.second));
    }
    return NL_NO_ERROR;
}

int ASCService::GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices)
{
    std::set<std::string> virtualAddrSet {};
    GetAllVirtualAudioAddr(virtualAddrSet);
    for (const auto& vAddr : virtualAddrSet) {
        HILOGI("GetVirtualAudioDeviceList %{public}s", GetEncryptAddr(vAddr).c_str());
        devices.emplace_back(NearlinkRawAddress(vAddr));
    }
    return NL_NO_ERROR;
}

int ASCService::GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, NL_ERR_INTERNAL_ERROR, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, NL_ERR_INTERNAL_ERROR, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            // 从保存的音频属性读取
            const std::vector<AscProp>& properties = GetPropertyList(info.addr_);
            for (const AscProp& prop : properties) {
                // 音源音宿取并集
                supportStreamType = (supportStreamType | prop.ability.supportType);
                HILOGD("[ASCService]GetSupportStreamType %{public}s supportStreamType %{public}d ability %{public}d",
                    GetEncryptAddr(info.addr_.GetAddress()).c_str(), supportStreamType, prop.ability.supportType);
            }

            break;
        }
    }

    return (supportStreamType == AUDIO_STREAM_NONE) ? NL_ERR_INTERNAL_ERROR : NL_NO_ERROR;
}


int ASCService::GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
    AudioStreamCodecInfo> &info)
{
    HILOGI("[ASCService]GetAudioDeviceCodecInfo %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    return NL_NO_ERROR;
}

int ASCService::SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType)
{
    HILOGD("[ASCService]SetActiveSinkDevice in %{public}s supportStreamType %{public}ld",
        GetEncryptAddr(device.GetAddress()).c_str(), supportStreamType);

    ASCMessage event(ASC_SET_ACTIVE_SINK_DEVICE_EVT);
    event.dev_ = device.GetAddress();
    event.streamTypeBitMap_ = supportStreamType;
    PostEvent(event);

    HILOGD("[ASCService]SetActiveSinkDevice Start");
    return NL_NO_ERROR;
}

bool ASCService::IsConnectedMemberExist(const RawAddress &device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        if (IsConnected(info.addr_)) {
            return true;
        }
    }

    return false;
}

void ASCService::StopSink()
{
    HILOGI("StopSink!");
    ASCMessage event(ASC_STOP_SINK_EVT);
    PostEvent(event);
}

void ASCService::ProcessStopSinkEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    if (!(activeSinkDevice_.GetAddress().empty())) {
        ASCState state = GetASCStatus(activeSinkDevice_);
        if (IsStarted(state) || IsInStartProcess(state)) {
            HILOGI("[ASCService]ProcessStopSinkEvent activeSinkDevice %{public}s",
                GetEncryptAddr(activeSinkDevice_.GetAddress()).c_str());
            ExcuteDelayStop(activeSinkDevice_);
        }
    }
}

bool ASCService::IsStartMemberExist(const RawAddress &device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        ASCState state = GetASCStatus(info.addr_);
        if (IsStarted(state)) {
            return true;
        }
    }

    return false;
}

void ASCService::ProcessSetActiveSinkDeviceEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    uint32_t streamType = event.streamTypeBitMap_;

    // streamType 0不切出声栈，用于立即断链路
    if (streamType == AUDIO_STREAM_NONE) {
        if (IsStopDelaying(device) && IsStartMemberExist(device)) {
            HILOGI("[ASCService]SetActiveSinkDevice device %{public}s StopPlay streamType %{public}ld",
                GetEncryptAddr(device.GetAddress()).c_str(), streamType);
            ExcuteDelayStop(device);
        }
        return;
    }

    if (!IsConnectedMemberExist(device)) {
        HILOGI("[ASCService]no connected");
        return;
    }
    DeviceBatteryManager::GetInstance().ProcessActiveDeviceChanged(device.GetAddress());
    SleAudioFrameworkAdapter::GetInstance().GetCurrentOutputPipeInfos();
    if (activeSinkDevice_ != device) {
        // 检查出声栈是否在延迟释放
        if (!(activeSinkDevice_.GetAddress().empty())) {
            if (IsStopDelaying(activeSinkDevice_)) {
                HILOGI("[ASCService]SetActiveSinkDevice activeSinkDevice %{public}s StopDelaying",
                    GetEncryptAddr(activeSinkDevice_.GetAddress()).c_str());
                ExcuteDelayStop(activeSinkDevice_);
            }
        }
        SetAutoConnectDevice(device, true);
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, activeSinkDevice_, SET_ACTIVE_DEVICE,
            CHANGE_ACTIVE_DEVICE);
        formerSinkDevice_ = activeSinkDevice_;
        activeSinkDevice_ = device;
        sinkDeviceStreamType_ = streamType;
        if (formerPlayRecord_.addr == formerSinkDevice_){
            SetPlayRecord(activeSinkDevice_, Qos::NL_SLE_QOS_NONE, 0, 0);
            HILOGI("[ASCService]Record sink device changed");
        }
        HILOGI("[ASCService]SetActiveSinkDevice OK %{public}s streamType %{public}ld formerSinkDevice %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType,
            GetEncryptAddr(formerSinkDevice_.GetAddress()).c_str());

        // 切出声栈后设置空间音频开关
        SetSpatialAudioModeEnabled(IsSpatialAudioModeEnabled(device.GetAddress()));
        SetSpatialAudioHeadTrackingEnabled(IsSpatialAudioHeadTrackingEnabled(device.GetAddress()));
        SetSpatialAdaptiveSwitchEnabled(IsSpatialAudioAdaptiveSwitchEnabled(device.GetAddress()));

        return;
    }
}

bool ASCService::IsConnected(const RawAddress &device)
{
    bool isConnected = false;
    connectedDev_.Iterate(
        [&device, &isConnected](std::string key, ASCConnectedDev value)-> void {
        if (device == value.dev) {
            isConnected = true;
            return;
        }
    });

    return isConnected;
}

void ASCService::ProcessUpdateDeviceRoleEvent(const ASCMessage &event)
{
    HILOGD("[ASCService], event(%{public}d)", event.whatM);
    RawAddress device(event.dev_);
    ASCState state = GetASCStatus(device);
    if (!IsStarted(state)) {
        HILOGI("[ASCService]ProcessUpdateDeviceRoleEvent %{public}s not started %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        return;
    }

    Qos cos = QosM::GetInstance().GetCOS(device);
    if (!ASCUtils::IsNeedSetDirection(cos)) {
        HILOGI("[ASCService]ProcessUpdateDeviceRoleEvent %{public}s no up qos %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), cos);
        return;
    }

    bool isRolePrimary = (event.devRole_ == static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY));
    uint8_t direction = ASCUtils::GetDirection(isRolePrimary, cos);
    int ret = SleASC::SetDirection(device, direction);
    if (ret != NL_NO_ERROR) {
        HILOGE("[ASCService]SetDirection ret %{public}d", ret);
        return;
    }

    // 主副切换需要通知DSP链路参数
    // 更改参数更新标记位
    SetQosmInfoUpdateFlag(device, true);
    RawAddress coSetDevice;
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        SetQosmInfoUpdateFlag(coSetDevice, true);
    }

    // 通知DSP链路参数
    std::string channelInfo = GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_START);
    SetAudioParameter(NL_SLE_ASC_CONTROL_CMD_START, "", channelInfo);

    HILOGI("[ASCService]ProcessUpdateDeviceRoleEvent finish %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    return;
}

int ASCService::UpdateDeviceRole(const RawAddress &device, uint8_t devRole)
{
    HILOGI("[ASCService]UpdateDeviceRole in %{public}s devRole:%{public}u",
        GetEncryptAddr(device.GetAddress()).c_str(), devRole);

    ASCMessage event(ASC_UPDATE_DEVICE_ROLE_EVT);
    event.dev_ = device.GetAddress();
    event.devRole_ = devRole;
    PostEvent(event);

    HILOGI("[ASCService]UpdateDeviceRole Start");
    return NL_NO_ERROR;
}

void ASCService::PostEvent(const ASCMessage &event)
{
    DoInAscThread([this, event]() { this->ProcessEvent(event); });
}


void ASCService::ProcessBaseEvent(const ASCMessage &event)
{
    switch (event.whatM) {
        case ASC_SERVICE_STARTUP_EVT:
            StartUp();
            break;
        case ASC_SERVICE_SHUTDOWN_EVT:
            ShutDown();
            break;
        case ASC_CONNECT_START_EVT:
            ProcessConnectEvent(event);
            break;
        case ASC_DISCONNECT_START_EVT:
            ProcessDisConnectEvent(event);
            break;
        case ASC_START_PLAYING_EVT:
            ProcessStartPlayingEvent(event);
            break;
        case ASC_STOP_PLAYING_EVT:
            ProcessStopPlayingEvent(event);
            break;
        case ASC_STACK_EVENT_CBK_EVT:
            ProcessStackCbkEvent(event);
            break;
        case ASC_STACK_PROP_CBK_EVT:
            ProcessStackCbkProp(event);
            break;
        case ASC_SET_ACTIVE_SINK_DEVICE_EVT:
            ProcessSetActiveSinkDeviceEvent(event);
            break;
        case ASC_UPDATE_VIRTUAL_DEVICE_EVT:
            ProcessUpdateVirtualDeviceEvent(event);
            break;
        case ASC_STACK_LOCATION_CBK_EVT:
            ProcessStackLocationChangeCbk(event);
            break;
        case ASC_STACK_STREAM_TYPE_CBK_EVT:
            ProcessStackStreamTypeChangeCbk(event);
            break;
        default:
            break;
    }
}

void ASCService::ProcessAssistEvent(const ASCMessage &event)
{
    switch (event.whatM) {
        case ASC_UPDATE_VIRTUAL_DEVICE_EVT:
            ProcessBaseEvent(event);
            break;
        case ASC_UPDATE_DEVICE_ROLE_EVT:
            ProcessUpdateDeviceRoleEvent(event);
            break;
        case ASC_STOP_DELAY_TIMEOUT_EVT:
            ProcessStopDelayTimeOutEvent(event);
            break;
        case ASC_CONN_RPT_DELAY_TIMEOUT_EVT:
            ProcessConnRptDelayTimeOutEvent(event);
            break;
        case ASC_SEND_PLAY_OR_PAUSE_EVT:
            ProcessSendPlayOrPauseEvent(event);
            break;
        case ASC_AUDIO_SCENE_CHANGE_EVT:
            ProcessAudioSceneChangeEvent(event);
            break;
        case ASC_STOP_SINK_EVT:
            ProcessStopSinkEvent(event);
            break;
        case ASC_SUBRATE_CHANGED_EVT:
            ProcessSubrateChangedEvent(event);
            break;
        case ASC_SUBRATE_CHANGE_REQ_EVT:
            ProcessSubrateChangeReq(event);
            break;
        case ASC_UPDATE_NATURE_EVT:
            ProcessUpdateDeviceNature(event);
            break;
        case ASC_BITRATE_CBK_EVT:
        case ASC_FRAME_CHANGED_EVT:
        case ASC_STACK_AUTORATE_MSG_CBK:
            ProcessAutoRateEvent(event);
            break;
        case ASC_SPATIAL_AUDIO_EVT:
        case ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT:
        case ASC_SPATIAL_AUDIO_SOURCE_EVT:
        case ASC_SPATIAL_AUDIO_ADAPTIVE_SWITCH_RENDER_EVT:
            ProcessSpatialAudioEvent(event);
            break;
        case ASC_COL_AUDIO_SWITCH_CHANGE_EVT:
            ProcessColAudioSwitchChangeEvent(event);
            break;
        default:
            break;
    }
}

void ASCService::ProcessAutoRateEvent(const ASCMessage &event)
{
    switch (event.whatM) {
        case ASC_BITRATE_CBK_EVT:
            ProcessChangeBitrateEvent(event);
            break;
        case ASC_FRAME_CHANGED_EVT:
            ProcessFrameTypeChangedEvent(event);
            break;
        case ASC_STACK_AUTORATE_MSG_CBK:
            ProcessVoiceCallAutoRateEvent(event);
            break;
        default:
            break;
    }
}

void ASCService::ProcessSpatialAudioEvent(const ASCMessage &event)
{
    switch (event.whatM) {
        case ASC_SPATIAL_AUDIO_EVT:
            ProcessSpatialAudioBaseEvent(event);
            break;
        case ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT:
            ProcessSpatialAudioMoveEvent(event);
            break;
        case ASC_SPATIAL_AUDIO_SOURCE_EVT:
            ProcessSpatialAudioSourceTypeEvent(event);
            break;
        case ASC_SPATIAL_AUDIO_ADAPTIVE_SWITCH_RENDER_EVT:
            ProcessSpatialAudioAdaptiveSwitchRenderEvent(event);
            break;
        default:
            break;
    }
}

void ASCService::ProcessEvent(const ASCMessage &event)
{
    HILOGI("[ASCService]ProcessEvent:[%{public}s], [%{public}d]", GetEncryptAddr(event.dev_).c_str(), event.whatM);
    switch (event.whatM) {
        case ASC_SERVICE_STARTUP_EVT:
        case ASC_SERVICE_SHUTDOWN_EVT:
        case ASC_CONNECT_START_EVT:
        case ASC_DISCONNECT_START_EVT:
        case ASC_START_PLAYING_EVT:
        case ASC_STOP_PLAYING_EVT:
        case ASC_STACK_EVENT_CBK_EVT:
        case ASC_STACK_PROP_CBK_EVT:
        case ASC_STACK_LOCATION_CBK_EVT:
        case ASC_SET_ACTIVE_SINK_DEVICE_EVT:
        case ASC_UPDATE_VIRTUAL_DEVICE_EVT:
        case ASC_STACK_STREAM_TYPE_CBK_EVT:
            ProcessBaseEvent(event);
            break;
        case ASC_UPDATE_DEVICE_ROLE_EVT:
        case ASC_BITRATE_CBK_EVT:
        case ASC_STACK_AUTORATE_MSG_CBK:
        case ASC_STOP_DELAY_TIMEOUT_EVT:
        case ASC_SPATIAL_AUDIO_EVT:
        case ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT:
        case ASC_CONN_RPT_DELAY_TIMEOUT_EVT:
        case ASC_SEND_PLAY_OR_PAUSE_EVT:
        case ASC_AUDIO_SCENE_CHANGE_EVT:
        case ASC_STOP_SINK_EVT:
        case ASC_SUBRATE_CHANGED_EVT:
        case ASC_FRAME_CHANGED_EVT:
        case ASC_SUBRATE_CHANGE_REQ_EVT:
        case ASC_UPDATE_NATURE_EVT:
        case ASC_SPATIAL_AUDIO_SOURCE_EVT:
        case ASC_SPATIAL_AUDIO_ADAPTIVE_SWITCH_RENDER_EVT:
        case ASC_COL_AUDIO_SWITCH_CHANGE_EVT:
            ProcessAssistEvent(event);
            break;
        default:
            break;
    }
}

const NearlinkRawAddress ASCService::GetActiveSinkDevice() const
{
    return NearlinkRawAddress(activeSinkDevice_);
}

int ASCService::GetDftUpdateVoiceStackReason(int action)
{
    static std::map<int, int> updateVoiceStackReasonMap = {
        { static_cast<int>(UpdateOutputStackAction::ACTION_WEAR),
            UPDATE_VOICE_STACK_REASON_WEAR },
        { static_cast<int>(UpdateOutputStackAction::ACTION_UNWEAR),
            UPDATE_VOICE_STACK_REASON_UNWEAR },
        { static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_FROM_REMOTE),
            UPDATE_VOICE_STACK_REASON_ENABLE_FROM_REMOTE },
        { static_cast<int>(UpdateOutputStackAction::ACTION_DISABLE_FROM_REMOTE),
            UPDATE_VOICE_STACK_REASON_DISABLE_FROM_REMOTE },
        { static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_WEAR_DETECTION),
            UPDATE_VOICE_STACK_REASON_ENABLE_WEAR_DETECTION },
        { static_cast<int>(UpdateOutputStackAction::ACTION_DISABLE_WEAR_DETECTION),
            UPDATE_VOICE_STACK_REASON_DISABLE_WEAR_DETECTION },
        { static_cast<int>(UpdateOutputStackAction::ACTION_USER_OPERATION),
            UPDATE_VOICE_STACK_REASON_USER_OPERATION_FROM_REMOTE },
    };
    auto it = updateVoiceStackReasonMap.find(action);
    if (it == updateVoiceStackReasonMap.end()) {
        return UPDATE_VOICE_STACK_REASON_INVALID;
    }
    return it->second;
}

void ASCService::SleAudioDeviceActionChanged(const NearlinkRawAddress &device, int action)
{
    RawAddress address(device.GetAddress());
    AudioStreamType streamType = GetProcessingStreamType(address);
    RawAddress reportAddr = GetReportAddr(address);

    int dftUpdateVoiceStackReason = GetDftUpdateVoiceStackReason(action);
    if (dftUpdateVoiceStackReason != UPDATE_VOICE_STACK_REASON_INVALID) {
        NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(reportAddr, UPDATE_VOICE_STACK_SCENE, streamType,
            dftUpdateVoiceStackReason);
    }

    // 上报设备状态变更给音频框架
    if (callback_ != nullptr) {
        NearlinkASCAudioStreamInfo streamInfo {};
        streamInfo.AddStreamState(streamType, AUDIO_STREAM_STATE_AVAILABLE);
        HILOGI("[ASCService]SleAudioDeviceActionChanged %{public}s streamType %{public}d action %{public}d",
            GET_ENCRYPT_ADDR(reportAddr), streamType, action);
        callback_->OnSleAudioDeviceActionChanged(NearlinkRawAddress(reportAddr), streamInfo, action);
    }
}

void ASCService::SleAudioDeviceActionChanged(const NearlinkRawAddress &device,
    const std::vector<struct AudioStreamInfo> &streamData, int action)
{
    RawAddress address(device.GetAddress());
    RawAddress reportAddr = GetReportAddr(address);

    int dftUpdateVoiceStackReason = GetDftUpdateVoiceStackReason(action);
    if (dftUpdateVoiceStackReason != UPDATE_VOICE_STACK_REASON_INVALID) {
        NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(reportAddr, UPDATE_VOICE_STACK_SCENE,
            GetProcessingStreamType(address), dftUpdateVoiceStackReason);
    }

    // 上报设备状态变更给音频框架
    if (callback_ != nullptr) {
        NearlinkASCAudioStreamInfo streamInfo {};
        streamInfo.SetStreamState(streamData);
        HILOGI("[ASCService]SleAudioDeviceActionChanged %{public}s multi streamType,action %{public}d",
            GET_ENCRYPT_ADDR(reportAddr), action);
        callback_->OnSleAudioDeviceActionChanged(NearlinkRawAddress(reportAddr), streamInfo, action);
    }
}

bool ASCService::IsCalling()
{
    return GetIsCallingFlag();
}

bool ASCService::IsPlaying(const RawAddress &device)
{
    ASCState status = GetASCStatus(device);
    return (IsStarted(status) || IsInStartProcess(status));
}

std::shared_ptr<AudioStandard::AudioDeviceDescriptor> GetAudioDeviceDescriptor(const std::string &macAddr)
{
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev =
        std::make_shared<AudioStandard::AudioDeviceDescriptor>();
    NL_CHECK_RETURN_RET(audioDev != nullptr, nullptr, "Device err: null audioDevDescriptor");
    audioDev->macAddress_ = macAddr;
    return audioDev;
}

bool IsSpatialAudioModeSupported(const std::string &macAddr)
{
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev = GetAudioDeviceDescriptor(macAddr);
    NL_CHECK_RETURN_RET(audioDev != nullptr, false, "Device err: null audioDevDescriptor");
    return AudioStandard::AudioSpatializationManager::GetInstance()->IsSpatializationSupportedForDevice(audioDev);
}

bool IsSpatialAudioHeadTrackingSupported(const std::string &macAddr)
{
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev = GetAudioDeviceDescriptor(macAddr);
    NL_CHECK_RETURN_RET(audioDev != nullptr, false, "Device err: null audioDevDescriptor");
    return AudioStandard::AudioSpatializationManager::GetInstance()->IsHeadTrackingSupportedForDevice(audioDev);
}

void ASCService::SleSpatialAudioModeChangeListener::OnSpatializationEnabledChangeForAnyDevice(
    const std::shared_ptr<AudioStandard::AudioDeviceDescriptor> &deviceDescriptor, const bool &enabled)
{
    NL_CHECK_RETURN(deviceDescriptor, "deviceDescriptor is null");
    NL_CHECK_RETURN(IsValidAddress(deviceDescriptor->macAddress_), "deviceDescriptor->macAddress_ is invalid");
    ProfileASC *ascProfileService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfileService, "cant find ASC service");
    RawAddress device = ascProfileService->GetActiveSinkDevice();
    if (device.GetAddress() != deviceDescriptor->macAddress_) {
        HILOGI("remote device %{public}s is not active device, do not handle %{public}s",
            GetEncryptAddr(deviceDescriptor->macAddress_).c_str(), GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    if (!IsSpatialAudioModeSupported(device.GetAddress())) {
        HILOGI("remote device %{public}s not support spatial audio mode", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    HILOGI("OnSpatializationEnabledChangeForAnyDevice: device=%{public}s, enable=%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), enabled);
    ASCService *ascService = static_cast<ASCService *>(ascProfileService);
    NL_CHECK_RETURN(ascService, "ASC service null");
    ASCMessage event(ASC_SPATIAL_AUDIO_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = enabled;
    ascService->PostEvent(event);
}

void ASCService::SleSpatialAudioHeadTrackingChangeListener::OnHeadTrackingEnabledChangeForAnyDevice(
    const std::shared_ptr<AudioStandard::AudioDeviceDescriptor> &deviceDescriptor, const bool &enabled)
{
    NL_CHECK_RETURN(deviceDescriptor, "deviceDescriptor is null");
    NL_CHECK_RETURN(IsValidAddress(deviceDescriptor->macAddress_), "deviceDescriptor->macAddress_ is invalid");
    ProfileASC *ascProfileService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfileService, "cant find ASC service");
    RawAddress device = ascProfileService->GetActiveSinkDevice();
    if (device.GetAddress() != deviceDescriptor->macAddress_) {
        HILOGI("remote device %{public}s is not active device, do not handle %{public}s",
            GetEncryptAddr(deviceDescriptor->macAddress_).c_str(), GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    if (!IsSpatialAudioHeadTrackingSupported(device.GetAddress())) {
        HILOGI("remote device %{public}s not support spatial audio mode", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    HILOGI("OnHeadTrackingEnabledChangeForAnyDevice: device=%{public}s, enable=%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), enabled);
    ASCService *ascService = static_cast<ASCService *>(ascProfileService);
    NL_CHECK_RETURN(ascService, "ASC service null");
    ASCMessage event(ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = enabled;
    ascService->PostEvent(event);
}

void ASCService::SleSpatialAudioSourceTypeChangeListener::OnSpatialAudioSourceTypeChange(
    const AudioStandard::SpatialAudioSourceType &mode)
{
    ProfileASC *ascProfileService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfileService, "cant find ASC service");
    ASCService *ascService = static_cast<ASCService *>(ascProfileService);
    NL_CHECK_RETURN(ascService, "ASC service null");
    ASCMessage event(ASC_SPATIAL_AUDIO_SOURCE_EVT);
    event.result_ = static_cast<uint8_t>(mode);
    ascService->PostEvent(event);
}

void ASCService::SleSpatialAudioAdaptiveSwitchChangeListener::OnAdaptiveSpatialRenderingEnabledChangeForAnyDevice(
    const std::shared_ptr<AudioStandard::AudioDeviceDescriptor> &deviceDescriptor, const bool &enabled)
{
    NL_CHECK_RETURN(deviceDescriptor, "deviceDescriptor is null");
    NL_CHECK_RETURN(IsValidAddress(deviceDescriptor->macAddress_), "deviceDescriptor->macAddress_ is invalid");
    ProfileASC *ascProfileService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfileService, "cant find ASC service");
    RawAddress device = ascProfileService->GetActiveSinkDevice();
    if (device.GetAddress() != deviceDescriptor->macAddress_) {
        HILOGI("remote device %{public}s is not active device, do not handle %{public}s",
            GetEncryptAddr(deviceDescriptor->macAddress_).c_str(), GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    HILOGI("OnAdaptiveSpatialRenderingEnabledChangeForAnyDevice: device=%{public}s, enable=%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), enabled);
    ASCService *ascService = static_cast<ASCService *>(ascProfileService);
    NL_CHECK_RETURN(ascService, "ASC service null");
    ASCMessage event(ASC_SPATIAL_AUDIO_ADAPTIVE_SWITCH_RENDER_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = enabled;
    ascService->PostEvent(event);
}

void ASCService::RegisterSpatialAudioListener()
{
    spatialAudioModeCallback_ = std::make_shared<SleSpatialAudioModeChangeListener>();
    spatialAudioHeadTrackingCallback_ = std::make_shared<SleSpatialAudioHeadTrackingChangeListener>();
    spatialAudioSourceTypeCallback_ = std::make_shared<SleSpatialAudioSourceTypeChangeListener>();
    spatialAudioAdaptiveSwitchCallback_ = std::make_shared<SleSpatialAudioAdaptiveSwitchChangeListener>();
    AudioStandard::AudioSpatializationManager::GetInstance()->RegisterSpatializationEnabledEventListener(
        spatialAudioModeCallback_);
    AudioStandard::AudioSpatializationManager::GetInstance()->RegisterHeadTrackingEnabledEventListener(
        spatialAudioHeadTrackingCallback_);
    AudioStandard::AudioSpatializationManager::GetInstance()->RegisterSpatialAudioSourceTypeEventListener(
        spatialAudioSourceTypeCallback_);
    AudioStandard::AudioSpatializationManager::GetInstance()->RegisterAdaptiveSpatialRenderingEnabledEventListener(
        spatialAudioAdaptiveSwitchCallback_);
    HILOGI("[ASCService] RegisterSpatialAudioListener");
}

bool ASCService::IsSpatialAudioModeEnabled(const std::string &macAddr)
{
    if (!IsSpatialAudioModeSupported(macAddr)) {
        return false;
    }
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev = GetAudioDeviceDescriptor(macAddr);
    NL_CHECK_RETURN_RET(audioDev != nullptr, false, "Device err: null audioDevDescriptor");
    return AudioStandard::AudioSpatializationManager::GetInstance()->IsSpatializationEnabled(audioDev);
}

bool ASCService::IsSpatialAudioHeadTrackingEnabled(const std::string &macAddr)
{
    if (!IsSpatialAudioHeadTrackingSupported(macAddr)) {
        return false;
    }
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev = GetAudioDeviceDescriptor(macAddr);
    NL_CHECK_RETURN_RET(audioDev != nullptr, false, "Device err: null audioDevDescriptor");
    return AudioStandard::AudioSpatializationManager::GetInstance()->IsHeadTrackingEnabled(audioDev);
}

bool ASCService::IsSpatialAudioAdaptiveSwitchEnabled(const std::string &macAddr)
{
    std::shared_ptr<AudioStandard::AudioDeviceDescriptor> audioDev = GetAudioDeviceDescriptor(macAddr);
    NL_CHECK_RETURN_RET(audioDev != nullptr, false, "Device err: null audioDevDescriptor");
    return AudioStandard::AudioSpatializationManager::GetInstance()->IsAdaptiveSpatialRenderingEnabled(audioDev);
}

void ASCService::SetSpatialAudioModeEnabled(const bool enabled)
{
    HILOGI("enabled=%{public}d audioModeEnable=%{public}d mask=%{public}d", enabled,
        IsSpatialAudioModeEnabled(), spatialAudioStateMask.load());
    if (IsSpatialAudioModeEnabled() == enabled) {
        return;
    }
    if (enabled) {
        spatialAudioStateMask |= NL_SLE_ASC_SPATIAL_AUDIO_MODE_ENABLED;
    } else {
        spatialAudioStateMask &= ~NL_SLE_ASC_SPATIAL_AUDIO_MODE_ENABLED;
    }
    ChangeIsoParamIfNeed();
    HILOGI("enabled=%{public}d audioModeEnable=%{public}d mask=%{public}d", enabled,
        IsSpatialAudioModeEnabled(), spatialAudioStateMask.load());
}

void ASCService::SetSpatialAudioHeadTrackingEnabled(const bool enabled)
{
    HILOGI("enabled=%{public}d audioModeEnable=%{public}d mask=%{public}d", enabled,
        IsSpatialAudioHeadTrackingEnabled(), spatialAudioStateMask.load());
    if (IsSpatialAudioHeadTrackingEnabled() == enabled) {
        return;
    }
    if (enabled) {
        spatialAudioStateMask |= NL_SLE_ASC_SPATIAL_AUDIO_HEAD_TRACKING_ENABLED;
    } else {
        spatialAudioStateMask &= ~NL_SLE_ASC_SPATIAL_AUDIO_HEAD_TRACKING_ENABLED;
    }
    ChangeIsoParamIfNeed();
    HILOGI("enabled=%{public}d audioModeEnable=%{public}d mask=%{public}d end", enabled,
        IsSpatialAudioHeadTrackingEnabled(), spatialAudioStateMask.load());
}

void ASCService::SetSpatialAudioSourceTypeSupported(const uint8_t sourceType)
{
    HILOGI("sourceType=%{public}d audioSourcrTypeEnable=%{public}d mask=%{public}d", sourceType,
        IsSpatialAudioSourceTypeSupported(), spatialAudioStateMask.load());
    bool enabled = (sourceType != NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_STEREO);
    if (IsSpatialAudioSourceTypeSupported() == enabled) {
        return;
    }
    if (enabled) {
        spatialAudioStateMask |= NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_ENABLED;
    } else {
        spatialAudioStateMask &= ~NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_ENABLED;
    }
    ChangeIsoParamIfNeed();
    HILOGI("sourceType=%{public}d audioSourcrTypeEnable=%{public}d mask=%{public}d", sourceType,
        IsSpatialAudioSourceTypeSupported(), spatialAudioStateMask.load());
}

void ASCService::SetSpatialAdaptiveSwitchEnabled(const bool enabled)
{
    HILOGI("enabled=%{public}d adaptiveSwitchEnable=%{public}d mask=%{public}d", enabled,
        IsSpatialAudioAdaptiveSwitchEnabled(), spatialAudioStateMask.load());
    if (IsSpatialAudioAdaptiveSwitchEnabled() == enabled) {
        return;
    }
    if (enabled) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(activeSinkDevice_, activeSinkDevice_,
            SPATIAL_AUDIO, SWITCH_ON);
        spatialAudioStateMask |= NL_SLE_ASC_SPATIAL_AUDIO_ADAPTIVE_ENABLED;
    } else {
        spatialAudioStateMask &= ~NL_SLE_ASC_SPATIAL_AUDIO_ADAPTIVE_ENABLED;
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(activeSinkDevice_, activeSinkDevice_,
            SPATIAL_AUDIO, SWITCH_OFF);
    }
    ChangeIsoParamIfNeed();
    HILOGI("enabled=%{public}d adaptiveSwitchEnable=%{public}d mask=%{public}d", enabled,
        IsSpatialAudioAdaptiveSwitchEnabled(), spatialAudioStateMask.load());
}

void ASCService::SetColAudioSwitchEnabled(const bool enabled)
{
    HILOGI("[ASCService] enabled=%{public}d", enabled);
    isColAudioBeforeEnabled_ = isColAudioEnabled_.load();
    isColAudioEnabled_ = enabled;
    // 通过历史状态，判断是否为首次打开，防抖
    if (!isColAudioBeforeEnabled_ && isColAudioEnabled_) {
        ChangeIsoParamForColAudioIfNeed();
    }
}

bool ASCService::IsSpatialAudioModeEnabled(void)
{
    return (spatialAudioStateMask & NL_SLE_ASC_SPATIAL_AUDIO_MODE_ENABLED);
}

bool ASCService::IsSpatialAudioHeadTrackingEnabled(void)
{
    return (spatialAudioStateMask & NL_SLE_ASC_SPATIAL_AUDIO_HEAD_TRACKING_ENABLED);
}

bool ASCService::IsSpatialAudioSourceTypeSupported()
{
    return (spatialAudioStateMask & NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_ENABLED);
}

bool ASCService::IsSpatialAudioAdaptiveSwitchEnabled()
{
    return (spatialAudioStateMask & NL_SLE_ASC_SPATIAL_AUDIO_ADAPTIVE_ENABLED);
}

void ASCService::ProcessSpatialAudioBaseEvent(const ASCMessage &event)
{
    SetSpatialAudioModeEnabled(event.result_);
}

void ASCService::ProcessSpatialAudioMoveEvent(const ASCMessage &event)
{
    SetSpatialAudioHeadTrackingEnabled(event.result_);
}

void ASCService::ProcessSpatialAudioSourceTypeEvent(const ASCMessage &event)
{
    SetSpatialAudioSourceTypeSupported(event.result_);
}

void ASCService::ProcessSpatialAudioAdaptiveSwitchRenderEvent(const ASCMessage &event)
{
    SetSpatialAdaptiveSwitchEnabled(event.result_);
}

void ASCService::ProcessColAudioSwitchChangeEvent(const ASCMessage &event)
{
    SetColAudioSwitchEnabled(event.result_);
}

void ASCService::ChangeIsoParamIfNeed()
{
    const RawAddress& device = activeSinkDevice_;
    HILOGI("[ASCService]ChangeIsoParamIfNeed in %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), GetASCStatus(activeSinkDevice_));

    // 需要重配置
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

    for (const auto& info : cdsmList) {
        if (IsStarted(GetASCStatus(info.addr_))) {
            const std::list<AudioStreamType>& listStream = GetStartedStreamList(info.addr_);
            if (!listStream.empty()) {
                ProcSpatialIfNeed(info.addr_, listStream.front());
            }
        }
    }
}

void ASCService::ChangeIsoParamForColAudioIfNeed()
{
    const RawAddress& device = activeSinkDevice_;
    HILOGI("[ASCService] device %{public}s state %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), GetASCStatus(activeSinkDevice_));

    // 需要重配置
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<NearlinkCdsmInfo> cdsmList;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");
    for (const auto& info : cdsmList) {
        if (IsStarted(GetASCStatus(info.addr_))) {
            const std::list<AudioStreamType>& listStream = GetStartedStreamList(info.addr_);
            if (!listStream.empty() && IsNeedTransferForColAudio(listStream.front())) {
                ProcColAudioIfNeed(info.addr_, listStream.front());
            }
        }
    }
}

void ASCService::SetIsCallingFlag(bool isCalling)
{
    HILOGI("[ASCService]SetIsCallingFlag in %{public}d", isCalling);
    isCalling_ = isCalling;
}

bool ASCService::GetIsCallingFlag()
{
    HILOGI("[ASCService]GetIsCallingFlag isCalling %{public}d", isCalling_.load());
    return isCalling_;
}

void ASCService::RegisterAudioSceneListener()
{
    HILOGI("[ASCService]RegisterAudioSceneListener");
    if (extAudioSceneListener_ == nullptr) {
        extAudioSceneListener_ = std::make_shared<AudioSceneChangedListener>();
    }
    AudioStandard::AudioSystemClientPolicyManager::GetInstance().SetAudioSceneChangeCallback(extAudioSceneListener_);
}

void AudioSceneChangedListener::OnAudioSceneChange(const AudioStandard::AudioScene audioScene)
{
    HILOGI("[ASCService]OnAudioSceneChange audioScene: %{public}d", audioScene);
    bool isCalling = false;
    if ((audioScene == AudioStandard::AudioScene::AUDIO_SCENE_RINGING) ||
        (audioScene == AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CALL) ||
        (audioScene == AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CHAT) ||
        (audioScene == AudioStandard::AudioScene::AUDIO_SCENE_VOICE_RINGING)) {
        isCalling = true;
    }

    ASCService *ascService = static_cast<ASCService *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");

    ASCMessage event(ASC_AUDIO_SCENE_CHANGE_EVT);
    event.isCalling_ = isCalling;
    ascService->PostEvent(event);
}

void ASCService::ProcessAudioSceneChangeEvent(const ASCMessage &event)
{
    if (event.isCalling_ != GetIsCallingFlag()) {
        HILOGI("[ASCService]ProcessAudioSceneChangeEvent Switch Volume isCalling: %{public}d", event.isCalling_);
        SetIsCallingFlag(event.isCalling_);

        CdsmService* cdsmService = CdsmService::GetService();
        NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
        std::vector<NearlinkCdsmInfo> cdsmList;
        NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(activeSinkDevice_, cdsmList);
        NL_CHECK_RETURN(ret == NL_NO_ERROR, "CdsmGetAllMemberInfo error.");

        // 初始化结果记录状态信息
        for (const auto& info : cdsmList) {
            if (IsConnected(info.addr_)) {
                // 音量控制
                SwitchAbsVolumeDevice(info.addr_, true);
            }
        }
    }

    return;
}

void ASCService::SwitchAbsVolumeDevice(const RawAddress &device, bool isNeedSetVolume)
{
    VcpService* vcpService = VcpService::GetService();
    if (vcpService != nullptr) {
        vcpService->SwitchAbsVolumeDevice(device, isNeedSetVolume);
    }
}

void ASCService::OpenVoiceAssistant(const RawAddress &device, AudioStreamType streamType)
{
    VasService *vasService = VasService::GetService();
    if (vasService != nullptr && streamType == AUDIO_STREAM_VOICE_ASSISTANT) {
        vasService->OpenVoiceAssistant(device);
    }
}

void ASCService::CloseVoiceAssistant(const RawAddress &device, AudioStreamType streamType)
{
    VasService *vasService = VasService::GetService();
    if (vasService != nullptr && streamType == AUDIO_STREAM_VOICE_ASSISTANT) {
        SetIsCallingFlag(false);
        vasService->CloseVoiceAssistant(device);
    }
}

void ASCService::CheckStreamIsNeedNotifyCcp(const RawAddress &device, AudioStreamType type, int cmd)
{
    /* 非VoIP流类型不处理 */
    if (type != AUDIO_STREAM_VOIP) {
        return;
    }
    CcpService* ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "[ASCService]CcpService is null.");
    if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
        ccpService->HandleVoipStart(device);
    } else {
        ccpService->HandleVoipStop(device);
    }
}

void ASCService::ProcessMcpInit(const RawAddress& device)
{
    HILOGD("[ASCService]Enter");
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    mcpService->LoadMediaSo();
    mcpService->InitMediaListener();
}

void ASCService::ProcessCcpInit(const RawAddress& device)
{
    HILOGD("[ASCService]Enter");
    CcpService* ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "[ASCService]CcpService is null.");
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "[ASCService]cdsmService is null.");

    if (cdsmService->CdsmCheckIsCooperationDevice(device)) {
        if (IsRolePrimary(device)) {
            ccpService->TryResumeCurrentCalls();
        }
    } else {
        ccpService->TryResumeCurrentCalls();
    }
}

void ASCService::RegisterAudioPreferredOutPutDeviceChangeListener()
{
    HILOGD("[ASCService]: RegisterAudioPreferredOutPutDeviceChangeListener.");
    if (audioPreferredOutPutDeviceCallback_ != nullptr) {
        return;
    }
    audioPreferredOutPutDeviceCallback_ = std::make_shared<SleAudioPreferredOutPutDeviceChangeListener>();
    AudioStandard::AudioRendererInfo rendererInfo;
    rendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_MUSIC;
    rendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    int32_t ret = AudioStandard::AudioRoutingClientManager::GetInstance().SetPreferredOutputDeviceChangeCallback(
        rendererInfo, audioPreferredOutPutDeviceCallback_);
    HILOGI("SetAudioPreferredOutputDeviceChangeCallback ret=%{public}d", ret);
}

void ASCService::UnregisterAudioPreferredOutPutDeviceChangeListener()
{
    NL_CHECK_RETURN(
        audioPreferredOutPutDeviceCallback_ != nullptr, "audioPreferredOutPutDeviceCallback_ is nullptr");
    auto ret = AudioStandard::AudioRoutingClientManager::GetInstance().UnsetPreferredOutputDeviceChangeCallback();
    if (ret != 0) {
        HILOGE("UnsetAudioPreferredOutputDeviceChangeCallback fail");
    }
    audioPreferredOutPutDeviceCallback_ = nullptr;
}

bool ASCService::IsAudioOutputToNlAudio()
{
    AudioStandard::DeviceType deviceType =
        AudioStandard::AudioDevicesClientManager::GetInstance().GetActiveOutputDevice();
    if (deviceType == AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK) {
        return true;
    }
    return false;
}

void ASCService::SetCurrentDeviceMute()
{
    HILOGI("[ASCService]:SetCurrentDeviceMute.");
    ASCService *ascService =
        static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    if (timerForRestoreVolumeIfPaused_ != nullptr) {
        HILOGI("Another timer has started, stop it before set mute");
        timerForRestoreVolumeIfPaused_->Stop();
        if (ascService->SetMusicUnmute() != NL_NO_ERROR) {
            HILOGI("[ASCService]:SetMusicUnmute error.");
            return;
        }
        timerForRestoreVolumeIfPaused_ = nullptr;
    }
    if (ascService->SetMusicMuteWhenAudioRelease() != NL_NO_ERROR) {
        HILOGI("[ASCService]:SetMusicMuteWhenAudioRelease error.");
        return;
    }
    // if music is paused, unmute after 1s
    RawAddress device = ascService->GetActiveSinkDevice();
    auto timerCallback = [device]() -> void {
        HILOGI("[ASCService]: time callback.");
        ASCService *ascService =
            static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
        NL_CHECK_RETURN(ascService, "cant find ASC service");
        bool isNavigationRunning = false;
        std::list<AudioStreamType>& startedList = ascService->GetStartedStreamList(device);
        for (const AudioStreamType& stream : startedList) {
            if (stream == AUDIO_STREAM_GUID) {
                HILOGI("[ASCService]: Navigation is running.");
                isNavigationRunning = true;
                break;
            }
        }
        bool isMusicActive = SleAudioFrameworkAdapter::GetInstance().IsMusicActive();
        bool isNlAudioOutputNow = ascService->IsAudioOutputToNlAudio();
        bool isNeedUnmute = isNlAudioOutputNow || !isMusicActive || isNavigationRunning;

        HILOGI("RestoreVolumeTimer: isMusicActive:%{public}d, isA2dpOutputNow:%{public}d, "
            "isNavigationRunning:%{public}d, isNeedUnmute:%{public}d",
            isMusicActive, isNlAudioOutputNow, isNavigationRunning, isNeedUnmute);

        if (isNeedUnmute) {
            HILOGI("RestoreVolumeTimer: Unmute stream after %{public}d ms", TIMER_FOR_RESTORE_VOLUME_IF_PAUSED_MS);
            if (ascService->SetMusicUnmute() != NL_NO_ERROR) {
                HILOGI("[ASCService]:SetMusicUnmute error.");
                return;
            }
        }
    };
    timerForRestoreVolumeIfPaused_ = std::make_shared<NearlinkTimer>(timerCallback);
    timerForRestoreVolumeIfPaused_->Start(TIMER_FOR_RESTORE_VOLUME_IF_PAUSED_MS);
}

int32_t ASCService::SetMusicMuteWhenAudioRelease()
{
    HILOGI("[ASCService]: SetMusicMuteWhenAudioRelease.");
    ProfileASC *ascProfileService =
        static_cast<ProfileASC *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN_RET(ascProfileService != nullptr, NL_ERR_INTERNAL_ERROR, "cant find ASC service");
    RawAddress device = ascProfileService->GetActiveSinkDevice();
    if (device.GetAddress() == INVALID_MAC_ADDRESS || device.GetAddress().empty()) {
        HILOGI("activeDevice is empty");
        return NL_ERR_INTERNAL_ERROR;
    }
    if (!SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(device)) {
        // 非vendor耳机, 不设置静音逻辑
        HILOGI("activeDevice is not vendor device");
        return NL_ERR_INTERNAL_ERROR;
    }
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN_RET(twsService != nullptr, NL_ERR_INTERNAL_ERROR, "cant find TWS service");
    uint8_t mediaState = twsService->TwsGetDeviceAudioMusicType(device);
    HILOGI("[ASCService]: mediaState: %{public}d", mediaState);
    if (mediaState != static_cast<uint8_t>(TwsMediaState::DISABLE)) {
        return NL_ERR_INTERNAL_ERROR;
    }
    if (AudioStandard::AudioVolumeClientManager::GetInstance().GetVolume(
        AudioStandard::AudioVolumeType::STREAM_MUSIC) == 0) {
        HILOGI("no need mute, before spk volume is zero.");
        return NL_ERR_INTERNAL_ERROR;
    }
    auto audioGroupManager = OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().GetGroupManager(
        OHOS::AudioStandard::DEFAULT_VOLUME_GROUP_ID);
    NL_CHECK_RETURN_RET(audioGroupManager != nullptr, NL_ERR_INTERNAL_ERROR, "audioGroup nullptr");
    HILOGI("[ASCService]: ready to set mute.");
    auto ret = audioGroupManager->SetMute(
        AudioStandard::AudioVolumeType::STREAM_MUSIC, true, AudioStandard::DEVICE_TYPE_SPEAKER);
    if (ret != 0) {
        HILOGE("set mute failed");
        return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}

void ASCService::SleAudioPreferredOutPutDeviceChangeListener::OnPreferredOutputDeviceUpdated(
    const std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>> &deviceDescriptor)
{
    HILOGI("[ASCService]: OnPreferredOutputDeviceUpdated.");
    if (deviceDescriptor.empty() || !deviceDescriptor[0]) {
        HILOGE("deviceDescriptor is empty or invalid");
        return;
    }
    ASCService *service = ASCService::GetService();
    NL_CHECK_RETURN(service != nullptr, "asc service nullptr");

    switch (deviceDescriptor[0]->deviceType_) {
        case AudioStandard::DEVICE_TYPE_SPEAKER:
            HILOGI("networkid is %{public}s, outPutDeviceType_ is %{public}d", deviceDescriptor[0]->networkId_.c_str(),
                static_cast<int>(outPutDeviceType_));
            if (deviceDescriptor[0]->networkId_ == "LocalDevice" &&
                outPutDeviceType_ == AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK) { // localdevice 表示切到扬声器
                service->SetCurrentDeviceMute();
            }
            break;
        default:
            break;
    }
    outPutDeviceType_ = deviceDescriptor[0]->deviceType_;
}

int32_t ASCService::SetMusicUnmute()
{
    HILOGI("[ASCService]:SetMusicUnmute.");
    auto audioGroupManager = OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().GetGroupManager(
        OHOS::AudioStandard::DEFAULT_VOLUME_GROUP_ID);
    NL_CHECK_RETURN_RET(audioGroupManager != nullptr, NL_ERR_INTERNAL_ERROR, "audioGroup nullptr");
    auto ret = audioGroupManager->SetMute(
        AudioStandard::AudioVolumeType::STREAM_MUSIC, false, AudioStandard::DEVICE_TYPE_SPEAKER);
    if (ret != 0) {
        HILOGE("set unmute failed");
        return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}


void ASCService::AcbSubrateChanged(const RawAddress &device, uint32_t subrate)
{
    HILOGI("[ASCService] AcbSubrateChanged %{public}s subrate: %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), subrate);
    ASCMessage event(ASC_SUBRATE_CHANGED_EVT);
    event.dev_ = device.GetAddress();
    event.subrate_ = subrate;
    PostEvent(event);
}

void ASCService::SetSubratePreConfigStream(const RawAddress &device)
{
    SleAcbSubrateParam subrateParam {};
    subrateParam.onlySubrate = true;
    subrateParam.subrate = static_cast<NLSTK_SubrateType_E>(NLSTK_DEFAULT_SUBRATE);
    SetSubrate(device, subrateParam);
}

/**
 * 收到上一次完成后继续处理之前未设置的subrate下发
 * needConfigStream 含义是串行处理未完成设置的subrate后,是否继续配置起流
 * device  含义是对端外设地址
 * subrate 含义是最新底层设置成功的值
 */
void ASCService::SerialManagerSubrate(bool &needConfigStream, const RawAddress &device, uint16_t subrate)
{
    if (IsASCNeedStartStreamChangeSubrate(device)) {
        SetASCStartStreamChangeSubrateFlag(device, false);
        SetSubratePreConfigStream(device);
        needConfigStream = false;
        HILOGW("[ASCService] %{public}s Self-healing start stream", GetEncryptAddr(device.GetAddress()).c_str());
    }
    if (subrate != NLSTK_DEFAULT_SUBRATE) {
        needConfigStream = false;
        return;
    }
}

/* 收到上一次完成后继续处理之前未设置的subrate 2下发 */
void ASCService::ProcessCachedSubrate()
{
    if (subrateCachedInfo_.isCachedProc) {
        HILOGI("[ASCService] CachedSubrateChange %{public}s ",
            GetEncryptAddr(subrateCachedInfo_.dev.GetAddress()).c_str());
        SleAcbSubrateParam subrateParam {};
        subrateParam.onlySubrate = true;
        subrateParam.subrate = static_cast<uint16_t>(NLSTK_SUBRATE_2);
        SetSubrate(subrateCachedInfo_.dev, subrateParam);
        subrateCachedInfo_ = {};
    }
}

void ASCService::ProcessSubrateChangedEvent(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    ASCState state = GetASCStatus(device);
    uint16_t subrate = event.subrate_;
    HILOGI("[ASCService] %{public}s subrate: %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), subrate);
    SetASCSubRateStatus(device, NL_SLE_ASC_SETTED);
    bool needConfigStream = true;
    SerialManagerSubrate(needConfigStream, device, subrate);
    if (!needConfigStream) {
        return;
    }
    if (!(IsConfiguring(state))) {
        HILOGD("[ASCService]SubrateChanged state illegeal %{public}s state %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), state);
        return;
    }
    // 状态：重配置流程-已切换subrate
    ASCState stateNew = (state == NL_SLE_ASC_RECONFIGURING) ?
        NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED : NL_SLE_ASC_CONFIG_SUBRATE_CHANGED;
    SetASCStatus(device, stateNew);

    // 音量控制
    SwitchAbsVolumeDevice(device, false);

    // 取出处理中的流类型
    AudioStreamType streamType = GetProcessingStreamType(device);
    // subRate设置后继续配置音频流
    int result = ConfigStreamToActm(device, streamType);
    // 结果检查
    if (result != NL_NO_ERROR) {
        // 上报状态: 音频流打开,失败
        ReportAudioControlComplete(device, streamType, NL_SLE_ASC_CONTROL_CMD_START,
            NL_SLE_ASC_RESULT_FAIL, result);
        return;
    }
    ProcessCachedSubrate();
}

void ASCService::AcbSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
    HILOGI("[ASCService]AcbSubrateChangeReq %{public}s subrate: %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), subrateParam.subrate);
    ASCMessage event(ASC_SUBRATE_CHANGE_REQ_EVT);
    event.dev_ = device.GetAddress();
    event.subratePara_ = subrateParam;
    PostEvent(event);
}

bool ASCService::IsAllowSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &eventParam)
{
    if (GetASCSubRateStatus(device) == NL_SLE_ASC_SETTING) {
        return false;
    }
    if (IsRejectInActivateDeviceReq(device, eventParam.subrate)) {
        return false;
    }

    ASCState state = GetASCStatus(device);
    if (IsInStartProcess(state)) {
        return false;
    } else {
        // 不在起播中subrate设置
        return true;
    }
}

bool ASCService::IsRejectInActivateDeviceReq(const RawAddress &device, uint16_t subrate)
{
    RawAddress reportAddr = GetReportAddr(device);

    // 蓝牙+星闪，蓝牙是出声设备，星闪提速
    if (SleAudioFrameworkAdapter::GetInstance().IsBtOut() && 
        subrate <= NLSTK_DEFAULT_SUBRATE &&
        (SleAudioFrameworkAdapter::GetInstance().IsMusicActive() ||
        SleAudioFrameworkAdapter::GetInstance().IsVoiceCallActive())) {
            // 激活设备正在音频业务中，拒绝非激活设备的subrate切换请求
            HILOGI("[ASCService]reject subrate change req for not activate device");
            return true;
    }
    // 星闪+星闪，星闪是出声设备，星闪提速
    if ((!(activeSinkDevice_.GetAddress().empty()) && reportAddr != activeSinkDevice_) &&
        subrate <= NLSTK_DEFAULT_SUBRATE && 
        (SleAudioFrameworkAdapter::GetInstance().IsMusicActive() ||
        SleAudioFrameworkAdapter::GetInstance().IsVoiceCallActive())) {
            // 激活设备正在音频业务中，拒绝非激活设备的subrate切换请求
            HILOGI("[ASCService]reject subrate change req for not activate device");
            return true;
    }
    return false;
}

void ASCService::SetSubrateCachedInfo(const RawAddress &device, const SleAcbSubrateParam &eventParam)
{
    if (eventParam.subrate == static_cast<uint16_t>(NLSTK_SUBRATE_2)) {
        subrateCachedInfo_.dev = device;
        subrateCachedInfo_.isCachedProc = true;
    }
}

void ASCService::ProcessSubrateChangeReq(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    const SleAcbSubrateParam &eventParam = event.subratePara_;
    if (!IsAllowSubrateChangeReq(device, eventParam)) {
        SetSubrateCachedInfo(device, eventParam);
        HILOGW("[ASCService] dev:%{public}s subrate: %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            eventParam.subrate);
        return;
    }
    HILOGI("[ASCService] dev:%{public}s subrate: %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        eventParam.subrate);
    SleAcbSubrateParam subrateParam;
    subrateParam.onlySubrate = false;
    subrateParam.subrate = eventParam.subrate;
    subrateParam.subrateMax = eventParam.subrateMax;
    subrateParam.maxLatency = eventParam.maxLatency;
    subrateParam.continuationNum = eventParam.continuationNum;
    subrateParam.supervisionTimeout = eventParam.supervisionTimeout;
    SetSubrate(device, subrateParam);
}

void ASCService::UpdateDeviceNature(const RawAddress &device)
{
    ASCMessage event(ASC_UPDATE_NATURE_EVT);
    event.dev_ = device.GetAddress();
    PostEvent(event);
}

void ASCService::ProcessUpdateDeviceNature(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    RawAddress reportAddr = GetReportAddr(device);
    if (reportAddr != activeSinkDevice_) {
        HILOGI("[ASCService]not allow notify dsp Update finish %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    // 更改参数更新标记位
    SetQosmInfoUpdateFlag(device, true);
    RawAddress coSetDevice;
    if (IsCoSetDeviceExist(device, coSetDevice)) {
        SetQosmInfoUpdateFlag(coSetDevice, true);
    }

    // 通知DSP链路参数
    std::string channelInfo = GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_START);
    SetAudioParameter(NL_SLE_ASC_CONTROL_CMD_START, "", channelInfo);

    HILOGI("[ASCService]Update nature finish %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
}

void ASCService::ProcessStackLocationChangeCbk(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    bool isLeft = event.isLeft_;
    HILOGI("[ASCService]LocationChangeCbk %{public}s isLeft: %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isLeft);
    bool isFind = false;
    auto fun = [device, isLeft, &isFind](const std::string& addr, ASCConnectedDev& data) -> void {
        if (device.GetAddress() == addr) {
            // 更新设备的左右耳属性
            data.isLeft = isLeft;
            isFind = true;
            HILOGI("[ASCService]find device and update location prop success: isLeft=%{public}d", data.isLeft);
        }
    };
    connectedDev_.GetValueAndOpt(device.GetAddress(), fun);

    if (isFind) {
        UpdateDeviceNature(device);
    }
}

void ASCService::ProcessStackStreamTypeChangeCbk(const ASCMessage &event)
{
    RawAddress device(event.dev_);
    uint32_t streamType = event.availableStreamType_;
    std::vector<AudioStreamInfo> streamData = { };
    uint32_t arraySize = sizeof(ASC_AUDIO_STREAM_TYPE_LIST) / sizeof(uint32_t);
    for (uint32_t index = 0; index < arraySize; index++) {
        struct AudioStreamInfo data;
        data.streamType = ASC_AUDIO_STREAM_TYPE_LIST[index];
        data.streamState = (streamType & (1 << index)) ? AUDIO_STREAM_STATE_AVAILABLE :
            AUDIO_STREAM_STATE_NOT_AVAILABLE;
        streamData.push_back(data);
    }

    int action = static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_FROM_REMOTE);

    /* 检查流类型是否变换， 有以下三种情况：
       1. 耳机业务为音乐流，向非业务手机发送音乐不可用通话可用，action为disable，禁用音乐
       2. 耳机业务为通话流，向非业务手机发送音乐通话均不可用，action为disable，禁用音乐和通话
       3. 耳机业务释放，向所有手机发送音乐和通话均可用，action为enable，启用音乐和通话 */
    if ((streamType & AUDIO_STREAM_MUSIC) == 0 || (streamType & AUDIO_STREAM_VOICE_CALL) == 0) {
        action = static_cast<int>(UpdateOutputStackAction::ACTION_DISABLE_FROM_REMOTE);
    }

    HILOGI("[ASCService]:audio stream state changed,action:%{public}d,streamType:0x%{public}x", action, streamType);

    SleAudioDeviceActionChanged(NearlinkRawAddress(device), streamData, action);
}

void ASCService::SetSubrate(const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    SetASCSubRateStatus(device, NL_SLE_ASC_SETTING);
    bool ret = false;
    ServiceManagerPluginLoader::GetInstance()->SetAcbSubrate(ret, device, subrateParam);
    if (!ret) {
        HILOGE("set acb subrate failed");
        ASCMessage event(ASC_SUBRATE_CHANGED_EVT);
        event.dev_ = device.GetAddress();
        event.subrate_ = subrateParam.subrate;
        PostEvent(event);
    }
}

void ASCService::SetAutoConnectDevice(const RawAddress &device, bool isActive)
{
    RawAddress reportAddr(device);
    CdsmService* cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(device, reportAddr);
    }
    int pairState = SleRemoteDeviceAdapter::GetInstance()->GetPairState(reportAddr);
    std::string btAddr = "";
    SleRemoteDeviceAdapter::GetInstance()->GetBtAddrBySleAddrTask(reportAddr.GetAddress(), btAddr);
    SleReconnectManager::GetInstance().SetAutoConnectDevice(reportAddr, isActive, pairState, btAddr);
}

bool ASCService::GetLocalDualRecordAbility()
{
    std::unique_lock<std::mutex> lock(dualRecCapStateMutex_);
    if (dualRecCapState_ == UNKNOWN) {
        dualRecCapState_ = NearlinkSystemConfig::IsDualRecordSupported() ? SUPPORTED : NOT_SUPPORTED;
    }
    return dualRecCapState_ == SUPPORTED;
}

bool ASCService::GetDualRecordAbility(const RawAddress &device)
{
    bool isLocalSupport = GetLocalDualRecordAbility();
    // 外设能力
    int dualRecordIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_DUAL_EAR_HIGH_QUALITY_RECORDING);
    bool isPeerSupport = (dualRecordIndex >= 0) &&
        SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(device, static_cast<uint8_t>(dualRecordIndex));
    HILOGI("[ASCService] isLocalSupport:%{public}d, isPeerSupport:%{public}d", isLocalSupport, isPeerSupport);
    bool res = (isLocalSupport && isPeerSupport);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, DUAL_REC_CAP, res);
    return res;
}

bool ASCService::GetLocalKaraokeAbility()
{
    std::unique_lock<std::mutex> lock(karaokeCapStateMutex_);
    karaokeCapState_ = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    return karaokeCapState_ == SUPPORTED;
}

bool ASCService::GetKaraokeAbility(const RawAddress &device)
{
    bool isLocalSupport = GetLocalKaraokeAbility();
    int karaokeIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_DUAL_EAR_KARAOKE);
    bool isPeerSupport = (karaokeIndex >= 0) &&
        SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(device, static_cast<uint8_t>(karaokeIndex));
    HILOGI("[ASCService] Karaoke isLocalSupport:%{public}d, isPeerSupport:%{public}d, karaokeIndex:%{public}d",
        isLocalSupport, isPeerSupport, karaokeIndex);
    return isLocalSupport && isPeerSupport;
}

void ASCService::NotifyVoiceCallAutorateAblityToActm(const RawAddress& device, const AscQosmInfo& qosmInfo)
{
    QosM& qosM = QosM::GetInstance();
    Qos qos = qosM.GetCOS(device);
    if (!IsVoiceCallService(qos) || !GetLongRangeVoiceCallAbility(device)) {
        return;
    }
    HILOGI("device %{public}s in voice call, support frame4 & autorate", GET_ENCRYPT_ADDR(device));
    uint32_t groupId = GetGroupIdByAddress(device);
    NL_CHECK_RETURN(groupId != CDSM_SERVICE_INVALID_GROUP_ID, "[ASCService] invalid group id.");
    NLSTK_ActmAutoRateRecvMsg_S msg;
    msg.qosId = groupId;
    msg.qosIndex = qosmInfo.qosIndex;
    msg.msgType = ASC_NTF_VOICE_CALL_AUTORATE_ABILITY;
    msg.result = NL_SLE_ASC_RESULT_SUCC;
    int ret = SleASC::SendVoiceCallAutoRateMsg(device, msg);
    NL_CHECK_RETURN(ret == NL_NO_ERROR, "[ASCService]SendVoiceCallAutoRateMsg err, ret %{public}d", ret);
}

bool ASCService::GetLocalVocieCallFrameFourAbility()
{
    bool isSupport = false;
    ServiceManagerPluginLoader::GetInstance()->GetLocalVocieCallFrameFourAbility(isSupport);
    return isSupport;
}

bool ASCService::GetLongRangeVoiceCallAbility(const RawAddress &device)
{
    // 本端帧4通话能力，通过查询芯片feature获取
    bool isLocalFrameFourSupport = GetLocalVocieCallFrameFourAbility();

    // 对端帧4通话 & Autorate能力，通过双端能力协商获取
    int callAutorate = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_VOICE_CALL_AUTORATE);
    bool isPeerAutorateSupport = (callAutorate >= 0) &&
        SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(device, static_cast<uint8_t>(callAutorate));

    int callFrameFour = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_VOICE_CALL_FRAME_FOUR);
    bool isPeerFrameFourSupport = (callFrameFour >= 0) &&
        SleRemoteDeviceAdapter::GetInstance()->GetManufacturerAbility(device, static_cast<uint8_t>(callFrameFour));

    return isLocalFrameFourSupport && isPeerAutorateSupport && isPeerFrameFourSupport;
}

ASCSubRateState ASCService::GetASCSubRateStatus(const RawAddress &device)
{
    // 如果还没有记录状态设置默认值
    ASCSubRateState state = NL_SLE_ASC_INIT;
    auto it = ascSubrateMap_.find(device.GetAddress());
    if (it != ascSubrateMap_.end()) {
        state = it->second.subrateState;
    }
    HILOGI("[ASCService] %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
    return state;
}

void ASCService::SetASCSubRateStatus(const RawAddress &device, ASCSubRateState state)
{
    HILOGI("[ASCService] %{public}s state %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), state);
    ascSubrateMap_[device.GetAddress()].subrateState = state;
}

bool ASCService::IsASCNeedStartStreamChangeSubrate(const RawAddress &device)
{
    bool val = false;
    auto it = ascSubrateMap_.find(device.GetAddress());
    if (it != ascSubrateMap_.end()) {
        val = it->second.isStartStrChangeSubrate;
    }
    return val;
}

void ASCService::SetASCStartStreamChangeSubrateFlag(const RawAddress &device, bool val)
{
    ascSubrateMap_[device.GetAddress()].isStartStrChangeSubrate = val;
}

void ASCService::ClearASCSubrateInfo(const RawAddress &device)
{
    ascSubrateMap_.erase(device.GetAddress());
    subrateCachedInfo_ = {};
}
REGISTER_CLASS_CREATOR(ASCService);

} // namespace Nearlink
} // namespace OHOS