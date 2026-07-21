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

#include <memory>
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "nearlink_sle_datatransfer.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log_util.h"
#include "napi_parser_utils.h"
#include "napi_nearlink_datatransfer.h"
#include "napi_nearlink_datatransfer_callback.h"
#include "napi_ha_manager.h"

namespace OHOS {
namespace Nearlink {
namespace {
std::shared_ptr<SleDataTransfer> SleDataTransferGetInstance(void)
{
    static std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    return instance;
}
}  // namespace

void NapiNearlinkDataTransfer::DefineDataTransferJSObject(napi_env env, napi_value exports)
{
    HILOGD("enter");
    PropertyInit(env, exports);
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createPort", CreatePort),
        DECLARE_NAPI_FUNCTION("destroyPort", DestroyPort),
        DECLARE_NAPI_FUNCTION("connect", Connect),
        DECLARE_NAPI_FUNCTION("disconnect", Disconnect),
        DECLARE_NAPI_FUNCTION("getConnectionState", GetConnectionState),
        DECLARE_NAPI_FUNCTION("writeData", WriteData),
        DECLARE_NAPI_FUNCTION("onConnectionStateChanged", OnConnectionStateChanged),
        DECLARE_NAPI_FUNCTION("offConnectionStateChanged", OffConnectionStateChanged),
        DECLARE_NAPI_FUNCTION("onReadData", onReadData),
        DECLARE_NAPI_FUNCTION("offReadData", offReadData),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_status CheckPortUuid(napi_env env, std::string &uuid)
{
    ConvertUuidToUpperCase(uuid); // port UUID统一转换为大写字母形式
    if (uuid == "060D") { // 专用于数字车钥匙
        return napi_ok;
    }
    if (!IsValidUuid(uuid)) {
        HILOGE("Invalid UUID format");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_UUID);
        return napi_invalid_arg;
    }
    if (CheckBaseUuid(uuid)) {
        HILOGE("Standard UUID not allowed");
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_value NapiNearlinkDataTransfer::CreatePort(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    std::string uuid{};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ParseString(env, uuid, argv[PARAM0]), NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckPortUuid(env, uuid) == napi_ok, NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    NAPI_NL_ASSERT_RETURN_UNDEF(env, sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = sleDataTransfer->CreatePort(uuid, NapiNearlinkDataTransferCallback::GetInstance());
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    napi_get_boolean((env), true, &res);
    return res;
}

napi_value NapiNearlinkDataTransfer::DestroyPort(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INTERNAL_ERROR);
    std::string uuid{};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ParseString(env, uuid, argv[PARAM0]), NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckPortUuid(env, uuid) == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto sleDataTransfer = SleDataTransferGetInstance();
    NAPI_NL_ASSERT_RETURN_UNDEF(env, sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = sleDataTransfer->DestroyPort(uuid);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    napi_get_boolean((env), true, &res);
    return res;
}

napi_status CheckDataTransferConnectionParamWith(napi_env env, napi_value object, ConnectionParams &params)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"address", "uuid", "transferMode"}));

    napi_value property;
    std::string address{};
    std::string uuid{};
    uint8_t transMode;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "address", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, address));
    if (!IsValidAddress(address)) {
        HILOGE("Invalid address");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
        return napi_invalid_arg;
    }
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "uuid", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, uuid));
    NAPI_NL_CALL_RETURN(CheckPortUuid(env, uuid));
    bool exist;
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, "transferMode", property, exist));
    if (exist) {
        NAPI_NL_CALL_RETURN(NapiParseTransMode(env, property, transMode));
        params.SetTransMode(transMode);
    }
    params.SetAddress(address);
    params.SetUuid(uuid);
    return napi_ok;
}

napi_status CheckDataTransferConnStateParam(napi_env env, napi_value object, ConnStateParams &params)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"address", "uuid"}));

    napi_value property;
    std::string address{};
    std::string uuid{};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "address", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, address));
    if (!IsValidAddress(address)) {
        HILOGE("Invalid address");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
        return napi_invalid_arg;
    }
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "uuid", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, uuid));
    NAPI_NL_CALL_RETURN(CheckPortUuid(env, uuid));

    params.SetAddress(address);
    params.SetUuid(uuid);
    return napi_ok;
}

