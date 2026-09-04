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
#include "ani_nearlink_error.h"

using namespace taihe;
using namespace OHOS::Nearlink;

namespace {
void enable()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().EnableNl();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void disable()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().DisableNl();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

::ohos::nearlink::manager::NearlinkState getState()
{
    HILOGI("enter");
    int32_t invalidState = static_cast<int32_t>(SleStateID::STATE_TURN_OFF);
    bool result = NearlinkHost::GetInstance().IsNearlinkSupport();
    ANI_NL_ASSERT_RETURN(result, NL_ERR_API_NOT_SUPPORT,
        ohos::nearlink::manager::NearlinkState::from_value(invalidState));
    int32_t state = (NearlinkHost::GetInstance().IsSleEnabled()) ?
        static_cast<int32_t>(SleStateID::STATE_TURN_ON) : static_cast<int32_t>(SleStateID::STATE_TURN_OFF);
    ohos::nearlink::manager::NearlinkState stateValue =
        ohos::nearlink::manager::NearlinkState::from_value(state);
    return stateValue;
}

::taihe::array<::taihe::string> getPairedDevices()
{
    HILOGI("enter");
    ::taihe::array<::taihe::string> result {};
    std::vector<NearlinkRemoteDevice> pairedDevices;
    NlErrCode err = NearlinkHost::GetInstance().GetPairedDevices(SleTransport::ADAPTER_SLE, pairedDevices);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, result);

    std::vector<std::string> addrVector;
    for (auto &device : pairedDevices) {
        addrVector.push_back(device.GetDeviceAddr());
    }
    result = ::taihe::array<::taihe::string>(taihe::copy_data_t{}, addrVector.data(), addrVector.size());
    HILOGI("end");
    return result;
}

::taihe::string getLocalAddress()
{
    HILOGI("enter");
    std::string localAddress;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalAddress(localAddress);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, "");
    return localAddress;
}

string getLocalName()
{
    HILOGI("enter");
    std::string localName;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalName(localName);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, "");
    return localName;
}

bool isNearLinkSupported()
{
    HILOGI("enter");
    return NearlinkHost::GetInstance().IsNearlinkSupport();
}

void setConnectionMode(::ohos::nearlink::manager::ConnectionMode mode, int duration)
{
    HILOGI("enter");
    int32_t connectionMode = mode.get_value();
    if (connectionMode < 0 || connectionMode > 1) {
        ANI_NL_ASSERT_RETURN_VOID(false, NL_ERR_INVALID_INTERGER);
    }
    if (duration < 0) {
        ANI_NL_ASSERT_RETURN_VOID(false, NL_ERR_INVALID_INTERGER);
    }
    NlErrCode err = NearlinkHost::GetInstance().SetConnectionMode(connectionMode, duration);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}
 
void factoryReset()
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().NearlinkFactoryReset();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void OnStateChange(::taihe::callback_view<void(::ohos::nearlink::manager::NearlinkState data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    ANI_NL_ASSERT_RETURN_VOID(g_stateChangedObserverVec.size() <= MAX_CB_NUM, NL_ERR_INVALID_PARAM);

    ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>> stateChangeCb =
        ::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>>{
            std::in_place_t{}, callback};
    std::unique_lock<std::shared_mutex> guard(g_stateChangedMutex);
    if (std::find(g_stateChangedObserverVec.begin(), g_stateChangedObserverVec.end(), stateChangeCb) !=
        g_stateChangedObserverVec.end()) {
        return;
    }
    g_stateChangedObserverVec.emplace_back(stateChangeCb);
}

void OffStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    std::unique_lock<std::shared_mutex> guard(g_stateChangedMutex);
    ANI_NL_ASSERT_RETURN_VOID(!g_stateChangedObserverVec.empty(), NL_ERR_INVALID_PARAM);
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
}  // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_enable(enable);
TH_EXPORT_CPP_API_disable(disable);
TH_EXPORT_CPP_API_getState(getState);
TH_EXPORT_CPP_API_getPairedDevices(getPairedDevices);
TH_EXPORT_CPP_API_getLocalAddress(getLocalAddress);
TH_EXPORT_CPP_API_getLocalName(getLocalName);
TH_EXPORT_CPP_API_isNearLinkSupported(isNearLinkSupported);
TH_EXPORT_CPP_API_setConnectionMode(setConnectionMode);
TH_EXPORT_CPP_API_factoryReset(factoryReset);
TH_EXPORT_CPP_API_OnStateChange(OnStateChange);
TH_EXPORT_CPP_API_OffStateChange(OffStateChange);
// NOLINTEND
