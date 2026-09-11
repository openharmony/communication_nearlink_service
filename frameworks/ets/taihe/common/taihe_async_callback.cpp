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

#include "taihe_async_callback.h"
#include "ani_nearlink_utils.h"
#include "log.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {

TaiheAsyncCallback::TaiheAsyncCallback(ani_vm *vm) : vm_(vm)
{
    ani_env *env = GetCurrentEnv(vm_, isAttach_);
    if (env == nullptr) {
        HILOGE("taihe get current env is nullptr");
        return;
    }
    ani_resolver deferred = nullptr;
    auto status = env->Promise_New(&deferred, &promise_);
    if (status != ANI_OK) {
        HILOGE("Promise_New failed, status: %{public}d", status);
        return;
    }
    bindDeferred_.store(deferred);
}

TaiheAsyncCallback::~TaiheAsyncCallback()
{
    if (isAttach_) {
        vm_->DetachCurrentThread();
    }
}

void TaiheAsyncCallback::CallFunction(int errCode, const std::shared_ptr<TaiheNativeObject> &object)
{
    if (object == nullptr) {
        HILOGE("taihe native object is nullptr");
        return;
    }
    if (vm_ == nullptr) {
        HILOGE("vm is nullptr");
        return;
    }
    // 本函数运行在竞争线程（业务回调/超时/Complete 工作线程），attach 只在当前线程记账，
    // 结算后立即 detach，避免污染成员 isAttach_、导致析构在错误线程 DetachCurrentThread（卡死）。
    bool isAttach = false;
    ani_env *curEnv = GetCurrentEnv(vm_, isAttach);
    if (curEnv == nullptr) {
        HILOGE("taihe get current env is nullptr");
        return;
    }
    TaiheCreateLocalScope(curEnv);
    // 原子取走 deferred（exchange 为 nullptr），保证并发下同一 resolver 至多被一个线程 settle
    ani_resolver deferred = bindDeferred_.exchange(nullptr);
    if (deferred) {
        if (errCode == NL_NO_ERROR) {
            ani_ref val = object->ToTaiheValue(curEnv);
            curEnv->PromiseResolver_Resolve(deferred, val);
        } else {
            ani_ref code = GetCallbackErrorValue(curEnv, errCode);
            curEnv->PromiseResolver_Reject(deferred, reinterpret_cast<ani_error>(code));
        }
    } else {
        HILOGE("promise or deferred is nullptr, maybe already settled");
    }
    TaiheDestroyLocalScope(curEnv);
    if (isAttach) {
        vm_->DetachCurrentThread();
    }
    return;
}

ani_object TaiheAsyncCallback::GetPromiseRet(void)
{
    return promise_;
}

void TaiheCreateLocalScope(ani_env* env)
{
    ani_size nr_refs = 16;
    ani_status status = env->CreateLocalScope(nr_refs);
    if (status != ANI_OK) {
        HILOGE("CreateLocalScope failed, status(%{public}d)", status);
    }
}

void TaiheDestroyLocalScope(ani_env* env)
{
    ani_status status = env->DestroyLocalScope();
    if (status != ANI_OK) {
        HILOGE("DestroyLocalScope failed, status(%{public}d)", status);
    }
}

ani_env *GetCurrentEnv(ani_vm *vm, bool &isAttach)
{
    if (vm == nullptr) {
        HILOGE("taihe vm is nullptr");
        return nullptr;
    }

    ani_env *threadEnv;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &threadEnv)) {
        HILOGE("GetEnv failed, AttachCurrentThread");
        ani_options aniArgs {0, nullptr};
        ani_status status = vm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &threadEnv);
        if (status != ANI_OK) {
            HILOGE("GetCurrentEnv failed, status: %{public}d", status);
            return nullptr;
        }
        isAttach = true;
    }

    return threadEnv;
}

} // namespace Nearlink
} // namespace OHOS