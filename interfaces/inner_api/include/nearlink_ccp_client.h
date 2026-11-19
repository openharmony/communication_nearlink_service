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

#ifndef NEARLINK_CCP_CLIENT_H
#define NEARLINK_CCP_CLIENT_H

#include <cstdint>
#include <string>

#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Class for Call Control Profile API.
 *
 * @since 6
 */
class NEARLINK_API NearlinkCcpClient {
public:
    /**
     * @brief Get the instance of NearlinkCcpClient object.
     *
     * @return Returns the reference to the process-wide NearlinkCcpClient instance;
     *         it is a function-local static and never invalid, no null check needed.
     * @since 6
     */
    static NearlinkCcpClient &GetProfile();

    /**
     * @brief Push phone call state changed to CCP service.
     *
     * @param[in] callId id of the changed call; -1 means no specific call, any
     *            other value not less than -1 is accepted and mapped to a local
     *            short id by the service.
     * @param[in] numActive number of active calls, diagnostic only; the service
     *            re-queries the authoritative call list instead of using this value.
     * @param[in] numHeld number of held calls, diagnostic only; the service
     *            re-queries the authoritative call list instead of using this value.
     * @param[in] callState current phone call state.
     * @param[in] number call number.
     * @return 0 if the push is accepted for asynchronous handling; otherwise
     *         NL_ERR_SLE_OFF, NL_ERR_UNAVAILABLE_PROXY or NL_ERR_IPC_TRANS_FAILED.
     * @since 6
     */
    int32_t PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld, int32_t callState,
        const std::string &number);

    /**
     * @brief Push single call details changed to CCP service.
     *
     * @param[in] callId id of the changed call; -1 means no specific call and is
     *            skipped, any other value not less than -1 is accepted and mapped
     *            to a local short id by the service.
     * @param[in] callState state of the changed call.
     * @param[in] callType call type of the changed call, the value is the same as Telephony::CallType.
     * @return 0 if the push is accepted for asynchronous handling; otherwise
     *         NL_ERR_SLE_OFF, NL_ERR_UNAVAILABLE_PROXY or NL_ERR_IPC_TRANS_FAILED.
     * @since 6
     */
    int32_t CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType);

private:
    NearlinkCcpClient() = default;
    ~NearlinkCcpClient() = default;

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCcpClient);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_CCP_CLIENT_H
