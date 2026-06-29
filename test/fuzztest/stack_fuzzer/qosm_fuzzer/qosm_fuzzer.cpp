/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/socket.h>
#include "securec.h"

#include "qosm_fuzzer.h"

#include "qosm_uevent.h"
#include "sdf_mem.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "sdf_errno_base.h"
#include "cp_worker.h"

#define SCM_CREDENTIALSl 0x02

uint8_t* g_fuzzData = nullptr;
size_t g_fuzzDataSize = 0;

extern "C" ssize_t FuzzRecvMsgStub(int sockfd, struct msghdr *msg, int flags)
{
    msg->msg_iov->iov_base = static_cast<void*>(g_fuzzData);
    char* base = static_cast<char*>(msg->msg_iov->iov_base);
    size_t msgLen = 0;
    const char *audioMisc = "audio_misc";
    const char *patten = "NAME=sle_dsp_data ";
    size_t audioMiscLen = strlen(audioMisc);
    size_t pattenLen = strlen(patten);
    if (msgLen + audioMiscLen + 1 < g_fuzzDataSize) {
        if (g_fuzzData[0] % 2 == 0) {
            (void)memcpy_s(base, audioMiscLen, audioMisc, audioMiscLen);
            msgLen += audioMiscLen;
            base[msgLen++] = '\0';
            if (msgLen + pattenLen < g_fuzzDataSize) {
                (void)memcpy_s(base + msgLen, pattenLen, patten, pattenLen);
            }
        }
    }
    msgLen = g_fuzzDataSize;
    struct cmsghdr *hdr = (struct cmsghdr *)msg->msg_control;
    hdr->cmsg_type = SCM_CREDENTIALSl;

    return msgLen;
}

static void FuzzAudioDfxProcessDspData(const char *dftInfo)
{
    return;
}

extern "C" uint32_t SDF_EvcInstanceCreate(int *handle, const char *name)
{
    return SDF_OK;
}

extern "C" void SDF_EvcInstanceClose(int handle)
{
    return;
}

extern "C" void SDF_EvcCancelEvent(int eventHandle)
{
    return;
}

extern "C" uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event)
{
    event->callback(event->eventHandle, nullptr);
    return SDF_OK;
}

extern "C" uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    cb(arg);
    return 0;
}

uint32_t SDF_ModifyEventArgs(int eventHandle, SDF_EvcEventModifyFunc func)
{
    return 0;
}

namespace OHOS {
    void FuzzQosmUeventApi(const uint8_t* data, size_t size)
    {
        g_fuzzDataSize = size;
        uint8_t* fuzzData = static_cast<uint8_t*>(std::malloc(size));
        if (fuzzData == nullptr) {
            return;
        }
        (void)memcpy_s(fuzzData, size, data, size);
        g_fuzzData = fuzzData;
        QOSM_UeventInit(FuzzAudioDfxProcessDspData);
        QOSM_UeventDeinit();
        free(fuzzData);
    }
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::FuzzQosmUeventApi(data, size);
    return 0;
}