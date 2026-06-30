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
#ifndef CCP_SERVICE_H
#define CCP_SERVICE_H

#include <atomic>
#include <unordered_map>

#include "context.h"
#include "call_manager_info.h"
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "BaseObserverList.h"
#include "SleInterfaceAdapter.h"
#include "ProfileServiceManager.h"
#include "CcpDefines.h"
#include "NearlinkCallPhoneState.h"

namespace OHOS {
namespace Nearlink {
class CcpService : public SleInterfaceProfile, public utility::Context {
public:
    static CcpService *GetService();
    explicit CcpService();
    ~CcpService() override;
    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;
    /* 作为CcsServer时，内部没有连接相关的信息 */
    int Connect(const RawAddress &device) override
    {
        return 0;
    }
    int Disconnect(const RawAddress &device) override
    {
        return 0;
    }
    std::list<RawAddress> GetConnectDevices() override
    {
        return {};
    }
    int GetConnectState() override
    {
        return 0;
    }

    void HandleAnswer(const RawAddress &device, int32_t instanceId, uint32_t requestId, uint8_t callId);
    void HandleHangUp(const RawAddress &device, int32_t instanceId, uint32_t requestId, uint8_t callId);
    void HandleStartInstance(int32_t instanceId, bool isSuccess);
    void HandleAuthorize(int32_t instanceId, uint32_t requestId, int32_t property);
    void HandlePhoneStateChange(const NearlinkCallPhoneState &phoneState);
    void HandleCallDetailChange(const Telephony::CallAttributeInfo &info);
    void HandleMeeTimeDetailsChange(const Telephony::CallAttributeInfo &info);
    bool IsNearlinkActiveDevice(const RawAddress &reportAddr);
    
    void SetGeneralInstanceId(int32_t instanceId)
    {
        instanceId_ = instanceId;
    }

    void HandleVoipStart(const RawAddress &device);
    void HandleVoipStop(const RawAddress &device);
    void TryResumeCurrentCalls();
    void CreateVoipCallInfo();

private:
    void ProcessCallDetailChange(const Telephony::CallAttributeInfo &info);
    void ProcessCallStateInfo(std::vector<Telephony::CallAttributeInfo> callInfoVec);
    void ProcessCallInOutInfo(const Telephony::CallAttributeInfo &info);
    void ProcessCallTerminateInfo(const Telephony::CallAttributeInfo &info);
    CallFlagBits GetCallFlag(const Telephony::CallAttributeInfo &info);
    bool ProcessAccountInfo(std::shared_ptr<CallInOutInfoProp> prop, const Telephony::CallAttributeInfo &info);
    void NotifyAudioDeviceAction(const RawAddress &devAddr, int action);
    bool HasVoipCallId();

    /* 通用通话控制实例ID */
    uint8_t instanceId_ {CCP_INSTANCE_ID};
    NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // CCP_SERVICE_H