napi_value NapiNearlinkDataTransfer::Connect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("dataTransfer.connect", beginTime, true);
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    ConnectionParams params;
    auto status = CheckDataTransferConnectionParamWith(env, argv[PARAM0], params);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    auto func = [params]() {
        auto sleDataTransfer = SleDataTransferGetInstance();
        if (sleDataTransfer == nullptr) {
            HILOGE("sleDataTransfer is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }

        int ret = sleDataTransfer->Connect(params);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkDataTransfer::Disconnect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("dataTransfer.disconnect", beginTime, true);
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_HA_EXCEP_REPORT(getCbRes == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_HA_EXCEP_REPORT(argc == ARGS_SIZE_ONE, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    ConnectionParams params;
    auto status = CheckDataTransferConnectionParamWith(env, argv[PARAM0], params);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    auto func = [params]() {
        auto sleDataTransfer = SleDataTransferGetInstance();
        if (sleDataTransfer == nullptr) {
            HILOGE("sleDataTransfer is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }

        int ret = sleDataTransfer->Disconnect(params);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkDataTransfer::GetConnectionState(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INTERNAL_ERROR);
    ConnStateParams params;
    auto status = CheckDataTransferConnStateParam(env, argv[PARAM0], params);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    NAPI_NL_ASSERT_RETURN_UNDEF(env, sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);
    int32_t connState;
    NlErrCode err = sleDataTransfer->GetConnectionState(params, connState);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, err == NL_NO_ERROR, err);
    napi_create_int32(env, connState, &res);
    return res;
}

napi_status NapiParseDataParam(napi_env env, napi_value object, NapiDataTransferParam &outParam)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"address", "uuid", "data"}));

    napi_value property;
    std::string address{};
    std::string uuid{};
    std::vector<uint8_t> propertyValue{};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "address", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, address));
    if (!IsValidAddress(address)) {
        HILOGE("Invalid address");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
        return napi_invalid_arg;
    }
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "uuid", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, uuid));
    NAPI_NL_CALL_RETURN(CheckPortUuid(env, uuid));
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "data", propertyValue));

    outParam.address_ = address;
    outParam.uuid_ = uuid;
    outParam.data_ = std::move(propertyValue);
    return napi_ok;
}

napi_value NapiNearlinkDataTransfer::WriteData(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("dataTransfer.WriteData", 0, false);
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);

    NapiDataTransferParam napiDataParam;
    auto parseStatus = NapiParseDataParam(env, argv[PARAM0], napiDataParam);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, parseStatus == napi_ok, NL_ERR_INVALID_PARAM);

    DataParams params(napiDataParam.address_, napiDataParam.uuid_);
    params.SetData(napiDataParam.data_.data(), napiDataParam.data_.size());
    auto func = [params]() mutable {
        auto sleDataTransfer = SleDataTransferGetInstance();
        if (sleDataTransfer == nullptr) {
            HILOGE("sleDataTransfer is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }

        int ret = sleDataTransfer->WriteData(params);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkDataTransfer::OnConnectionStateChanged(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkDataTransferCallback::GetInstance()->eventSubscribe.RegisterWithName(
        env, info, SLE_DATATRANSFER_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkDataTransfer::OffConnectionStateChanged(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkDataTransferCallback::GetInstance()->eventSubscribe.DeregisterWithName(
        env, info, SLE_DATATRANSFER_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkDataTransfer::onReadData(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkDataTransferCallback::GetInstance()->eventSubscribe.RegisterWithName(
        env, info, SLE_DATATRANSFER_CALLBACK_READ_DATA);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkDataTransfer::offReadData(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkDataTransferCallback::GetInstance()->eventSubscribe.DeregisterWithName(
        env, info, SLE_DATATRANSFER_CALLBACK_READ_DATA);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkDataTransfer::PropertyInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value transferMode = nullptr;
    napi_create_object(env, &transferMode);
    SetNamedPropertyByInteger(env, transferMode, static_cast<int>(TransferMode::BASIC), "BASIC");
    SetNamedPropertyByInteger(env, transferMode, static_cast<int>(TransferMode::RELIABLE), "RELIABLE");
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("TransferMode", transferMode),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----DataTransfer Init start------");

    NapiNearlinkDataTransfer::DefineDataTransferJSObject(env, exports);
    HILOGI("-----DataTransfer Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkDataTransferModule = {.nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.datatransfer",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkDataTransferModule nm_modname:%{public}s", nearlinkDataTransferModule.nm_modname);
    napi_module_register(&nearlinkDataTransferModule);
}
} // namespace Nearlink
} // namespace OHOS