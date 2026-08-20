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
#include "napi_nearlink_ssap_client.h"
#include <unistd.h>
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log.h"
#include "napi_async_callback.h"
#include "napi_nearlink_ssap_utils.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "napi_parser_utils.h"
#include "napi_ha_manager.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
namespace {
static const int32_t MIN_MTU_SIZE = 22;
static const int32_t MAX_MTU_SIZE = 1024;
}

thread_local napi_ref NapiNearlinkSsapClient::consRef_ = nullptr;

static NapiNearlinkSsapClient *NapiGetSsapClient(napi_env env, napi_value thisVar)
{
    NapiNearlinkSsapClient *ssapClient = nullptr;
    auto status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&ssapClient));
    if (status != napi_ok) {
        return nullptr;
    }
    return ssapClient;
}

static NapiNearlinkSsapClient *NapiGetSsapClient(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        return nullptr;
    }
    return NapiGetSsapClient(env, thisVar);
}

static int32_t CheckCreateClientParams(napi_env env, napi_callback_info info, napi_value *argv, size_t *argc)
{
    size_t expectedArgsCount = ARGS_SIZE_ONE;

    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, argc, argv, nullptr, nullptr));
    NAPI_NL_RETURN_IF((*argc) != expectedArgsCount, "expect 1 args", NlErrCode::NL_ERR_INVALID_PARAM);

    std::string deviceId {};
    NAPI_NL_CALL_RETURN(NapiParseString(env, argv[PARAM0], deviceId));
    if (!IsValidAddress(deviceId)) {
        HILOGE("Invalid deviceId");
        return NlErrCode::NL_ERR_INVALID_ADDRESS;
    }

    return NlErrCode::NL_NO_ERROR;
}

napi_value NapiNearlinkSsapClient::CreateClient(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {0};
    auto status = CheckCreateClientParams(env, info, argv, &argc);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == NlErrCode::NL_NO_ERROR, status);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    napi_value constructor = nullptr;
    NAPI_NL_ASSERT_RETURN_UNDEF(env,
        napi_get_reference_value(env, NapiNearlinkSsapClient::consRef_, &constructor) == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value result;
    NAPI_NL_ASSERT_RETURN_UNDEF(env, napi_new_instance(env, constructor, argc, argv, &result) == napi_ok,
        NL_ERR_INVALID_PARAM);

    return result;
}

void NapiNearlinkSsapClient::DefineSsapClientJSClass(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("connect", Connect),
        DECLARE_NAPI_FUNCTION("disconnect", Disconnect),
        DECLARE_NAPI_FUNCTION("close", Close),
        DECLARE_NAPI_FUNCTION("getServices", GetServices),
        DECLARE_NAPI_FUNCTION("getServicesByUuid", GetServicesByUuid),
        DECLARE_NAPI_FUNCTION("onPropertyChange", OnPropertyChange),
        DECLARE_NAPI_FUNCTION("offPropertyChange", OffPropertyChange),
        DECLARE_NAPI_FUNCTION("onConnectionStateChange", OnConnectionStateChange),
        DECLARE_NAPI_FUNCTION("offConnectionStateChange", OffConnectionStateChange),
        DECLARE_NAPI_FUNCTION("onMtuChange", OnMtuChange),
        DECLARE_NAPI_FUNCTION("offMtuChange", OffMtuChange),
        DECLARE_NAPI_FUNCTION("onEventNotify", OnEventNotify),
        DECLARE_NAPI_FUNCTION("offEventNotify", OffEventNotify),
        DECLARE_NAPI_FUNCTION("requestMtuSize", RequestMtuSize),
        DECLARE_NAPI_FUNCTION("readProperty", ReadProperty),
        DECLARE_NAPI_FUNCTION("callMethod", CallMethod),
        DECLARE_NAPI_FUNCTION("readPropertyByUuid", ReadPropertyByUuid),
        DECLARE_NAPI_FUNCTION("writeProperty", WriteProperty),
        DECLARE_NAPI_FUNCTION("readDescriptor", ReadDescriptor),
        DECLARE_NAPI_FUNCTION("writeDescriptor", WriteDescriptor),
        DECLARE_NAPI_FUNCTION("setPropertyNotification", SetPropertyNotification),
        DECLARE_NAPI_FUNCTION("setPropertyIndication", SetPropertyIndication),
    };

    napi_value constructor = nullptr;
    napi_define_class(env, "Client", NAPI_AUTO_LENGTH, SsapClientConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    napi_create_reference(env, constructor, 1, &consRef_);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createClient", CreateClient),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_value NapiNearlinkSsapClient::SsapClientConstructor(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    napi_value thisVar = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};

    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    std::string deviceId;
    NapiParseString(env, argv[PARAM0], deviceId);
    NapiNearlinkSsapClient *ssapClient = new (std::nothrow) NapiNearlinkSsapClient(deviceId);
    if (ssapClient == nullptr) {
        HILOGE("ssapClient is nullptr");
        return nullptr;
    }

    auto finalize = [](napi_env env, void* data, void* hint) {
        NapiNearlinkSsapClient* client = static_cast<NapiNearlinkSsapClient*>(data);
        if (client != nullptr) {
            delete client;
            client = nullptr;
        }
    };

    napi_status status = napi_wrap(env, thisVar, ssapClient, finalize, nullptr, nullptr);
    if (status != napi_ok) {
        HILOGE("napi_wrap failed");
        delete ssapClient;
        ssapClient = nullptr;
        return nullptr;
    }
    HILOGI("Constructor ssapClient success.");
    return thisVar;
}


