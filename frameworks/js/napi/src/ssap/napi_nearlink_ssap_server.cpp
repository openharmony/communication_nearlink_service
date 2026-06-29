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
#include "log_util.h"
#include "napi_nearlink_ssap_server.h"
#include "nearlink_errorcode.h"
#include "nearlink_ssap_service.h"
#include "nearlink_host.h"
#include "napi_nearlink_ssap_utils.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "napi_parser_utils.h"
#include "napi_ha_manager.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint32_t OPERATION_WRITE_CLIENT_CONFIG_MASK = 0x200;
}

NearlinkSafeList<std::string> NapiNearlinkSsapServer::deviceList;
thread_local napi_ref NapiNearlinkSsapServer::consRef_ = nullptr;

static NapiNearlinkSsapServer *NapiGetSsapServer(napi_env env, napi_value thisVar)
{
    NapiNearlinkSsapServer *ssapServer = nullptr;
    auto status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&ssapServer));
    if (status != napi_ok) {
        return nullptr;
    }
    return ssapServer;
}

static NapiNearlinkSsapServer *NapiGetSsapServer(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        return nullptr;
    }
    return NapiGetSsapServer(env, thisVar);
}

napi_value NapiNearlinkSsapServer::CreateServer(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    napi_value result;
    napi_value constructor = nullptr;

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    napi_get_reference_value(env, consRef_, &constructor);
    napi_new_instance(env, constructor, 0, nullptr, &result);

    return result;
}

void NapiNearlinkSsapServer::DefineSsapServerJSClass(napi_env env, napi_value exports)
{
    DescriptorInit(env, exports);
    PermissionInit(env, exports);

    napi_property_descriptor ssapserverDesc[] = {
        DECLARE_NAPI_FUNCTION("addService", AddService),
        DECLARE_NAPI_FUNCTION("close", Close),
        DECLARE_NAPI_FUNCTION("onConnectionStateChange", OnConnectionStateChange),
        DECLARE_NAPI_FUNCTION("offConnectionStateChange", OffConnectionStateChange),
        DECLARE_NAPI_FUNCTION("onPropertyRead", OnPropertyRead),
        DECLARE_NAPI_FUNCTION("offPropertyRead", OffPropertyRead),
        DECLARE_NAPI_FUNCTION("onPropertyWrite", OnPropertyWrite),
        DECLARE_NAPI_FUNCTION("offPropertyWrite", OffPropertyWrite),
        DECLARE_NAPI_FUNCTION("onMtuChange", OnMtuChange),
        DECLARE_NAPI_FUNCTION("offMtuChange", OffMtuChange),
        DECLARE_NAPI_FUNCTION("removeService", RemoveService),
        DECLARE_NAPI_FUNCTION("notifyPropertyChanged", NotifyPropertyChanged),
        DECLARE_NAPI_FUNCTION("sendResponse", SendResponse),
        DECLARE_NAPI_FUNCTION("disconnect", Disconnect),
    };

    napi_value constructor = nullptr;
    napi_define_class(env, "Server", NAPI_AUTO_LENGTH, SsapServerConstructor, nullptr,
        sizeof(ssapserverDesc) / sizeof(ssapserverDesc[0]), ssapserverDesc, &constructor);
    napi_create_reference(env, constructor, 1, &consRef_);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createServer", CreateServer),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_value NapiNearlinkSsapServer::SsapServerConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    NapiNearlinkSsapServer* ssapServer = new (std::nothrow) NapiNearlinkSsapServer();
    if (ssapServer == nullptr) {
        HILOGE("ssapServer is nullptr");
        return nullptr;
    }

    auto finalize = [](napi_env env, void* data, void* hint) {
        NapiNearlinkSsapServer* server = static_cast<NapiNearlinkSsapServer*>(data);
        if (server != nullptr) {
            delete server;
            server = nullptr;
        }
    };

    napi_status status = napi_wrap(env, thisVar, ssapServer, finalize, nullptr, nullptr);
    if (status != napi_ok) {
        HILOGE("napi_wrap failed");
        delete ssapServer;
        ssapServer = nullptr;
        return nullptr;
    }
    HILOGI("Constructor ssapServer success.");
    return thisVar;
}

