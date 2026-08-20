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

#ifndef NAPI_ASYNC_WORK_H
#define NAPI_ASYNC_WORK_H

#include <memory>
#include <mutex>
#include "napi_nearlink_utils.h"
#include "napi_native_object.h"

namespace OHOS {
namespace Nearlink {
struct NapiAsyncCallback;

enum NapiAsyncType : int {
    SSAP_CLIENT_READ_CHARACTER,
    SSAP_CLIENT_READ_REMOTE_RSSI_VALUE,
    SSAP_CLIENT_READ_DESCRIPTOR,
    SSAP_CLIENT_READ_PROPERTY,
    SSAP_CLIENT_READ_PROPERTY_BY_UUID,
    SSAP_CLIENT_WRITE_CHARACTER,
    SSAP_CLIENT_WRITE_DESCRIPTOR,
    SSAP_CLIENT_WRITE_PROPERTY,
    SSAP_CLIENT_ENABLE_CHARACTER_CHANGED,
    SSAP_CLIENT_SET_PROPERTY_NOTIFY,
    SSAP_CLIENT_SET_PROPERTY_INDICATE,
    SSAP_SERVER_NOTIFY_CHARACTERISTIC,
    GET_ADVERTISING_HANDLE,
    GET_SCANNER_RESULT,
    SSAP_CLIENT_CALL_METHOD,
    GET_REMOTE_DEVICE_RSSI
};

struct AsyncWorkContext {
    std::string apiName;
    int64_t beginTime = 0;
    bool isNeedHaReport = false;
    AsyncWorkContext(const std::string name, int64_t time, bool needHaReport) :
        apiName(name), beginTime(time), isNeedHaReport(needHaReport) {};
};

static constexpr bool ASYNC_WORK_NEED_CALLBACK = true;
static constexpr bool ASYNC_WORK_NO_NEED_CALLBACK = false;

struct NapiAsyncWorkRet {
    NapiAsyncWorkRet(int errCode) : errCode(errCode) {}
    NapiAsyncWorkRet(int errCode, std::shared_ptr<NapiNativeObject> object)
        : errCode(errCode), object(std::move(object)) {}

    int errCode = -1;
    std::shared_ptr<NapiNativeObject> object = nullptr;
};

class NapiAsyncWork : public std::enable_shared_from_this<NapiAsyncWork> {
public:
    NapiAsyncWork(napi_env env, std::function<NapiAsyncWorkRet(void)> func,
        std::shared_ptr<NapiAsyncCallback> asyncCallback, std::shared_ptr<AsyncWorkContext> asyncWorkContext,
        bool needCallback = false): env_(env), func_(func),
        napiAsyncCallback_(asyncCallback), needCallback_(needCallback), context_(asyncWorkContext) {}

    ~NapiAsyncWork() = default;

    void Run();
    void CallFunction(int errorCode, std::shared_ptr<NapiNativeObject> object);
    std::shared_ptr<AsyncWorkContext> GetAsyncWorkContext() const;
    napi_value GetRet(void);

    struct Info {
        void Execute(void);
        void Complete(void);
        int errCode = -1;
        bool needCallback = false;
        napi_async_work asyncWork;
        std::shared_ptr<NapiNativeObject> object;
        std::shared_ptr<NapiAsyncWork> napiAsyncWork = nullptr;
    };

private:
    friend class NapiAsyncWorkMap;

    void TimeoutCallback(void);

    napi_env env_;
    uint32_t timerId_ = 0;  // Is used to reference a timer.
    std::function<NapiAsyncWorkRet(void)> func_;
    std::shared_ptr<NapiAsyncCallback> napiAsyncCallback_ = nullptr;
    std::atomic_bool needCallback_ = false; // Indicates whether an asynchronous work needs to wait for callback.
    std::atomic_bool triggered_ = false; // Indicates whether the asynchronous callback is called.
    std::shared_ptr<AsyncWorkContext> context_ = nullptr;
};

class NapiAsyncWorkFactory {
public:
    static std::shared_ptr<NapiAsyncWork> CreateAsyncWork(napi_env env, napi_callback_info info,
        std::function<NapiAsyncWorkRet(void)> asyncWork, std::shared_ptr<AsyncWorkContext> asyncWorkContext,
        bool needCallback = ASYNC_WORK_NO_NEED_CALLBACK);
};

class NapiAsyncWorkMap {
public:
    bool TryPush(NapiAsyncType type, std::shared_ptr<NapiAsyncWork> asyncWork);
    void Erase(NapiAsyncType type);
    std::shared_ptr<NapiAsyncWork> Get(NapiAsyncType type);

private:
    mutable std::mutex mutex_ {};
    std::map<int, std::shared_ptr<NapiAsyncWork>> map_ {};
};

void AsyncWorkCallFunction(NapiAsyncWorkMap &map, NapiAsyncType type, std::shared_ptr<NapiNativeObject> nativeObject,
    int status);

}  // namespace Nearlink
}  // namespace OHOS
#endif // NAPI_ASYNC_WORK_H