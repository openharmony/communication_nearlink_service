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

#include <string>
#include "log.h"
#include "nearlink_dft_car_key_loader.h"

namespace OHOS {
namespace Nearlink {

namespace CarKeyDft {
    const std::string DFT_EXT_LIB_NAME("libnearlink_dft_manager_ext.z.so");
    const std::string RECORD_WALLET_CHR_FUNC_NAME("RecordWalletChrWrapper");
} // namespace CarKeyDft

NearlinkDftCarKeyLoader::NearlinkDftCarKeyLoader() : loader_(CarKeyDft::DFT_EXT_LIB_NAME) {}

// once lib loaded, it is not easily unloaded
NearlinkDftCarKeyLoader::~NearlinkDftCarKeyLoader() = default;

NearlinkDftCarKeyLoader& NearlinkDftCarKeyLoader::GetInstance()
{
    static NearlinkDftCarKeyLoader loader;
    return loader;
}

void NearlinkDftCarKeyLoader::EnsureLibraryLoaded(void)
{
    if (!loader_.IsLibraryLoaded()) {
        loader_.OpenLib();
        recordWalletChrFunc_ =
            reinterpret_cast<RecordWalletChrFuncType>(loader_.GetSymbol(CarKeyDft::RECORD_WALLET_CHR_FUNC_NAME));
        HILOGI("[NEARLINK_DFT_CAR_KEY] load dft manager ext lib");
    }
}

void NearlinkDftCarKeyLoader::RecordWalletChr(DftEventEnum eventId, const DftParam::ParamTypeSet &set)
{
    EnsureLibraryLoaded();
    NL_CHECK_RETURN(recordWalletChrFunc_ != nullptr,
        "[NEARLINK_DFT_CAR_KEY] dlsym %{public}s failed", (CarKeyDft::RECORD_WALLET_CHR_FUNC_NAME).c_str());

    std::string tmp = "";
    for (auto it = set.begin(); it != set.end(); ++it) {
        tmp.append((*it)->ToJsonString());
        tmp.append(",");
    }
    NL_CHECK_RETURN(!tmp.empty(), "ignore");
    tmp.pop_back();

    recordWalletChrFunc_(static_cast<int32_t>(eventId), tmp.c_str(), tmp.size());
}

} // namespace Nearlink
} // namespace OHOS