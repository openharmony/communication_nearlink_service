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
#include <sstream>
#include "nearlink_native_token_mock.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "log.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Nearlink {

// 获取Native进程tokenID
static AccessTokenID GetNativeTokenIdFromProcess(const std::string &process)
{
    std::string dumpInfo;
    AtmToolsParamInfo info;
    info.processName = process;
    AccessTokenKit::DumpTokenInfo(info, dumpInfo);
    size_t pos = dumpInfo.find("\"tokenID\": ");
    if (pos == std::string::npos) {
        return 0;
    }

    pos += std::string("\"tokenID\": ").length();
    std::string numStr;
    while (pos < dumpInfo.length() && std::isdigit(dumpInfo[pos])) {
        numStr += dumpInfo[pos];
        ++pos;
    }

    std::istringstream iss(numStr);
    AccessTokenID tokenID;
    iss >> tokenID;
    return tokenID;
}

// 传入有效的系统SA进程
NearlinkMockNativeToken::NearlinkMockNativeToken(const std::string& process)
{
    // 1. 获取当前进程tokenID
    selfToken_ = GetSelfTokenID();
    HILOGI("selfToken_ = 0x%{public}lx", selfToken_);
    // 2. 获取需要mock的SA进程的tokenID
    uint32_t mockTokenId = GetNativeTokenIdFromProcess(process);
    HILOGI("mockTokenId = 0x%{public}x", mockTokenId);
    // 3. 设置当前进程的tokenID为需要mock的进程对的tokenID
    SetSelfTokenID(mockTokenId);
    HILOGI("check current tokenId = 0x%{public}lx", GetSelfTokenID());
}

// 析构时恢复环境，恢复
NearlinkMockNativeToken::~NearlinkMockNativeToken()
{
    // reset tokenID
    HILOGI("check current tokenId = 0x%{public}lx", GetSelfTokenID());
}

} // namespace OHOS
} // namespace Nearlink