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
 
#ifndef STACK_DLI_LAYER_MOCK_H
#define STACK_DLI_LAYER_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
#include <stdbool.h>
#include "dli_layer_stru.h"

namespace OHOS {
class DliLayerMockInterface {
public:
    DliLayerMockInterface() {};
    virtual ~DliLayerMockInterface() {};
    virtual uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf) = 0;
    virtual uint32_t DLI_GetFragmentMaxLen(void) = 0;
    virtual uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt) = 0;
    virtual uint32_t DLI_DataSend(DLI_DataStru *data) = 0;
    virtual uint32_t DLI_CmdSend(DLI_CmdStru *cmd) = 0;
    virtual void DLI_PostNextTask(DLI_TaskType type) = 0;
    /*
    virtual uint32_t DLI_LayerInit(void) = 0;
    virtual void DLI_LayerDeinit(void) = 0;
    virtual uint32_t DLI_LayerEnable(void) = 0;
    virtual void DLI_LayerDisable(void);
    */
};

class DliLayerMock : public DliLayerMockInterface {
public:
    DliLayerMock();
    ~DliLayerMock() override;
    MOCK_METHOD(uint32_t, DLI_GetDataFragmentNums, (SDF_Buff_S *buf), (override));
    MOCK_METHOD(uint32_t, DLI_GetFragmentMaxLen, (), (override));
    MOCK_METHOD(uint32_t, DLI_SplitData, (DLI_DataStru *data, SDF_Buff_S *fragmentBuf[],
        uint32_t fragmentCnt), (override));
    MOCK_METHOD(uint32_t, DLI_DataSend, (DLI_DataStru *data), (override));
    MOCK_METHOD(uint32_t, DLI_CmdSend, (DLI_CmdStru *cmd), (override));
    MOCK_METHOD(void, DLI_PostNextTask, (DLI_TaskType type), (override));
    /*
    MOCK_METHOD(void, DLI_LayerDeinit, (), (override));
    MOCK_METHOD(uint32_t, DLI_LayerEnable, (), (override));
    MOCK_METHOD(void, DLI_LayerDisable, (), (override));
    MOCK_METHOD(uint32_t, DLI_LayerInit, (), (override)); */

    static DliLayerMock& GetMock();
private:
    static DliLayerMock *gMock;
};

}; // namespace OHOS
#endif
