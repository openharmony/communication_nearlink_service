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

#ifndef OHOS_NEARLINK_CCP_INTERFACE_CODE_H
#define OHOS_NEARLINK_CCP_INTERFACE_CODE_H

namespace OHOS {
namespace Nearlink {

/**
 * @brief Call Control Profile 客户端 IPC 接口码。
 *        stub 与 proxy 共用，新增接口码需同时更新两侧。
 */
enum NearlinkCcpInterfaceCode {
    NL_CCP_CLIENT_PHONE_STATE_CHANGED = 0,
    NL_CCP_CLIENT_CALL_DETAILS_CHANGED = 1,
    // The last code, if you want to add a new code, please add it before this
    NL_CCP_CLIENT_BUTT
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // OHOS_NEARLINK_CCP_INTERFACE_CODE_H
