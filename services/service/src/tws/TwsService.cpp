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
#include "TwsService.h"
#include "ManufacturerAbilityLoader.h"
#include "nearlink_dft_exception.h"
#include <sys/time.h>
#include <ctime>
#include <chrono>
#include <openssl/sha.h>
#include "ClassCreator.h"
#include "sle_uuid.h"
#include "SleConfig.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileASC.h"
#include "ASCService.h"
#include "CdsmService.h"
#include "VcpDefines.h"
#include "sysdep.h"
#include "ThreadUtil.h"
#include "nearlink_errorcode.h"
#include "McpDefines.h"
#include "McpServerService.h"
#include "audio_devices_client_manager.h"
#include "audio_volume_client_manager.h"
#include "audio_device_info.h"
#include "account_info.h"
#include "ohos_account_kits.h"
#include "os_account_manager.h"
#include "SleUtils.h"
#include "parameters.h"
#include "SleAudioFrameworkAdapter.h"
#include "SleRemoteDeviceAdapter.h"
#include "nlstk_cfgdb_api.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint32_t AUDIO_DELAY_MS = 400;
constexpr int TWS_HANG_UP_DEFAULT_VALUE = -1;
constexpr int64_t AFTER_HANG_UP_SHOW_CAPSULE_TIME_DIFF_MS = 1500; /* 单位:毫秒 */
constexpr uint8_t SLE_PROFILE_STATE_CONNECTED = 1;                /* 服务状态：已连接 */
}  // namespace

uint8_t TwsService::GetScenesFromQoS(uint8_t qosId)
{
    const std::map<uint8_t, uint8_t> SCENE_QOS_MAP = {
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_1), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_MUSIC)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_2), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_GAME)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_3), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_CALL)},
        {
            static_cast<uint8_t>(Qos::NL_SLE_QOS_4),
            static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_SPATIAL_AUDIO)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_5), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_6), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_7), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS)},
        {static_cast<uint8_t>(Qos::NL_SLE_QOS_9), static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS)}
    };

    NL_CHECK_RETURN_RET(qosId >= NL_SLE_QOS_1 && qosId <= NL_SLE_QOS_9,
        static_cast<uint8_t>(AudioScene::SLE_AUDIO_SCENE_OTHERS),
        "[Tws Service]: GetScenesFromQoS with invalid qosId %{public}d .", qosId);
    return SCENE_QOS_MAP.at(qosId);
}

/* 服务启动、退出日志 */
TwsService::TwsService() : utility::Context(PROFILE_NAME_TWS, "1.0.0")
{
    HILOGI("%{public}s: service Create", PROFILE_NAME_TWS.c_str());
}

TwsService::~TwsService()
{
    HILOGI("%{public}s: service Destroy", PROFILE_NAME_TWS.c_str());
}

/************************* 服务框架依赖的服务接口 Start ****************************/
/* 服务连接 */
int TwsService::Connect(const RawAddress &device)
{
    HILOGD("[Tws service]:connect Enter,dev addr:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    if (isShuttingDown_.load()) {
        HILOGI("[Tws Service]:addr:%{public}s is shutting down", GetEncryptAddr(device.GetAddress()).c_str());
        return NL_ERR_SERVICE_DISCONNECTED;
    }

    /* 任意设备连接时触发动态库加载 */
    NL_CHECK_RETURN_RET(TwsSharedLibApi::GetInstance().Init(), NL_ERR_INTERNAL_ERROR,
        "[Tws Service]:init shared lib failed!!");

    TwsMessage event(TWS_SERVCICE_CONNECT_START_EVT);
    event.dev_ = device.GetAddress();
    PostEvent(event);
    return NL_NO_ERROR;
}

/* 服务断开 */
int TwsService::Disconnect(const RawAddress &device)
{
    HILOGD("Tws service,disconnect Enter,dev addr:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    if (isShuttingDown_.load()) {
        HILOGI("[Tws Service]:addr:%{public}s is shutting down", GetEncryptAddr(device.GetAddress()).c_str());
        return NL_ERR_SERVICE_DISCONNECTED;
    }

    TwsMessage event(TWS_SERVICE_DISCONNECT_START_EVT);
    event.dev_ = device.GetAddress();
    PostEvent(event);
    return NL_NO_ERROR;
}

int TwsService::GetConnectState()
{
    return 0;
}

std::list<RawAddress> TwsService::GetConnectDevices()
{
    std::list<RawAddress> devices;
    return devices;
}

utility::Context *TwsService::GetContext()
{
    return this;
}

TwsService *TwsService::GetService()
{
    return static_cast<TwsService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
}

void TwsService::RegisterObserver(TwsObserver &serviceObserver)
{
    HILOGI("Enter");
    twsObservers_.Register(serviceObserver);
}

void TwsService::DeregisterObserver(TwsObserver &serviceObserver)
{
    HILOGI("Enter");
    twsObservers_.Deregister(serviceObserver);
}

/************ 服务消息处理入口 ***************/
void TwsService::PostEvent(const TwsMessage &event)
{
    DoInTwsThread([this, event]() { this->ProcessEvent(event); });
}

void TwsService::ProcessEvent(const TwsMessage &event)
{
    switch (event.whatM) {
        case TWS_SERVICE_STARTUP_EVT:
            StartUp();
            break;
        case TWS_SERVICE_SHUTDOWN_EVT:
            ShutDown();
            break;
        case TWS_SERVCICE_CONNECT_START_EVT:
            ProcessConnectEvent(event);
            break;
        case TWS_SERVICE_DISCONNECT_START_EVT:
            ProcessDisconnectEvent(event);
            break;
        case TWS_SERVICE_SSAP_RECV_DATA_EVENT:
            TwsServiceStartDecodeRecvData(event);
            break;
        case TWS_SERVICE_SSAP_SEND_DATA_EVENT:
            TwsServiceSendDataBySsap(event);
            break;
        case TWS_SERVICE_RECV_REQ_EVENT:
        case TWS_SERVICE_RECV_RSP_EVENT:
            TwsServiceProcessRecvMessage(event);
            break;
        case TWS_SERVICE_SEND_REQ_EVENT:
            TwsServiceProcessSendReq(event);
            break;
        case TWS_SERVICE_SEND_RSP_EVENT:
            TwsServiceProcessSendRsp(event);
            break;
        case TWS_SERVICE_SEND_REMOTE_INFO_EVENT:
            TwsServiceProcessSendRemoteInfo(event);
            break;
        default:
            HILOGI("[Tws Service]:event type not support:%{public}d", event.whatM);
            break;
    }
}

/** 根据产品配置更新双耳录音能力 */
void TwsService::UpdateDualRecordAbility()
{
    int featureIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_DUAL_EAR_HIGH_QUALITY_RECORDING);
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "ASCService is null.");
    ManufacturerAbilityLoader::GetInstance().SetLocalAbility(featureIndex, ascService->GetLocalDualRecordAbility());
}

/** 根据产品配置更新双耳K歌能力 */
void TwsService::UpdateDualKaraokeAbility()
{
    int featureIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_DUAL_EAR_KARAOKE);
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "ASCService is null.");
    ManufacturerAbilityLoader::GetInstance().SetLocalAbility(featureIndex, ascService->GetLocalKaraokeAbility());
}

void TwsService::UpdateVoiceCallFrameFourAndAutoRateAbility()
{
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "ASCService is null.");
    int callAutoRateFeatureIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_VOICE_CALL_AUTORATE);
    ManufacturerAbilityLoader::GetInstance().SetLocalAbility(callAutoRateFeatureIndex, true);

    int frameFourCallFeatureIndex = ManufacturerAbilityLoader::GetInstance().GetAbilityIndex(
        MANU_ABILITY_VOICE_CALL_FRAME_FOUR);
    ManufacturerAbilityLoader::GetInstance().SetLocalAbility(frameFourCallFeatureIndex,
        ascService->GetLocalVocieCallFrameFourAbility());
}

/* 服务消息处理接口 */
void TwsService::StartUp()
{
    HILOGI("==========<tws service start>==========");
    if (isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_TWS, true);
        HILOGW("TwsService has already been started before.");
        return;
    }
    /* 更新本端双耳能力位图 */
    UpdateDualRecordAbility();
    UpdateVoiceCallFrameFourAndAutoRateAbility();

    /* 更新本端双耳K歌能力位图 */
    UpdateDualKaraokeAbility();

    /* 获取本端能力位图 */
    manuAbility_ = ManufacturerAbilityLoader::GetInstance().GetLocalAbility();

    GetContext()->OnEnable(PROFILE_NAME_TWS, true);
    isStarted_.store(true);
    HILOGI("TwsService started");
}

void TwsService::ShutDown()
{
    HILOGD("Enter");
    if (!isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_TWS, true);
        HILOGW("[Tws Service]:already been shutdown before.");
        return;
    }

    isShuttingDown_.store(true);

    twsClientDatas_.Clear();
    pauseRecordMap_.Clear();
    twsDevWearStatus_.Clear();
    msgCache_.Clear();

    GetContext()->OnDisable(PROFILE_NAME_TWS, true);
    isStarted_.store(false);
    isShuttingDown_.store(false);
    HILOGI("TwsService shutdown");
}

void TwsService::Enable()
{
    HILOGI("Enter");

    TwsMessage event(TWS_SERVICE_STARTUP_EVT);
    PostEvent(event);
}

void TwsService::Disable()
{
    TwsSharedLibApi::GetInstance().DeInit();    /* 服务注销时，动态库卸载 */

    TwsMessage event(TWS_SERVICE_SHUTDOWN_EVT);
    PostEvent(event);
}

void TwsService::UpdateClientState(const RawAddress &device, int newState)
{
    auto updateState = [newState](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        clientData->clientState = newState;
    };
    if (!twsClientDatas_.GetValueAndOpt(device.GetAddress(), updateState)) {
        std::shared_ptr<TwsClientData> clientData = CreateNewClientData(device);
        clientData->clientState = newState;
    }
}

void TwsService::PauseRecordProcessWhenDisconnect(const RawAddress &device)
{
    HILOGI("[Tws Service]:remove pause record,addr:%{public}s", GET_ENCRYPT_ADDR(device));
    pauseRecordMap_.Erase(device.GetAddress());
    twsDevWearStatus_.Erase(device.GetAddress());
    hangUpRecord_.Erase(device.GetAddress());
}

