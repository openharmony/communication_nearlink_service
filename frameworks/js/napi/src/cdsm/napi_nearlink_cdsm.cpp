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
#include "napi_nearlink_cdsm.h"

#include <unistd.h>
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "napi_async_callback.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "napi_parser_utils.h"
#include "napi_ha_manager.h"
#include "nearlink_remote_device.h"
#include "napi_nearlink_cdsm_callback.h"

namespace OHOS {
namespace Nearlink {

thread_local napi_ref NapiNearlinkCdsm::consRef_ = nullptr;

napi_value NapiNativeCdsmInfo::ToNapiValue(napi_env env) const
{
    HILOGI("enter");
    napi_value napiCdsObj = nullptr;
    napi_create_object(env, &napiCdsObj);

    napi_value napiMembers;
    napi_create_array(env, &napiMembers);
    size_t idx = 0;
    for (auto& member : cdsInfo_.GetCdsMemberList()) {
        NapiNativeCdsmMemberInfo nativeMember(member);
        napi_value napiMemberObj = nativeMember.ToNapiValue(env);
        napi_set_element(env, napiMembers, idx, napiMemberObj);
        idx++;
    }
    napi_set_named_property(env, napiCdsObj, "members", napiMembers);

    return napiCdsObj;
}

napi_value NapiNativeCdsmMemberInfo::ToNapiValue(napi_env env) const
{
    HILOGI("enter");
    napi_value napiMemberObj;
    napi_create_object(env, &napiMemberObj);

    napi_value address;
    napi_create_string_utf8(env, cdsMemberInfo_.GetDeviceAddr().c_str(), NAPI_AUTO_LENGTH, &address);
    napi_set_named_property(env, napiMemberObj, "address", address);

    napi_value state;
    napi_create_int32(env, cdsMemberInfo_.GetState(), &state);
    napi_set_named_property(env, napiMemberObj, "state", state);

    return napiMemberObj;
}

std::shared_ptr<NearlinkCdsmClient> NapiNearlinkCdsm::GetCdsmClient() const
{
    return cdsmClient_;
}

std::shared_ptr<NapiNearlinkCdsmClientCallback> NapiNearlinkCdsm::GetCdsmCallback() const
{
    return cdsmCallback_;
}

NapiNearlinkCdsm::NapiNearlinkCdsm(const std::string &address)
{
    HILOGI("enter");
    NearlinkRemoteDevice device(address, 1);
    cdsmCallback_ = std::make_shared<NapiNearlinkCdsmClientCallback>();
    auto callback = std::dynamic_pointer_cast<NearlinkCdsmClientCallback>(cdsmCallback_);
    cdsmClient_ = NearlinkCdsmClient::CreateNearlinkCdsmClient(device, callback);
}

void NapiNearlinkCdsm::DefineCdsmJSClass(napi_env env, napi_value exports)
{
    CdsmPropertyValueInit(env, exports);
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getCdsmInfo", GetCdsmInfo),
        DECLARE_NAPI_FUNCTION("onCdsmInfoChange", OnCdsmInfoChange),
        DECLARE_NAPI_FUNCTION("offCdsmInfoChange", OffCdsmInfoChange),
    };

    napi_value constructor = nullptr;
    napi_define_class(env, "Cdsm", NAPI_AUTO_LENGTH, CdsmConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    napi_create_reference(env, constructor, 1, &consRef_);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createCdsmClient", CreateCdsmClient),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_value NapiNearlinkCdsm::CdsmPropertyValueInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value connectionStateObj = CdsmTsConnectionStateInit(env);
    napi_property_descriptor exportProperty[] = {
        DECLARE_NAPI_PROPERTY("CdsmConnectionState", connectionStateObj),
    };
    napi_define_properties(env, exports, sizeof(exportProperty) / sizeof(*exportProperty), exportProperty);
    HILOGD("end");
    return exports;
}

napi_value NapiNearlinkCdsm::CdsmTsConnectionStateInit(napi_env env)
{
    HILOGD("enter");
    napi_value connectionState = nullptr;
    napi_create_object(env, &connectionState);
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(CdsmConnectionState::DISCONNECTED), "DISCONNECTED");
    SetNamedPropertyByInteger(env, connectionState,
        static_cast<int>(CdsmConnectionState::CONNECTED), "CONNECTED");
    return connectionState;
}

napi_value NapiNearlinkCdsm::CdsmConstructor(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    napi_value thisVar = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};

    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    std::string deviceAddr;
    NapiParseString(env, argv[PARAM0], deviceAddr);
    auto nearlinkCdsm = new (std::nothrow) NapiNearlinkCdsm(deviceAddr);
    if (nearlinkCdsm == nullptr) {
        HILOGE("nearlinkCdsm is nullptr");
        return nullptr;
    }

    auto finalize = [](napi_env env, void* data, void* hint) {
        auto cdsm = static_cast<NapiNearlinkCdsm*>(data);
        if (cdsm != nullptr) {
            delete cdsm;
            cdsm = nullptr;
        }
    };

