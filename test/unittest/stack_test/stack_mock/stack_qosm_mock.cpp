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

#include <gtest/gtest.h>
#include <securec.h>
 
#include "stack_qosm_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackQosmMock;
 
QosmMock::QosmMock()
{
    g_stackQosmMock = reinterpret_cast<void *>(this);
}

QosmMock::~QosmMock()
{
    g_stackQosmMock = nullptr;
}

static QosmMockInterface *QosmMock()
{
    return reinterpret_cast<QosmMockInterface *>(g_stackQosmMock);
}

extern "C" {
uint32_t QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback)
{
    return QosmMock()->QOSM_AutoRateRegisterCallback(callback);
}

uint32_t QOSM_AutoRateUnregisterCallback(void)
{
    return QosmMock()->QOSM_AutoRateUnregisterCallback();
}

uint32_t QOSM_AutoRateSetTestParam(const QOSM_AutoRateParam *param)
{
    return QosmMock()->QOSM_AutoRateSetTestParam(param);
}

uint32_t QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *callback)
{
    return QosmMock()->QOSM_AutoRateAddConnection(callback);
}

uint32_t QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *callback)
{
    return QosmMock()->QOSM_AutoRateAddDataPath(callback);
}

uint32_t QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *callback)
{
    return QosmMock()->QOSM_AutoRateDeleteDataPath(callback);
}

uint32_t QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *callback)
{
    return QosmMock()->QOSM_AutoRateDeleteConnection(callback);
}

uint32_t QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *callback)
{
    return QosmMock()->QOSM_AutoRateSetEarphoneFeedback(callback);
}

uint32_t QOSM_AutoRateGetICGG2TParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    return 0;
}

uint32_t QOSM_AutoRateGetICGT2GParam(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    return 0;
}
}
}