void TwsService::ResetStreamState(std::vector<NearlinkCdsmInfo> &cdsmInfo)
{
    for (auto &member : cdsmInfo) {
        auto clearStream = [](std::string dev, std::shared_ptr<TwsClientData> clientData) {
            /* 音频流状态重置为可用 */
            (void)memset_s(clientData->audioState_.audioType, NEARLINK_STREAM_TYPE_NUM,
                static_cast<uint8_t>(TwsStreamUsability::AVAILABLE), NEARLINK_STREAM_TYPE_NUM);
        };
        twsClientDatas_.GetValueAndOpt(member.addr_.GetAddress(), clearStream);
    }
}

/* 设备断链后处理 */
void TwsService::DisconnectProcess(const RawAddress &device)
{
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    std::vector<NearlinkCdsmInfo> cdsmInfo = { };
    if (cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
        return;
    }

    HILOGD("cdsmInfo.size=%{public}d", cdsmInfo.size());
    RawAddress reportAddr = device;
    for (auto &member : cdsmInfo) {
        int state = static_cast<int>(SleConnectState::CONNECTED);
        auto getState = [&state](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
            state = clientData->clientState;
        };
        /* 任意设备未断链，直接返回 */
        NL_CHECK_RETURN_LOGD(twsClientDatas_.GetValueAndOpt(member.addr_.GetAddress(), getState) &&
            state == static_cast<int>(SleConnectState::DISCONNECTED), "no dev disconnected");
        if (member.isReportAddr_) {
            reportAddr = member.addr_;
        }
    }

    /* 所有设备都断开，清理暂停记录，重置音频流状态 */
    PauseRecordProcessWhenDisconnect(reportAddr);
    ResetStreamState(cdsmInfo);
}

/* tws服务客户端连接状态变更 */
void TwsService::NotifyStateChanged(const RawAddress &device, TwsClientState preState, TwsClientState toState)
{
    int newState = stateMap_.at(toState);
    int oldState = stateMap_.at(preState);

    /* 连接后发送数据传输能力 */
    if (newState == static_cast<int>(SleConnectState::CONNECTED)) {
        TwsHiBoxParser::GetInstance().SendMessage(device,
            static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), nullptr, 0, false);
        TwsSendAccountInfo(device);
        SendAllCacheMessage(device); /* 连接后发送缓存消息队列 */
    }

    twsObservers_.ForEach([device, newState, oldState](TwsObserver &observer) {
        observer.OnConnectionStateChanged(device, newState, oldState);
    });

    UpdateClientState(device, newState);

    /* 连接断开时清理数据 */
    if (newState == static_cast<int>(SleConnectState::DISCONNECTED)) {
        DisconnectProcess(device);
        // 删除当前地址对应的client对象
        twsClient_.Erase(device.GetAddress());
    }
}

void TwsService::ProcessConnectEvent(const TwsMessage &event)
{
    HILOGD("[Tws Service]:connect event enter,addr:%{public}s", GetEncryptAddr(event.dev_).c_str());
    auto func = [&event](std::string address, std::shared_ptr<TwsClient> clientInstance) -> void {
        clientInstance->ConnectTws();
    };

    bool ret = twsClient_.GetValueAndOpt(event.dev_, func);
    if (!ret) {
        HILOGD("[Tws Service]:new client,addr:%{public}s", GetEncryptAddr(event.dev_).c_str());
        std::shared_ptr<TwsClient> newClient = TwsClient::CreateTwsClient(event.dev_);
        if (newClient == nullptr) {
            HILOGE("new client alloc failed");
            RawAddress addr(event.dev_);
            NotifyStateChanged(addr, TwsClientState::TWS_STATE_CONNECTING, TwsClientState::TWS_STATE_DISCONNECTED);
            return;
        }
        newClient->ConnectTws();
        twsClient_.EnsureInsert(event.dev_, newClient);
    }
}

void TwsService::ProcessDisconnectEvent(const TwsMessage &event)
{
    HILOGD("Tws Service disconnect event enter,addr:%{public}s", GetEncryptAddr(event.dev_).c_str());
    auto func = [&event](std::string address, std::shared_ptr<TwsClient> clientInstance) -> void {
        clientInstance->DisconnectTws();
    };

    bool ret = twsClient_.GetValueAndOpt(event.dev_, func);
    if (!ret) {
        HILOGE("[Tws Service]:disconnect tws failed,addr:%{public}s", GetEncryptAddr(event.dev_).c_str());
    }
}

int TwsService::RegisterApplication(const std::shared_ptr<InterfaceTwsClientObserver> &callback)
{
    HILOGI("RegisterApplication");
    callback_ = callback;
    return NL_NO_ERROR;
}

int TwsService::DeregisterApplication()
{
    HILOGD("DeregisterApplication");
    callback_ = nullptr;
    return NL_NO_ERROR;
}

/* 内部逻辑：将编码后的数据通过ssap发送 */
void TwsService::TwsServiceSendDataBySsap(const TwsMessage &event)
{
    auto sendProc = [event](std::string peerAddr, std::shared_ptr<TwsClient> clientInstance) -> void {
        NL_CHECK_RETURN(clientInstance, "[Tws Service]:send msg fail,client instance invalid,addr:%{public}s.",
            GetEncryptAddr(peerAddr).c_str());

        std::vector<uint8_t> dataTmp(event.dataStream_.get(), event.dataStream_.get() + event.streamLen_);
        int sendRet = clientInstance->SendData(dataTmp);
        NL_CHECK_RETURN(sendRet == NL_NO_ERROR, "[Tws Service]:send msg fail,client send fail:%{public}d.", sendRet);
    };
    if (!twsClient_.GetValueAndOpt(event.dev_, sendProc)) {
        HILOGE("[Tws Service]:send msg fail,client(%{public}s) instanc not existed.",
            GetEncryptAddr(event.dev_).c_str());
    }
}

void TwsService::TwsServiceStartDecodeRecvData(const TwsMessage &event)
{
    /* 调用解码 */
    if (!TwsSharedLibApi::GetInstance().DecodeMessage(event)) {
        HILOGD("[Tws Service]:recv ssap data,decode message failed.");
        return;
    }
}

std::shared_ptr<TwsClientData> TwsService::CreateNewClientData(RawAddress peerAddr)
{
    std::shared_ptr<TwsClientData> clientDataPtr = std::make_shared<TwsClientData>(peerAddr);
    twsClientDatas_.EnsureInsert(peerAddr.GetAddress(), clientDataPtr);
    return clientDataPtr;
}

void TwsService::TwsServiceSendReqInner(const TwsMessage &event)
{
    HILOGI("enter");
    TwsServiceProcessSendReq(event);
}

/* 调用动态库接口编码消息 */
void TwsService::TwsServiceProcessSendReq(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.msgDirect_ == TWS_MSG_DIRECT_REQ,
        "[Tws Service]:send data transfer request message fail,direct invalid:%{public}u.", event.msgDirect_);

    bool ret = TwsSharedLibApi::GetInstance().EncodeMessage(event);
    NL_CHECK_RETURN(ret, "[Tws Service]:send data transfer request message fail,encode failed,addr:%{public}s.",
        GetEncryptAddr(event.dev_).c_str());
}

void TwsService::TwsServiceProcessSendRsp(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.msgDirect_ == TWS_MSG_DIRECT_RSP,
        "[Tws Service]:send data transfer response message fail,direct invalid:%{public}u.", event.msgDirect_);
    bool ret = TwsSharedLibApi::GetInstance().EncodeMessage(event);
    NL_CHECK_RETURN(ret, "[Tws Service]:send data transfer response message fail,encode failed,addr:%{public}s.",
        GetEncryptAddr(event.dev_).c_str());
}

void TwsService::TwsServiceProcessSendRemoteInfo(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.arg2M, "[Tws Service]:TwsServiceProcessSendRemoteInfo, arg2M is nullptr.");
    std::string value(static_cast<const char*>(event.arg2M));
    std::vector<uint8_t> vec(value.begin(), value.end());

    if (callback_ != nullptr) {
        RawAddress addr(event.dev_);
        RawAddress reportAddr = TwsService::GetReportAddr(addr);
        callback_->OnTwsRemoteInfo(reportAddr.GetAddress(), vec);
    }
    delete[] static_cast<char *>(event.arg2M);
}

/* 收到对端（请求/响应）消息 */
void TwsService::TwsServiceProcessRecvMessage(const TwsMessage &event)
{
    HILOGD("[Tws service]:recv msg,addr:%{public}s", GetEncryptAddr(event.dev_).c_str());
    bool ret = TwsHiBoxParser::GetInstance().RecvMessage(event);
    NL_CHECK_RETURN_LOGD(ret, "[Tws Service]:recv message process failed.");
}

/* 更新本端数据，或者创建本端数据节点 */
void TwsService::UpdateClientDataOrCreate(uint8_t dataType, TwsClientData &clientData)
{
    std::shared_ptr<TwsClientData> dataInstance;
    if (!twsClientDatas_.GetValue(clientData.peerAddr_.GetAddress(), dataInstance)) {
        std::shared_ptr<TwsClientData> clientDataPtr = CreateNewClientData(clientData.peerAddr_);
        clientDataPtr->UpdateData(dataType, clientData);
        return;
    }
    dataInstance->UpdateData(dataType, clientData);
}

/* 音频流状态变化上报 */
void TwsService::OnProfileStateChange(TwsClientData &clientData)
{
    uint8_t preMediaState = static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    uint8_t preCallState = static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    bool ret = GetProfileStatus(clientData.peerAddr_.GetAddress(), preMediaState, preCallState);

    /* 新来状态和前一次状态比较：如果新来是INVALID, 需要和前一次保持一致 */
    uint8_t newMediaState = clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)];
    uint8_t newCallState = clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)];
    uint8_t mediaState =
        newMediaState == static_cast<uint8_t>(TwsStreamUsability::INVALID) ? preMediaState : newMediaState;
    uint8_t callsState =
        newCallState == static_cast<uint8_t>(TwsStreamUsability::INVALID) ? preCallState : newCallState;
    clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] = mediaState;
    clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)] = callsState;

    /* 检查流类型是否变换 */
    int action = static_cast<int>(UpdateOutputStackAction::ACTION_DISABLE_FROM_REMOTE);
    if (newMediaState == static_cast<uint8_t>(TwsStreamUsability::AVAILABLE) ||
        newCallState == static_cast<uint8_t>(TwsStreamUsability::AVAILABLE)) {
        action = static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_FROM_REMOTE);
    }
    HILOGI("[Tws service]:audio stream state changed,action:%{public}u,media:%{public}u,cellCall:%{public}u,"
        "preMediaState:%{public}u,preCallState:%{public}u,newMediaState:%{public}u,newCallState:%{public}u",
        action, mediaState, callsState, preMediaState, preCallState, newMediaState, newCallState);

    std::vector<struct AudioStreamInfo> streamData = { };
    uint32_t arraySize = sizeof(ASC_AUDIO_STREAM_TYPE_LIST) / sizeof(uint32_t);
    for (uint32_t index = 0; index < arraySize; index++) {
        struct AudioStreamInfo data;
        data.streamType = ASC_AUDIO_STREAM_TYPE_LIST[index];
        data.streamState = static_cast<AudioStreamState>(clientData.audioState_.audioType[index]);
        streamData.push_back(data);
    }
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    ascService->SleAudioDeviceActionChanged(NearlinkRawAddress(clientData.peerAddr_), streamData, action);
}