    napi_status status = napi_wrap(env, thisVar, nearlinkCdsm, finalize, nullptr, nullptr);
    if (status != napi_ok) {
        HILOGE("napi_wrap failed");
        delete nearlinkCdsm;
        nearlinkCdsm = nullptr;
        return nullptr;
    }
    HILOGI("Constructor nearlink cdsm success.");
    return thisVar;
}

static napi_status CheckCreateCdsmParams(napi_env env, napi_callback_info info, napi_value *argv, size_t *argc)
{
    size_t expectedArgsCount = ARGS_SIZE_ONE;

    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, argc, argv, nullptr, nullptr));
    NAPI_NL_RETURN_IF((*argc) != expectedArgsCount, "expect 1 args", napi_invalid_arg);

    // check device addr
    std::string deviceAddr {};
    NAPI_NL_CALL_RETURN(NapiParseString(env, argv[PARAM0], deviceAddr));
    if (!IsValidAddress(deviceAddr)) {
        HILOGE("Invalid deviceId");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
        return napi_invalid_arg;
    }

    return napi_ok;
}

napi_value NapiNearlinkCdsm::CreateCdsmClient(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {0};
    auto status = CheckCreateCdsmParams(env, info, argv, &argc);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    bool checkCdsmSupport = NearlinkHost::GetInstance().IsNearlinkAudioSupport();
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkCdsmSupport, NL_ERR_CDSM_NOT_SUPPORT);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    napi_value constructor = nullptr;
    NAPI_NL_ASSERT_RETURN_UNDEF(env,
        napi_get_reference_value(env, NapiNearlinkCdsm::consRef_, &constructor) == napi_ok, NL_ERR_INVALID_PARAM);

    napi_value result;
    NAPI_NL_ASSERT_RETURN_UNDEF(env, napi_new_instance(env, constructor, argc, argv, &result) == napi_ok,
        NL_ERR_INVALID_PARAM);

    return result;
}

static NapiNearlinkCdsm *NapiGetNearlinkCdsm(napi_env env, napi_value thisVar)
{
    NapiNearlinkCdsm* nearlinkCdsm = nullptr;
    const auto status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&nearlinkCdsm));
    if (status != napi_ok) {
        return nullptr;
    }
    return nearlinkCdsm;
}

static NapiNearlinkCdsm *NapiGetNearlinkCdsm(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    if (napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr) != napi_ok) {
        return nullptr;
    }
    return NapiGetNearlinkCdsm(env, thisVar);
}

napi_value NapiNearlinkCdsm::OnCdsmInfoChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    const auto cdsm = NapiGetNearlinkCdsm(env, info);
    if (cdsm && cdsm->GetCdsmCallback()) {
        const auto status = cdsm->GetCdsmCallback()->eventSubscribe.RegisterWithName(
            env, info, NEARLINK_CALLBACK_CDSM_INFO_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkCdsm::OffCdsmInfoChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    const auto cdsm = NapiGetNearlinkCdsm(env, info);
    if (cdsm && cdsm->GetCdsmCallback()) {
        const auto status = cdsm->GetCdsmCallback()->eventSubscribe.DeregisterWithName(env, info,
            NEARLINK_CALLBACK_CDSM_INFO_CHANGE);
        NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    }
    return NapiGetUndefinedRet(env);
}

static napi_status CheckCdsmNoArgc(napi_env env, napi_callback_info info, NapiNearlinkCdsm **outCdsm)
{
    size_t argc = ARGS_SIZE_ZERO;
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_ZERO, "No need arguments.", napi_invalid_arg);
    NapiNearlinkCdsm *cdsm = NapiGetNearlinkCdsm(env, thisVar);
    NAPI_NL_RETURN_IF(cdsm == nullptr || outCdsm == nullptr, "nearlink cdsm is nullptr.", napi_invalid_arg);

    *outCdsm = cdsm;
    return napi_ok;
}

napi_value NapiNearlinkCdsm::GetCdsmInfo(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkCdsm *cdsm = nullptr;
    const auto status = CheckCdsmNoArgc(env, info, &cdsm);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    std::shared_ptr<NearlinkCdsmClient> cdsmClient = cdsm->GetCdsmClient();
    NAPI_NL_ASSERT_RETURN_FALSE(env, cdsmClient != nullptr, NL_ERR_INTERNAL_ERROR);

    NearlinkCdsInfo cdsInfo;
    NlErrCode err = cdsmClient->GetCdsmInfo(cdsInfo);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);

    auto object = std::make_shared<NapiNativeCdsmInfo>(cdsInfo);
    return object->ToNapiValue(env);
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----Cdsm Init start------");

    NapiNearlinkCdsm::DefineCdsmJSClass(env, exports);
    NapiHaManager::AddProcessor();
    HILOGI("-----Cdsm Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkCdsmModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.cdsm",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkCdsmModule nm_modname:%{public}s", nearlinkCdsmModule.nm_modname);
    napi_module_register(&nearlinkCdsmModule);
}
} // namespace Nearlink
} // namespace OHOS
