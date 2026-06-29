/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef VAS_SERVICE_H
#define VAS_SERVICE_H

#include <atomic>
#include <unordered_map>

#include "context.h"
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
class VasService : public SleInterfaceProfile, public utility::Context {
public:
    static VasService *GetService();
    explicit VasService();
    ~VasService() override;
    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;
    /* 作为VAS作为Server时，内部没有连接相关的信息 */
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

    /* 手机内部触发&关闭语音助手 */
    void OpenVoiceAssistant(const RawAddress &device);
    void CloseVoiceAssistant(const RawAddress &device);
    /* 对端请求触发&关闭语音助手 */
    void HandleActivateVoiceAssistant(const RawAddress &device, uint32_t requestId);
    void HandleCloseVoiceAssistant(const RawAddress &device, uint32_t requestId);

private:
    int32_t WakeUpVoiceRecognition();
    void DftVoiceAssistantSubScene(const RawAddress &device, int subSceneCode, int32_t controlResult);

    NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // VAS_SERVICE_H