napi_value NapiNearlinkSsapClient::OnPropertyChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OffPropertyChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OnConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OffConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OnMtuChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OffMtuChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OnEventNotify(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapClient::OffEventNotify(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapClient = NapiGetSsapClient(env, info);
    if (ssapClient && ssapClient->GetCallback()) {
        auto status = ssapClient->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

static napi_status CheckSsapClientNoArgc(napi_env env, napi_callback_info info, NapiNearlinkSsapClient **outSsapClient)
{
    size_t argc = ARGS_SIZE_ZERO;
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_ZERO, "No need arguments.", napi_invalid_arg);
    NapiNearlinkSsapClient *ssapClient = NapiGetSsapClient(env, thisVar);
    NAPI_NL_RETURN_IF(ssapClient == nullptr || outSsapClient == nullptr, "ssapClient is nullptr.", napi_invalid_arg);

    *outSsapClient = ssapClient;
    return napi_ok;
}

static napi_status CheckSsapClientArgc(napi_env env, napi_callback_info info,
    NapiNearlinkSsapClient **outSsapClient, napi_value *argv, size_t *argc)
{
    size_t argvLength = *argc;
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, argc, argv, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argvLength != (*argc), "Invalid arguments.", napi_invalid_arg);
    NapiNearlinkSsapClient *ssapClient = NapiGetSsapClient(env, thisVar);
    NAPI_NL_RETURN_IF(ssapClient == nullptr || outSsapClient == nullptr, "ssapClient is nullptr.", napi_invalid_arg);

    *outSsapClient = ssapClient;
    return napi_ok;
}

napi_value NapiNearlinkSsapClient::Connect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("Client.connect", beginTime, true);
    NapiNearlinkSsapClient *ssapClient = nullptr;
    auto status = CheckSsapClientNoArgc(env, info, &ssapClient);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    NAPI_HA_EXCEP_REPORT(
        ssapClient->GetCallback() != nullptr, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ssapClient->GetCallback() != nullptr, NL_ERR_INVALID_PARAM);

    std::shared_ptr<SsapClient> client = ssapClient->GetClient();
    NAPI_HA_EXCEP_REPORT(client != nullptr, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, client != nullptr, NL_ERR_INTERNAL_ERROR);

    auto func = [ssapClient, client]() {
        int ret = client->Connect(ssapClient->GetCallback());
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::Disconnect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("Client.disconnect", beginTime, true);
    NapiNearlinkSsapClient* ssapClient = nullptr;
    auto status = CheckSsapClientNoArgc(env, info, &ssapClient);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    std::shared_ptr<SsapClient> client = ssapClient->GetClient();
    NAPI_HA_EXCEP_REPORT(client != nullptr, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, client != nullptr, NL_ERR_INTERNAL_ERROR);

    auto func = [client]() {
        int ret = client->Disconnect();
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::GetServices(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("GetServices", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    auto status = CheckSsapClientNoArgc(env, info, &client);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);

    auto func = [ssapClient]() {
        HILOGI("start discover services");
        int ret = ssapClient->FindStructure();
        if (ret != NL_NO_ERROR) {
            return NapiAsyncWorkRet(ret, nullptr);
        }
        std::vector<SsapService> services;
        NlErrCode err = ssapClient->GetService(services);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err, nullptr);
        }
        auto object = std::make_shared<NapiNativeSsapServiceArray>(services);
        return NapiAsyncWorkRet(err, object);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::GetServicesByUuid(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("GetServicesByUuid", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapService napiSsapService;
    auto parseStatus = NapiParseSsapService(env, argv[PARAM0], napiSsapService);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);

    auto func = [ssapClient, napiSsapService]() {
        HILOGI("start discover services by uuid");
        int ret = ssapClient->FindStructureByUuid(napiSsapService.uuid_);
        if (ret != NL_NO_ERROR) {
            return NapiAsyncWorkRet(ret, nullptr);
        }
        std::shared_ptr<SsapService> service = ssapClient->GetService(napiSsapService.uuid_);
        if (!service) {
            HILOGE("service not found");
            return NapiAsyncWorkRet(NL_ERR_INVALID_PARAM, nullptr);
        }
        std::vector<SsapService> services;
        services.push_back(*service);
        auto object = std::make_shared<NapiNativeSsapServiceArray>(services);
        return NapiAsyncWorkRet(NL_NO_ERROR, object);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::Close(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkSsapClient* ssapClient = nullptr;
    auto status = CheckSsapClientNoArgc(env, info, &ssapClient);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    std::shared_ptr<SsapClient> client = ssapClient->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, client != nullptr, NL_ERR_INTERNAL_ERROR);

    int ret = client->Close();
    HILOGI("ret: %{public}d", ret);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ret == NL_NO_ERROR, ret);
    return NapiGetBooleanTrue(env);
}

napi_value NapiNearlinkSsapClient::RequestMtuSize(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("RequestMtuSize", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    int32_t mtu = 0;
    auto parseStatus = NapiParseInt32(env, argv[PARAM0], mtu);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == napi_ok, NL_ERR_INVALID_PARAM);
    if (mtu < MIN_MTU_SIZE) {
        HILOGI("covert requested MTU %{public}d to %{public}d", mtu, MIN_MTU_SIZE);
        mtu = MIN_MTU_SIZE;
    } else if (mtu > MAX_MTU_SIZE) {
        HILOGI("covert requested MTU %{public}d to %{public}d", mtu, MAX_MTU_SIZE);
        mtu = MAX_MTU_SIZE;
    }

    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);

    auto func = [ssapClient, mtu]() {
        NlErrCode err = ssapClient->RequestSleMtuSize(mtu);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::ReadProperty(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("ReadProperty", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapProperty napiSsapProperty;
    auto parseStatus = NapiParseSsapProperty(env, argv[PARAM0], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);

    auto func = [ssapClient, property]() mutable {
        HILOGI("start read property");
        NlErrCode err = ssapClient->ReadProperty(property);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err, nullptr);
        }
        auto object = std::make_shared<NapiNativeSsapProperty>(property);
        return NapiAsyncWorkRet(err, object);
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(NapiAsyncType::SSAP_CLIENT_READ_PROPERTY, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::CallMethod(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("CallMethod", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapMethod napiSsapMethod;
    auto parseStatus = NapiParseSsapMethod(env, argv[PARAM0], napiSsapMethod);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    HILOGI("client->GetClient");
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapMethod method(0, napiSsapMethod.methodUuid_, 0);

    method.SetParameter(napiSsapMethod.parameter_.data(), napiSsapMethod.parameter_.size());
    size_t length = 0;
    method.GetParameter(&length);

    auto func = [ssapClient, method]() mutable {
        size_t length = 0;
        method.GetParameter(&length);
        HILOGI("%{public}s", method.GetUuid().GetEncryptUuid().c_str());

        NlErrCode err = ssapClient->CallMethod(method);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err, nullptr);
        }
        auto object = std::make_shared<NapiNativeSsapMethod>(method);
        return NapiAsyncWorkRet(err, object);
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    HILOGI("CreateAsyncWork");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    HILOGI("client->GetCallback");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    HILOGI("asyncPromiseMap_.TryPush");
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(NapiAsyncType::SSAP_CLIENT_CALL_METHOD, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);

    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::ReadPropertyByUuid(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("ReadPropertyByUuid", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapProperty napiSsapProperty;
    auto parseStatus = NapiParseSsapProperty(env, argv[PARAM0], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);

    auto func = [ssapClient, property]() mutable {
        HILOGI("start read property");
        NlErrCode err = ssapClient->ReadPropertyByUuid(property);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err, nullptr);
        }
        std::vector<SsapProperty> properties;
        auto object = std::make_shared<NapiNativeSsapPropertyArray>(properties);
        return NapiAsyncWorkRet(err, object);
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_READ_PROPERTY_BY_UUID, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::WriteProperty(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("writeProperty", beginTime, true);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapProperty napiSsapProperty;
    auto parseStatus = NapiParseSsapProperty(env, argv[PARAM0], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);

    int propertyWriteType;
    parseStatus = NapiParseInt32(env, argv[PARAM1], propertyWriteType);
    NAPI_HA_EXCEP_REPORT(parseStatus == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, parseStatus == napi_ok, NL_ERR_INVALID_PARAM);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_HA_EXCEP_REPORT(ssapClient != nullptr, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);
    property.SetValue(napiSsapProperty.value_.data(), napiSsapProperty.value_.size());
    property.SetWriteType(propertyWriteType);
    auto func = [ssapClient, property]() mutable {
        HILOGI("start write property");
        NlErrCode err = ssapClient->WriteProperty(property);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_WRITE_PROPERTY, asyncWork);
    NAPI_HA_EXCEP_REPORT(success, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::ReadDescriptor(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("ReadDescriptor", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapDescriptor napiSsapDescriptor;
    auto parseStatus = NapiParseSsapDescriptor(env, argv[PARAM0], napiSsapDescriptor);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);

    NAPI_NL_ASSERT_RETURN_FALSE(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    NAPI_NL_ASSERT_RETURN_FALSE(env, NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF);

    uint16_t handle = 0;
    bool ret = ssapClient->GetHandle(napiSsapDescriptor.serviceUuid_, napiSsapDescriptor.propertyUuid_, handle);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ret == true, NL_ERR_INTERNAL_ERROR);
    SsapDescriptor descriptor(handle, napiSsapDescriptor.descriptorType_, 0);

    auto func = [ssapClient, descriptor]() mutable {
        HILOGI("start read descriptor");
        NlErrCode err = ssapClient->ReadDescriptor(descriptor);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err, nullptr);
        }
        auto object = std::make_shared<NapiNativeSsapDescriptor>(descriptor);
        return NapiAsyncWorkRet(err, object);
    };

    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_READ_DESCRIPTOR, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::WriteDescriptor(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("WriteDescriptor", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapDescriptor napiSsapDescriptor;
    auto parseStatus = NapiParseSsapDescriptor(env, argv[PARAM0], napiSsapDescriptor);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);

    NAPI_NL_ASSERT_RETURN_FALSE(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    NAPI_NL_ASSERT_RETURN_FALSE(env, NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF);

    uint16_t handle = 0;
    bool ret = ssapClient->GetHandle(napiSsapDescriptor.serviceUuid_, napiSsapDescriptor.propertyUuid_, handle);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ret == true, NL_ERR_INTERNAL_ERROR);
    SsapDescriptor descriptor(handle, napiSsapDescriptor.descriptorType_, 0);
    descriptor.SetValue(napiSsapDescriptor.descriptorValue_.data(), napiSsapDescriptor.descriptorValue_.size());

    auto func = [ssapClient, descriptor]() mutable {
        HILOGI("start write descriptor");
        NlErrCode err = ssapClient->WriteDescriptor(descriptor);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_WRITE_DESCRIPTOR, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::SetPropertyNotification(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("SetPropertyNotification", 0, false);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    NapiSsapProperty napiSsapProperty;
    auto parseStatus = NapiParseSsapProperty(env, argv[PARAM0], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    bool enable;
    parseStatus = NapiParseBoolean(env, argv[PARAM1], enable);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == napi_ok, NL_ERR_INVALID_PARAM);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);
    auto func = [ssapClient, property, enable]() mutable {
        NlErrCode err = ssapClient->SetNotifyProperty(property, enable);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_SET_PROPERTY_NOTIFY, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapClient::SetPropertyIndication(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("setPropertyIndication", beginTime, true);
    NapiNearlinkSsapClient *client = nullptr;
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {};
    auto status = CheckSsapClientArgc(env, info, &client, argv, &argc);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    NapiSsapProperty napiSsapProperty;
    auto parseStatus = NapiParseSsapProperty(env, argv[PARAM0], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == NlErrCode::NL_NO_ERROR, parseStatus);
    bool enable;
    parseStatus = NapiParseBoolean(env, argv[PARAM1], enable);
    NAPI_HA_EXCEP_REPORT(parseStatus == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, parseStatus == napi_ok, NL_ERR_INVALID_PARAM);
    std::shared_ptr<SsapClient> ssapClient = client->GetClient();
    NAPI_HA_EXCEP_REPORT(ssapClient != nullptr, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapClient != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);
    auto func = [ssapClient, property, enable]() mutable {
        NlErrCode err = ssapClient->SetIndicateProperty(property, enable);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, client->GetCallback() != nullptr, NL_ERR_INTERNAL_ERROR);
    bool success = client->GetCallback()->asyncPromiseMap_.TryPush(
        NapiAsyncType::SSAP_CLIENT_SET_PROPERTY_INDICATE, asyncWork);
    NAPI_HA_EXCEP_REPORT(success, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}
} // namespace Nearlink
} // namespace OHOS
