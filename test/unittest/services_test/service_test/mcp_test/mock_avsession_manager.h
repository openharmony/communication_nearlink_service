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

#ifndef MOCK_AVSESSION_MANAGER_H
#define MOCK_AVSESSION_MANAGER_H

#include <gmock/gmock.h>

#include "avsession_manager.h"
#include "log.h"

namespace OHOS {
namespace AVSession {

class MockAVSessionManager : public AVSessionManager {
public:
    virtual ~MockAVSessionManager() = default;
    static MockAVSessionManager& StubGetInstance()
    {
        HILOGI("MockAVSessionManager::StubGetInstance called!");
        static MockAVSessionManager instance;
        return instance;
    }
    MOCK_METHOD(int32_t, SendSystemAVKeyEvent, (const MMI::KeyEvent& keyEvent), (override));
    MOCK_METHOD(std::shared_ptr<AVSession>, CreateSession, (const std::string& tag, int32_t type,
        const AppExecFwk::ElementName& elementName), (override));
    MOCK_METHOD(int32_t, CreateSession, (const std::string& tag, int32_t type,
        const AppExecFwk::ElementName& elementName, std::shared_ptr<AVSession>& session), (override));
    MOCK_METHOD(int32_t, GetAllSessionDescriptors, (std::vector<AVSessionDescriptor>& descriptors), (override));
    MOCK_METHOD(int32_t, GetActivatedSessionDescriptors, (std::vector<AVSessionDescriptor>& activatedSessions),
        (override));
    MOCK_METHOD(int32_t, GetSessionDescriptorsBySessionId, (const std::string& sessionId,
        AVSessionDescriptor& descriptor), (override));
    MOCK_METHOD(int32_t, GetSessionDescriptors, (int32_t category, std::vector<AVSessionDescriptor>& descriptors),
        (override));
    MOCK_METHOD(int32_t, GetHistoricalSessionDescriptors, (int32_t maxSize,
        std::vector<AVSessionDescriptor>& descriptors), (override));
    MOCK_METHOD(int32_t, GetHistoricalAVQueueInfos, (int32_t maxSize, int32_t maxAppSize,
        std::vector<AVQueueInfo>& avQueueInfo), (override));
    MOCK_METHOD(int32_t, CreateController, (const std::string& sessionId,
        std::shared_ptr<AVSessionController>& controller), (override));
    MOCK_METHOD(int32_t, RegisterSessionListener, (const std::shared_ptr<SessionListener>& listener), (override));
    MOCK_METHOD(int32_t, RegisterSessionListenerForAllUsers, (const std::shared_ptr<SessionListener>& listener),
        (override));
    MOCK_METHOD(int32_t, RegisterServiceDeathCallback, (const DeathCallback& callback), (override));
    MOCK_METHOD(int32_t, UnregisterServiceDeathCallback, (), (override));
    MOCK_METHOD(int32_t, RegisterServiceStartCallback, (const std::function<void()> serviceStartCallback), (override));
    MOCK_METHOD(int32_t, UnregisterServiceStartCallback, (), (override));
    MOCK_METHOD(int32_t, SendSystemAVKeyEvent, (const MMI::KeyEvent& keyEvent, const AAFwk::Want &wantParam),
        (override));
    MOCK_METHOD(int32_t, SendSystemControlCommand, (const AVControlCommand& command), (override));
    MOCK_METHOD(int32_t, SendSystemCommonCommand, (const std::string& commonCommand,
        const AAFwk::WantParams& commandArgs), (override));
    MOCK_METHOD(int32_t, CastAudio, (const SessionToken& token,
        const std::vector<AudioStandard::AudioDeviceDescriptor>& descriptors), (override));
    MOCK_METHOD(int32_t, CastAudioForAll, (const std::vector<AudioStandard::AudioDeviceDescriptor>& descriptors),
        (override));
    MOCK_METHOD(int32_t, StartAVPlayback, (const std::string& bundleName, const std::string& assetId,
        const std::string& moduleName), (override));
    MOCK_METHOD(int32_t, RegisterAncoMediaSessionListener, (const std::shared_ptr<AncoMediaSessionListener> &listener),
        (override));
    MOCK_METHOD(int32_t, Close, (), (override));
    MOCK_METHOD(int32_t, GetDistributedSessionControllers, (const DistributedSessionType& sessionType,
        std::vector<std::shared_ptr<AVSessionController>>& sessionControllers), (override));
    MOCK_METHOD(int32_t, GetSession, (const AppExecFwk::ElementName& elementName,
        std::string& tag, std::shared_ptr<AVSession>& session), (override));
    MOCK_METHOD(int32_t, IsDesktopLyricSupported, (bool &isSupported), (override));
};

}  // namespace AVSession
}  // namespace OHOS

#endif  // MOCK_AVSESSION_MANAGER_H