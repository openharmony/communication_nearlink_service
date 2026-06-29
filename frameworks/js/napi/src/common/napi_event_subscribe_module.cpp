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

#include "napi_event_subscribe_module.h"

#include <algorithm>
#include "napi_parser_utils.h"
#include "string_ex.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace Nearlink {
static std::string ToLogString(const std::vector<std::string> &validNameVec)
{
    std::string str = "[";
    for (const auto &s : validNameVec) {
        str += s;
        str += ", ";
    }
    str += "]";
    return str;
}

NapiEventSubscribeModule::NapiEventSubscribeModule(const char *validEventName, const char *moduleName)
    : validEventNameVec_(std::vector<std::string>{validEventName}), moduleName_(moduleName)
{}
NapiEventSubscribeModule::NapiEventSubscribeModule(std::vector<std::string> validEventNameVec, const char *moduleName)
    : validEventNameVec_(validEventNameVec), moduleName_(moduleName)
{}

napi_status NapiEventSubscribeModule::Register(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_TWO, "Requires 2 arguments", napi_invalid_arg);

    std::string name {};
    NAPI_NL_CALL_RETURN(NapiParseString(env, argv[PARAM0], name));
    if (!IsValidEventName(name)) {
        HILOGE("Invalid name %{public}s, valid name is %{public}s",
            name.c_str(), ToLogString(validEventNameVec_).c_str());
        return napi_invalid_arg;
    }

    napi_value callback = argv[PARAM1];
    NAPI_NL_RETURN_IF(NapiIsFunction(env, callback) != napi_ok, "invalid arg", napi_invalid_arg);
    RegisterCallback(env, name, callback);
    return napi_ok;
}

void NapiEventSubscribeModule::DeregisterInDeque(
    napi_env &env, napi_value &callback, std::deque<std::shared_ptr<NapiCallback>> &callbackDeque)
{
    for (auto dequeIt = callbackDeque.begin(); dequeIt != callbackDeque.end();) {
        if ((*dequeIt)->Equal(env, callback)) {
            dequeIt = callbackDeque.erase(dequeIt);
            continue;
        }
        dequeIt++;
    }
}

// Attempt clear invalid napi env
void NapiEventSubscribeModule::EraseInvalidCallback(
    std::map<uint64_t, std::deque<std::shared_ptr<NapiCallback>>> &napiCallbackMap)
{
    for (auto mapit = napiCallbackMap.begin(); mapit != napiCallbackMap.end();) {
        std::deque<std::shared_ptr<NapiCallback>> &callbackDeque = mapit->second;
        for (auto dequeIt = callbackDeque.begin(); dequeIt != callbackDeque.end();) {
            if (*dequeIt == nullptr || !(*dequeIt)->IsValidNapiEnv()) {
                dequeIt = callbackDeque.erase(dequeIt);
                continue;
            }
            dequeIt++;
        }
        if (callbackDeque.empty()) {
            mapit = napiCallbackMap.erase(mapit);
            continue;
        }
        mapit++;
    }
}

napi_status NapiEventSubscribeModule::Deregister(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(
        argc != ARGS_SIZE_ONE && argc != ARGS_SIZE_TWO, "Requires 1 or 2 arguments", napi_invalid_arg);

    std::string name {};
    NAPI_NL_CALL_RETURN(NapiParseString(env, argv[PARAM0], name));
    if (!IsValidEventName(name)) {
        HILOGE("Invalid name %{public}s, valid name is %{public}s",
            name.c_str(), ToLogString(validEventNameVec_).c_str());
        return napi_invalid_arg;
    }

    if (argc == ARGS_SIZE_ONE) {
        HILOGI("Deregister all %{public}s callback in %{public}s module", name.c_str(), moduleName_.c_str());
        eventSubscribeMap_.Erase(name);
        return napi_ok;
    }
    // The argc is ARGS_SIZE_TWO
    napi_value callback = argv[PARAM1];
    NAPI_NL_RETURN_IF(NapiIsFunction(env, callback) != napi_ok, "invalid arg", napi_invalid_arg);

    DeregisterCallback(env, name, callback);
    return napi_ok;
}

bool NapiEventSubscribeModule::IsValidEventName(const std::string &eventName) const
{
    auto it = std::find(validEventNameVec_.begin(), validEventNameVec_.end(), eventName);
    return it != validEventNameVec_.end();
}

void NapiEventSubscribeModule::PublishEventInDeque(
    const std::deque<std::shared_ptr<NapiCallback>> &callbackDeque,
    const std::shared_ptr<NapiNativeObject> &nativeObject, std::string eventName)
{
    for (auto &callback : callbackDeque) {
        if (callback == nullptr) {
            continue;
        }
        auto func = [nativeObject, callback]() {
            callback->CallFunction(nativeObject);
        };
        DoInJsMainThread(callback->GetNapiEnv(), func, eventName);
    }
}

