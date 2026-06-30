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

#include <gtest/gtest.h>
#include <securec.h>

#include "stack_qosm_transchannel_mock.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
void *g_stackQOSMTransChannelMock;

QOSMTransChannelMock::QOSMTransChannelMock()
{
    g_stackQOSMTransChannelMock = reinterpret_cast<void *>(this);
}

QOSMTransChannelMock::~QOSMTransChannelMock()
{
    g_stackQOSMTransChannelMock = nullptr;
}

static QOSMTransChannelMockInterface *QOSMTransChannelMock()
{
    return reinterpret_cast<QOSMTransChannelMockInterface *>(g_stackQOSMTransChannelMock);
}

extern "C" {

uint32_t QOSM_TransChannelCbksRegister(const QOSM_TransChannelCbks_S *cbks)
{
    return QOSMTransChannelMock()->QOSM_TransChannelCbksRegister(cbks);
}

uint32_t QOSM_TransChannelCbksUnregister(void)
{
    return QOSMTransChannelMock()->QOSM_TransChannelCbksUnregister();
}

uint32_t QOSM_TransChannelCreate(const QOSM_TransChannelParams_S *params)
{
    return QOSMTransChannelMock()->QOSM_TransChannelCreate(params);
}

uint32_t QOSM_TransChannelDestroy(const QOSM_TransChannelReleaseParams_S *params)
{
    return QOSMTransChannelMock()->QOSM_TransChannelDestroy(params);
}

uint32_t QOSM_TransChannelInit()
{
    return QOSMTransChannelMock()->QOSM_TransChannelInit();
}

void QOSM_TransChannelDeInit()
{
    return QOSMTransChannelMock()->QOSM_TransChannelDeInit();
}

}

}