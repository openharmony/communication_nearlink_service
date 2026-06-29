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

#ifndef ANI_NEARLINK_MANAGER_CALLBACK_H
#define ANI_NEARLINK_MANAGER_CALLBACK_H

#include "nearlink_host.h"
#include "ohos.nearlink.manager.proj.hpp"
#include "ohos.nearlink.manager.impl.hpp"
#include "taihe/runtime.hpp"

namespace OHOS {
namespace Nearlink {
using namespace Nearlink;

constexpr int MAX_CB_NUM = 100;
extern std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>>>
    g_stateChangedObserverVec;
extern std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingRequestParam const&)>>>
    g_pairingRequestObserverVec;
extern std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingStateParam const&)>>>
    g_pairStatusChangedObserverVec;
extern std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::ConnectionStateParam const&)>>>
    g_connectionStateChangedObserverVec;
extern std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::AcbStateParam const&)>>>
    g_acbStateChangedObserverVec;

class AniNearlinkManagerObserver : public NearlinkHostObserver {
public:
    void OnStateChanged(const int transport, const int status) override;
};

class AniRemoteDeviceObserver : public NearlinkRemoteDeviceObserver {
public:
    void OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type) override;
    void OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override;
    void OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override;
    void OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason) override;
    void OnRemoteUuidChanged(const NearlinkRemoteDevice &device, const std::vector<UUID> &uuids) override {};
    void OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName) override {};
    void OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias) override {};
    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override {};
};

class AniNearlinkManager {
public:
    static void CallbackInit();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // ANI_NEARLINK_MANAGER_CALLBACK_H