void NapiEventSubscribeModule::PublishEvent(
    std::string eventName, const std::shared_ptr<NapiNativeObject> &nativeObject)
{
    eventSubscribeMap_.Iterate([&eventName, &nativeObject, this](
        const std::string &first, std::map<uint64_t, std::deque<std::shared_ptr<NapiCallback>>> &napiCallbackMap) {
        if (eventName != first) {
            return;
        }
        for (auto napiCallback : napiCallbackMap) {
            const auto &callbackDeque = napiCallback.second;
            PublishEventInDeque(callbackDeque, nativeObject, eventName);
        }
    });
}

napi_status NapiEventSubscribeModule::RegisterWithName(napi_env env, napi_callback_info info, std::string name)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argc != ARGS_SIZE_ONE, "Requires 1 arguments", napi_invalid_arg);

    if (!IsValidEventName(name)) {
        HILOGE("Invalid name %{public}s, valid name is %{public}s",
            name.c_str(), ToLogString(validEventNameVec_).c_str());
        return napi_invalid_arg;
    }

    napi_value callback = argv[PARAM0];
    NAPI_NL_RETURN_IF(NapiIsFunction(env, callback) != napi_ok, "invalid arg", napi_invalid_arg);

    RegisterCallback(env, name, callback);
    return napi_ok;
}

napi_status NapiEventSubscribeModule::DeregisterWithName(napi_env env, napi_callback_info info, std::string name)
{
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(
        argc != ARGS_SIZE_ZERO && argc != ARGS_SIZE_ONE, "Requires 0 or 1 arguments", napi_invalid_arg);

    if (!IsValidEventName(name)) {
        HILOGE("Invalid name %{public}s, valid name is %{public}s",
            name.c_str(), ToLogString(validEventNameVec_).c_str());
        return napi_invalid_arg;
    }

    if (argc == ARGS_SIZE_ZERO) {
        HILOGI("Deregister all %{public}s callback in %{public}s module", name.c_str(), moduleName_.c_str());
        eventSubscribeMap_.Erase(name);
        return napi_ok;
    }
    // The argc is ARGS_SIZE_ONE
    napi_value callback = argv[PARAM0];
    NAPI_NL_RETURN_IF(NapiIsFunction(env, callback) != napi_ok, "invalid arg", napi_invalid_arg);

    DeregisterCallback(env, name, callback);
    return napi_ok;
}

void NapiEventSubscribeModule::RegisterCallback(napi_env env, std::string name, napi_value callback)
{
    uint64_t tokenId = IPCSkeleton::GetSelfTokenID();

    bool findFlag = false;
    auto func = [&name, &tokenId, &callback, &env, &findFlag, this](
        const std::string &first, std::map<uint64_t, std::deque<std::shared_ptr<NapiCallback>>> &napiCallbackMap) {
        if (name != first) {
            return;
        }
        findFlag = true;
        EraseInvalidCallback(napiCallbackMap);

        auto napiCallback = std::make_shared<NapiCallback>(env, callback);
        if (napiCallbackMap[tokenId].size() >= MAX_CALLBACK_NUM) {
            napiCallbackMap[tokenId].pop_front();
        }
        napiCallbackMap[tokenId].push_back(napiCallback);
    };
    eventSubscribeMap_.Iterate(func);
    if (findFlag == false) {
        auto napiCallback = std::make_shared<NapiCallback>(env, callback);
        std::map<uint64_t, std::deque<std::shared_ptr<NapiCallback>>> napiCallbackmap;
        napiCallbackmap[tokenId].push_back(napiCallback);
        eventSubscribeMap_.Insert(name, napiCallbackmap);
    }
}

void NapiEventSubscribeModule::DeregisterCallback(napi_env env, std::string name, napi_value callback)
{
    auto func = [&name, &env, &callback, this](
        const std::string &first, std::map<uint64_t, std::deque<std::shared_ptr<NapiCallback>>> &napiCallbackMap) {
        if (name != first) {
            return;
        }
        for (auto mapIt = napiCallbackMap.begin(); mapIt != napiCallbackMap.end();) {
            std::deque<std::shared_ptr<NapiCallback>> &callbackDeque = mapIt->second;
            DeregisterInDeque(env, callback, callbackDeque);
            if (callbackDeque.size() == 0) {
                mapIt = napiCallbackMap.erase(mapIt);
                continue;
            }
            mapIt++;
        }
    };
    eventSubscribeMap_.Iterate(func);
}
}  // namespace Nearlink
}  // namespace OHOS
