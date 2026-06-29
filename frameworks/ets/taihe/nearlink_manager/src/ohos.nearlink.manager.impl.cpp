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

#include "ohos.nearlink.manager.proj.hpp"
#include "ohos.nearlink.manager.impl.hpp"
#include "ani_nearlink_manager_callback.h"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "nearlink_host.h"
#include "log.h"
#include "nearlink_errorcode.h"

using namespace taihe;
using namespace OHOS::Nearlink;

namespace {
void enable()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().EnableNl();
    NL_CHECK_RETURN(err == NL_NO_ERROR, "enable failed");
}

void disable()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().DisableNl();
    NL_CHECK_RETURN(err == NL_NO_ERROR, "disable failed");
}

::ohos::nearlink::manager::NearlinkState getState()
{
    HILOGI("enter");
    int32_t state = (NearlinkHost::GetInstance().IsSleEnabled()) ?
        static_cast<int32_t>(SleStateID::STATE_TURN_ON) : static_cast<int32_t>(SleStateID::STATE_TURN_OFF);
    return {static_cast<::ohos::nearlink::manager::NearlinkState::key_t>(state)};
}

array<string> getPairedDevices()
{
    HILOGI("enter");
    std::vector<NearlinkRemoteDevice> pairedDevices;
    NlErrCode err = NearlinkHost::GetInstance().GetPairedDevices(SleTransport::ADAPTER_SLE, pairedDevices);
    NL_CHECK_RETURN_RET(err == NL_NO_ERROR, {}, "getPairedDevices failed");

    std::vector<std::string> addrVector;
    for (auto &device : pairedDevices) {
        addrVector.push_back(device.GetDeviceAddr());
    }
    array<string> result(taihe::copy_data_t{}, addrVector.data(), addrVector.size());
    HILOGI("end");
    return result;
}

string getLocalAddress()
{
    HILOGI("enter");
    std::string localAddress;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalAddress(localAddress);
    NL_CHECK_RETURN_RET(err == NL_NO_ERROR, "", "getLocalAddress failed");
    return localAddress;
}

string getLocalName()
{
    HILOGI("enter");
    std::string localName;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalName(localName);
    NL_CHECK_RETURN_RET(err == NL_NO_ERROR, "", "getLocalName failed");
    return localName;
}

void setConnectionMode(::ohos::nearlink::manager::ConnectionMode mode, int duration)
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().SetConnectionMode(mode, duration);
    NL_CHECK_RETURN(err == NL_NO_ERROR, "setConnectionMode failed");
}
 
void factoryReset()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().NearlinkFactoryReset();
    NL_CHECK_RETURN(err == NL_NO_ERROR, "factoryReset failed");
}

void onStateChange(::taihe::callback_view<void(::ohos::nearlink::manager::NearlinkState data)> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(g_stateChangedObserverVec.size() <= MAX_CB_NUM, "cb Exceeding the maximum value!");

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>> stateChangeCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>>{
            std::in_place_t{}, callback};

    if (std::find(g_stateChangedObserverVec.begin(), g_stateChangedObserverVec.end(), stateChangeCb) !=
        g_stateChangedObserverVec.end()) {
        return;
    }
    g_stateChangedObserverVec.emplace_back(stateChangeCb);
}

void offStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(!g_stateChangedObserverVec.empty(), "cb not registered!");
    if (callback.has_value()) {
        for (size_t i = 0; i < g_stateChangedObserverVec.size(); ++i) {
            if (g_stateChangedObserverVec[i] == callback) {
                g_stateChangedObserverVec.erase(g_stateChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_stateChangedObserverVec.clear();
    }
}

void onPairingRequest(::taihe::callback_view<void(::ohos::nearlink::manager::PairingRequestParam const&)> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(g_pairingRequestObserverVec.size() <= MAX_CB_NUM, "cb Exceeding the maximum value!");

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingRequestParam const&)>> pairingRequestCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingRequestParam const&)>>{
            std::in_place_t{}, callback};

    if (std::find(g_pairingRequestObserverVec.begin(), g_pairingRequestObserverVec.end(), pairingRequestCb) !=
        g_pairingRequestObserverVec.end()) {
        return;
    }
    g_pairingRequestObserverVec.emplace_back(pairingRequestCb);
}

