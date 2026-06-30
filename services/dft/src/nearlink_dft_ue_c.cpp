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

#include "nearlink_dft_ue_c.h"
#include "log.h"
#include "def.h"
#include "nearlink_dft_ue.h"
#include "raw_address.h"

using namespace OHOS::Nearlink;

void WriteTemplateOneUe(DftEventEnum eventId, uint8_t devAddr[ADDR_LEN], const int sceneCode, const int subSceneCode)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, RawAddress::ConvertToString(devAddr), sceneCode,
        subSceneCode);
}

void WriteTemplateTwoUe(DftEventEnum eventId, uint8_t devAddr[ADDR_LEN], const int sceneCode, const int subSceneCode,
    const char* callingName)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, RawAddress::ConvertToString(devAddr), sceneCode, subSceneCode,
        std::string(callingName));
}

void WriteTemplateThreeUe(DftEventEnum eventId, uint8_t devAddr[ADDR_LEN], const int sceneCode, const char* callingName)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, RawAddress::ConvertToString(devAddr), sceneCode,
        std::string(callingName));
}

void WriteTemplateFiveUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, sceneCode, subSceneCode);
}
void WriteTemplateSevenUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode, const char* callingName)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, sceneCode, subSceneCode, std::string(callingName));
}

void WriteTemplateEightUe(DftEventEnum eventId, uint8_t devAddr[ADDR_LEN], int accessType, const int sceneCode,
    const int subSceneCode)
{
    NearlinkDftUe::GetInstance().WriteCommonUe(eventId, RawAddress::ConvertToString(devAddr),
        accessType, sceneCode, subSceneCode);
}