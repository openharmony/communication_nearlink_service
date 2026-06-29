/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SLE_CONTROLLER_SERVICE_H
#define SLE_CONTROLLER_SERVICE_H

#include <map>
#include <string>
#include "interface_sle_controller.h"
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "cm_api.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief SLE Controller Service implementation
 *        Service 层实现，直接调用 UAPI
 */
class SleControllerService : public InterfaceSleController {
public:
    static SleControllerService &GetInstance();

    SleControllerService();
    ~SleControllerService() override;

    // InterfaceSleController interface
    bool SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle) override;
    bool UpdateConnectInterval(const std::string &device, int32_t intervalType) override;

private:
    // Helper function to convert interval type to interval value
    bool FetchInterval(int32_t intervalType, uint16_t &intervalValue);

    bool GetConnectionParams(const std::string &device,
        uint16_t &intervalValue, CM_ConnectUpdateParamReq_S &updateParam);

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SleControllerService);
};

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_CONTROLLER_SERVICE_H
