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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines Call Control Profile interface that provides basic nearlink telephony
 *
 * @since 6
 */

/**
 * @file nearlink_ccp_client.h
 *
 * @brief Call Control Profile common functions.
 *
 * @since 6
 */

#ifndef NEARLINK_CCP_CLIENT_H
#define NEARLINK_CCP_CLIENT_H

#include <cstdint>
#include <string>

#include "nearlink_ccp_export.h"

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
     * @return Returns the pointer to the NearlinkCcpClient instance.
     * @since 6
     */
    static NearlinkCcpClient *GetProfile();

    /**
     * @brief Check whether CCP service is available.
     *        The ccp proxy is cached after the first successful lookup; returning
     *        true therefore costs no IPC. When the lookup fails (nearlink is off
     *        or the proxy is not ready) the host is re-probed at most once per
     *        retry interval instead of on every call state change.
     *        Note that true only means the client proxy is reachable; it does
     *        not promise that a push will be accepted. When the server side
     *        CCP service object is missing (for example the product has no CCP
     *        profile configured), the push interfaces still return
     *        NL_CCP_ERR_UNAVAILABLE_PROXY.
     *
     * @return Returns true if CCP service is available, otherwise false.
     * @since 6
     */
    bool IsAvailable();

    /**
     * @brief Push phone call state changed to CCP service.
     *        The cached proxy is reused, so a healthy push costs one IPC.
     *        When nearlink is switched off, or the CCP service object is not
     *        ready, the service drops the push and returns NL_CCP_ERR_SLE_OFF
     *        or NL_CCP_ERR_UNAVAILABLE_PROXY, which callers may treat as
     *        degraded.
     *
     * @param[in] callId id of the changed call, -1 means no specific call.
     * @param[in] numActive number of active calls, diagnostic only; the service
     *            re-queries the authoritative call list instead of using this value.
     * @param[in] numHeld number of held calls, diagnostic only; the service
     *            re-queries the authoritative call list instead of using this value.
     * @param[in] callState current phone call state.
     * @param[in] number call number.
     * @return 0 when the push is accepted, which only means the request has been
     *         queued for asynchronous handling, not that the call state has been
     *         processed; otherwise NL_CCP_ERR_SLE_OFF when nearlink is off,
     *         NL_CCP_ERR_UNAVAILABLE_PROXY when the proxy or the CCP service
     *         object is unavailable, NL_CCP_ERR_IPC_TRANS_FAILED when the IPC
     *         transaction fails. NL_CCP_ERR_UNAVAILABLE_PROXY is a degradation
     *         signal rather than a retryable failure: recovery is left to the
     *         next call event, so callers should not retry in a loop.
     * @since 6
     */
    int32_t PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld, int32_t callState,
        const std::string &number);

    /**
     * @brief Push single call details changed to CCP service.
     *        Only CALL_STATUS_DISCONNECTED of a cellular call takes effect, which cleans the
     *        mapping of the given callId; details of the other states are carried by
     *        PhoneStateChanged and VoIP calls are not handled here.
     *
     * @param[in] callId id of the changed call, -1 means no specific call and is skipped.
     * @param[in] callState state of the changed call.
     * @param[in] callType call type of the changed call, the value is the same as Telephony::CallType.
     *            A VoIP call type is ignored by this interface.
     * @return 0 when the push is accepted, which only means the request has been
     *         queued for asynchronous handling, not that the call state has been
     *         processed; otherwise NL_CCP_ERR_SLE_OFF when nearlink is off,
     *         NL_CCP_ERR_UNAVAILABLE_PROXY when the proxy or the CCP service
     *         object is unavailable, NL_CCP_ERR_IPC_TRANS_FAILED when the IPC
     *         transaction fails. NL_CCP_ERR_UNAVAILABLE_PROXY is a degradation
     *         signal rather than a retryable failure: recovery is left to the
     *         next call event, so callers should not retry in a loop.
     * @since 6
     */
    int32_t CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType);

private:
    NearlinkCcpClient();
    ~NearlinkCcpClient();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCcpClient);
    NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_CCP_CLIENT_H
