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

#ifndef STACK_DLI_MOCK_H
#define STACK_DLI_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>
#include "dli_opcode.h"
#include "dli_cmd_struct.h"

namespace OHOS {
class DliMockInterface {
public:
    DliMockInterface() {};
    virtual ~DliMockInterface() {};
    virtual uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size) = 0;
    virtual void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size) = 0;
    virtual uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param) = 0;
    virtual uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param) = 0;
    virtual uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param) = 0;
    virtual uint32_t DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param) = 0;
    virtual uint32_t DLI_EnableEncryption(DLI_EnableEncryptParam *param) = 0;
};

class DliMock : public DliMockInterface {
public:
    DliMock();
    ~DliMock() override;
    MOCK_METHOD(uint32_t, DLI_CmdCbkReg, (const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size), (override));
    MOCK_METHOD(void, DLI_CmdCbkUnReg, (const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size), (override));
    MOCK_METHOD(uint32_t, DLI_SetMeasureParam, (DLI_SetMeasureConfigParam *param), (override));
    MOCK_METHOD(uint32_t, DLI_SetMeasureEnable, (DLI_SetMeasureEnableParam *param), (override));
    MOCK_METHOD(uint32_t, DLI_ReadRemoteMeasureCaps, (DLI_ReadRemoteMeasureCapsParam *param), (override));
    MOCK_METHOD(uint32_t, DLI_EnableIMGEncryption, (DLI_IMGEncryptParam *param), (override));
    MOCK_METHOD(uint32_t, DLI_EnableEncryption, (DLI_EnableEncryptParam *param), (override));
    static DliMock& GetMock();

private:
    static DliMock *gMock;
};

}; // namespace OHOS
#endif