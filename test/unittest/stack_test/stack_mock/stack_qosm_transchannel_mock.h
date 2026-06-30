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

#ifndef STACK_QOSM_TRANS_CHANNEL_MOCK_H
#define STACK_QOSM_TRANS_CHANNEL_MOCK_H

#include <stdint.h>
#include <gmock/gmock.h>

#include "securec.h"

#include "nai_log.h"
#include "qosm_trans_channel.h"
#include "qosm_errno.h"

namespace OHOS {

class QOSMTransChannelMockInterface {
public:
    QOSMTransChannelMockInterface() {};
    virtual ~QOSMTransChannelMockInterface() {};

    virtual uint32_t QOSM_TransChannelCbksRegister(const QOSM_TransChannelCbks_S *cbks) = 0;
    virtual uint32_t QOSM_TransChannelCbksUnregister(void) = 0;
    virtual uint32_t QOSM_TransChannelCreate(const QOSM_TransChannelParams_S *params) = 0;
    virtual uint32_t QOSM_TransChannelDestroy(const QOSM_TransChannelReleaseParams_S *params) = 0;
    virtual uint32_t QOSM_TransChannelInit(void) = 0;
    virtual void QOSM_TransChannelDeInit(void) = 0;
};

class QOSMTransChannelMock : public QOSMTransChannelMockInterface{
    public:
    QOSMTransChannelMock();
    ~QOSMTransChannelMock() override;
    MOCK_METHOD(uint32_t, QOSM_TransChannelCbksRegister, (const QOSM_TransChannelCbks_S *cbks), (override));
    MOCK_METHOD(uint32_t, QOSM_TransChannelCbksUnregister, (), (override));
    MOCK_METHOD(uint32_t, QOSM_TransChannelCreate, (const QOSM_TransChannelParams_S *params), (override));
    MOCK_METHOD(uint32_t, QOSM_TransChannelDestroy, (const QOSM_TransChannelReleaseParams_S *params), (override));
    MOCK_METHOD(uint32_t, QOSM_TransChannelInit, (), (override));
    MOCK_METHOD(void, QOSM_TransChannelDeInit, (), (override));

    static QOSMTransChannelMock& GetMock();

private:
    static QOSMTransChannelMock *gMock;
};

}; // namespace OHOS
#endif