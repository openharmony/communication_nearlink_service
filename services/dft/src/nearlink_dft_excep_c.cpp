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

#include "nearlink_dft_excep_c.h"
#include "log.h"
#include "def.h"
#include "nearlink_dft_excep.h"
#include "raw_address.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_dft_manager.h"

using namespace OHOS::Nearlink;

void WriteTemplateOneExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode)
{
    NearlinkDftExcep::GetInstance().WriteCommonExcep(eventId, errorCode, subErrorCode);
}

void WriteTemplateTwoExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode, const char* strValue)
{
    NearlinkDftExcep::GetInstance().WriteCommonExcep(eventId, errorCode, subErrorCode, std::string(strValue));
}

void WriteTemplateThreeExcep(DftEventEnum eventId, uint8_t devAddr[ADDR_LEN], int accessType,
    const int errorCode, const int subErrorCode)
{
    NearlinkDftExcep::GetInstance().WriteCommonExcep(eventId, RawAddress::ConvertToString(devAddr),
        accessType, errorCode, subErrorCode);
}
