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
#include <map>

#include "sle_uuid.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceManager.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "ssap_data.h"
#include "TwsDefines.h"
#include "TwsMessage.h"
#include "TwsService.h"
#include "TwsClient.h"
#include "SleUtils.h"
#include "ManufacturerAbilityLoader.h"

namespace OHOS {
namespace Nearlink {

TwsClient::TwsClient(const std::string &address) : address_(address)
{
    // 能力位图初始化由 ManufacturerAbilityManager 统一管理
}

TwsClient::~TwsClient()
{
    DoInTwsThread([appid = appId_]() {
        /* 去注册ssap client service回调 */
        InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
        NL_CHECK_RETURN(ssapClientService, "[Tws Client]:deregister ssap client callback fail,ssap service invalid.");

        int ret = ssapClientService->DeregisterApplication(appid);
        if (ret != static_cast<int>(ReturnValue::RET_NO_ERROR)) {
            HILOGE("[Tws Client]:deregister ssap client callback fail,ret:%{public}d.", ret);
        }
    });
    appId_ = -1; /* -1:非法appid */
}

InterfaceProfileSsapClient *TwsClient::GetSsapClientService()
{
    return static_cast<InterfaceProfileSsapClient *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_CLIENT));
}

/* 创建客户端实例 */
std::shared_ptr<TwsClient> TwsClient::CreateTwsClient(const std::string address)
{
    HILOGD("[Tws Client]:new tws client,addr:%{public}s", GetEncryptAddr(address).c_str());
    std::shared_ptr<TwsClient> twsClientInstance = std::make_shared<TwsClient>(address);
    NL_CHECK_RETURN_RET(twsClientInstance->Init(twsClientInstance), nullptr, "[Tws Client]:Init tws client failed.");
    return twsClientInstance;
}

/* 设备状态变更并上报给ProfileManager */
void TwsClient::NotifyStateChanged(TwsClientState toState)
{
    HILOGI("[Tws Client]:client state change,addr:%{public}s,%{public}d -> %{public}d",
        GetEncryptAddr(address_).c_str(), TwsClientGetState(), toState);

    if (TwsClientGetState() == toState) {
        return;
    }

    /* 上报状态给ProfileManager */
    TwsService *twsService = TwsService::GetService();
    if (twsService != nullptr) {
        twsService->NotifyStateChanged(RawAddress(address_), TwsClientGetState(), toState);
    }

    TwsClientSetState(toState);
}

/* 内部接口，私有服务客户端初始化 */
bool TwsClient::Init(std::weak_ptr<TwsClient> twsClient)
{
    /* 注册ssap client service回调 */
    ssapCallback_ = std::make_shared<TwsClientCallback>(twsClient);

    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN_RET(ssapClientService, false,
        "[Tws Client]:register ssap client callback fail,ssap service invalid.");

    int transport = ADAPTER_SLE;
    appId_ = ssapClientService->RegisterApplication(ssapCallback_, RawAddress(address_), transport, SSAP_SEC_NONE);
    NL_CHECK_RETURN_RET(appId_ >= 0, false,
        "[Tws Client]:register ssap client callback fail,ssap service app id %{public}d invalid.", appId_);

    HILOGD("[tws client]:ssap client callback appId_ %{public}d ", appId_);
    return true;
}

/* 连接私有服务 */
void TwsClient::ConnectTws()
{
    NL_CHECK_RETURN(appId_ >= 0, "[Tws Client]:ssap client service app id invalid:%{public}d", appId_);
    NL_CHECK_RETURN(TwsClientGetState() != TwsClientState::TWS_STATE_CONNECTED,
        "[Tws Client]:already in connected,addr:%{public}s", GetEncryptAddr(address_).c_str());

    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "[Tws Client]:ssap client service invalid.");

    int ret = ssapClientService->Connect(appId_, false);
    NL_CHECK_RETURN(ret == static_cast<int>(ReturnValue::RET_NO_ERROR),
        "[Tws Client]:ssap client service connect fail:%{public}d.", ret);

    NotifyStateChanged(TwsClientState::TWS_STATE_CONNECTING);
    HILOGD("Tws Client connect,ssap client appId=%{public}d", appId_);
    return;
}

/* 断开私有服务 */
void TwsClient::DisconnectTws()
{
    HILOGI("Enter");
    /* appid非法值为-1 */
    NL_CHECK_RETURN(appId_ >= 0, "[Tws Client]:ssap client service app id invalid:%{public}d", appId_);
    if (TwsClientGetState() == TwsClientState::TWS_STATE_DISCONNECTED) {
        HILOGI("[Tws Client]:already in disconnected,addr:%{public}s.", GetEncryptAddr(address_).c_str());
        return;
    }

    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "[Tws Client]:ssap client service invalid.");

    int ret = ssapClientService->Disconnect(appId_);
    NL_CHECK_RETURN(ret == static_cast<int>(ReturnValue::RET_NO_ERROR),
        "[Tws Client]:ssap client service disconnect fail:%{public}d.", ret);

    NotifyStateChanged(TwsClientState::TWS_STATE_DISCONNECTING);
    return;
}

