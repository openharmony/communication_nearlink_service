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
#include "mock_service_manager_plugin.h"

namespace OHOS {
namespace Nearlink {

MockServiceManagerPlugin &GetAntennaMockPlugin()
{
    static MockServiceManagerPlugin instance;
    return instance;
}

// 测试内定义 GetInstance 返回本地 mock，替代产品侧的外部插件加载链
ServiceManagerPluginInterface *ServiceManagerPluginInterface::GetInstance()
{
    return &GetAntennaMockPlugin();
}
}  // namespace Nearlink
}  // namespace OHOS