bool TwsService::GetProfileStatus(const std::string addr, uint8_t &mediaState, uint8_t &callState)
{
    std::shared_ptr<TwsClientData> dataInstance;
    if (twsClientDatas_.GetValue(addr, dataInstance)) {
        mediaState = dataInstance->audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)];
        callState = dataInstance->audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)];
        return true;
    }
    return false;
}

void TwsService::SendCapsuleInfo(RawAddress &devAddr, uint8_t mediaState, std::string &devName)
{
    std::string capsuleInfo = "+PROFILESTATE=";
    capsuleInfo.append(std::to_string(mediaState));
    capsuleInfo.append(",");
    capsuleInfo.append(devName);

    std::vector<uint8_t> vec(capsuleInfo.begin(), capsuleInfo.end());
    vec.push_back('\0');

    if (callback_ != nullptr) {
        RawAddress reportAddr = TwsService::GetReportAddr(devAddr);
        callback_->OnTwsRemoteInfo(reportAddr.GetAddress(), vec);
    }

    HILOGI("[tws service]:send capsule info:%{public}s,state:%{public}u",
        GET_ENCRYPT_ADDR(devAddr), mediaState);
}

/* 胶囊显示控制 */
void TwsService::ShowCapsule(TwsClientData &clientData)
{
    /* step1: 获取媒体流状态 */
    uint8_t mediaState = GetMediaState(clientData);

    /* step2: 出声栈状态检查 */
    RawAddress devAddr(clientData.peerAddr_);
    RawAddress otherAddr(devAddr);
    RawAddress reportAddr(devAddr);
    CdsmService* cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetOtherAddr(devAddr, otherAddr);
        cdsmService->CdsmGetReportAddr(devAddr, reportAddr);
    }

    /* 是否当前星闪设备出声 */
    ProfileASC *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "[Tws Service]:ShowCapsule, ascService is nullptr.");
    NearlinkRawAddress device = ascService->GetActiveSinkDevice();
    bool isActiveDevNow = ((device.GetAddress() == devAddr.GetAddress()) ||
        (device.GetAddress() == otherAddr.GetAddress()));

    /* 是否星闪设备出声 */
    bool isNearlinkOut = SleAudioFrameworkAdapter::GetInstance().IsNearlinkOut();

    /* 是否星闪音频流播放态 */
    bool isPlaying = ascService->IsPlaying(device);

    /* 是否在星闪电话中 */
    bool isInCalling = ascService->IsCalling();

    /* 是否在星闪媒体流中：非通话且播放态，认为是媒体中 */
    bool isInMedia = !isInCalling && isPlaying;

    /* step3: 判断是否需要胶囊弹框 */
    bool isNeedCapsule = false;
    if (mediaState == static_cast<uint8_t>(TwsMediaState::ENABLE) && !clientData.audioState_.isUserSelected) {
        /* 抢占：当前设备业务中，播控中心选择耳机出声，耳机上报媒体可用，弹窗胶囊 */
        isNeedCapsule = isActiveDevNow && (isInCalling || isInMedia);
    }

    if (mediaState == static_cast<uint8_t>(TwsMediaState::DISABLE)) {
        /* 抢占：当前设备业务中，耳机设备被抢占 */
        isNeedCapsule = isNearlinkOut && (IsMusicActive() || isInCalling);

        /* 抢占：距离上次挂断电话时间小于1.5s */
        int64_t lastTime = GetHangUpTimeStamp(reportAddr);
        int64_t currTime = GetTimeStamp();
        if (lastTime != TWS_HANG_UP_DEFAULT_VALUE) {
            isNeedCapsule = isNeedCapsule || ((currTime - lastTime) <= AFTER_HANG_UP_SHOW_CAPSULE_TIME_DIFF_MS);
        }
        HILOGI("[tws service]:media disable,last time:%{public}lld,curr time:%{public}lld,", lastTime, currTime);
    }

    // 查看媒体音量：媒体音量为0，不弹
    int mediaVolume = SleConfig::GetInstance().GetDeviceMediaVolume(device.GetAddress(), VCP_DEFAULT_VOLUME_LEVEL);
    if (mediaVolume == 0) {
        isNeedCapsule = false;
    }

    /* 耳机检测到空/静音流 */
    if (clientData.audioState_.audioServiceType == static_cast<uint8_t>(TwsAudioServiceType::EMPTY_MUTE)) {
        isNeedCapsule = false;
    }

    HILOGI("[tws service]:show capsule,mediaState=%{public}u,isNearlinkOut:%{public}u,isActiveDevNow:%{public}u,"
        "isInCalling:%{public}u,isInMedia:%{public}u,isNeedCapsule:%{public}u,mediaVolume:%{public}d",
        mediaState, isNearlinkOut, isActiveDevNow, isInCalling, isInMedia, isNeedCapsule, mediaVolume);

    /* step4: 仅抢占时，当前设备业务中，胶囊弹窗 */
    if (isNeedCapsule) {
        std::string targetDevName(reinterpret_cast<const char*>(clientData.audioState_.targetDevName),
            NEARLINK_DEVICE_NAME_LEN);
        SendCapsuleInfo(reportAddr, mediaState, targetDevName);
    }
}

uint8_t TwsService::GetMediaState(TwsClientData &clientData)
{
    if (clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)] ==
        static_cast<uint8_t>(TwsStreamUsability::AVAILABLE)) {
        return static_cast<uint8_t>(TwsMediaState::ENABLE);
    }
    return static_cast<uint8_t>(TwsMediaState::DISABLE);
}

void TwsService::UpdateHangUpTimeStamp(RawAddress &devAddr)
{
    int64_t currentTime = GetTimeStamp();
    hangUpRecord_.EnsureInsert(devAddr.GetAddress(), currentTime);
}

int64_t TwsService::GetHangUpTimeStamp(RawAddress &reportAddr)
{
    int64_t timeStamp = TWS_HANG_UP_DEFAULT_VALUE;
    auto getTime = [&timeStamp](std::string addr, int64_t time) -> void {
        timeStamp = time;
    };
    if (!hangUpRecord_.GetValueAndOpt(reportAddr.GetAddress(), getTime)) {
        HILOGE("[tws service]:get hang up time stamp fail,addr:%{public}s.", GET_ENCRYPT_ADDR(reportAddr));
    }

    return timeStamp;
}

bool TwsService::IsNeedShowCapsule(TwsClientData &clientData)
{
    uint8_t preMediaState = static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    uint8_t preCallState = static_cast<uint8_t>(TwsStreamUsability::AVAILABLE);
    std::shared_ptr<TwsClientData> dataInstance;
    if (twsClientDatas_.GetValue(clientData.peerAddr_.GetAddress(), dataInstance)) {
        preMediaState = dataInstance->audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)];
        preCallState = dataInstance->audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)];
    }
    uint8_t newMediaState = clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)];
    uint8_t newCallState = clientData.audioState_.audioType[static_cast<uint8_t>(TwsStreamType::CELL_CALL)];
    if ((preMediaState != newMediaState) || (preCallState != newCallState)) {
        return true;
    }
    HILOGI("NO need ShowCapsule");
    return false;
}

/* 仅联动数据类型，变化上报一次 */
void TwsService::OnClientDataChanged(uint8_t dataType, TwsClientData &clientData)
{
    switch (dataType) {
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE):
            OnProfileStateChange(clientData);
            if (IsNeedShowCapsule(clientData)) {
                ShowCapsule(clientData);
            }
            break;
        default:
            HILOGD("[tws service]:on client data change,type:%{public}u,addr:%{public}s,no need.",
                dataType, GET_ENCRYPT_ADDR(clientData.peerAddr_));
            break;
    }
}

void TwsService::UpdateClientData(uint8_t dataType, TwsClientData &clientData)
{
    OnClientDataChanged(dataType, clientData);
    UpdateClientDataOrCreate(dataType, clientData);

    /* 数据联动 */
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    RawAddress otherAddr(clientData.peerAddr_);
    if (!cdsmService->CdsmGetOtherAddr(clientData.peerAddr_, otherAddr)) {
        HILOGI("[tws service]:get other cdsm addr fail,msg:%{public}u,addr:%{public}s",
            dataType, GET_ENCRYPT_ADDR(clientData.peerAddr_));
        return;
    }

    /* 同步数据 */
    switch (dataType) {
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE): {
            uint8_t newRoleType = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
            if (clientData.roleType_ == static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY)) {
                newRoleType = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY);
            }
            clientData.roleType_ = newRoleType;
            break;
        }
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_CAP_QUERY): /*  pass though 数据直接拷贝 */
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH):
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY):
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE):
            break;
        default:
            HILOGD("[tws service]:message type:%{public}u not need data linkage changes,dev:%{public}s.",
                dataType, GET_ENCRYPT_ADDR(clientData.peerAddr_));
            return;
    }
    clientData.peerAddr_ = otherAddr;

    HILOGD("[tws service]:message type:0x%{public}02x need data linkage changes,dev:%{public}s.",
        dataType, GET_ENCRYPT_ADDR(clientData.peerAddr_));

    UpdateClientDataOrCreate(dataType, clientData);
}

void TwsService::SetDeviceManufacturerAbility(const RawAddress &device,
    const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manufacturerAbility) const
{
    SLE_Addr_S stackAddr = {};
    device.ConvertToUint8(stackAddr.addr);
    NLSTK_ManufacturerAbility_S mAbility = {0};
    for (int i = 0; i < SLE_MANU_ABILITY_LEN; ++i) {
        mAbility.ability[i] = manufacturerAbility[i];
    }
    HILOGI("device=%{public}s, ManufacturerAbility[0]=%{public}u", GET_ENCRYPT_ADDR(device), mAbility.ability[0]);
    uint32_t ret = NLSTK_CfgdbSetManufacturerAbility(&stackAddr, &mAbility);
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "ret=%{public}d", ret);
}

