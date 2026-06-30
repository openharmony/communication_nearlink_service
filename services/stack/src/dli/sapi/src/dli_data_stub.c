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

#include "dli_data_stub.h"
#include <stddef.h>
#include <stdlib.h>
#include "securec.h"
#include "dli_def.h"
#include "dli_log.h"
#include "dli_errno.h"
#include "sdf_evc.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NEARLINK_SERVICE_STACK_LOCAL_TEST
struct ThreadStru {
    int evcHandle;
    int eventHandle;
    SDF_Worker_S *worker;
};

static struct ThreadStru g_ThreadStru = {
    .evcHandle = 0,
    .eventHandle = 0,
    .worker = NULL
};

#define DLI_EVENT_HEADER_LEN 4
#define DATA_LEN_OFFSET 2
#define PD_BITS_OFFSET 12
#define DLI_OPCODE_BITS_LEN 2
#define DATA_LEN 16

#define FULL_FRAGMENT 0
#define FIRST_FRAGMENT 1
#define MIDDLE_FRAGMENT 2
#define LAST_FRAGMENT 3

#define DECODE2BYTES(_ptr)     \
    (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))
#define ENCODE2BYTE(_ptr, value) \
    do { \
        *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(value) >> 8); \
        *(uint8_t *)(_ptr) = (uint8_t)(value); \
    } while (0)

typedef void (*SleProcess)(const SlePacket *packet);
struct ProcessCmdStru {
    uint16_t cmd;
    int (*ProcessCmd)(const SlePacket *packet);
};

static void WorkerRunOnce(void *args)
{
    (void)args;
    SDF_WorkerRunOnce(g_ThreadStru.worker);
}

static uint32_t MainThreadInit()
{
    uint32_t ret = SDF_EvcInstanceCreate(&g_ThreadStru.evcHandle, "control-thread");
    DLI_CHECK_RETURN_RET(ret == 0, DLI_STACK_EVC_CREATE_ERRNO, "SDF_EvcInstanceCreate errno %u", ret);
    g_ThreadStru.worker = SDF_CreateWorker();
    if (g_ThreadStru.worker == NULL) {
        SDF_EvcInstanceClose(g_ThreadStru.evcHandle);
        DLI_LOGE("SDF_CreateWorker err");
        return DLI_STACK_WORKER_CREATE_ERRNO;
    }
    SDF_EventParam param = {g_ThreadStru.evcHandle, WorkerRunOnce, NULL};
    ret = SDF_EventAdd(&g_ThreadStru.eventHandle, &param);
    if (ret != 0) {
        DLI_LOGE("SDF_EventAdd errno %u", ret);
        SDF_DestroyWorker(g_ThreadStru.worker);
        g_ThreadStru.worker = NULL;
        SDF_EvcInstanceClose(g_ThreadStru.evcHandle);
        return DLI_STACK_EVENT_ADD_ERRNO;
    }
    return 0;
}

static SleDliCallbackFunc *g_dliCb = NULL;
int SleHalInit(SleDliCallbackFunc *func)
{
    if (func == NULL || func->dliPacketReceived == NULL ||
        func->initializationComplete == NULL || g_dliCb != NULL) {
        return INITIALIZATION_ERROR;
    }

    g_dliCb = (SleDliCallbackFunc*)malloc(sizeof(SleDliCallbackFunc));
    if (g_dliCb == NULL) {
        return INITIALIZATION_ERROR;
    }

    g_dliCb->dliPacketReceived = func->dliPacketReceived;
    g_dliCb->initializationComplete = func->initializationComplete;
    g_dliCb->initializationComplete(SUCCESS);
    (void)MainThreadInit();
    return 0;
}

void SleReset()
{
    return;
}

int GetDliVersion(void)
{
    return 1;
}

void SleHalClose(void)
{
    free(g_dliCb);
    g_dliCb = NULL;
    SDF_EvcInstanceClose(g_ThreadStru.evcHandle);
    SDF_DestroyWorker(g_ThreadStru.worker);
    (void)memset_s(&g_ThreadStru, sizeof(g_ThreadStru), 0, sizeof(g_ThreadStru));
}

void SleSendToDliStub(SlePacketType type, const SlePacket *packet)
{
    if (g_dliCb == NULL || g_dliCb->dliPacketReceived == NULL) {
        return;
    }
    g_dliCb->dliPacketReceived(type, packet);
}