void offPairingRequest(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::PairingRequestParam const&)>> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(!g_pairingRequestObserverVec.empty(), "cb not registered!");
    if (callback.has_value()) {
        for (size_t i = 0; i < g_pairingRequestObserverVec.size(); ++i) {
            if (g_pairingRequestObserverVec[i] == callback) {
                g_pairingRequestObserverVec.erase(g_pairingRequestObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_pairingRequestObserverVec.clear();
    }
}

void onPairingStateChange(::taihe::callback_view<void(::ohos::nearlink::manager::PairingStateParam const&)> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(g_pairStatusChangedObserverVec.size() <= MAX_CB_NUM, "cb Exceeding the maximum value!");

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingStateParam const&)>>
        pairingStateChangeCb =
            ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingStateParam const&)>>{
                std::in_place_t{}, callback};

    if (std::find(g_pairStatusChangedObserverVec.begin(), g_pairStatusChangedObserverVec.end(), pairingStateChangeCb) !=
        g_pairStatusChangedObserverVec.end()) {
        return;
    }
    g_pairStatusChangedObserverVec.emplace_back(pairingStateChangeCb);
}

void offPairingStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::PairingStateParam const&)>> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(!g_pairStatusChangedObserverVec.empty(), "cb not registered!");
    if (callback.has_value()) {
        for (size_t i = 0; i < g_pairStatusChangedObserverVec.size(); ++i) {
            if (g_pairStatusChangedObserverVec[i] == callback) {
                g_pairStatusChangedObserverVec.erase(g_pairStatusChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_pairStatusChangedObserverVec.clear();
    }
}

void onConnectionStateChange(
    ::taihe::callback_view<void(::ohos::nearlink::manager::ConnectionStateParam const&)> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(g_connectionStateChangedObserverVec.size() <= MAX_CB_NUM, "cb Exceeding the maximum value!");

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::ConnectionStateParam const&)>>
        connectionStateChangeCb =
            ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::ConnectionStateParam const&)>>{
                std::in_place_t{}, callback};

    if (std::find(g_connectionStateChangedObserverVec.begin(), g_connectionStateChangedObserverVec.end(),
        connectionStateChangeCb) != g_connectionStateChangedObserverVec.end()) {
        return;
    }
    g_connectionStateChangedObserverVec.emplace_back(connectionStateChangeCb);
}

void offConnectionStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::ConnectionStateParam const&)>> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(!g_connectionStateChangedObserverVec.empty(), "cb not registered!");
    if (callback.has_value()) {
        for (size_t i = 0; i < g_connectionStateChangedObserverVec.size(); ++i) {
            if (g_connectionStateChangedObserverVec[i] == callback) {
                g_connectionStateChangedObserverVec.erase(g_connectionStateChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_connectionStateChangedObserverVec.clear();
    }
}

void onAcbStateChange(::taihe::callback_view<void(::ohos::nearlink::manager::AcbStateParam const&)> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(g_acbStateChangedObserverVec.size() <= MAX_CB_NUM, "cb Exceeding the maximum value!");

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::AcbStateParam const&)>> acbStateChangeCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::AcbStateParam const&)>>{
            std::in_place_t{}, callback};

    if (std::find(g_acbStateChangedObserverVec.begin(), g_acbStateChangedObserverVec.end(), acbStateChangeCb) !=
        g_acbStateChangedObserverVec.end()) {
        return;
    }
    g_acbStateChangedObserverVec.emplace_back(acbStateChangeCb);
}

void offAcbStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::AcbStateParam const&)>> callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(!g_acbStateChangedObserverVec.empty(), "cb not registered!");
    if (callback.has_value()) {
        for (size_t i = 0; i < g_acbStateChangedObserverVec.size(); ++i) {
            if (g_acbStateChangedObserverVec[i] == callback) {
                g_acbStateChangedObserverVec.erase(g_acbStateChangedObserverVec.begin() + i);
                return;
            }
        }
    } else {
        g_acbStateChangedObserverVec.clear();
    }
}
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_enable(enable);
TH_EXPORT_CPP_API_disable(disable);
TH_EXPORT_CPP_API_getState(getState);
TH_EXPORT_CPP_API_getPairedDevices(getPairedDevices);
TH_EXPORT_CPP_API_getLocalAddress(getLocalAddress);
TH_EXPORT_CPP_API_getLocalName(getLocalName);
TH_EXPORT_CPP_API_setConnectionMode(setConnectionMode);
TH_EXPORT_CPP_API_factoryReset(factoryReset);
TH_EXPORT_CPP_API_onStateChange(onStateChange);
TH_EXPORT_CPP_API_offStateChange(offStateChange);
TH_EXPORT_CPP_API_onPairingRequest(onPairingRequest);
TH_EXPORT_CPP_API_offPairingRequest(offPairingRequest);
TH_EXPORT_CPP_API_onPairingStateChange(onPairingStateChange);
TH_EXPORT_CPP_API_offPairingStateChange(offPairingStateChange);
TH_EXPORT_CPP_API_onConnectionStateChange(onConnectionStateChange);
TH_EXPORT_CPP_API_offConnectionStateChange(offConnectionStateChange);
TH_EXPORT_CPP_API_onAcbStateChange(onAcbStateChange);
TH_EXPORT_CPP_API_offAcbStateChange(offAcbStateChange);
// NOLINTEND