void TwsClientData::HandleRoleTypeUpdate(TwsClientData &clientData)
{
    if (clientData.roleType_ == roleType_) {
        HILOGI("[tws service]:role change,role type same:%{public}u,addr:%{public}s",
            roleType_, GET_ENCRYPT_ADDR(clientData.peerAddr_));
        return;
    }

    uint8_t oldRoleType = roleType_;
    roleType_ = clientData.roleType_;

    HILOGI("[Tws Service]:role change,addr:%{public}s,role:%{public}u -> %{public}u",
        GetEncryptAddr(peerAddr_.GetAddress()).c_str(), oldRoleType, roleType_);

    TwsService *twsInstance = TwsService::GetService();
    if (twsInstance != nullptr) {
        twsInstance->NotifyAudioDeviceAction(peerAddr_,
            static_cast<int>(UpdateOutputStackAction::ACTION_ROLE_TYPE_CHANGE));
    }

    ProfileASC *ascProfile = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascProfile, "ascProfile is null");
    (void)ascProfile->UpdateDeviceRole(peerAddr_, clientData.roleType_);
}

void TwsClientData::HandleDtsCapUpdate(TwsClientData &clientData)
{
    size_t vectorSize = clientData.devDtsCap_.size();
    NL_CHECK_RETURN(vectorSize != 0, "[Tws Service]:update dts cap failed,addr:%{public}s",
        GetEncryptAddr(peerAddr_.GetAddress()).c_str());
    devDtsCap_.clear();
    devDtsCap_.resize(vectorSize);
    errno_t secRet = memcpy_s(devDtsCap_.data(), vectorSize, clientData.devDtsCap_.data(), vectorSize);
    NL_CHECK_RETURN(secRet == EOK, "[Tws Service]:update dts cap failed,memcpy ret:%{public}d", secRet);
}

void TwsClientData::HandleWearStatusUpdate(TwsClientData &clientData)
{
    TwsService* serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN(serviceInstance, "TwsService is null.");
    RawAddress reportAddr = serviceInstance->GetReportAddr(peerAddr_);
    TwsDevWearStatus previousWearStatus;
    if (!serviceInstance->twsDevWearStatus_.GetValue(reportAddr.GetAddress(), previousWearStatus)) {
        previousWearStatus.leftStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
        previousWearStatus.rightStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    }
    if ((clientData.wearData_.leftWear == static_cast<uint8_t>(TwsWearDetect::WORN_INVALID)) &&
        (clientData.wearData_.rightWear == static_cast<uint8_t>(TwsWearDetect::WORN_INVALID))) {
        HILOGE("[Tws Service]:addr:%{public}s,wear status invalid,left:%{public}u,right:%{public}u!!",
            GetEncryptAddr(peerAddr_.GetAddress()).c_str(),
            clientData.wearData_.leftWear, clientData.wearData_.rightWear);
        return;
    } else if (clientData.wearData_.leftWear == static_cast<uint8_t>(TwsWearDetect::WORN_INVALID)) {
        HILOGE("[Tws Service]:addr:%{public}s, leftWear status invalid,right:%{public}u!!",
            GetEncryptAddr(peerAddr_.GetAddress()).c_str(), clientData.wearData_.rightWear);
        clientData.wearData_.leftWear = previousWearStatus.leftStatus;
    } else if (clientData.wearData_.rightWear == static_cast<uint8_t>(TwsWearDetect::WORN_INVALID)) {
        HILOGE("[Tws Service]:addr:%{public}s, rightWear status invalid,left:%{public}u!!",
            GetEncryptAddr(peerAddr_.GetAddress()).c_str(), clientData.wearData_.leftWear);
        clientData.wearData_.rightWear = previousWearStatus.rightStatus;
    }
    wearData_ = clientData.wearData_;
    HILOGI("[Tws Service]:HIBOX_WEAR_STATUS, addr:%{public}s, leftWear:%{public}d, rightWear:%{public}d",
        GetEncryptAddr(reportAddr.GetAddress()).c_str(),
        clientData.wearData_.leftWear, clientData.wearData_.rightWear);

    serviceInstance->UpdateDeviceWearState(peerAddr_, clientData.wearData_);
}

void TwsClientData::HandleAtCmdData(TwsClientData &clientData)
{
    uint8_t cmd = clientData.subCmd_;
    switch (cmd) {
        case static_cast<uint8_t>(TwsAtCmdType::VENDOR_BATTERY_DIALOG): /* pass thought，弹框和更新都是同类数据 */
            /* 触发电量弹框 */
        case static_cast<uint8_t>(TwsAtCmdType::UPDATE_VENDOR_BATTERY):
            vendorBatt_ = clientData.vendorBatt_;
            break;
        case static_cast<uint8_t>(TwsAtCmdType::DEVICE_INFO): {
            devInfo_ = clientData.devInfo_;
            DeviceModel model;
            model.SetModelId(devInfo_.modelId);
            model.SetIconId(devInfo_.iconId);
            model.SetDevType(devInfo_.devType);
            SleInterfaceAdapter *adapter = SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE);
            NL_CHECK_RETURN(adapter, "[Tws Service] adapterInterfaceSle is nullptr.");
            adapter->UpdateDeviceModelInfo(peerAddr_.GetAddress(), model, devInfo_.newModelId);
            break;
        }
        default:
            HILOGE("[Tws Service]:update client data failed,at cmd type invalid:%{public}u.", cmd);
            break;
    }
}

void TwsClientData::HandleAudioExcep(TwsClientData &clientData)
{
    DftAudioExceptionData data;
    if (memcpy_s(&data, sizeof(data), &clientData.audioExcep_, sizeof(clientData.audioExcep_)) != EOK) {
        HILOGW("[HIBOX_AUDIO_HEADSET_EXCEPTION]: DftAudioExceptionData is null.");
        return;
    }
    SleInterfaceAdapter *adapter = SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE);
    if (adapter != nullptr) {
        DftHandleAudioExcep(clientData.peerAddr_, data, adapter->GetLocalAddress());
    } else {
        DftHandleAudioExcep(clientData.peerAddr_, data);
    }
}

/* 多连接特性：echo 5,A 虚拟自动抢占 */
void TwsClientData::HandleQueryPreemption(TwsClientData &clientData)
{
    /* 抢占响应消息 */
    if (clientData.captureRes_ != static_cast<uint8_t>(TwsCaptureResult::INVALID)) {
        HILOGI("[Tws Service]:auto preemption,dev:%{public}s,capture result:%{public}u->%{public}u.",
            GET_ENCRYPT_ADDR(peerAddr_), captureRes_, clientData.captureRes_);
        captureRes_ = clientData.captureRes_;
        return;
    }

    /* 抢占请求消息 */
    virtualAutoConnType_ = clientData.virtualAutoConnType_;
    virtualAutoBussinessType_ = clientData.virtualAutoBussinessType_;

    HILOGI("[Tws Service]:auto preemption req,dev:%{public}s,connect type:%{public}u,bussiness type:%{public}u.",
        GET_ENCRYPT_ADDR(peerAddr_), virtualAutoConnType_, virtualAutoBussinessType_);
}

/* echo 5,e 上报是否可以自动连接 */
void TwsClientData::HandleAutoConnectSwitch(TwsClientData &clientData)
{
    /* 写xml */
    SleConfig::GetInstance().SetAutoConnectSwitch(clientData.peerAddr_.GetAddress(), clientData.autoConnSwitch_);

    HILOGD("[Tws Service]:auto connect switch,dev:%{public}s,switch:%{public}u->%{public}u.",
        GET_ENCRYPT_ADDR(peerAddr_), autoConnSwitch_, clientData.autoConnSwitch_);

    /* 刷新数据 */
    autoConnSwitch_ = clientData.autoConnSwitch_;

    SleConfig::GetInstance().Save();
}

/* echo 1,2 上报对端设备的能力位图 */
void TwsClientData::HandleManufacturerAbility(TwsClientData &clientData)
{
    /* 过滤支持的能力位图 */
    ManufacturerAbilityLoader::GetInstance().FilterAbility(clientData.manufacturerAbility_);

    /* 写xml */
    std::string manuAbility = SleUtils::ConvertIntToHexString(
            clientData.manufacturerAbility_.data(), SLE_MANU_ABILITY_LEN);
    SleConfig::GetInstance().SetManufacturerAbility(clientData.peerAddr_.GetAddress(), manuAbility);

    /* 设置协议栈能力位图 */
    SleRemoteDeviceAdapter::GetInstance()->UpdateDeviceManufacturerAbility(clientData.peerAddr_,
        clientData.manufacturerAbility_);
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws Service]:read data fail,tws service instance invalid");
    twsService->SetDeviceManufacturerAbility(clientData.peerAddr_, clientData.manufacturerAbility_);

    HILOGD("[Tws Service]:manufacturer ability,dev:%{public}s, manuAbility:%{public}s.",
        GET_ENCRYPT_ADDR(peerAddr_), manuAbility.c_str());

    /* 刷新数据 */
    manufacturerAbility_ = clientData.manufacturerAbility_;

    SleConfig::GetInstance().Save();
}

/* 根据数据类型刷新数据 */
void TwsClientData::UpdateData(uint8_t dataType, TwsClientData &clientData)
{
    switch (dataType) {
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE):
            HandleRoleTypeUpdate(clientData);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP): {  /* 存储对端设备的数通能力信息 */
            HandleDtsCapUpdate(clientData);
            break;
        }
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS): {
            HandleWearStatusUpdate(clientData);
            break;
        }
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD):
            HandleAtCmdData(clientData);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_HEADSET_EXCEPTION):
            HandleAudioExcep(clientData);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE):
            nature_ = clientData.nature_;
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS):
            HandleQueryPreemption(clientData);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH):
            HandleAutoConnectSwitch(clientData);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_CAP_QUERY):
            (void)memcpy_s(&sleConfig_, sizeof(sleConfig_), &clientData.sleConfig_, sizeof(clientData.sleConfig_));
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE):
            audioState_ = clientData.audioState_;
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE):
            notifyValue_ = clientData.notifyValue_;
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY):
            HandleManufacturerAbility(clientData);
            break;
        default:
            HILOGE("[Tws Service]:update client data failed,message type not support:%{public}u.", dataType);
            break;
    }
}

