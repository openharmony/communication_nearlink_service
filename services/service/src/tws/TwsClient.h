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
#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

#include <map>
#include "ssap_data.h"
#include "ssap_def.h"
#include "log_util.h"
#include "interface_profile_ssap_client.h"
#include "SleInterfaceProfileManager.h"
#include "TwsMessage.h"
#include "SleInterfaceProfileTws.h"
#include "BaseObserverList.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {

class TwsClient {
public:
    explicit TwsClient(const std::string &address);
    ~TwsClient();

    /*  Construct a new TwsClient object */
    static std::shared_ptr<TwsClient> CreateTwsClient(const std::string address);

    /* 客户端接口待添加 */
    void ConnectTws();
    void DisconnectTws();

    /* 数据发送：手机->外设 */
    int SendData(std::vector<uint8_t> &data);
private:
    /* 嵌套类定义：ssap回调注册 */
    class TwsClientCallback : public InterfaceSsapClientCallback {
    public:
        explicit TwsClientCallback(std::weak_ptr<TwsClient> twsClient);
        ~TwsClientCallback() = default;
        void OnConnectionStateChanged(uint8_t state, int ret) override;
        void OnPropertyChanged(const Property &property) override;      /* 应用示例：外设->手机 */

    private:
        std::weak_ptr<TwsClient> twsClient_;
    };

    /* 私有服务内部变量 */
    std::string address_;        /* 设备地址 */
    int appId_ = -1;             /* ssap clietn service分配的应用ID,-1:非法值 */
    std::shared_ptr<TwsClientCallback> ssapCallback_ = nullptr;
    std::atomic<TwsClientState> twsClientState_ = TwsClientState::TWS_STATE_DISCONNECTED;

    /* 私有服务发送数据接口依赖数据 */
    Property sendDataProperty_;  /* 手机->外设，发送句柄 */
    Property recvDataProperty_;  /* 外设->手机，接收句柄 */

    void TwsClientSetState(TwsClientState newState);
    TwsClientState TwsClientGetState(void);

    static InterfaceProfileSsapClient *GetSsapClientService();
    bool Init(std::weak_ptr<TwsClient> twsClient);
    void NotifyStateChanged(TwsClientState toState);
    void SaveTwsServicePropertyInfo();

    /* ssap 回调处理 */
    void OnConnectionStateChangedTask(uint8_t clientState);
    void OnPropertyChangedTask(const Property &property);
    void TwsSendManufacturerAbility(const RawAddress &devAddr);
}; // TwsClient
} // namespace Sle
} // namespace OHOS

#endif