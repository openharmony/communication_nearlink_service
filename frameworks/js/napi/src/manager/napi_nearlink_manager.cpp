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

#include "napi_nearlink_manager.h"

#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"

#include "napi_nearlink_error.h"
#include "napi_nearlink_manager_observer.h"
#include "napi_nearlink_utils.h"
#include "napi_parser_utils.h"
#include "napi_nearlink_remote_device_callback.h"
#include "napi_ha_manager.h"
namespace OHOS {
namespace Nearlink {
namespace {

std::shared_ptr<NapiNearlinkManagerObserver> g_nearlinkManagerObserver =
    std::make_shared<NapiNearlinkManagerObserver>();;

}  // namespace

napi_value NapiNearlinkManager::DefineManagerJSFunction(napi_env env, napi_value exports)
{
    HILOGD("enter");
    RegisterManagerObserverToHost();
    ManagerPropertyValueInit(env, exports);
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("getState", GetState),
        DECLARE_NAPI_FUNCTION("enable", Enable),
        DECLARE_NAPI_FUNCTION("disable", Disable),
        DECLARE_NAPI_FUNCTION("onStateChange", OnStateChange),
        DECLARE_NAPI_FUNCTION("offStateChange", OffStateChange),
        DECLARE_NAPI_FUNCTION("getLocalAddress", GetLocalAddress),
        DECLARE_NAPI_FUNCTION("getLocalName", GetLocalName),
        DECLARE_NAPI_FUNCTION("setLocalName", SetLocalName),
        DECLARE_NAPI_FUNCTION("getPairedDevices", GetPairedDevices),
        DECLARE_NAPI_FUNCTION("setConnectionMode", SetConnectionMode),
        DECLARE_NAPI_FUNCTION("factoryReset", FactoryReset),
        DECLARE_NAPI_FUNCTION("isNearLinkSupported", IsNearLinkSupported),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    HILOGI("end");
    return exports;
}

void NapiNearlinkManager::RegisterManagerObserverToHost()
{
    HILOGI("enter");
    NearlinkHost::GetInstance().RegisterObserver(g_nearlinkManagerObserver);
}

napi_value NapiNearlinkManager::Enable(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().EnableNl();
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    return NapiGetBooleanTrue(env);
}

napi_value NapiNearlinkManager::Disable(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NlErrCode err = NearlinkHost::GetInstance().DisableNl();
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    return NapiGetBooleanTrue(env);
}

napi_value NapiNearlinkManager::GetState(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    int32_t state = (NearlinkHost::GetInstance().IsSleEnabled()) ?
        static_cast<int32_t>(SleStateID::STATE_TURN_ON) : static_cast<int32_t>(SleStateID::STATE_TURN_OFF);

    napi_value result = nullptr;
    napi_create_int32(env, state, &result);
    return result;
}


napi_value NapiNearlinkManager::OnStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkManagerObserver->eventSubscribe.RegisterWithName(
        env, info, REGISTER_NEARLINK_STATE_CHANGE_TYPE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkManager::OffStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkManagerObserver->eventSubscribe.DeregisterWithName(
        env, info, REGISTER_NEARLINK_STATE_CHANGE_TYPE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkManager::GetLocalAddress(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::string localAddress;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalAddress(localAddress);
    napi_value value = nullptr;
    napi_create_string_utf8(env, localAddress.c_str(), NAPI_AUTO_LENGTH, &value);
    NAPI_NL_ASSERT_RETURN(env, err == NL_NO_ERROR, err, value);
    return value;
}

napi_value NapiNearlinkManager::GetLocalName(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::string localName;
    NlErrCode err = NearlinkHost::GetInstance().GetLocalName(localName);
    napi_value value = nullptr;
    napi_create_string_utf8(env, localName.c_str(), NAPI_AUTO_LENGTH, &value);
    NAPI_NL_ASSERT_RETURN(env, err == NL_NO_ERROR, err, value);
    return value;
}

napi_value NapiNearlinkManager::SetLocalName(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    std::string localName {};
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseString(env, localName, argv[0]), NL_ERR_INVALID_PARAM);

    NlErrCode err = NearlinkHost::GetInstance().SetLocalName(localName);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    napi_value res = nullptr;
    napi_get_boolean((env), true, &res);
    return res;
}

napi_value NapiNearlinkManager::GetPairedDevices(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::vector<NearlinkRemoteDevice> pairedDevices;
    NlErrCode err = NearlinkHost::GetInstance().GetPairedDevices(SleTransport::ADAPTER_SLE, pairedDevices);

    std::vector<std::string> addrVector;
    for (auto &device : pairedDevices) {
        addrVector.push_back(device.GetDeviceAddr());
    }

    napi_value array;
    napi_create_array(env, &array);
    ConvertStrVectorToJS(env, array, addrVector);
    NAPI_NL_ASSERT_RETURN(env, err == NL_NO_ERROR, err, array);
    return array;
}

napi_value NapiNearlinkManager::SetConnectionMode(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("SetConnectionMode", 0, false);
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    int32_t connectionMode = 0;
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseInt32(env, connectionMode, argv[PARAM0]), NL_ERR_INVALID_PARAM);
    if (connectionMode < 0 || connectionMode > 1) {
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_INTERGER);
        return NapiGetUndefinedRet(env);
    }
    int32_t duration = 0;
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseInt32(env, duration, argv[PARAM1]), NL_ERR_INVALID_PARAM);
    if (duration < 0) {
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_INTERGER);
        return NapiGetUndefinedRet(env);
    }

    auto func = [connectionMode, duration]() -> NapiAsyncWorkRet {
        NlErrCode ret = NearlinkHost::GetInstance().SetConnectionMode(connectionMode, duration);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkManager::FactoryReset(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("FactoryReset", 0, false);
    auto func = []() {
        NlErrCode ret = NearlinkHost::GetInstance().NearlinkFactoryReset();
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkManager::IsNearLinkSupported(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    bool isNearlinkSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NapiNativeBool object(isNearlinkSupport);
    NapiHaManager::GetInstance().ReportEvent("isNearlinkSupported", beginTime, NL_NO_ERROR);
    return object.ToNapiValue(env);
}

napi_value NapiNearlinkManager::ManagerPropertyValueInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value stateObj = TsNearlinkStateValueInit(env);
    napi_value connectionMode = ConnectionModeInit(env);
    napi_property_descriptor exportProperty[] = {
        DECLARE_NAPI_PROPERTY("NearlinkState", stateObj),
        DECLARE_NAPI_PROPERTY("ConnectionMode", connectionMode),
    };
    napi_define_properties(env, exports, sizeof(exportProperty) / sizeof(*exportProperty), exportProperty);
    HILOGI("end");
    return exports;
}

napi_value NapiNearlinkManager::TsNearlinkStateValueInit(napi_env env)
{
    HILOGD("enter");
    napi_value state = nullptr;
    napi_create_object(env, &state);
    SetNamedPropertyByInteger(env, state, static_cast<int>(SleStateID::STATE_TURNING_ON), "STATE_TURNING_ON");
    SetNamedPropertyByInteger(env, state, static_cast<int>(SleStateID::STATE_TURN_ON), "STATE_ON");
    SetNamedPropertyByInteger(env, state, static_cast<int>(SleStateID::STATE_TURNING_OFF), "STATE_TURNING_OFF");
    SetNamedPropertyByInteger(env, state, static_cast<int>(SleStateID::STATE_TURN_OFF), "STATE_OFF");
    return state;
}

napi_value NapiNearlinkManager::ConnectionModeInit(napi_env env)
{
    HILOGD("enter");
    napi_value reason = nullptr;
    napi_create_object(env, &reason);
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionMode::SLE_MODE_UNCONNECTABLE),
        "SLE_MODE_UNCONNECTABLE");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionMode::SLE_MODE_CONNECTABLE),
        "SLE_MODE_CONNECTABLE");
    return reason;
}

EXTERN_C_START
/***********************************************
 * Module export and register
 ***********************************************/
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----Manager Init start------");

    NapiNearlinkManager::DefineManagerJSFunction(env, exports);
    HILOGI("-----Manager Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkManagerModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.manager",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register NearlinkManagerModule nm_modname:%{public}s", nearlinkManagerModule.nm_modname);
    napi_module_register(&nearlinkManagerModule);
}
}  // namespace Nearlink
}  // namespace OHOS