void TwsService::GetTwsAudioDelay(const RawAddress &devAddr, uint32_t &audioDelay)
{
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>(
                    SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(sleService, "sleService invalid.");
    uint16_t icbDelay = 0;
    ProfileASC *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    AscQosmInfo qosmInfo = {};
    uint8_t qosId = NL_SLE_QOS_NONE;
    if (ascService->GetAscQosmInfo(primaryAddr, qosmInfo)) {
        icbDelay = qosmInfo.ft * qosmInfo.icbInterval * 25 / 100;  // 1个icbInterval为0.25ms
        qosId = qosmInfo.qosIndex;
        HILOGI("[Tws Service]icbDelay(%{public}d) = ft(%{public}d) * icbInterval(%{public}d) * 25 / 100. Get qosId: "
               "%{public}d.", icbDelay, qosmInfo.ft, qosmInfo.icbInterval, qosId);
    }

    uint8_t sduIntervalTimes = 0;
    int16_t deltaMs = 0;
    auto getRoleFunc = [this, &sduIntervalTimes, &deltaMs, &qosId](
        std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        deltaMs = clientData->sleConfig_.offset;
        // 1个icbInterval为0.25ms
        uint8_t sceneNum =
            clientData->sleConfig_.sceneNum < MAX_SCENE_NUM ? clientData->sleConfig_.sceneNum : MAX_SCENE_NUM;
        HILOGI("[Tws Service]:clientData->sleConfig_.sceneNum: %{public}d, sceneNum: %{public}d",
            clientData->sleConfig_.sceneNum, sceneNum);
        uint8_t targetScene = GetScenesFromQoS(qosId);
        for (uint8_t i = 0; i < sceneNum; i++) {
            if (targetScene == clientData->sleConfig_.timesMap[i].scene) {
                HILOGI("[Tws Service]:qosId:%{public}d, GetScenesFromQoS: %{public}u", qosId, GetScenesFromQoS(qosId));
                sduIntervalTimes = clientData->sleConfig_.timesMap[i].times;
                break;
            }
        }
    };
    if (!twsClientDatas_.GetValueAndOpt(primaryAddr.GetAddress(), getRoleFunc)) {
        HILOGE("[Tws Service]:get device audio delay failed,addr:%{public}s",
            GetEncryptAddr(primaryAddr.GetAddress()).c_str());
    }
    uint16_t dspDelay = 0;
    // 理论最大音画补偿值 = 手机DSP PCM Buffer + 同步链路FT次数 * icbInterval + Times * icbInterval + deltaMs
    int32_t totalDelay = dspDelay + icbDelay + sduIntervalTimes * qosmInfo.icbInterval * 25 / 100 + deltaMs;
    audioDelay = totalDelay < 0 ? AUDIO_DELAY_MS : static_cast<uint32_t>(totalDelay);
    HILOGD("[Tws Service]audioDelay(%{public}d) = dspDelay(%{public}d) + icbDelay(%{public}d) + \
        Times(%{public}d) * icbInterval(%{public}d) * 25 / 100 + deltaMs(%{public}d))",
        audioDelay, dspDelay, icbDelay, sduIntervalTimes, qosmInfo.icbInterval, deltaMs);
}

// 获取主耳地址
RawAddress TwsService::GetPrimaryAddr(const RawAddress& device)
{
    CdsmService* cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, device, "cdsmService is null.");
    std::vector<NearlinkCdsmInfo> cdsmList = { };
    NL_CHECK_RETURN_RET(cdsmService->CdsmGetAllMemberInfo(device, cdsmList) == NL_NO_ERROR, device,
        "[Tws Service]:get primary addr, cdsm data fail");

    for (auto &member : cdsmList) {
        bool isPrimary = false;
        auto checkRole = [&isPrimary](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
            if (clientData->roleType_ == static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY)) {
                isPrimary = true;
            }
        };
        if (twsClientDatas_.GetValueAndOpt(member.addr_.GetAddress(), checkRole) && isPrimary) {
            return member.addr_;
        }
    }

    HILOGE("[Tws Service]GetPrimaryAddr %{public}s primaryAddr not in cdsm.",
        GetEncryptAddr(device.GetAddress()).c_str());
    return device;
}

// 获取report地址
RawAddress TwsService::GetReportAddr(const RawAddress& device)
{
    RawAddress reportAddr {};
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN_RET(cdsmService, device, "ProfileCdsm is null.");
    if (cdsmService->CdsmGetReportAddr(device, reportAddr)) {
        HILOGD("[Tws Service]GetReportAddr %{public}s reportAddr %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(reportAddr.GetAddress()).c_str());
        return reportAddr;
    }

    reportAddr = device;
    HILOGI("[Tws Service]GetReportAddr %{public}s reportAddr %{public}s not in cdsm.",
        GetEncryptAddr(device.GetAddress()).c_str(), GetEncryptAddr(reportAddr.GetAddress()).c_str());
    return reportAddr;
}

void TwsService::TwsGetDeviceRole(const RawAddress &devAddr, uint8_t &devRole)
{
    bool isSilentPort = false;
    auto getRoleFunc = [&devRole, &isSilentPort](
        std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        devRole = clientData->roleType_;
        if (clientData->notifyValue_.notifyValue == static_cast<uint8_t>(TwsProfileAction::DISCONNECT)) {
            isSilentPort = true;
        }
    };
    if (!twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), getRoleFunc)) {
        HILOGE("[Tws Service]:get device role failed,addr:%{public}s", GET_ENCRYPT_ADDR(devAddr));
    }

    HILOGD("[Tws Service]:get device role,addr:%{public}s,role:%{public}u.",
        GET_ENCRYPT_ADDR(devAddr), devRole);
}

/* 获取左右耳信息 */
uint8_t TwsService::TwsGetDeviceNature(const RawAddress &devAddr)
{
    uint8_t nature = static_cast<uint8_t>(TwsNatureType::TWS_LEFT);
    auto func = [&nature](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        nature = clientData->nature_;
    };
    if (!twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), func)) {
        HILOGE("[Tws Service]:get device nature failed, addr:%{public}s", GET_ENCRYPT_ADDR(devAddr));
    }
    HILOGI("[Tws Service]:addr:%{public}s, nature=%{public}d", GET_ENCRYPT_ADDR(devAddr), nature);
    return nature;
}

uint8_t TwsService::TwsGetDeviceAudioMusicType(const RawAddress &devAddr)
{
    HiBoxNearlinkProfile audioState = {};
    auto func = [&audioState](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        audioState = clientData->audioState_;
    };
    NL_CHECK_RETURN_RET(twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), func),
        static_cast<uint8_t>(TwsMediaState::DISABLE),
        "[Tws Service]:get device audioState failed, addr:%{public}s", GET_ENCRYPT_ADDR(devAddr));
    HILOGI("[Tws Service]:addr:%{public}s, audioMusicType=%{public}d", GET_ENCRYPT_ADDR(devAddr),
        audioState.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)]);
    return audioState.audioType[static_cast<uint8_t>(TwsStreamType::MUSIC)];
}

/* 对端设备是否支撑佩戴检测（佩戴检测能力查询） */
bool TwsService::TwsIsSupportWearDetect(const RawAddress &devAddr)
{
    bool isSupportWear = false;
    auto checkWearAbility = [&isSupportWear](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        auto it = find(clientData->devDtsCap_.begin(), clientData->devDtsCap_.end(),
            static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_WEAR_DETECT));
        if (it != clientData->devDtsCap_.end()) {
            isSupportWear = true;
        }
    };
    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    if (!twsClientDatas_.GetValueAndOpt(primaryAddr.GetAddress(), checkWearAbility)) {
        HILOGE("[Tws Service]:get device wear ability failed,addr:%{public}s",
            GetEncryptAddr(primaryAddr.GetAddress()).c_str());
    }
    HILOGD("[Tws Service]:TwsIsSupportWearDetect device:%{public}s, isSupportWear:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), isSupportWear);
    return isSupportWear;
}

void TwsService::TwsGetDeviceWearStatus(const RawAddress &devAddr, TwsDevWearStatus &wearStatus)
{
    auto getWearStatus = [&wearStatus](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        wearStatus.leftStatus = clientData->wearData_.leftWear;
        wearStatus.rightStatus = clientData->wearData_.rightWear;
    };
    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    if (!twsClientDatas_.GetValueAndOpt(primaryAddr.GetAddress(), getWearStatus)) {
        HILOGE("[Tws Service]:get device role failed,addr:%{public}s",
            GetEncryptAddr(primaryAddr.GetAddress()).c_str());
    }
}

void TwsService::UpdateDeviceWearState(const RawAddress &devAddr, const TwsWearStateData &wearData)
{
    RawAddress reportAddr = GetReportAddr(devAddr);
    TwsDevWearStatus previousWearStatus;
    if (!twsDevWearStatus_.GetValue(reportAddr.GetAddress(), previousWearStatus)) {
        previousWearStatus.leftStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
        previousWearStatus.rightStatus = static_cast<uint8_t>(TwsWearDetect::NOT_WORN);
    }

    // Insert or update wear status
    TwsDevWearStatus wearStatus;
    wearStatus.leftStatus = wearData.leftWear;
    wearStatus.rightStatus = wearData.rightWear;
    twsDevWearStatus_.EnsureInsert(reportAddr.GetAddress(), wearStatus);

    // Wear Detection States
    int supportStates = TwsGetWearDetectionState(reportAddr);
    bool isActiveDevWill = false;
    bool isActiveDevNow = false;
    switch (supportStates) {
        case static_cast<int>(TwsWearDetectSupport::SUPPORT_UNKNOWN):
            TwsEnableWearDetection(reportAddr);
            NotifyWearAction(devAddr, wearData, previousWearStatus, isActiveDevWill);
            break;
        case static_cast<int>(TwsWearDetectSupport::SUPPORT_ON):
            isActiveDevNow = IsActiveDeviceNow(devAddr);
            NotifyWearAction(devAddr, wearData, previousWearStatus, isActiveDevWill);
            ChangePlayState(devAddr, wearData, previousWearStatus, isActiveDevWill, isActiveDevNow);
            break;
        case static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF):
            // No action needed
            break;
        default:
            HILOGI("[Tws Service]: invailed supportStates");
            break;
    }
}

void TwsService::SetCurrentAVPlaybackState(int &currentPlayState)
{
    McpServerService* mcpService = McpServerService::GetService();
    NL_CHECK_RETURN(mcpService, "mcpService is null.");
    if (mcpService->GetMediaLoader()->IsAVPlaybackStatePlay()) {
        currentPlayState = NL_SLE_ASC_STATE_STARTED;
        HILOGD("[Tws Service]:avPlayState is play");
    }
}

