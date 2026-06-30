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
#include <sys/socket.h>
#include "securec.h"

#include "qosm_uevent.h"
#include "cp_worker.h"
#include "qosm_log.h"
#include "qosm_errno.h"
#include "sdf_util.h"
#include "sdf_mem.h"
#include "sdf_evc.h"
#include "qosm_autorate_test_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UEVENT_MSG_LEN          2048

static SDF_EvcEvent g_event = {};
static struct cmsghdr g_msgControl = {};
static uint8_t g_buffer[UEVENT_MSG_LEN] = {};
static uint32_t g_evcCreateRet = 0;
static uint32_t g_evcListenRet = 0;
static uint32_t g_recvMsgRet = 0;
static bool g_isHdrNull = false;

uint32_t SDF_EvcInstanceCreate(int *handle, const char *name)
{
    return g_evcCreateRet;
}

void SDF_EvcCancelEvent(int eventHandle)
{
}

void SDF_EvcInstanceClose(int handle)
{
}

uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event)
{
    if (memcpy_s(&g_event, sizeof(SDF_EvcEvent), event, sizeof(SDF_EvcEvent)) != EOK) {
        return 1;
    }
    return g_evcListenRet;
}

ssize_t recvmsg(int fd, struct msghdr *msg, int flags)
{
    if (g_recvMsgRet != 0) {
        return 0;
    }
    // 组装数据：change=....audio_misc\0XXX=YYY\0NAME=sle_dsp_data ZZZZ\0
    uint8_t data[] = {
        // change=....audio_misc
        'c', 'h', 'a', 'n', 'g', 'e', '=', '.', '.', '.', '.', 'a', 'u', 'd', 'i', 'o', '_', 'm', 'i', 's', 'c',
        // \0 分隔符
        0x00,
        // XXX=YYY
        'X', 'X', 'X', '=', 'Y', 'Y', 'Y',
        // \0 分隔符
        0x00,
        // NAME=sle_dsp_data ZZZZ
        'N', 'A', 'M', 'E', '=', 's', 'l', 'e', '_', 'd', 's', 'p', '_', 'd', 'a', 't', 'a', ' ', 'Z', 'Z', 'Z', 'Z',
        // \0 结尾
        0x00
    };
    if (memcpy_s(msg->msg_iov->iov_base, sizeof(data), data, sizeof(data)) != EOK) {
        return 0;
    }
    g_msgControl.cmsg_type = SCM_CREDENTIALS;
    msg->msg_control = (void *)&g_msgControl;
    msg->msg_controllen = g_isHdrNull ? 0 : sizeof(g_msgControl);
    return sizeof(data);
}

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != nullptr) {
        cb(arg);
    }
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

class UT_QOSM_UEVENT_TEST : public testing::Test {
protected:
    void SetUp()
    {
        QOSM_UnhookLog();
        g_evcCreateRet = 0;
        g_evcListenRet = 0;
        g_recvMsgRet = 0;
        g_isHdrNull = false;
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

static void QOSM_DspDataProcessCbMock(const char *dspData)
{
    (void)memcpy_s(g_buffer, sizeof(g_buffer), dspData, sizeof(g_buffer));
}

TEST_F(UT_QOSM_UEVENT_TEST, TestCaseUeventInit)
{
    QOSM_HookLog();
    g_evcCreateRet = 1;
    QOSM_UeventInit(QOSM_DspDataProcessCbMock);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "create uevent thread failed"), nullptr);

    g_evcCreateRet = 0;
    g_evcListenRet = 1;
    QOSM_UeventInit(QOSM_DspDataProcessCbMock);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "add uevent listen failed"), nullptr);

    QOSM_ClearHookLog();
    g_evcListenRet = 0;
    QOSM_UeventInit(QOSM_DspDataProcessCbMock);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "uevent inited, sock fd"), nullptr);
    EXPECT_EQ(strstr(QOSM_GetHookLog(), "add uevent listen failed"), nullptr);

    // init repeatly
    QOSM_UeventInit(QOSM_DspDataProcessCbMock);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "already inited"), nullptr);
}

TEST_F(UT_QOSM_UEVENT_TEST, TestCaseUeventRead)
{
    QOSM_HookLog();
    g_recvMsgRet = 1;
    EXPECT_NE(g_event.callback, nullptr);
    g_event.callback(0, g_event.args);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "read error, ret=0"), nullptr);

    g_recvMsgRet = 0;
    g_isHdrNull = true;
    g_event.callback(0, g_event.args);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "Unexpected control message, ignored"), nullptr);

    g_isHdrNull = false;
    g_event.callback(0, g_event.args);
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "read completed"), nullptr);
    EXPECT_EQ(g_buffer[0], 'Z');
    EXPECT_EQ(g_buffer[1], 'Z');
    EXPECT_EQ(g_buffer[2], 'Z');
    EXPECT_EQ(g_buffer[3], 'Z');
    EXPECT_EQ(g_buffer[4], 0x00);
}

TEST_F(UT_QOSM_UEVENT_TEST, TestCaseUeventDeinit)
{
    QOSM_HookLog();
    QOSM_UeventDeinit();
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "deinit done"), nullptr);

    // deinit repeatly
    QOSM_UeventDeinit();
    EXPECT_NE(QOSM_GetHookLog(), nullptr);
    EXPECT_NE(strstr(QOSM_GetHookLog(), "thread is already stopped"), nullptr);
}
