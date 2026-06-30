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

#include <gmock/gmock.h>

#include "nearlink_call_client.h"

namespace OHOS {
namespace Telephony {

class MockNearlinkCallClient : public NearlinkCallClient {
public:
    MOCK_METHOD(std::vector<CallAttributeInfo>, GetCurrentCallList, (int32_t slotId), ());
    MOCK_METHOD(int32_t, AnswerCall, (), ());
    MOCK_METHOD(int32_t, HangUpCall, (), ());
    MOCK_METHOD(int32_t, RejectCall, (), ());
    MOCK_METHOD(int32_t, RegisterCallBack, (std::unique_ptr<CallManagerCallback> callback), ());
    MOCK_METHOD(int32_t, UnRegisterCallBack, (), ());
};

}  // namespace AVSession
}  // namespace OHOS