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

#ifndef ANI_ASYNC_WORK_H
#define ANI_ASYNC_WORK_H

#include <map>
#include <memory>
#include <mutex>

#include "taihe_native_object.h"

namespace OHOS {
namespace Nearlink {
struct TaiheAsyncCallback;

enum TaiheAsyncType : int {
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

struct TaiheAsyncWorkRet {
    explicit TaiheAsyncWorkRet(int errCode) : errCode(errCode) {}
    TaiheAsyncWorkRet(int errCode, std::shared_ptr<TaiheNativeObject> object)
        : errCode(errCode), object(object) {}

    int errCode = -1;
    std::shared_ptr<TaiheNativeObject> object = nullptr;
};

class TaiheAsyncWork : public std::enable_shared_from_this<TaiheAsyncWork> {
public:
    TaiheAsyncWork(ani_env *env, std::function<TaiheAsyncWorkRet(void)> func,
        std::shared_ptr<TaiheAsyncCallback> asyncCallback):
        env_(env), func_(func), taiheAsyncCallback_(asyncCallback) {}
    ~TaiheAsyncWork() = default;

    void Run();
    void CallFunction(int errCode, std::shared_ptr<TaiheNativeObject> object);
    ani_object GetRet(void);

    struct Info {
        void Execute(void);
        void Complete(void);

        int errCode = -1;
        std::shared_ptr<TaiheNativeObject> object;
        std::shared_ptr<TaiheAsyncWork> taiheAsyncWork = nullptr;
    };

private:
    friend class TaiheAsyncWorkMap;

    void TimeoutCallback(void);

    ani_env *env_ = nullptr;
    uint32_t timerId_ = 0;  // Is used to reference a timer.
    std::function<TaiheAsyncWorkRet(void)> func_;
    std::shared_ptr<TaiheAsyncCallback> taiheAsyncCallback_ = nullptr;
    std::atomic_bool triggered_ = false; // Indicates whether the asynchronous callback is called.
};

class TaiheAsyncWorkFactory {
public:
    static std::shared_ptr<TaiheAsyncWork> CreateAsyncWork(ani_env *env, ani_object info,
        std::function<TaiheAsyncWorkRet(void)> asyncWork);
};

class TaiheAsyncWorkMap {
public:
    bool TryPush(TaiheAsyncType type, std::shared_ptr<TaiheAsyncWork> asyncWork);
    void Erase(TaiheAsyncType type);
    std::shared_ptr<TaiheAsyncWork> Get(TaiheAsyncType type);
private:
    mutable std::mutex mutex_ {};
    std::map<int, std::shared_ptr<TaiheAsyncWork>> map_ {};
};

void AsyncWorkCallFunction(TaiheAsyncWorkMap &map, TaiheAsyncType type, std::shared_ptr<TaiheNativeObject> nativeObject,
    int status);
std::shared_ptr<TaiheAsyncCallback> TaiheParseAsyncCallback(ani_env *env, ani_object info);
}  // namespace Nearlink
}  // namespace OHOS

#endif  // ANI_ASYNC_WORK_H
