/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef STACK_TRANS_MOCK_H
#define STACK_TRANS_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>

#include "securec.h"

#include "nai_log.h"
#include "dtap_errno.h"
#include "transport.h"
#include "transport_errno.h"
#include "transport_internal.h"

namespace OHOS {

class TransMockInterface {
public:
    TransMockInterface() {};
    virtual ~TransMockInterface() {};

    virtual uint32_t TRANS_RegisterCbks(const TRANS_Cbks_S *cbks) = 0;
    virtual void TRANS_UnregisterCbks(void) = 0;
    virtual uint32_t TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen) = 0;
    virtual void TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result) = 0;
};

class TransMock : public TransMockInterface{
    public:
    TransMock();
    ~TransMock() override;
    MOCK_METHOD(uint32_t, TRANS_RegisterCbks, (const TRANS_Cbks_S *cbks), (override));
    MOCK_METHOD(void, TRANS_UnregisterCbks, (), (override));
    MOCK_METHOD(uint32_t, TRANS_SendData, (TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen), (override));
    MOCK_METHOD(void, TRANS_ChannelSetStatus, (SLE_Addr_S *addr, uint8_t tcid, uint8_t result), (override));

    static TransMock& GetMock();

private:
    static TransMock *gMock;
};

}; // namespace OHOS
#endif