void TwsClient::SaveTwsServicePropertyInfo()
{
    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "[Tws Client]:ssap client service invalid.");

    std::list<Service> serviceList = ssapClientService->GetServices(appId_);
    auto item = std::find_if(serviceList.begin(), serviceList.end(), [](Service service) {
        return service.uuid_ == Uuid::ConvertFromString(SLE_UUID_TWS_PROFILE);
    });
    NL_CHECK_RETURN(item != serviceList.end(),
        "[Tws Client]:peer addr:%{public}s not contain tws service.", GetEncryptAddr(address_).c_str());

    /* 获取属性信息 */
    for (auto &property : item->properties_) {
        if (property.uuid_ == Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_WRITE)) {
            sendDataProperty_ = property;
        }
        if (property.uuid_ == Uuid::ConvertFromString(SLE_UUID_TWS_PROPERTY_READ)) {
            recvDataProperty_ = property;
            RawAddress rawAddr(address_);
            TwsSendManufacturerAbility(rawAddr);
            ssapClientService->SetPropertyNotification(appId_, property, true);
        }
    }
}

/* echo 1,2 通知对端能力位图 */
void TwsClient::TwsSendManufacturerAbility(const RawAddress &devAddr)
{
    uint16_t dataLen = sizeof(ManufacturerAbilityInfo);
    std::unique_ptr<uint8_t[]> abilityInfo = std::make_unique<uint8_t[]>(dataLen);
    (void)memset_s(abilityInfo.get(), dataLen, 0, dataLen);

    ManufacturerAbilityInfo *manuInfo = reinterpret_cast<ManufacturerAbilityInfo *>(abilityInfo.get());
    manuInfo->protocolType[0] = FIRST_NEARLINK_PROTOCOL_VERSION;         /* 星闪协议大版本 */
    manuInfo->protocolType[1] = SECOND_NEARLINK_PROTOCOL_VERSION;        /* 星闪协议小版本 */
    manuInfo->deviceOsType[0] = FIRST_MANUFACTURER_OS_TYPE_HMOS_VERSION; /* 系统大版本 */
    manuInfo->deviceOsType[1] = SECOND_NEARLINK_PROTOCOL_VERSION;        /* 系统小版本 */
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws Client]:read data fail,tws service instance invalid.");
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility =
        ManufacturerAbilityLoader::GetInstance().GetLocalAbility(); /* 能力位图 */
    (void)memcpy_s(manuInfo->manufacturerAbility, SLE_MANU_ABILITY_LEN, manuAbility.data(), SLE_MANU_ABILITY_LEN);

    TwsMessage event(TWS_SERVICE_SEND_REQ_EVENT);
    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    event.dev_ = devAddr.GetAddress();
    event.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY);
    event.serviceDataLen_ = dataLen;
    event.serviceData_ = std::move(abilityInfo);

    twsService->TwsServiceSendReqInner(event);
    HILOGD("[tws service]:send manufacturerAbility to peer:%{public}s", GET_ENCRYPT_ADDR(devAddr));
}

/* 服务连接状态变化回调处理, run in tws thread */
void TwsClient::OnConnectionStateChangedTask(uint8_t clientState)
{
    static const std::map<const uint8_t, const TwsClientState> stateMap = {
        { static_cast<uint8_t>(SleConnectState::DISCONNECTED), TwsClientState::TWS_STATE_DISCONNECTED },
        { static_cast<uint8_t>(SleConnectState::CONNECTING), TwsClientState::TWS_STATE_CONNECTING },
        { static_cast<uint8_t>(SleConnectState::DISCONNECTING), TwsClientState::TWS_STATE_DISCONNECTING },
        { static_cast<uint8_t>(SleConnectState::CONNECTED), TwsClientState::TWS_STATE_CONNECTED }
    };

    auto item = stateMap.find(clientState);
    NL_CHECK_RETURN(item != stateMap.end(), "[Tws Client]:get state failed,src:%{public}d.", clientState);

    TwsClientState clientNewState = item->second;
    NotifyStateChanged(clientNewState);

    if (clientState == static_cast<int>(SleConnectState::CONNECTED)) {
        SaveTwsServicePropertyInfo();
    }
}