static SlePacket *CreateRePlyEventHeader(uint8_t *data, uint32_t len, uint16_t event)
{
    SlePacket *packet = (SlePacket *)malloc(sizeof(SlePacket));
    if (packet == NULL) {
        return NULL;
    }

    packet->data = (uint8_t*)malloc(len + DLI_EVENT_HEADER_LEN);
    if (packet->data == NULL) {
        free(packet);
        return NULL;
    }

    DLI_ENCODE2BYTE(packet->data, event);
    DLI_ENCODE2BYTE(packet->data + DLI_OPCODE_BITS_LEN, len);
    packet->size = len + DLI_EVENT_HEADER_LEN;
    (void)memcpy_s(&packet->data[DLI_EVENT_HEADER_LEN], len, data, len);
    return packet;
}

static int SleSetReadAdvDataLen(const SlePacket *packet)
{
    (void)packet;
    uint8_t len = DATA_LEN;
    SlePacket *packets = CreateRePlyEventHeader(&len, sizeof(uint8_t), 0x0c06);
    if (packets == NULL) {
        DLI_LOGE("CreateRePlyEventHeader failed");
        return -1;
    }
    SleSendToDliStub(PACKET_TYPE_SLE_EVENT, packets);
    free(packets->data);
    free(packets);
    return 0;
}

static struct ProcessCmdStru g_procCmd[] = {
    {0x0C06, SleSetReadAdvDataLen},
};

#define SLE_CMD_PROCESS_SIZE (sizeof(g_procCmd) / sizeof(struct ProcessCmdStru))

static int SleCmdProcess(const SlePacket *packet)
{
    uint16_t cmd = DECODE2BYTES(packet->data + 1);
    for (uint16_t i = 0; i < SLE_CMD_PROCESS_SIZE; i++) {
        if (g_procCmd[i].cmd == cmd) {
            return g_procCmd[i].ProcessCmd(packet);
        }
    }
    return 0;
}

static void DliSendDataPrint(uint8_t *data, uint32_t len)
{
#define MAX_PRINT_LEN 50 // 50 is max print len
    char dataStr[MAX_PRINT_LEN + MAX_PRINT_LEN + 1] = { 0 };
    int count = 0;
    for (uint32_t i = 0; i < len; i++) {
        (void)sprintf_s(&dataStr[2 * count], (MAX_PRINT_LEN - count) * 2, "%02x", data[i]); // 2 hex char
        if (++count >= MAX_PRINT_LEN - 1) {
            break;
        }
    }
    // 此文件后续会删除，此处仅做dli的数据打印（UT里面使用ERR级别的日志）验证UT里面数据分片信息是否完整.
    DLI_LOGE("len=%u, dli=%s\n", len, dataStr);
}

static void SleAcbFragments(SlePacket *packet, uint8_t pbFlag)
{
    uint8_t *p = packet->data;
    ENCODE2BYTE(p + DATA_LEN_OFFSET, DATA_LEN);
    ENCODE2BYTE(p, pbFlag << PD_BITS_OFFSET);
    DliSendDataPrint(packet->data, packet->size);
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packet);
}

// 连接管理能力查询请求的ack
static int SleConnectCapProcess(const SlePacket *packet)
{
    if (packet->size != DATA_LEN + 1) {
        return 0;
    }

    SlePacket *packets = (SlePacket *)malloc(sizeof(SlePacket));
    if (packets == NULL) {
        return -1;
    }
    uint32_t size = DLI_EVENT_HEADER_LEN + DATA_LEN;
    packets->data = (uint8_t*)malloc(size);
    if (packets->data == NULL) {
        free(packets);
        return -1;
    }
#define DTAP_OFFSET 4
    packets->size = DATA_LEN;
    uint16_t len = DATA_LEN;
    (void)memset_s(packets->data, size, 0, size);

    // dli
    ENCODE2BYTE(packets->data + DATA_LEN_OFFSET, len);
    ENCODE2BYTE(packets->data, FULL_FRAGMENT << PD_BITS_OFFSET);

    // dtap
    uint8_t *data = &packets->data[DTAP_OFFSET];
    *data = 0x02; // tcid 0x02
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET);

    data = &packets->data[DTAP_OFFSET + DTAP_OFFSET];
    // cm 连接管理的head
    *data = 0x02;
    ENCODE2BYTE(data + 2, DTAP_OFFSET);
    // data部分是0
    DliSendDataPrint(packets->data, packets->size);
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packets);
    free(packets->data);
    free(packets);
    return 0;
}