void TwsService::ChangePlayState(const RawAddress &devAddr, const TwsWearStateData &wearData,
    const TwsDevWearStatus &previousWearStatus, bool &isActiveDevWill, bool &isActiveDevNow)
{
    int action = GetWearAction(wearData.leftWear, wearData.rightWear, previousWearStatus);
    HILOGD("[Tws Service]: ACTION = %{public}d", action);
    NL_CHECK_RETURN(action != static_cast<int>(TwsWearAction::ACTION_NONE), "");
    int currentPlayState = 0;
    SetCurrentAVPlaybackState(currentPlayState);
    int64_t currentTime = GetTimeStamp();
    HILOGI("[Tws Service]: ACTION = %{public}d, currentPlayState = %{public}d, currentTime = %{public}lld",
        action, currentPlayState, currentTime);
    RawAddress reportAddr = GetReportAddr(devAddr);
    switch (action) {
        case static_cast<int>(TwsWearAction::ACTION_REMOVE_DOUBLE):
        case static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT):
        case static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT): {
            NL_CHECK_RETURN(isActiveDevNow, "device not active.");
            bool isNeedChangeState = currentPlayState != NL_SLE_ASC_STATE_STARTED &&
                (lastSendPlayTime_ > 0) && currentTime > lastSendPlayTime_ &&
                (currentTime - lastSendPlayTime_ <= DOUBLE_REMOVE_CHECK_TIME);
            if (isNeedChangeState) {
                HILOGI("[Tws Service]: remove after send play less than 2s, fix to playing state");
                currentPlayState = NL_SLE_ASC_STATE_STARTED;
            }
            int pauseReason = GetPauseReason(reportAddr, action, previousWearStatus, currentPlayState);
            PauseAndRecordIfNeeded(reportAddr, pauseReason, currentTime);
            break;
        }
        case static_cast<int>(TwsWearAction::ACTION_ADD_DOUBLE):
        case static_cast<int>(TwsWearAction::ACTION_ADD_LEFT):
        case static_cast<int>(TwsWearAction::ACTION_ADD_RIGHT): {
            if (!isActiveDevWill && !isActiveDevNow) {
                HILOGW("device not active, not send play");
                pauseRecordMap_.Erase(reportAddr.GetAddress());
                return;
            }
            ResumePlayIfNeeded(reportAddr, action, currentTime);
            break;
        }
        default:
            HILOGI("[Tws Service]: invailed action");
            break;
    }
}

bool TwsService::IsMusicActive()
{
    return OHOS::AudioStandard::AudioVolumeClientManager::GetInstance().IsStreamActive(
        OHOS::AudioStandard::AudioStreamType::STREAM_MUSIC);
}

bool TwsService::IsCalling() const
{
    ProfileASC *ascService = ASCService::GetService();
    NL_CHECK_RETURN_RET(ascService, false, "ProfileASC is null.");
    return ascService->IsCalling();
}

void TwsService::ResumePlayIfNeeded(const RawAddress &devAddr, int action, int64_t currentTime)
{
    TwsPauseRecord pauseRecord;
    if (!pauseRecordMap_.GetValue(devAddr.GetAddress(), pauseRecord)) {
        HILOGW("[Tws Service]: do nothing for no pause record");
        return;
    }

    if (IsCalling()) {
        HILOGI("[Tws Service]: is calling, not need play");
        pauseRecordMap_.Erase(devAddr.GetAddress());
        return;
    }

    bool isNoNeedResume = IsMusicActive() && currentTime > pauseRecord.pauseTime &&
                          currentTime - pauseRecord.pauseTime > DOUBLE_REMOVE_CHECK_TIME;
    if (isNoNeedResume) {
        HILOGW("[Tws Service]: do nothing, current audio music active");
        pauseRecordMap_.Erase(devAddr.GetAddress());
        return;
    }

    switch (pauseRecord.pauseReason) {
        case static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT):
        case static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_RIGHT):
            if (action == static_cast<int>(TwsWearAction::ACTION_ADD_DOUBLE)) {
                HILOGW("[Tws Service]: ACTION_ADD_DOUBLE wrong pause reason");
                pauseRecordMap_.Erase(devAddr.GetAddress());
                break;
            }
            [[fallthrough]];
        case static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE):
        case static_cast<int>(TwsWearPauseReason::PAUSE_REASON_RIGHT_REMOVE_RIGHT):
        case static_cast<int>(TwsWearPauseReason::PAUSE_REASON_LEFT_REMOVE_LEFT): {
            bool isNeedResume = currentTime > pauseRecord.pauseTime &&
                              currentTime - pauseRecord.pauseTime <= MAX_RESUME_PLAY_TIME_SIMPLE;
            if (isNeedResume) {
                ProfileASC *ascService = ASCService::GetService();
                NL_CHECK_RETURN(ascService, "ProfileASC is null.");
                ascService->SendPlayOrPauseByWearDetection(devAddr, MCP_ID_PLAY);
                lastSendPlayTime_ = GetTimeStamp();
                HILOGI("[Tws Service]: send play");
            } else {
                HILOGI("[Tws Service]: not resume for time too long, pauseTime = %{public}d", pauseRecord.pauseTime);
            }
            pauseRecordMap_.Erase(devAddr.GetAddress());
            break;
        }
        default:
            HILOGW("[Tws Service]: invalid pauseReason is %{public}d", pauseRecord.pauseReason);
            pauseRecordMap_.Erase(devAddr.GetAddress());
            break;
    }
}

void TwsService::PauseAndRecordIfNeeded(const RawAddress &devAddr, int pauseReason, int64_t currentTime)
{
    HILOGI("[Tws Service]: pause reason : %{public}d", pauseReason);
    if (pauseReason == static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED)) {
        HILOGW("[Tws Service]: pause no need");
        pauseRecordMap_.Erase(devAddr.GetAddress());
        return;
    }
    TwsPauseRecord pauseRecord;
    if (!pauseRecordMap_.GetValue(devAddr.GetAddress(), pauseRecord)) {
        ProfileASC *ascService = ASCService::GetService();
        NL_CHECK_RETURN(ascService, "ProfileASC is null.");
        ascService->SendPlayOrPauseByWearDetection(devAddr, MCP_ID_PAUSE);
        HILOGI("[Tws Service]: send pause, devAddr %{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    } else {
        HILOGI("[Tws Service]: drop duplicate pause, devAddr %{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    }
    TwsPauseRecord newPauseRecord;
    newPauseRecord.pauseReason = pauseReason;
    newPauseRecord.pauseTime = currentTime;
    pauseRecordMap_.EnsureInsert(devAddr.GetAddress(), newPauseRecord);
}

int TwsService::GetPauseReasonNotPlaying(const RawAddress &devAddr, int action, const TwsDevWearStatus &previousWear)
{
    int64_t currentTime = GetTimeStamp();

    TwsPauseRecord pauseRecord;
    bool isNoNeedReason = !pauseRecordMap_.GetValue(devAddr.GetAddress(), pauseRecord) ||
        (currentTime > pauseRecord.pauseTime &&
        currentTime - pauseRecord.pauseTime > DOUBLE_REMOVE_CHECK_TIME && IsMusicActive());
    if (isNoNeedReason) {
        return static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED);
    }
    if ((pauseRecord.pauseReason == static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT)) &&
        (!previousWear.leftStatus && previousWear.rightStatus) &&
        (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT))) {
        return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    } else if ((pauseRecord.pauseReason == static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_RIGHT)) &&
        (previousWear.leftStatus && !previousWear.rightStatus) &&
        (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT))) {
        return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE);
    } else {
        return static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED);
    }
}

int TwsService::GetPauseReason(const RawAddress &devAddr, int action, const TwsDevWearStatus &previousWear,
    int currentPlayState)
{
    if (currentPlayState != NL_SLE_ASC_STATE_STARTED) {
        return GetPauseReasonNotPlaying(devAddr, action, previousWear);
    }

    if (previousWear.leftStatus && !previousWear.rightStatus &&
        (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT))) {
        return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_LEFT_REMOVE_LEFT); // 10->00
    } else if (!previousWear.leftStatus && previousWear.rightStatus &&
        (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT))) {
        return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_RIGHT_REMOVE_RIGHT); // 01->00
    } else if (previousWear.leftStatus && previousWear.rightStatus) {
        if (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT)) {
            return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_LEFT); // 11->01
        } else if (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT)) {
            return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_RIGHT); // 11->10
        } else if (action == static_cast<int>(TwsWearAction::ACTION_REMOVE_DOUBLE)) {
            return static_cast<int>(TwsWearPauseReason::PAUSE_REASON_DOUBLE_REMOVE_DOUBLE); // 11->00
        } else {
            HILOGI("[Tws Service]: GetPauseReason pause no need");
            return static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED);
        }
    } else {
        return static_cast<int>(TwsWearPauseReason::PAUSE_NO_NEED);
    }
}

int64_t TwsService::GetTimeStamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool TwsService::IsActiveDeviceNow(const RawAddress &devAddr)
{
    bool isActiveDevNow = true;
    ProfileASC *ascService = ASCService::GetService();
    if (ascService != nullptr) {
        RawAddress reportAddr = GetReportAddr(devAddr);
        NearlinkRawAddress device = ascService->GetActiveSinkDevice(); // 获取到的是report地址
        OHOS::AudioStandard::DeviceType deviceType =
            OHOS::AudioStandard::AudioDevicesClientManager::GetInstance().GetActiveOutputDevice();
        isActiveDevNow = (deviceType == OHOS::AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK) &&
            (device.GetAddress() == reportAddr.GetAddress());
    }
    return isActiveDevNow;
}

int TwsService::GetWearActionRecord(bool leftIn, bool rightIn, bool wasLeftIn, bool wasRightIn)
{
    bool bothWasNotIn = !wasLeftIn && !wasRightIn;
    bool bothWasIn = wasLeftIn && wasRightIn;
    // 双耳都戴上
    if (leftIn && rightIn) {
        if (bothWasNotIn) {
            return static_cast<int>(TwsWearAction::ACTION_ADD_DOUBLE); // 00->11
        }
        if (!wasRightIn) {
            return static_cast<int>(TwsWearAction::ACTION_ADD_RIGHT); // 10->11
        }
        if (!wasLeftIn) {
            return static_cast<int>(TwsWearAction::ACTION_ADD_LEFT); // 01->11
        }
    }

    // 只有左耳戴上
    if (leftIn) {
        if (bothWasNotIn) {
            return static_cast<int>(TwsWearAction::ACTION_ADD_LEFT); // 00->10
        }
        if (bothWasIn) {
            return static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT); // 11->10
        }
        if (wasRightIn) {
            return static_cast<int>(TwsWearAction::ACTION_SWITCH); // 01->10
        }
    }

    // 只有右耳戴上
    if (rightIn) {
        if (bothWasNotIn) {
            return static_cast<int>(TwsWearAction::ACTION_ADD_RIGHT); // 00->01
        }
        if (bothWasIn) {
            return static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT); // 11->01
        }
        if (wasLeftIn) {
            return static_cast<int>(TwsWearAction::ACTION_SWITCH); // 10->01
        }
    }

    // 两耳都没有戴上
    if (bothWasIn) {
        return static_cast<int>(TwsWearAction::ACTION_REMOVE_DOUBLE); // 11->00
    }
    if (wasLeftIn) {
        return static_cast<int>(TwsWearAction::ACTION_REMOVE_LEFT); // 10->00
    }
    if (wasRightIn) {
        return static_cast<int>(TwsWearAction::ACTION_REMOVE_RIGHT); // 01->00
    }

    return static_cast<int>(TwsWearAction::ACTION_NONE);
}

