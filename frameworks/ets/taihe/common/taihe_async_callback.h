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

#ifndef TAIHE_ASYNC_CALLBACK_H
#define TAIHE_ASYNC_CALLBACK_H

#include <map>
#include <memory>
#include <mutex>
#include "taihe_async_work.h"
#include "taihe_native_object.h"

namespace OHOS {
namespace Nearlink {
class TaiheCallback;
class TaihePromise;
class TaiheNativeObject;

struct TaiheAsyncCallback {
    explicit TaiheAsyncCallback(ani_vm *vm);
    ~TaiheAsyncCallback();
    void CallFunction(int errCode, const std::shared_ptr<TaiheNativeObject> &object);

    ani_object GetPromiseRet(void);
    ani_vm *vm_;
    ani_resolver bindDeferred_;
    ani_object promise_;
    bool isAttach_ = false;
};

class TaiheCallback {
public:
    TaiheCallback(ani_vm *vm, ani_object callback);
    ~TaiheCallback();

    void CallFunction(int errCode, const std::shared_ptr<TaiheNativeObject> &object);

private:
    TaiheCallback(const TaiheCallback &) = delete;
    TaiheCallback &operator=(const TaiheCallback &) = delete;
    TaiheCallback(TaiheCallback &&) = delete;
    TaiheCallback &operator=(TaiheCallback &&) noexcept = delete;

    ani_vm *vm_;
    ani_ref callbackRef_;
    bool isAttach_ = false;
};

class TaihePromise {
public:
    explicit TaihePromise(ani_vm *vm);
    ~TaihePromise();

    void ResolveOrReject(int errCode, const std::shared_ptr<TaiheNativeObject> &object);
    void Resolve(ani_env *env, ani_ref resolution);
    void Reject(ani_env *env, ani_ref rejection);
    ani_object GetPromise(void) const;

private:
    ani_vm *vm_;
    ani_object promise_;
    ani_resolver bindDeferred_;
    bool isResolvedOrRejected_ = false;
    bool isAttach_ = false;
};

void TaiheCreateLocalScope(ani_env *env);
void TaiheDestroyLocalScope(ani_env *env);
ani_env *GetCurrentEnv(ani_vm *vm, bool &isValid);
}  // namespace Nearlink
}  // namespace OHOS
#endif  // TAIHE_ASYNC_CALLBACK_H