static int SleConnectCapProcess1(const SlePacket *packet)
{
    if (packet->size != DATA_LEN + 1) {
        return 0;
    }
#define DTAP_OFFSET 4
#define DATA_CAP_LEN 20
    SlePacket *packets = (SlePacket *)malloc(sizeof(SlePacket));
    if (packets == NULL) {
        return -1;
    }
    uint32_t size = DATA_CAP_LEN + DATA_LEN;
    packets->data = (uint8_t*)malloc(size);
    if (packets->data == NULL) {
        free(packets);
        return -1;
    }

    packets->size = size;
    uint16_t len = size;
    (void)memset_s(packets->data, size, 0, size);

    // dli
    ENCODE2BYTE(packets->data + DATA_LEN_OFFSET, len);
    ENCODE2BYTE(packets->data, FULL_FRAGMENT << PD_BITS_OFFSET);

    // dtap
    uint8_t *data = &packets->data[DTAP_OFFSET];
    *data = 0x02; // tcid 0x02
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET);

    data = &packets->data[DTAP_OFFSET + DTAP_OFFSET];
    // cm 连接管理的head
    *data = 0x02;
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET - DTAP_OFFSET);
    data = &packets->data[DATA_LEN - DTAP_OFFSET];
    (void)memset_s(data, len - DATA_LEN - DTAP_OFFSET, 1, len - DATA_LEN - DTAP_OFFSET);
    // data部分是0
    DliSendDataPrint(packets->data, packets->size);
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packets);
    free(packets->data);
    free(packets);
    return 0;
}

static int SleConnectCapProcess2(const SlePacket *packet)
{
    if (packet->size != DATA_LEN + 1) {
        return 0;
    }
#define DTAP_OFFSET 4
#define DATA_CAP_LEN 20
    SlePacket *packets = (SlePacket *)malloc(sizeof(SlePacket));
    if (packets == NULL) {
        return -1;
    }
    uint32_t size = DATA_CAP_LEN + DATA_LEN;
    packets->data = (uint8_t*)malloc(size);
    if (packets->data == NULL) {
        free(packets);
        return -1;
    }

    packets->size = size;
    uint16_t len = size;
    (void)memset_s(packets->data, size, 0, size);

    // dli
    ENCODE2BYTE(packets->data + DATA_LEN_OFFSET, len);
    ENCODE2BYTE(packets->data, FULL_FRAGMENT << PD_BITS_OFFSET);

    // dtap
    uint8_t *data = &packets->data[DTAP_OFFSET];
    *data = 0x02; // tcid 0x02
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET);

    data = &packets->data[DTAP_OFFSET + DTAP_OFFSET];
    // cm 连接管理的head
    *data = 0x02;
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET - DTAP_OFFSET);

    // 数据部分
    data = &packets->data[DATA_LEN - DTAP_OFFSET];
    len -= (DATA_LEN - DTAP_OFFSET); // 36 - 12 = 24;
    (void)memset_s(data, len, 0xff, len);
    // data部分是0
    DliSendDataPrint(packets->data, packets->size);
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packets);
    free(packets->data);
    free(packets);
    return 0;
}

static int SleConnectCapProcess3(const SlePacket *packet)
{
    if (packet->size != DATA_LEN + 1) {
        return 0;
    }
#define DTAP_OFFSET 4
#define DATA_CAP_LEN 20
    SlePacket *packets = (SlePacket *)malloc(sizeof(SlePacket));
    if (packets == NULL) {
        return -1;
    }
    uint32_t size = DATA_CAP_LEN + DATA_LEN;
    packets->data = (uint8_t*)malloc(size);
    if (packets->data == NULL) {
        free(packets);
        return -1;
    }

    packets->size = size;
    uint16_t len = size;
    (void)memset_s(packets->data, size, 0, size);

    // dli
    ENCODE2BYTE(packets->data + DATA_LEN_OFFSET, len);
    ENCODE2BYTE(packets->data, FULL_FRAGMENT << PD_BITS_OFFSET);

    // dtap
    uint8_t *data = &packets->data[DTAP_OFFSET];
    *data = 0x02; // tcid 0x02
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET);

    data = &packets->data[DTAP_OFFSET + DTAP_OFFSET];
    // cm 连接管理的head
    *data = 0x02;
    ENCODE2BYTE(data + 2, len - DTAP_OFFSET - DTAP_OFFSET - DTAP_OFFSET);

    // 数据部分
    data = &packets->data[DATA_LEN - DTAP_OFFSET];
    len -= (DATA_LEN - DTAP_OFFSET); // 36 - 12 = 24;
    (void)memset_s(data, len, 0xff, len);

    // trans mode
    ENCODE2BYTE(data + 2, 1);
    // mtu
    ENCODE2BYTE(data + 14, 20);

    // mps
    ENCODE2BYTE(data + 16, 60);

    // version
    ENCODE2BYTE(data + 18, 101);
    // data部分是0
    DliSendDataPrint(packets->data, packets->size);
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packets);
    free(packets->data);
    free(packets);
    return 0;
}

