/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef STACK_CM_MOCK_H
#define STACK_CM_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>
#include "cm_def.h"
#include "cm_api.h"
#include "cm_trans_channel_api.h"

namespace OHOS {
class CmMockInterface {
public:
    CmMockInterface() {};
    virtual ~CmMockInterface() {};
    virtual uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks) = 0;
    virtual uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId) = 0;
    virtual uint32_t CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink) = 0;
    virtual uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk) = 0;
    virtual uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink) = 0;
    virtual void CM_UnRegTransChannelListener(void) = 0;
    virtual uint32_t CM_GetLogicLinkConnectedSize(void) = 0;
};

class CmMock : public CmMockInterface {
public:
    CmMock();
    ~CmMock() override;
    MOCK_METHOD(uint32_t, CM_RegLogicLinkListener, (CM_LogicLinkCbks_S *cbks), (override));
    MOCK_METHOD(uint32_t, CM_UnRegLogicLinkListener, (uint8_t moduleId), (override));
    MOCK_METHOD(uint32_t, CM_GetLogicLinkByAddr, (SLE_Addr_S *addr, CM_LogicLink_S *logicLink), (override));
    MOCK_METHOD(uint32_t, CM_GetLogicLinkByLcid, (uint16_t lcid, CM_LogicLink_S *logicLink), (override));
    MOCK_METHOD(uint32_t, CM_RegTransChannelListener, (CM_TransChannelCbk cbk), (override));
    MOCK_METHOD(void, CM_UnRegTransChannelListener, (), (override));
    MOCK_METHOD(uint32_t, CM_GetLogicLinkConnectedSize, (), (override));
    static CmMock& GetMock();

private:
    static CmMock *gMock;
};
}; // namespace OHOS
#endif