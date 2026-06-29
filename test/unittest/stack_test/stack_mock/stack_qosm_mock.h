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
 
#ifndef STACK_QOSM_MOCK_H
#define STACK_QOSM_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
#include <qosm_autorate_def.h>

namespace OHOS {
class QosmMockInterface {
public:
    QosmMockInterface() {};
    virtual ~QosmMockInterface() {};
    virtual uint32_t QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback) = 0;
    virtual uint32_t QOSM_AutoRateUnregisterCallback(void) = 0;
    virtual uint32_t QOSM_AutoRateSetTestParam(const QOSM_AutoRateParam *param) = 0;
    virtual uint32_t QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *callback) = 0;
    virtual uint32_t QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *callback) = 0;
    virtual uint32_t QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *callback) = 0;
    virtual uint32_t QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *callback) = 0;
    virtual uint32_t QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *callback) = 0;
};
 
class QosmMock : public QosmMockInterface {
public:
    QosmMock();
    ~QosmMock() override;
    MOCK_METHOD(uint32_t, QOSM_AutoRateRegisterCallback, (const QOSM_AutoRateCallback *callback), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateUnregisterCallback, (), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateSetTestParam, (const QOSM_AutoRateParam *param), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateAddConnection, (const QOSM_AutoRateConnParam *callback), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateAddDataPath, (const QOSM_AutoRateDataPath *callback), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateDeleteDataPath, (const QOSM_AutoRateDeletedDataPath *callback), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateDeleteConnection, (const QOSM_AutoRateConnParam *callback), (override));
    MOCK_METHOD(uint32_t, QOSM_AutoRateSetEarphoneFeedback, (const QOSM_AutoRateEarphoneFeedbackParam *callback),
                                                            (override));
    static QosmMock& GetMock();
 
private:
    static QosmMock *gMock;
};
 
}; // namespace OHOS
#endif