int TwsService::GetWearAction(uint8_t leftWearState, uint8_t rightWearState,
    const TwsDevWearStatus &previousWear)
{
    bool leftIn = leftWearState == static_cast<uint8_t>(TwsWearDetect::WORN);
    bool rightIn = rightWearState == static_cast<uint8_t>(TwsWearDetect::WORN);
    bool wasLeftIn = previousWear.leftStatus == static_cast<uint8_t>(TwsWearDetect::WORN);
    bool wasRightIn = previousWear.rightStatus == static_cast<uint8_t>(TwsWearDetect::WORN);

    if ((wasLeftIn == leftIn) && (wasRightIn == rightIn)) {
        HILOGD("[Tws Service]: Wear state has not changed, no action needed");
        return static_cast<int>(TwsWearAction::ACTION_NONE);
    }
    return GetWearActionRecord(leftIn, rightIn, wasLeftIn, wasRightIn);
}

void TwsService::NotifyWearAction(const RawAddress &devAddr,  const TwsWearStateData &wearData,
    const TwsDevWearStatus &previousWearStatus, bool &isActiveDevWill)
{
    bool isWearing = wearData.leftWear == static_cast<uint8_t>(TwsWearDetect::WORN) ||
        wearData.rightWear == static_cast<uint8_t>(TwsWearDetect::WORN);
    bool wasWearing = previousWearStatus.leftStatus == static_cast<uint8_t>(TwsWearDetect::WORN) ||
        previousWearStatus.rightStatus == static_cast<uint8_t>(TwsWearDetect::WORN);

    NL_CHECK_RETURN_LOGD(isWearing != wasWearing, "[Tws Service]: Wear state has not changed, no action needed");
    int action = isWearing ? static_cast<int>(UpdateOutputStackAction::ACTION_WEAR) :
        static_cast<int>(UpdateOutputStackAction::ACTION_UNWEAR);
    NotifyAudioDeviceAction(devAddr, action);
    if (action == static_cast<int>(UpdateOutputStackAction::ACTION_WEAR)) {
        isActiveDevWill = true;
    }
}

void TwsService::NotifyAudioDeviceAction(const RawAddress &devAddr, int action)
{
    HILOGI("[Tws Service]:NotifyAudioDeviceAction, addr:%{public}s, action:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), action);
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    ascService->SleAudioDeviceActionChanged(NearlinkRawAddress(devAddr), action);
}

int TwsService::TwsGetWearDetectionState(const RawAddress &devAddr)
{
    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    int state = SleConfig::GetInstance().GetConfigWearDetectionState(primaryAddr.GetAddress());
    HILOGD("[Tws Service]:TwsGetWearDetectionState, addr:%{public}s, state:%{public}d",
        GetEncryptAddr(primaryAddr.GetAddress()).c_str(), state);
    return state;
}

bool TwsService::SaveWearDetectionTask(const RawAddress &device, int wearDetectionState)
{
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN_RET(cdsmService, false, "ProfileCdsm is null.");
    bool isCdsDevice = cdsmService->CdsmCheckIsCooperationDevice(device);
    if (isCdsDevice) {
        std::vector<NearlinkCdsmInfo> cdsmList;
        NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(device, cdsmList);
        NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "CdsmGetAllMemberInfo error.");
        for (const auto& info : cdsmList) {
            // 对于合作集设备，给组内所有成员保存一下佩戴检测能力
            SleConfig::GetInstance().SetConfigWearDetectionState(info.addr_.GetAddress(), wearDetectionState);
            SleConfig::GetInstance().Save();
        }
        return true;
    }
    return false;
}

void TwsService::TwsEnableWearDetection(const RawAddress &devAddr)
{
    HILOGI("[Tws Service]:TwsEnableWearDetection, addr:%{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    if (TwsGetWearDetectionState(devAddr) == static_cast<int>(TwsWearDetectSupport::SUPPORT_ON)) {
        HILOGI("[Tws Service]:TwsEnableWearDetection unsupported change");
        return;
    }

    if (!SaveWearDetectionTask(devAddr, static_cast<int>(TwsWearDetectSupport::SUPPORT_ON))) {
        SleConfig::GetInstance().SetConfigWearDetectionState(devAddr.GetAddress(),
            static_cast<int>(TwsWearDetectSupport::SUPPORT_ON));
        SleConfig::GetInstance().Save();
    }
    RawAddress reportAddr = GetReportAddr(devAddr);
    bool isWearing = TwsIsDeviceWearing(reportAddr);
    int action = isWearing ? static_cast<int>(UpdateOutputStackAction::ACTION_WEAR) :
        static_cast<int>(UpdateOutputStackAction::ACTION_UNWEAR);
    NotifyAudioDeviceAction(reportAddr, action);

    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    NotifyAudioDeviceAction(primaryAddr, static_cast<int>(UpdateOutputStackAction::ACTION_ENABLE_WEAR_DETECTION));
    TwsHiBoxParser::GetInstance().SendMessage(primaryAddr,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), nullptr, 0, false);
}

void TwsService::TwsDisableWearDetection(const RawAddress &devAddr)
{
    HILOGI("[Tws Service]:TwsDisableWearDetection, addr:%{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    if (TwsGetWearDetectionState(devAddr) == static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF)) {
        HILOGI("[Tws Service]:TwsDisableWearDetection unsupported change");
        return;
    }

    if (!SaveWearDetectionTask(devAddr, static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF))) {
        SleConfig::GetInstance().SetConfigWearDetectionState(devAddr.GetAddress(),
            static_cast<int>(TwsWearDetectSupport::SUPPORT_OFF));
        SleConfig::GetInstance().Save();
    }

    RawAddress primaryAddr = GetPrimaryAddr(devAddr);
    NotifyAudioDeviceAction(primaryAddr, static_cast<int>(UpdateOutputStackAction::ACTION_DISABLE_WEAR_DETECTION));
}

bool TwsService::TwsIsDeviceWearing(const RawAddress &devAddr)
{
    bool isWearing = false;
    auto getWearStatus = [&isWearing](std::string dev, TwsDevWearStatus wearStatus) -> void {
        isWearing = wearStatus.leftStatus == static_cast<uint8_t>(TwsWearDetect::WORN) ||
            wearStatus.rightStatus == static_cast<uint8_t>(TwsWearDetect::WORN);
    };
    RawAddress reportAddr = GetReportAddr(devAddr);
    if (!twsDevWearStatus_.GetValueAndOpt(reportAddr.GetAddress(), getWearStatus)) {
        HILOGE("[Tws Service]:get device wear state failed, addr:%{public}s",
            GetEncryptAddr(reportAddr.GetAddress()).c_str());
    }
    HILOGD("[Tws Service]:TwsIsDeviceWearing, addr:%{public}s, isWearing:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), isWearing);
    return isWearing;
}

/* 合作集设备连接完成后，更新一次角色 */
void TwsService::TwsUpdateDeviceDefaultRole(const RawAddress &devAddr, const uint8_t roleType)
{
    if (roleType != static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY) &&
        roleType != static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY)) {
        return;
    }

    /* 切换到tws线程，参数必须拷贝赋值 */
    DoInTwsThread([this, devAddr, roleType]() {
        TwsClientData clientData;
        clientData.peerAddr_ = devAddr;
        clientData.roleType_ = roleType;
        this->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), clientData);
    });
}

/* 获取Vendor账户hash */
bool TwsService::GetVendorAccountHash(uint8_t *outHashArray, size_t arrayLen)
{
    if (arrayLen != VENDOR_ACCOUNT_HASH_LEN) {
        return false;
    }
    /* get uid begin */
    std::pair<bool, OHOS::AccountSA::OhosAccountInfo> accountInfo =
        OHOS::AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    NL_CHECK_RETURN_RET(accountInfo.first && !accountInfo.second.name_.empty(), false,
        "[tws service]:get vendor account name fail,empty");
    const char *accountName = accountInfo.second.name_.c_str();
    NL_CHECK_RETURN_RET(accountInfo.second.name_.compare("ohosAnonymousName") != 0, false,
        "[tws service]:get vendor account name fail,invalid.");

    uint8_t *ret = SHA256(reinterpret_cast<const unsigned char *>(accountName), strlen(accountName), outHashArray);
    NL_CHECK_RETURN_RET(ret, false, "[tws service]:get vendor account name fail,hash calc fail.");

    for (int i = 0; i < VENDOR_ACCOUNT_HASH_LEN; i++) {
        int8_t value = static_cast<int8_t>(outHashArray[i]);
        outHashArray[i] = value >= 0 ? value : -value;
    }
    return true;
}

/* echo 5,d 通知对端vendor账户hash */
void TwsService::TwsSendAccountInfo(const RawAddress &devAddr)
{
    uint16_t dataLen = sizeof(VendorAccountHash);
    std::unique_ptr<uint8_t[]> accountInfo = std::make_unique<uint8_t[]>(dataLen);
    (void)memset_s(accountInfo.get(), dataLen, 0, dataLen);

    VendorAccountHash *payload = reinterpret_cast<VendorAccountHash *>(accountInfo.get());
    payload->deviceOsType = TWS_MOBILE_OS_TYPE_HMOS_VERSION; /* 系统版本 */
    if (!GetVendorAccountHash(payload->accountHash, sizeof(payload->accountHash))) {
        HILOGD("[tws service]:get vendor account hash fail.");
        (void)memset_s(payload->accountHash, sizeof(payload->accountHash), 0, sizeof(payload->accountHash));
    }

    TwsHiBoxParser::GetInstance().SendMessage(devAddr,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_VENDOR_ACCOUNT_HASH), accountInfo.get(), dataLen, false);
    HILOGD("[tws service]:send vendor account hash to peer:%{public}s", GET_ENCRYPT_ADDR(devAddr));
}