static int SleConnectCapProcess4(const SlePacket *packet)
{
    if (packet->size != DATA_LEN + 1) {
        return 0;
    }
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, packet);
    return 0;
}

// 验证分片重组功能
static int SleAcbProcess(const SlePacket *packet)
{
    // 仅验证数据长度为16的数据，返回分片重组的数据。
    if (packet->size != DATA_LEN) {
        (void)SleConnectCapProcess(packet);
        (void)SleConnectCapProcess2(packet);
        (void)SleConnectCapProcess3(packet);
        (void)SleConnectCapProcess4(packet);
        return SleConnectCapProcess1(packet);
    }

    SlePacket *packets = (SlePacket *)malloc(sizeof(SlePacket));
    if (packets == NULL) {
        return -1;
    }

    uint32_t size = DLI_EVENT_HEADER_LEN + DATA_LEN;
    packets->data = (uint8_t*)malloc(size);
    if (packets->data == NULL) {
        free(packets);
        return -1;
    }
    packets->size = size;
    (void)memset_s(packets->data, size, 0, size);
    (void)memset_s(&packets->data[DLI_EVENT_HEADER_LEN], size - DLI_EVENT_HEADER_LEN,
        1, size - DLI_EVENT_HEADER_LEN);

    // 模拟对端发送数据，本端接收数据，重组数据返回给DTAP
    SleAcbFragments(packets, FULL_FRAGMENT);
    SleAcbFragments(packets, LAST_FRAGMENT);
    SleAcbFragments(packets, MIDDLE_FRAGMENT);
    SleAcbFragments(packets, MIDDLE_FRAGMENT);
    SleAcbFragments(packets, FIRST_FRAGMENT);
    free(packets->data);
    free(packets);
    return 0;
}

static int SleIcbProcess(const SlePacket *packet)
{
    return 0;
}

static inline uint32_t PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    uint32_t ret = SDF_AddWork(g_ThreadStru.worker, cb, arg, freeCb);
    if (ret != 0) {
        DLI_LOGE("SDF_AddWork failed ret %u", ret);
        return ret;
    }

    SDF_EventPost(g_ThreadStru.eventHandle);
    return ret;
}

static void ProcessTask(void *arg)
{
    if (arg == NULL) {
        return;
    }

    int ret = -1;
    SlePacket *packet = (SlePacket *)arg;
    uint8_t dataType = *(packet->data);
    switch (dataType) {
        case PACKET_TYPE_SLE_CMD:
            ret = SleCmdProcess(packet);
            break;
        case PACKET_TYPE_SLE_ICB:
            ret = SleIcbProcess(packet);
            break;
        case PACKET_TYPE_SLE_ACB:
            ret = SleAcbProcess(packet);
            break;
        default:
            break;
    }
    if (ret != 0) {
        DLI_LOGE("dataType %hu, task failed ret %d", dataType, ret);
    }
}

static SlePacket* PacketCreate(const SlePacket *packet)
{
    SlePacket *info = malloc(sizeof(SlePacket));
    if (info == NULL) {
        return NULL;
    }

    DLI_LOGI("PacketCreate size %u", packet->size);
    info->data = malloc(packet->size);
    if (info->data == NULL) {
        free(info);
        return NULL;
    }
    info->size = packet->size;
    (void)memcpy_s(info->data, info->size, packet->data, info->size);
    return info;
}

static void PacketDestroy(void *arg)
{
    if (arg == NULL) {
        return;
    }

    SlePacket *packet = (SlePacket *)arg;
    free(packet->data);
    free(packet);
}

int SleSendDliPacket(const SlePacket *packet)
{
    if (packet == NULL || packet->data == NULL) {
        return -1;
    }

    SlePacket *param = PacketCreate(packet);
    if (param == NULL)  {
        DLI_LOGE("PacketCreate failed");
        return -1;
    }
    if (PostTask(ProcessTask, param, PacketDestroy) != 0) {
        DLI_LOGE("PostTask failed");
        PacketDestroy(param);
        return -1;
    }
    return 0;
}

#endif
#ifdef __cplusplus
}
#endif