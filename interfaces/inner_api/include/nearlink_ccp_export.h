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

#ifndef OHOS_NEARLINK_CCP_EXPORT_H
#define OHOS_NEARLINK_CCP_EXPORT_H

#include <cstdint>

#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief CCP 直连接口返回码，派生自服务侧 NlErrCode，不在此处重复取值：
 *        0 表示成功，非 0 表示失败。
 */
constexpr int32_t NL_CCP_SUCCESS = static_cast<int32_t>(NL_NO_ERROR);
constexpr int32_t NL_CCP_ERR_IPC_TRANS_FAILED = static_cast<int32_t>(NL_ERR_IPC_TRANS_FAILED);
constexpr int32_t NL_CCP_ERR_SLE_OFF = static_cast<int32_t>(NL_ERR_SLE_OFF);
constexpr int32_t NL_CCP_ERR_UNAVAILABLE_PROXY = static_cast<int32_t>(NL_ERR_UNAVAILABLE_PROXY);

}  // namespace Nearlink
}  // namespace OHOS

#endif  // OHOS_NEARLINK_CCP_EXPORT_H