/* 发送抢占消息 */
void TwsService::TwsSendUserSelection(const RawAddress &device,
    const std::vector<struct AudioStreamInfo> &streamInfo)
{
    uint16_t dataLen = static_cast<uint16_t>(sizeof(ASC_AUDIO_STREAM_TYPE_LIST) / sizeof(uint32_t));
    std::unique_ptr<uint8_t[]> streamMap = std::make_unique<uint8_t[]>(dataLen);

    for (uint16_t index = 0; index < dataLen; index++) {
        uint8_t streamState = static_cast<uint8_t>(AudioStreamState::AUDIO_STREAM_STATE_INVALID);
        for (auto &item : streamInfo) {
            if (item.streamType == ASC_AUDIO_STREAM_TYPE_LIST[index]) {
                streamState = static_cast<uint8_t>(item.streamState);
            }
        }
        streamMap[index] = streamState;
    }

    TwsHiBoxParser::GetInstance().SendMessage(device,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_OUT_DATAPATH),
        streamMap.get(), dataLen, false);

    HILOGI("[Tws Service]:TwsSendUserSelection,dev:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
}

/* echo 5,e 查询对端是否支持虚拟自动连接 */
bool TwsService::TwsIsSupportVirtualAutoConnect(const RawAddress &devAddr)
{
    bool isAutoConnectOn = false;
    auto readSwitch = [&isAutoConnectOn](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        if (clientData->autoConnSwitch_ == static_cast<uint8_t>(TwsAutoConnSwitch::ON)) {
            isAutoConnectOn = true;
        }
    };
    twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), readSwitch);

    HILOGI("[tws service]:get is support auto conn from mem,addr:%{public}s,is support:%{public}d",
            GET_ENCRYPT_ADDR(devAddr), isAutoConnectOn);
    return isAutoConnectOn;
}

/* run tws thread */
void TwsService::SendVirtualAutoConnectType(const RawAddress &devAddr)
{
    int connType = 0;
    int businessType = 0;
    auto getData = [&connType, &businessType](std::string dev, std::shared_ptr<TwsClientData> dataInstance) -> void {
        connType = dataInstance->virtualAutoConnType_;
        businessType = dataInstance->virtualAutoBussinessType_;
    };
    NL_CHECK_RETURN(twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), getData),
        "[tws service]:get connect type or business type fail:%{public}s.", GET_ENCRYPT_ADDR(devAddr));

    uint16_t dataLen = sizeof(QueryPairConn);
    std::unique_ptr<uint8_t[]> queryMsg = std::make_unique<uint8_t[]>(dataLen);
    (void)memset_s(queryMsg.get(), dataLen, 0, dataLen);

    QueryPairConn *payload = reinterpret_cast<QueryPairConn *>(queryMsg.get());
    payload->connType = static_cast<uint8_t>(connType);
    payload->mobileBusinessType = static_cast<uint8_t>(businessType);
    if (!GetVendorAccountHash(payload->accountHash, sizeof(payload->accountHash))) {
        HILOGD("[tws service]:get vendor account hash fail.");
        (void)memset_s(payload->accountHash, sizeof(payload->accountHash), 0, sizeof(payload->accountHash));
    }

    TwsHiBoxParser::GetInstance().SendMessage(devAddr,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), queryMsg.get(), dataLen, false);

    HILOGI("[tws service]:set virtual auto conn type,addr:%{public}s,connType:%{public}d,businessType:%{public}d",
        GET_ENCRYPT_ADDR(devAddr), connType, businessType);
}

void TwsService::SetVirtualAutoConnectTypeInner(const RawAddress &devAddr, int32_t connType, int32_t businessType)
{
    TwsClientData dataTmp;
    dataTmp.virtualAutoBussinessType_ = businessType;
    dataTmp.virtualAutoConnType_ = connType;
    UpdateClientDataOrCreate(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), dataTmp);

    int profileState = static_cast<int>(SleConnectState::DISCONNECTED);
    auto getState = [&profileState](std::string dev, std::shared_ptr<TwsClientData> dataInstance) -> void {
        profileState = dataInstance->clientState;
    };
    twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), getState);
    if (profileState == static_cast<int>(SleConnectState::CONNECTED)) {
        SendVirtualAutoConnectType(devAddr);
        return;
    }

    /* 缓存消息 */
    AddToCacheMessage(devAddr, TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS);
}

/* 设置自动连接类型 */
void TwsService::SetVirtualAutoConnectType(const RawAddress &devAddr, int32_t connType, int32_t businessType)
{
    /* 切线程，devAddr需要拷贝赋值，不能引用 */
    DoInTwsThread([this, devAddr, connType, businessType]() {
        this->SetVirtualAutoConnectTypeInner(devAddr, connType, businessType);
    });
}

/* 添加缓存消息到队列 */
void TwsService::AddToCacheMessage(const RawAddress &devAddr, TwsHiBoxMsgType msgType)
{
    auto addMsg = [msgType](std::string dev, TwsCacheMessage &msgBuf) -> void {
        msgBuf.msgQueue.push_back(msgType);
    };
    if (!msgCache_.GetValueAndOpt(devAddr.GetAddress(), addMsg)) {
        TwsCacheMessage msgBuf;
        msgBuf.msgQueue.push_back(msgType);
        msgCache_.EnsureInsert(devAddr.GetAddress(), msgBuf);
    }
}

/* 发送单个缓存消息 */
void TwsService::SendSingleCacheMessage(const std::string &devAddr, TwsHiBoxMsgType msgType)
{
    HILOGI("[tws service]:send cache message to peer:%{public}s,type:%{public}u",
        GetEncryptAddr(devAddr).c_str(), static_cast<uint8_t>(msgType));
    if (msgType == TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS) {
        SendVirtualAutoConnectType(RawAddress(devAddr));
        return;
    }
}

/* 发送所有缓存消息 */
void TwsService::SendAllCacheMessage(const RawAddress &devAddr)
{
    uint32_t msgCnt = 0;
    auto sendMsg = [this, &msgCnt](std::string dev, TwsCacheMessage &msg) -> void {
        for (auto msgType : msg.msgQueue) {
            SendSingleCacheMessage(dev, msgType);
        }
        msgCnt = msg.msgQueue.size();
        msg.msgQueue.clear();
    };
    NL_CHECK_RETURN_LOGD(msgCache_.GetValueAndOpt(devAddr.GetAddress(), sendMsg),
        "[tws service]:no valid cache message for device:%{public}s.", GET_ENCRYPT_ADDR(devAddr));
    HILOGD("[tws service]:send cache message to device:%{public}s,cnt:%{public}u.",
        GET_ENCRYPT_ADDR(devAddr), msgCnt);
}

/* 重置虚拟开关状态 */
void TwsService::ResetVirtualAutoSwitch(const RawAddress &devAddr)
{
    TwsClientData virtualData;
    virtualData.peerAddr_ = devAddr;
    virtualData.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);
    UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), virtualData);

    HILOGI("[tws service]:dev:%{public}s disconnect,virtual switch turn off.", GET_ENCRYPT_ADDR(devAddr));
}

/* 从xml中恢复数据 */
void TwsService::RecoverDataFromConf(const RawAddress &devAddr)
{
    /* 恢复虚拟开关状态 */
    TwsClientData clientData;
    clientData.peerAddr_ = devAddr;
    uint8_t autoSwitch = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);
    int switchValue = SleConfig::GetInstance().GetAutoConnectSwitch(devAddr.GetAddress());
    if (switchValue != 0) {
        autoSwitch = static_cast<uint8_t>(TwsAutoConnSwitch::ON);
    }
    clientData.autoConnSwitch_ = autoSwitch;
    UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), clientData);
}

/* 主动查询音频流状态 */
void TwsService::QueryStreamState(const RawAddress &devAddr, std::vector<struct AudioStreamInfo> &streamData)
{
    streamData.clear();
    auto getStream = [&streamData](std::string dev, std::shared_ptr<TwsClientData> clientData) -> void {
        uint16_t dataLen = static_cast<uint16_t>(sizeof(ASC_AUDIO_STREAM_TYPE_LIST) / sizeof(uint32_t));
        if (dataLen > NEARLINK_STREAM_TYPE_NUM) {
            dataLen = NEARLINK_STREAM_TYPE_NUM;
        }

        for (uint16_t index = 0; index < dataLen; index++) {
            struct AudioStreamInfo streamTmp;
            streamTmp.streamType = ASC_AUDIO_STREAM_TYPE_LIST[index];
            streamTmp.streamState = static_cast<AudioStreamState>(clientData->audioState_.audioType[index]);
            streamData.push_back(streamTmp);
        }
    };
    NL_CHECK_RETURN(twsClientDatas_.GetValueAndOpt(devAddr.GetAddress(), getStream),
        "[Tws Service]:get stream state failed,addr:%{public}s", GET_ENCRYPT_ADDR(devAddr));

    HILOGD("[tws service]:dev:%{public}s,get stream state.", GET_ENCRYPT_ADDR(devAddr));
}

/* 处理暂停播放列表，超时后清理 */
void TwsService::ProcPauseRecordMap()
{
    int64_t currentTime = GetTimeStamp();
    std::vector<std::string> removeAddr;
    auto getRemoveList = [&removeAddr, &currentTime](const std::string& addr, const TwsPauseRecord& pauseRecord) {
        bool isNeedErase = (currentTime > pauseRecord.pauseTime);
        isNeedErase = isNeedErase && ((currentTime - pauseRecord.pauseTime) > DOUBLE_REMOVE_CHECK_TIME);
        if (isNeedErase) {
            removeAddr.push_back(addr);
        }
    };
    pauseRecordMap_.Iterate(getRemoveList);

    for (auto &addr : removeAddr) {
        pauseRecordMap_.Erase(addr);
        HILOGI("[tws service]:remove pause record addr:%{public}s.", GetEncryptAddr(addr).c_str());
    }
}

/* 通知对端服务连接完成 */
void TwsService::SendProfileConnected(const RawAddress &devAddr)
{
    uint8_t profileState = SLE_PROFILE_STATE_CONNECTED;
    TwsHiBoxParser::GetInstance().SendMessageEntry(devAddr,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_SLE_PROFILE_STATE), &profileState, sizeof(profileState), false);

    HILOGD("[tws service]:send profile state:%{public}u,addr:%{public}s.",
        profileState, GET_ENCRYPT_ADDR(devAddr));
}

REGISTER_CLASS_CREATOR(TwsService); /* 注册服务到类工厂，sleProfileManager动态调用 */

} // namespace Sle
} // namespace OHOS