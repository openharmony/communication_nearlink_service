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

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include "taihe_async_work.h"
#include "taihe_native_object.h"

namespace OHOS {
namespace Nearlink {
class TaiheNativeObject;

struct TaiheAsyncCallback {
    explicit TaiheAsyncCallback(ani_vm *vm);
    ~TaiheAsyncCallback();
    void CallFunction(int errCode, const std::shared_ptr<TaiheNativeObject> &object);

    ani_object GetPromiseRet(void);
    ani_vm *vm_ = nullptr;
    // 原子取走 deferred，保证并发（业务回调/超时/Complete 多通道）下同一 resolver 至多被一个线程 settle
    std::atomic<ani_resolver> bindDeferred_ { nullptr };
    ani_object promise_ = nullptr;
    // isAttach_ 仅由构造函数记账：当前所有构造点均在已 attach 的 JS/ANI 调用线程，GetEnv 成功、恒为 false。
    // 析构里的 DetachCurrentThread 只在「构造线程 == 析构线程」时正确，属防御性逻辑。
    // CallFunction 的 attach 使用局部变量并在结算后立即 detach，不写 isAttach_，避免析构在错误线程 detach。
    bool isAttach_ = false;
};

void TaiheCreateLocalScope(ani_env *env);
void TaiheDestroyLocalScope(ani_env *env);
ani_env *GetCurrentEnv(ani_vm *vm, bool &isValid);
}  // namespace Nearlink
}  // namespace OHOS
#endif  // TAIHE_ASYNC_CALLBACK_H