napi_value NapiNearlinkSsapServer::OnConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_SERVER_CALLBACK_CONNECT_STATE_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OffConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_SERVER_CALLBACK_CONNECT_STATE_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OnPropertyRead(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_SERVER_CALLBACK_PROPERTY_READ);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OffPropertyRead(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_SERVER_CALLBACK_PROPERTY_READ);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OnPropertyWrite(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_SERVER_CALLBACK_PROPERTY_WRITE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OffPropertyWrite(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_SERVER_CALLBACK_PROPERTY_WRITE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OnMtuChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.RegisterWithName(
            env, info, SLE_SSAP_SERVER_CALLBACK_MTU_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::OffMtuChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto ssapServer = NapiGetSsapServer(env, info);
    if (ssapServer && ssapServer->GetCallback()) {
        auto status = ssapServer->GetCallback()->eventSubscribe.DeregisterWithName(env, info,
            SLE_SSAP_SERVER_CALLBACK_MTU_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

static napi_status CheckSsapServerArgc(napi_env env, napi_callback_info info,
    NapiNearlinkSsapServer **outSsapServer, napi_value *argv, size_t *argc)
{
    size_t argvLength = *argc;
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, argc, argv, &thisVar, nullptr);
    NAPI_NL_RETURN_IF(getCbRes != napi_ok, "Invalid arguments.", napi_invalid_arg);

    NAPI_NL_RETURN_IF(argvLength != *argc, "Invalid arguments.", napi_invalid_arg);
    NapiNearlinkSsapServer *ssapServer = NapiGetSsapServer(env, thisVar);
    NAPI_NL_RETURN_IF(ssapServer == nullptr || outSsapServer == nullptr, "ssapServer is nullptr.", napi_invalid_arg);

    *outSsapServer = ssapServer;
    return napi_ok;
}


static void CheckDescriptorWriteOp(NapiSsapDescriptor &descriptor, uint32_t &operationIndication,
    uint32_t &opDescriptorBitMask)
{
    if (descriptor.writeable_) {
        operationIndication = opDescriptorBitMask | operationIndication;
    }
    opDescriptorBitMask = opDescriptorBitMask << 1;
}

static int32_t CheckSsapsAddService(napi_env env, napi_callback_info info, std::shared_ptr<SsapServer> &outServer,
    std::unique_ptr<SsapService> &outService)
{
    HILOGI("check service start");
    const int mDes = 24;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_ONE, "Requires 1 arguments.", NlErrCode::NL_ERR_INVALID_PARAM);
    NapiSsapService napiSsapService;
    int32_t parseRet = NapiParseSsapService(env, argv[PARAM0], napiSsapService);
    if (parseRet != NlErrCode::NL_NO_ERROR) {
        return parseRet;
    }
    if (napiSsapService.properties_.empty()) {
        return NlErrCode::NL_ERR_EMPTY_ARRAY;
    }
    NapiNearlinkSsapServer *ssapServer = NapiGetSsapServer(env, thisVar);
    NAPI_NL_RETURN_IF(ssapServer == nullptr, "ssapServer is nullptr.", NlErrCode::NL_ERR_INVALID_PARAM);
    outServer = ssapServer->GetServer();
    outService = std::make_unique<SsapService>(napiSsapService.uuid_, SsapServiceType::VENDOR_PROMARY);
    for (auto &napiProperty : napiSsapService.properties_) {
        uint32_t propOperationIndication = napiProperty.operation_;
        propOperationIndication |= OPERATION_WRITE_CLIENT_CONFIG_MASK; // 允许写cpcd操作
        SsapProperty property(SsapProperty::PropertyType::ENTRY_TYPE_VENDOR_PROPERTY,
            napiProperty.propertyUuid_, propOperationIndication, 0);
        property.SetValue(napiProperty.value_.data(), napiProperty.value_.size());
        NAPI_NL_RETURN_IF(napiProperty.descriptors_.size() > mDes, "num of descriptor out of range",
            NlErrCode::NL_ERR_INVALID_PARAM);
        uint32_t opDescriptorBitMask = 256;
        for (auto &napiDescriptor : napiProperty.descriptors_) {
            HILOGI("check descriptor");
            CheckDescriptorWriteOp(napiDescriptor, propOperationIndication, opDescriptorBitMask);
            SsapDescriptor descriptor(napiDescriptor.descriptorType_, 0);
            descriptor.SetServiceUuid(napiSsapService.uuid_);
            descriptor.SetValue(napiDescriptor.descriptorValue_.data(), napiDescriptor.descriptorValue_.size());
            property.AddDescriptor(descriptor);
        }
        property.SetOperationIndication(propOperationIndication);
        outService->AddProperty(property);
    }
    return NlErrCode::NL_NO_ERROR;
}

napi_value NapiNearlinkSsapServer::AddService(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<SsapServer> server {nullptr};
    std::unique_ptr<SsapService> ssapService {nullptr};
    auto status = CheckSsapsAddService(env, info, server, ssapService);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == NlErrCode::NL_NO_ERROR, status);
    NAPI_NL_ASSERT_RETURN_FALSE(env, server != nullptr, NlErrCode::NL_ERR_INTERNAL_ERROR);

    int ret = server->AddService(*ssapService);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ret == NL_NO_ERROR, ret);
    return NapiGetBooleanTrue(env);
}

static napi_status CheckSsapServerClose(napi_env env, napi_callback_info info, std::shared_ptr<SsapServer> &outServer)
{
    size_t argc = 0;
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, &thisVar, NULL));
    NAPI_NL_RETURN_IF(argc > 0, "no needed arguments.", napi_invalid_arg);
    NapiNearlinkSsapServer *ssapServer = NapiGetSsapServer(env, thisVar);
    NAPI_NL_RETURN_IF(ssapServer == nullptr, "ssapServer is nullptr.", napi_invalid_arg);

    outServer = ssapServer->GetServer();
    return napi_ok;
}

