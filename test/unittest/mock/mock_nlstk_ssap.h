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

#ifndef MOCK_NLSTK_SSAP_H
#define MOCK_NLSTK_SSAP_H

#include <cstdint>
#include <vector>

#include "nlstk_ssap_app_server.h"

// 适配层 AddService 传入参数的镜像记录，供用例校验服务/属性 type 与 uuid 的转换结果
struct MockNlstkSsapAddedService {
    NLSTK_SsapUuid_S serviceUuid {};
    NLSTK_SsapItemType_E serviceType {};
    std::vector<NLSTK_SsapUuid_S> propertyUuids;
    std::vector<NLSTK_SsapItemType_E> propertyTypes;
};

// 返回该 appId 最近一次 AddService 的记录，无记录返回 nullptr
const MockNlstkSsapAddedService *MockNlstkSsapGetLastAddedService(int32_t appId);

#endif // MOCK_NLSTK_SSAP_H