/* 向对端设备发送数据 */
int TwsClient::SendData(std::vector<uint8_t> &data)
{
    if (TwsClientGetState() != TwsClientState::TWS_STATE_CONNECTED) {
        HILOGI("[Tws Client]:send data fail,peer device:%{public}s not connected.", GetEncryptAddr(address_).c_str());
        return static_cast<int>(ReturnValue::RET_BAD_STATUS);
    }

    uint32_t dataLen = data.size();
    if (dataLen <= 0) {
        HILOGE("[Tws Client]:send data to peer fail,data invalid,data len:%{public}d.", dataLen);
        return static_cast<int>(ReturnValue::RET_BAD_PARAM);
    }

    InterfaceProfileSsapClient *ssapClientService_ = GetSsapClientService();
    NL_CHECK_RETURN_RET(ssapClientService_, static_cast<int>(ReturnValue::RET_BAD_STATUS),
        "[Tws Client]:send data to peer fail,ssap client service instance invalid.");

    sendDataProperty_.value_.clear();
    sendDataProperty_.value_.resize(dataLen);
    errno_t secRet = memcpy_s(sendDataProperty_.value_.data(), dataLen, data.data(), dataLen);
    if (secRet != EOK) {
        HILOGI("[Tws Client]:cpy data to property failed:%{public}d", secRet);
        return static_cast<int>(ReturnValue::RET_BAD_PARAM);
    }
    ssapClientService_->WriteProperty(appId_, sendDataProperty_, true);

    HILOGD("[Tws Client]:send data to peer dev:%{public}s,data len:%{public}u",
        GetEncryptAddr(address_).c_str(), dataLen);

    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

/* 收到对端设备数据 */
void TwsClient::OnPropertyChangedTask(const Property &property)
{
    HILOGD("[Tws Client]:property change,addr:%{public}s", GetEncryptAddr(address_).c_str());

    NL_CHECK_RETURN(property.handle_ == recvDataProperty_.handle_,
        "[Tws Client]:read data handle not match:%{public}u/%{public}u", property.handle_, recvDataProperty_.handle_);

    NL_CHECK_RETURN(property.value_.size() != 0 && property.value_.size() <= TWS_PROFILE_PROPERTY_MAX_LEN,
        "[Tws Client]:read data fail,data size invalid:%{public}u", property.value_.size());

    /* 收到HiBox原始数据码流 */
    TwsMessage event(TWS_SERVICE_SSAP_RECV_DATA_EVENT);
    event.dev_ = address_;
    event.streamLen_ = property.value_.size();
    event.dataStream_ = std::make_unique<uint8_t[]>(event.streamLen_);
    (void)memcpy_s(event.dataStream_.get(), property.value_.size(), property.value_.data(), property.value_.size());

    /* 记录消息的方向，是请求还是响应 */
    if (event.dataStream_[TWS_MSG_DIRECT_DATA_INDEX] == TWS_DYM_LIB_REQ_FIXED_HEADER) {
        event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    }

    if (event.dataStream_[TWS_MSG_DIRECT_DATA_INDEX] == TWS_DYM_LIB_RSP_FIXED_HEADER) {
        event.msgDirect_ = TWS_MSG_DIRECT_RSP;
    }

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws Client]:read data fail,tws service instance invalid");
    twsService->PostEvent(event);
}

/* 设置客户端Profile连接状态 */
void TwsClient::TwsClientSetState(TwsClientState newState)
{
    twsClientState_.store(newState);
}

TwsClientState TwsClient::TwsClientGetState(void)
{
    return twsClientState_.load();
}

TwsClient::TwsClientCallback::TwsClientCallback(
    std::weak_ptr<TwsClient> twsClient) : twsClient_(twsClient)
{}

/* 收到对端数据转交TWS线程处理 */
void TwsClient::TwsClientCallback::OnPropertyChanged(const Property &property)
{
    HILOGD("[Tws Client]:ssap client service property change callback.");
    /* 这里的property必须拷贝赋值 */
    DoInTwsThread([this, property]() {
        std::shared_ptr<TwsClient> twsClientPtr = twsClient_.lock();
        NL_CHECK_RETURN(twsClientPtr, "[Tws Client]:ssap client service property callback,client instance invalid.");
        twsClientPtr->OnPropertyChangedTask(property);
    });
}

/* ssap回调上报连接状态变化，转交TWS线程处理 */
void TwsClient::TwsClientCallback::OnConnectionStateChanged(uint8_t state, int ret)
{
    HILOGI("[Tws Client]:ssap client service state callback,state:%{public}u,ret:%{public}d", state, ret);
    DoInTwsThread([this, state]() {
        std::shared_ptr<TwsClient> twsClientPtr = twsClient_.lock();
        NL_CHECK_RETURN(twsClientPtr,
            "[Tws Client]:ssap client service connect state callback,client instance invalid.");
        twsClientPtr->OnConnectionStateChangedTask(state);
    });
}

} // namespace Sle
} // namespace OHOS