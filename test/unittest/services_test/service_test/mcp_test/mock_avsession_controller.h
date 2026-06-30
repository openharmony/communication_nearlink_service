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

#ifndef MOCK_AVSESSION_CONTROLLER_H
#define MOCK_AVSESSION_CONTROLLER_H

#include <gmock/gmock.h>

#include "avsession_controller.h"
#include "log.h"

namespace OHOS {
namespace AVSession {

class MockAVSessionController : public AVSessionController {
public:
    virtual ~MockAVSessionController() = default;
    MOCK_METHOD(int32_t, GetAVCallState, (AVCallState& avCallState), (override));
    MOCK_METHOD(int32_t, GetAVCallMetaData, (AVCallMetaData& avCallMetaData), (override));
    MOCK_METHOD(int32_t, GetAVPlaybackState, (AVPlaybackState& state), (override));
    MOCK_METHOD(int32_t, GetAVMetaData, (AVMetaData& data), (override));
    MOCK_METHOD(int32_t, SendAVKeyEvent, (const MMI::KeyEvent& keyEvent), (override));
    MOCK_METHOD(int32_t, GetLaunchAbility, (AbilityRuntime::WantAgent::WantAgent& ability), (override));
    MOCK_METHOD(int32_t, GetLaunchAbilityInner, (AbilityRuntime::WantAgent::WantAgent*& ability), (override));
    MOCK_METHOD(int32_t, GetValidCommands, (std::vector<int32_t>& cmds), (override));
    MOCK_METHOD(int32_t, IsSessionActive, (bool& isActive), (override));
    MOCK_METHOD(int32_t, SendControlCommand, (const AVControlCommand& cmd), (override));
    MOCK_METHOD(int32_t, SendCommonCommand,
        (const std::string& commonCommand, const AAFwk::WantParams& commandArgs), (override));
    MOCK_METHOD(int32_t, RegisterCallback, (const std::shared_ptr<AVControllerCallback>& callback), (override));
    MOCK_METHOD(int32_t, SetAVCallMetaFilter, (const AVCallMetaData::AVCallMetaMaskType& filter), (override));
    MOCK_METHOD(int32_t, SetAVCallStateFilter, (const AVCallState::AVCallStateMaskType& filter), (override));
    MOCK_METHOD(int32_t, SetMetaFilter, (const AVMetaData::MetaMaskType& filter), (override));
    MOCK_METHOD(int32_t, SetPlaybackFilter, (const AVPlaybackState::PlaybackStateMaskType& filter), (override));
    MOCK_METHOD(int32_t, GetAVQueueItems, (std::vector<AVQueueItem>& items), (override));
    MOCK_METHOD(int32_t, GetAVQueueTitle, (std::string& title), (override));
    MOCK_METHOD(int32_t, SkipToQueueItem, (int32_t& itemId), (override));
    MOCK_METHOD(int32_t, GetExtras, (AAFwk::WantParams& extras), (override));
    MOCK_METHOD(int32_t, GetExtrasWithEvent, (const std::string& extraEvent, AAFwk::WantParams& extras), (override));
    MOCK_METHOD(int32_t, Destroy, (), (override));
    MOCK_METHOD(std::string, GetSessionId, (), (override));
    MOCK_METHOD(int64_t, GetRealPlaybackPosition, (), (override));
    MOCK_METHOD(int32_t, SendCustomData, (const AAFwk::WantParams& data), (override));
    MOCK_METHOD(bool, IsDestroy, (), (override));
};

}  // namespace AVSession
}  // namespace OHOS

#endif  // MOCK_AVSESSION_CONTROLLER_H