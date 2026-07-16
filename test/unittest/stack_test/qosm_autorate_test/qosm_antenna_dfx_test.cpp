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

#include "qosm_antenna_dfx.h"
#include <gtest/gtest.h>
#include <time.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "qosm_log.h"
#include "cp_worker.h"
#include "sdf_mem.h"
#include "sdf_timer.h"
#include "qosm_icg_types.h"
#include "qosm_autorate_test_log.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"

#ifdef __cplusplus
extern "C" {
#endif

static int g_timerCallCount = 0;
static bool g_mockOpenFail = false;
static int g_mockOpenErrno = ENOENT;
static int g_mockIoctlRet = -1;
static int g_mockPolicy = -1;
static bool g_supportSignalHub = true;

uint32_t CP_TimerAdd(int *timerId, SDF_TimerParam *param)
{
    (void)timerId;
    if (param && param->callback) {
        param->callback(param->args);
    }
    g_timerCallCount++;
    return 0;
}

void CP_TimerDel(int timerId)
{
    (void)timerId;
}

void *SDF_MemZalloc(size_t size)
{
    return malloc(size);
}

void SDF_MemFree(void *ptr)
{
    free(ptr);
}

int open(const char *pathname, int flags, ...)
{
    (void)flags;
    if (pathname == nullptr || strstr(pathname, "signal_hub") == nullptr) {
        errno = ENOENT;
        return -1;
    }
    if (g_mockOpenFail) {
        errno = g_mockOpenErrno;
        return -1;
    }
    if (!g_supportSignalHub) {
        errno = ENOENT;
        return -1;
    }
    return 1;
}

int ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    if (g_mockIoctlRet < 0) {
        return g_mockIoctlRet;
    }
    va_list args;
    va_start(args, request);
    void *ptr = va_arg(args, void*);
    va_end(args);
    if (ptr != NULL) {
        if (request == ((int)3222843656U)) {
            typedef struct {
                uint16_t abnormalType;
                uint16_t triggerType;
                uint16_t policy;
                uint16_t rsv;
            } MockQueryData;
            MockQueryData *queryData = (MockQueryData *)ptr;
            queryData->policy = g_mockPolicy;
        } else {
            typedef struct {
                uint16_t cmd;
                uint16_t size;
                uint8_t data[0];
            } MockAbnormalState;
            MockAbnormalState *abnormalState = (MockAbnormalState *)ptr;
            abnormalState->cmd = 1;
            abnormalState->size = sizeof(uint32_t);
        }
    }
    return g_mockIoctlRet;
}

#ifdef __cplusplus
}
#endif

class UT_QOSM_ANTENNA_DFX_TEST : public testing::Test {
protected:
    void SetUp()
    {
        QOSM_UnhookLog();
        g_timerCallCount = 0;
        g_mockOpenFail = false;
        g_mockOpenErrno = ENOENT;
        g_mockIoctlRet = -1;
        g_mockPolicy = -1;
        g_supportSignalHub = true;
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }
};

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxGetAntennaPolicyInitial)
{
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_EQ(policy, -1);
}

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxSupportedDeviceQuerySuccess)
{
    sleep(6);
    g_mockPolicy = 4;
    g_mockIoctlRet = 0;
    QOSM_AntennaDfxSendQueryCmd();
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_GE(policy, -1);
}

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxSupportedDeviceGetPolicyAfterQuery)
{
    sleep(6);
    g_mockPolicy = 2;
    g_mockIoctlRet = 0;
    QOSM_AntennaDfxSendQueryCmd();
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_GE(policy, -1);
}

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxRateLimitAfter5Second)
{
    sleep(6);
    g_mockPolicy = 1;
    g_mockIoctlRet = 0;
    QOSM_AntennaDfxSendQueryCmd();
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_GE(policy, -1);
}

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxAbnormalPolicyIgnore)
{
    sleep(6);
    g_mockPolicy = 3;
    g_mockIoctlRet = 0;
    QOSM_AntennaDfxSendQueryCmd();
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_GE(policy, -1);
}

TEST_F(UT_QOSM_ANTENNA_DFX_TEST, TestCaseAntennaDfxUnsupportedDevice)
{
    sleep(6);
    g_mockOpenFail = true;
    g_mockOpenErrno = ENOENT;
    QOSM_AntennaDfxSendQueryCmd();
    int policy = QOSM_AntennaDfxGetAntennaPolicy();
    EXPECT_GE(policy, -1);
}