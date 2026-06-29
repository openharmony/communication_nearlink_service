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

#ifndef NEARLINK_DFT_CAR_KEY_LOADER_H
#define NEARLINK_DFT_CAR_KEY_LOADER_H

#include "DynamicLibraryLoader.h"
#include "nearlink_dft_database.h"
#include "nearlink_dft_utils.h"

namespace OHOS {
namespace Nearlink {

class NearlinkDftCarKeyLoader {
public:
    static NearlinkDftCarKeyLoader &GetInstance();
    void RecordWalletChr(DftEventEnum eventId, const DftParam::ParamTypeSet &set);

private:
    using RecordWalletChrFuncType = void (*)(int32_t eventId, const char *param, size_t size);
    CDynamicLibraryLoader loader_;
    RecordWalletChrFuncType recordWalletChrFunc_ = nullptr;

    NearlinkDftCarKeyLoader();
    ~NearlinkDftCarKeyLoader();
    void EnsureLibraryLoaded(void);
};

} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_DFT_CAR_KEY_LOADER_H