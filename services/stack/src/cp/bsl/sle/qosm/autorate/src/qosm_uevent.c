/****************************************************************************
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
****************************************************************************/
#include <ctype.h>
#include <errno.h>
#include <linux/netlink.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "qosm_log.h"
#include "qosm_errno.h"
#include "cp_worker.h"
#include "sdf_util.h"
#include "sdf_mem.h"
#include "sdf_evc.h"
#include "qosm_uevent.h"
#include "securec.h"

#define UEVENT_SOCKET_BUFF_SIZE (64 * 1024)
#define UEVENT_SOCKET_GROUPS    0xffffffff
#define UEVENT_MSG_LEN          2048
#define UEVENT_POLL_WAIT_TIME   (10 * 1000)
#define MOVE_NUM 16

#ifdef QOSM_FUZZER
#include "qosm_fuzzer.h"
#define RECVMSG FuzzRecvMsgStub
#else
#define RECVMSG recvmsg
#endif

static struct QOSM_UeventInfo {
    QOSM_DspDataProcessCb process;
    bool threadRunning;
    int evcHandle;
    int fd;
} g_qosmUeventData;

static int QOSM_UeventOpen(void)
{
    int socketFd = -1;
    int buffSize = UEVENT_SOCKET_BUFF_SIZE;
    const int32_t on = 1; // turn on passcred
    struct sockaddr_nl addr;

    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = ((uint32_t)gettid() << MOVE_NUM) | (uint32_t)getpid();
    addr.nl_groups = UEVENT_SOCKET_GROUPS;

    socketFd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (socketFd < 0) {
        QOSM_LOGE("socket failed, %d %s", errno, strerror(errno));
        return -1;
    }

    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize)) != 0) {
        QOSM_LOGE("setsockopt SO_RCVBUF failed, %d %s", errno, strerror(errno));
        close(socketFd);
        return -1;
    }

    if (setsockopt(socketFd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) != 0) {
        QOSM_LOGE("setsockopt SO_PASSCRED failed, %d %s", errno, strerror(errno));
        close(socketFd);
        return -1;
    }

    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        QOSM_LOGE("bind socket failed, %d %s", errno, strerror(errno));
        close(socketFd);
        return -1;
    }

    return socketFd;
}

static void QOSM_UeventProcessMsg(void *arg)
{
    if (arg == NULL) {
        QOSM_LOGE("invalid arg");
        return;
    }

    QOSM_LOGI("processs %s", (char *)arg);
    QOSM_DspDataProcessCb process = g_qosmUeventData.process;
    if (process != NULL) {
        process((char *)arg);
    }
}

static void QOSM_UeventParseMsg(const char *msg, uint32_t msgLen)
{
    const char *audioMisc = "audio_misc";
    const char *patten = "NAME=sle_dsp_data";
    size_t pattenLen = strlen(patten);

    /*
     * hifi_report通知上来的消息格式为change=....audio_misc\0XXX=YYY\0NAME=sle_dsp_data ZZZZ\0
     * 第一步，先检查消息开始的字符串是否包含audio_misc
     * 第二步，检查每个用'\0'分割的字符串，是否可以匹配上NAME=sle_dsp_data，匹配上后，空格之后的字符串就是DSP上报的数据
     */
    if (strstr(msg, audioMisc) == NULL) {
        QOSM_LOGD("invalid str %s", msg);
        return;
    }

    const char *p = msg;
    const char *end = msg + msgLen;
    while (p < end) {
        /* 跳过连续的字符串结束符 */
        if (*p == '\0') {
            p++;
            continue;
        }

        /* 获取当前字符串长度，长度必须要大于NAME=sle_dsp_data长度加空格的长度才表示有数据 */
        size_t plen = strnlen(p, (uint32_t)(uintptr_t)(end - p));
        if (plen > pattenLen + 1 && strncmp(p, patten, pattenLen) == 0) {
            QOSM_LOGI("get dsp str: %s, len: %zu", p + pattenLen + 1, plen - pattenLen - 1);
            CP_PostTask(QOSM_UeventProcessMsg, SDF_StrDup(p + pattenLen + 1), SDF_MemFree);
            break;
        }

        p += plen + 1;
    }
}

static void QOSM_UeventRead(int sockFd, void *arg)
{
    (void)arg;
    QOSM_LOGD("start to read");
    uint8_t buffer[UEVENT_MSG_LEN];
    char credMsg[CMSG_SPACE(sizeof(struct ucred))] = {0};
    struct iovec iov;
    struct sockaddr_nl addr;
    struct msghdr msghdr = {0};

    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));

    iov.iov_base = buffer;
    iov.iov_len = UEVENT_MSG_LEN - 1;

    msghdr.msg_name = &addr;
    msghdr.msg_namelen = sizeof(addr);
    msghdr.msg_iov = &iov;
    msghdr.msg_iovlen = 1;
    msghdr.msg_control = credMsg;
    msghdr.msg_controllen = sizeof(credMsg);
    ssize_t len = RECVMSG(sockFd, &msghdr, MSG_DONTWAIT);
    if (len <= 0 || len > UEVENT_MSG_LEN - 1) {
        QOSM_LOGE("read error, ret=%zd, error=%d,%s", len, errno, strerror(errno));
        return;
    }
    buffer[len] = '\0';

    struct cmsghdr *hdr = CMSG_FIRSTHDR(&msghdr);
    if (hdr == NULL || hdr->cmsg_type != SCM_CREDENTIALS) {
        QOSM_LOGW("Unexpected control message, ignored");
        return;
    }

    QOSM_UeventParseMsg((const char *)buffer, len);
    QOSM_LOGD("read completed");
}

void QOSM_UeventInit(QOSM_DspDataProcessCb process)
{
    if (g_qosmUeventData.threadRunning) {
        QOSM_LOGE("already inited");
        return;
    }

    if (SDF_EvcInstanceCreate(&g_qosmUeventData.evcHandle, "QOSM_UeventThread") != SDF_OK) {
        QOSM_LOGE("create uevent thread failed");
        return;
    }

    int socketFd = QOSM_UeventOpen();
    if (socketFd == -1) {
        SDF_EvcInstanceClose(g_qosmUeventData.evcHandle);
        QOSM_LOGE("open qosm uevent socket failed!");
        return;
    }

    QOSM_LOGI("uevent inited, sock fd = %d", socketFd);
    g_qosmUeventData.fd = socketFd;
    g_qosmUeventData.process = process;
    g_qosmUeventData.threadRunning = true;

    SDF_EvcEvent event = {SDF_EVC_USER_EVENT, socketFd, QOSM_UeventRead, NULL, NULL};
    if (SDF_EvcListenEvent(g_qosmUeventData.evcHandle, &event) != SDF_OK) {
        QOSM_LOGE("add uevent listen failed");
        QOSM_UeventDeinit();
    }
}

void QOSM_UeventDeinit(void)
{
    if (!g_qosmUeventData.threadRunning) {
        QOSM_LOGI("thread is already stopped");
        return;
    }

    if (g_qosmUeventData.fd != -1) {
        QOSM_LOGI("uevent deinited, fd=%d", g_qosmUeventData.fd);
        SDF_EvcCancelEvent(g_qosmUeventData.fd);
        close(g_qosmUeventData.fd);
        g_qosmUeventData.fd = -1;
    }

    SDF_EvcInstanceClose(g_qosmUeventData.evcHandle);

    g_qosmUeventData.threadRunning = false;
    QOSM_LOGI("deinit done");
}