napi_value NapiNearlinkSsapServer::Close(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<SsapServer> server {nullptr};
    auto status = CheckSsapServerClose(env, info, server);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, (status == napi_ok && server != nullptr), NL_ERR_INVALID_PARAM);

    int ret = server->Close();
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ret == NL_NO_ERROR, ret);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::DescriptorInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value descriptorTypeObj = nullptr;
    napi_create_object(env, &descriptorTypeObj);
    SetNamedPropertyByInteger(
        env, descriptorTypeObj, static_cast<int32_t>(NapiSsapDescriptorType::PROPERTY), "PROPERTY");
    SetNamedPropertyByInteger(
        env, descriptorTypeObj, static_cast<int32_t>(NapiSsapDescriptorType::CLIENT_PROPERTY_CONFIG),
        "CLIENT_PROPERTY_CONFIG");
    SetNamedPropertyByInteger(
        env, descriptorTypeObj, static_cast<int32_t>(NapiSsapDescriptorType::SERVER_PROPERTY_CONFIG),
        "SERVER_PROPERTY_CONFIG");
    SetNamedPropertyByInteger(
        env, descriptorTypeObj, static_cast<int32_t>(NapiSsapDescriptorType::PROPERTY_FORMAT), "PROPERTY_FORMAT");
    SetNamedPropertyByInteger(
        env, descriptorTypeObj, static_cast<int32_t>(NapiSsapDescriptorType::TYPE_VENDOR), "TYPE_VENDOR");
    napi_value operationIndicationObj = nullptr;
    napi_create_object(env, &operationIndicationObj);
    SetNamedPropertyByInteger(
        env, operationIndicationObj, static_cast<int32_t>(NapiOperationIndication::READABLE), "READABLE");
    SetNamedPropertyByInteger(
        env, operationIndicationObj, static_cast<int32_t>(NapiOperationIndication::WRITE_NO_RESPONSE),
        "WRITE_NO_RESPONSE");
    SetNamedPropertyByInteger(
        env, operationIndicationObj, static_cast<int32_t>(NapiOperationIndication::WRITE_WITH_RESPONSE),
        "WRITE_WITH_RESPONSE");
    SetNamedPropertyByInteger(
        env, operationIndicationObj, static_cast<int32_t>(NapiOperationIndication::NOTIFY), "NOTIFY");
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("PropertyDescriptorType", descriptorTypeObj),
        DECLARE_NAPI_PROPERTY("Operation", operationIndicationObj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

napi_value NapiNearlinkSsapServer::PermissionInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value permissionObj = nullptr;
    napi_create_object(env, &permissionObj);
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_WRITE_ENCRYPTION_NEED),
        "SSAP_PERMISSION_WRITE_ENCRYPTION_NEED");
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_WRITE_AUTHENTICATION_NEED),
        "SSAP_PERMISSION_WRITE_AUTHENTICATION_NEED");
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_WRITE_AUTHORIZATION_NEED),
        "SSAP_PERMISSION_WRITE_AUTHORIZATION_NEED");
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_READ_ENCRYPTION_NEED),
        "SSAP_PERMISSION_READ_ENCRYPTION_NEED");
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_READ_AUTHENTICATION_NEED),
        "SSAP_PERMISSION_READ_AUTHENTICATION_NEED");
    SetNamedPropertyByInteger(
        env, permissionObj, static_cast<int32_t>(NapiPermission::SSAP_PERMISSION_READ_AUTHORIZATION_NEED),
        "SSAP_PERMISSION_READ_AUTHORIZATION_NEED");

    napi_value propertyWriteTypeObj = nullptr;
    napi_create_object(env, &propertyWriteTypeObj);
    SetNamedPropertyByInteger(env, propertyWriteTypeObj,
        static_cast<int32_t>(PropertyWriteType::WRITE), "WRITE");
    SetNamedPropertyByInteger(env, propertyWriteTypeObj,
        static_cast<int32_t>(PropertyWriteType::WRITE_NO_RESPONSE), "WRITE_NO_RESPONSE");
    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("Permission", permissionObj),
        DECLARE_NAPI_PROPERTY("PropertyWriteType", propertyWriteTypeObj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

napi_value NapiNearlinkSsapServer::RemoveService(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkSsapServer *ssapServer = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckSsapServerArgc(env, info, &ssapServer, argv, &argc) == napi_ok,
        NL_ERR_INVALID_PARAM);

    std::string str {};
    auto parseStatus = NapiParseUuid(env, argv[PARAM0], str);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, parseStatus == napi_ok, parseStatus);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, !CheckBaseUuid(str), NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);

    UUID uuid = UUID::FromString(str);
    std::shared_ptr<SsapServer> server = ssapServer->GetServer();
    NAPI_NL_ASSERT_RETURN_FALSE(env, server != nullptr, NL_ERR_INTERNAL_ERROR);
    SsapService ssapService(uuid, SsapServiceType::VENDOR_PROMARY);
    NlErrCode err = server->RemoveSsapService(ssapService);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, err == NL_NO_ERROR, err);

    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::NotifyPropertyChanged(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("NotifyPropertyChanged", 0, false);
    NapiNearlinkSsapServer *server = nullptr;
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckSsapServerArgc(env, info, &server, argv, &argc) == napi_ok,
        NL_ERR_INVALID_PARAM);

    std::string sleAddress {};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NapiParseString(env, argv[PARAM0], sleAddress) == napi_ok, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, IsValidAddress(sleAddress), NlErrCode::NL_ERR_INVALID_ADDRESS);
    NapiSsapProperty napiSsapProperty;
    auto propParseStatus = NapiParseSsapProperty(env, argv[PARAM1], napiSsapProperty);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, propParseStatus == NlErrCode::NL_NO_ERROR, propParseStatus);

    auto func = [ssapServer = server->GetServer(), sleAddress, napiSsapProperty]() {
        if (ssapServer == nullptr) {
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        SsapProperty property(0, napiSsapProperty.propertyUuid_, napiSsapProperty.operation_, 0);
        Nearlink::NearlinkRemoteDevice device(sleAddress, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        property.SetValue(&napiSsapProperty.value_[0], napiSsapProperty.value_.size());
        NlErrCode err = ssapServer->NotifyPropertyChanged(device, property, false);
        if (err != NL_NO_ERROR) {
            return NapiAsyncWorkRet(err);
        }
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkSsapServer::SendResponse(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    NapiNearlinkSsapServer *server = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    NAPI_HA_EXCEP_REPORT(CheckSsapServerArgc(env, info, &server, argv, &argc) == napi_ok,
        "sendResponse", beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckSsapServerArgc(env, info, &server, argv, &argc) == napi_ok,
        NL_ERR_INVALID_PARAM);

    NapiSsapServerResponse napiSsapServerResponse;
    NAPI_HA_EXCEP_REPORT(NapiParseSsapServerResponse(env, argv[PARAM0], napiSsapServerResponse) == napi_ok,
        "sendResponse", beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NapiParseSsapServerResponse(env, argv[PARAM0], napiSsapServerResponse) == napi_ok,
        NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, IsValidAddress(napiSsapServerResponse.address_),
        NlErrCode::NL_ERR_INVALID_ADDRESS);

    std::shared_ptr<SsapServer> ssapServer = server->GetServer();
    NAPI_HA_EXCEP_REPORT(ssapServer != nullptr, "sendResponse", beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ssapServer != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = ssapServer->AuthorizeResponse(napiSsapServerResponse.requestId_, true);
    NAPI_HA_EXCEP_REPORT(err == NL_NO_ERROR, "sendResponse", beginTime, err);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, err == NL_NO_ERROR, err);
    NapiHaManager::GetInstance().ReportEvent("sendResponse", beginTime, NL_NO_ERROR);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkSsapServer::Disconnect(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("SsapServer.Disconnect", 0, false);
    NapiNearlinkSsapServer *server = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {};
    NAPI_NL_ASSERT_RETURN_UNDEF(env, CheckSsapServerArgc(env, info, &server, argv, &argc) == napi_ok,
        NL_ERR_INVALID_PARAM);

    std::vector<std::string> sleAddressVec;
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NapiParseArray(env, argv[PARAM0], sleAddressVec) == napi_ok, NL_ERR_INVALID_PARAM);
    for (auto it = sleAddressVec.begin(); it != sleAddressVec.end(); it++) {
        NAPI_NL_ASSERT_RETURN_UNDEF(env, IsValidAddress(*it), NlErrCode::NL_ERR_INVALID_ADDRESS);
    }

    auto func = [ssapServer = server->GetServer(), sleAddressVec]() {
        if (ssapServer == nullptr) {
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err;
        for (auto it = sleAddressVec.begin(); it != sleAddressVec.end(); it++) {
            Nearlink::NearlinkRemoteDevice device(*it, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
            err = ssapServer->CancelConnection(device);
            if (err != NL_NO_ERROR) {
                return NapiAsyncWorkRet(err);
            }
        }
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}
} // namespace Nearlink
} // namespace OHOS
