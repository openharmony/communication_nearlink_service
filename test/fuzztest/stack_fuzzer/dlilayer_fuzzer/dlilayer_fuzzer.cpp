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

#include "fuzzer/FuzzedDataProvider.h"
#include "dlilayer_fuzzer.h"
#include "securec.h"

#include "dli_layer.h"
#include "dli_layer_stru.h"
#include "dli_layer_config.h"
#include "dli_thread.h"
#include "dli_layer_utils.h"
#include "dli_layer_callback.h"
#include "dli_callback.h"
#include "sdf_worker.h"
#include "dli_def.h"
#include "dli_event.h"
#include "dli_event_struct.h"
#include "dli_log.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "sdf_buff.h"
#include "dli_sapi.h"
#include "SleDliLayerAdapter.h"


#define DLI_TEST_SEND_MAX_LEN 100
static SleDliCallbackFunc g_func = {NULL, NULL};
extern "C" int SleHalInit(SleDliCallbackFunc *func)
{
    static int count = -1;
    if (count == -1 || count == 0) {
        count++;
        return count;
    }
    if (func != NULL && func->initializationComplete != NULL) {
        func->initializationComplete(SUCCESS);
        g_func = *func;
    }
    count++;
    return 0;
}

extern "C" void SleReset()
{
    return;
}

// send to drive
extern "C" int SleSendDliPacket(const SlePacket *packet)
{
    if (packet == NULL || packet->size > DLI_TEST_SEND_MAX_LEN) {
        return -1;
    }
    return 0;
}

extern "C" void SleHalClose(void)
{
}

namespace OHOS {
/* PR#331 编译修复（二轮）：定义在调用点之后，OHOS 作用域内补前向声明 */
void FuzzDliLayerFragmentSeq(uint8_t* data, size_t size);

    const uint16_t MIN_ACB_LEN = 3;
    const uint16_t TEST_DEFAULT_NUM = 10;
    const uint32_t DLI_FUZZ_TEST_SLEEP_TIME = 100;
    /* DLI_SetCallback 三必选回调的同步桩：本工程 sources 无 cp_worker.c，直接执行
     * 任务与释放（照 hadm_fuzzer SchedulePostTask 同步 mock 模式）。注册后
     * DLI_PacketReceived 的 ACB/EVENT 包分发（DLI_PostOtherThread/DLI_PostTask）
     * 才能真正进入重组/命令节点路径（此前全进程未注册，全部空转）。 */
    static uint32_t FuzzDliPostOtherThreadSync(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
    {
        if (cb != NULL) {
            cb(arg);
        }
        if (freeCb != NULL) {
            freeCb(arg);
        }
        return 0;
    }

    static uint32_t FuzzDliPostOtherBlockedThreadSync(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb,
        int timeout)
    {
        (void)timeout;
        return FuzzDliPostOtherThreadSync(cb, arg, freeCb);
    }

    void FuzzWriteLog(uint16_t type, const uint8_t *data, uint32_t len, int result)
    {
    }

    void FuzzDlilayRecvData(uint8_t *data, uint32_t len)
    {
        SlePacket packet = {data, len};
        if (g_func.dliPacketReceived != NULL) {
            g_func.dliPacketReceived((SlePacketType)*data, &packet);
        }
    }

    void FuzzDliAcbRecvHander(uint16_t lcid, SDF_Buff_S *buf)
    {
        (void)lcid;
        SDF_BuffFree(buf); /* 投递后所有权移交消费者 */
    }

    void FuzzDliRecvEventHandler(uint16_t event, void *context, const uint8_t *data, uint32_t len)
    {
        (void)event;
        (void)context;
        (void)data;
        (void)len;
    }

    void FuzzDliWriteFileHandler(uint16_t type, const uint8_t *data, uint32_t len, int result)
    {
       (void)type;
       (void)data;
       (void)len;
       (void)result;
    }

    void FuzzDliLayerApi(uint8_t* data, size_t size)
    {
        DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(1, 1, data, size);
        DLI_CmdSend(cmd);
        DLI_CmdSend(NULL);
        DLI_PostNextTask(DLI_CMD_TASK);
        DLI_CmdNumSet(1);
        DLI_PostNextTask(DLI_CMD_TASK);
        SDF_ThreadSleep(DLI_FUZZ_TEST_SLEEP_TIME);

        SDF_Buff_S *buf = SDF_BuffNewWithReserve(size);
        (void)memcpy_s(SDF_BuffAppend(buf, size), SDF_BuffTailRoom(buf), data, size);
        DLI_DataStru *dataStru = DLI_DefaultDataStruCreate(0, DLI_DATATYPE_ACB, 0, 0, buf);
        uint16_t len = size < MIN_ACB_LEN ? size : size / MIN_ACB_LEN;
        DLI_AllDataSet(len, OHOS::TEST_DEFAULT_NUM, 0, 0);
        DLI_DataSend(dataStru);
        SDF_ThreadSleep(DLI_FUZZ_TEST_SLEEP_TIME);
        FuzzDlilayRecvData(data, size);
        SDF_ThreadSleep(DLI_FUZZ_TEST_SLEEP_TIME);
        uint8_t tmp[] = {0x00, 0x20, 0x10, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
        FuzzDlilayRecvData(tmp, sizeof(tmp));
    }

    void FuzzDliLayerCallbackApi(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
        DLI_AcbRecvHander(lcid, NULL);
        DLI_AcbRecvHander(lcid, NULL);

        DLI_SetRecvEventCallback(NULL);
        DLI_EventRecvHandler(0, NULL, data, size);
        DLI_SetRecvEventCallback(FuzzDliRecvEventHandler);
        DLI_EventRecvHandler(0, NULL, data, size);
    }

    void FuzzDliLayerUtilsApi(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t cmdIn = provider.ConsumeIntegral<uint16_t>();
        uint16_t event = provider.ConsumeIntegral<uint16_t>();
        DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(cmdIn, event, data, size);
        DLI_CmdTxNode *node = DLI_CmdNodeCreate(cmd);
        DLI_CmdNodeDestroy((void *)node);

        uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
        uint16_t type = provider.ConsumeIntegral<uint16_t>();
        uint8_t ts = provider.ConsumeIntegral<uint8_t>();
        uint8_t prio = provider.ConsumeIntegral<uint8_t>();

        DLI_DataStru *dataInfo = DLI_DefaultDataStruCreate(lcid, type, ts, prio, NULL);
        DLI_DataStruDestroy(dataInfo);

        uint16_t bufSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> bufData = provider.ConsumeBytes<uint8_t>(bufSize);
        SDF_Buff_S *buf = SDF_BuffNew(bufData.size());
        uint8_t *recvData = SDF_BuffAppend(buf, bufData.size());
        if (buf == NULL || recvData == NULL) {
            SDF_BuffFree(buf);
            DLI_LOGE("DLI_PacketReceived SDF_BuffNew error");
            return;
        }
        (void)memcpy_s(recvData, SDF_BuffLenGet(buf), bufData.data(), bufData.size());

        uint16_t pb = provider.ConsumeIntegral<uint16_t>();
        DLI_RecvDataNode *recvDataNode = DLI_RecvDataNodeCreate(lcid, pb, buf);
        (void)DLI_ReciveDataUpdate(recvDataNode, lcid, pb, buf);
        DLI_RecvDataNodeDestroy(recvDataNode);
    }

    void FuzzDliLayerConfigApi(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t acbLen = provider.ConsumeIntegral<uint16_t>();
        uint16_t acbNum = provider.ConsumeIntegral<uint16_t>();
        uint16_t icbLen = provider.ConsumeIntegral<uint16_t>();
        uint16_t icbNum = provider.ConsumeIntegral<uint16_t>();
        DLI_AllDataSet(acbLen, acbNum, icbLen, icbNum);

        uint8_t tmpSize = provider.ConsumeIntegral<uint8_t>();
        std::vector<uint8_t> tmpData = provider.ConsumeBytes<uint8_t>(tmpSize);
        uint8_t tmp[TEST_DEFAULT_NUM] = {0};
        (void)memcpy_s(tmp, sizeof(tmp),
            tmpData.data(), tmpData.size() > TEST_DEFAULT_NUM ? TEST_DEFAULT_NUM : tmpData.size());

        DLI_DataType type = static_cast<DLI_DataType>(provider.ConsumeIntegral<uint8_t>());
        DLI_DataNumGet(type);
        DLI_DataLenGet(type);
    }

    void FuzzDliLayerAllApi(uint8_t* data, size_t size)
    {
        DLI_LOGE("FuzzDliLayerAllApi enter");
        DLI_LayerInit();
        DLI_LayerEnable();
        DLI_Callback dliCb = {0};
        dliCb.postOtherThread = OHOS::FuzzDliPostOtherThreadSync;
        dliCb.postOtherBlockedThread = OHOS::FuzzDliPostOtherBlockedThreadSync;
        dliCb.recvAcbHandler = OHOS::FuzzDliAcbRecvHander;
        (void)DLI_SetCallback(&dliCb);
        FuzzDliLayerApi(data, size);
        FuzzDliLayerCallbackApi(data, size);
        FuzzDliLayerFragmentSeq(data, size);
        FuzzDliLayerUtilsApi(data, size);
        FuzzDliLayerConfigApi(data, size);
        DLI_LayerDisable();
        DLI_LayerDeinit();
        DLI_LOGE("FuzzDliLayerAllApi exit");
    }

    /* FUZZ-012: 分片重组序列——同 lcid 连续 FIRST/MIDDLE/LAST 真实帧投递
     * DLI_AcbRecvHander（原 buf 恒 NULL，重组状态机浅触）；超 FragmentMaxLen
     * 的 DataSend 触发 DLI_SplitData 真实分片；结构化 CMD_STATUS 回执触发
     * 命令节点回收（dli_layer.c:597 CmdNodeRemove 路径）。 */
    void FuzzDliLayerFragmentSeq(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
        uint8_t fragType = provider.ConsumeIntegral<uint8_t>();
        uint16_t cmdOpcode = provider.ConsumeIntegral<uint16_t>(); /* EVENT 帧回执 opcode */

        /* ACB 帧走真实收包入口 g_func.dliPacketReceived（SleHalInit 桩保存的
         * DLI_PacketReceived）：解析帧头 → AcbPacketReceived → 重组状态机
         * DLI_ReceiveDataNodeUpdate（dli_layer.c:472/:475）→ 投递 recvAcbHandler。 */
        static const DLI_PbFlag pbStages[] = {
            DLI_FIRST_FRAGMENT, DLI_MIDDLE_FRAGMENT, DLI_LAST_FRAGMENT,
        };
        for (size_t s = 0; s < sizeof(pbStages) / sizeof(pbStages[0]); s++) {
            uint16_t payloadLen = (uint16_t)(provider.ConsumeIntegral<uint8_t>() + 1);
            uint8_t frame[UINT8_MAX + 1 + DLI_HEADER] = {0}; /* PR331 修复：payloadLen≤256 另有 DLI_HEADER 帧头 */
            uint32_t frameLen = (uint32_t)(DLI_HEADER + payloadLen);
            /* 帧格式照 DLI_SplitData 写入：type(1) + lcid|pb<<12|ts<<14|prio<<15(2)
             * + length(2)，DLI_ENCODE2BYTE 为小端。 */
            frame[0] = fragType;
            uint16_t flags = (uint16_t)(lcid | ((uint16_t)pbStages[s] << 12));
            DLI_ENCODE2BYTE(frame + 1, flags);
            DLI_ENCODE2BYTE(frame + 3, payloadLen);
            for (uint16_t i = 0; i < payloadLen; i++) {
                frame[DLI_HEADER + i] = provider.ConsumeIntegral<uint8_t>();
            }
            if (g_func.dliPacketReceived != NULL) {
                SlePacket packet = {frame, frameLen};
                g_func.dliPacketReceived(static_cast<SlePacketType>(DLI_DATATYPE_ACB), &packet);
            }
        }

        /* EVENT 帧（STATUS/COMPLETE）同入口 → DLI_EventPacketReceived →
         * DLI_CmdNodeRemove（dli_layer.c:597，红蓝对抗确认唯一真实触发路径）：
         * 前 2 字节事件码 + 4 字节头 + 2 字节 opcode。 */
        if (g_func.dliPacketReceived != NULL) {
            uint8_t evtFrame[DLI_HEADER + 2] = {0};
            DLI_ENCODE2BYTE(evtFrame, DLI_CMD_STATUS_EVT);
            DLI_ENCODE2BYTE(evtFrame + DLI_HEADER, cmdOpcode);
            SlePacket evtPacket = {evtFrame, (uint32_t)sizeof(evtFrame)};
            g_func.dliPacketReceived(static_cast<SlePacketType>(DLI_DATATYPE_EVENT), &evtPacket);
        }

        /* SplitData 直调（公开 API，dli_layer.h:61；红蓝对抗确认 DataSend 链不触发
         * 分片，全仓唯一直调点在 dtap_scheduler——本工程自行构造 fragmentBuf 驱动）。
         * buf 由 DLI_DataStruDestroy 释放，fragmentBuf 自行释放。 */
        uint16_t bigLen = (uint16_t)(DLI_GetFragmentMaxLen() + DLI_HEADER + 64);
        SDF_Buff_S *bigBuf = SDF_BuffNewWithReserve(bigLen);
        if (bigBuf != nullptr) {
            uint8_t *bp = SDF_BuffAppend(bigBuf, bigLen);
            if (bp != nullptr) {
                for (uint16_t i = 0; i < bigLen; i++) {
                    bp[i] = provider.ConsumeIntegral<uint8_t>();
                }
                SDF_Buff_S *fragBufs[2] = {
                    SDF_BuffNewWithReserve(DLI_GetFragmentMaxLen() + DLI_HEADER),
                    SDF_BuffNewWithReserve(DLI_GetFragmentMaxLen() + DLI_HEADER),
                };
                DLI_DataStru *bigData = DLI_DefaultDataStruCreate(lcid, DLI_DATATYPE_ACB, 0, 0, bigBuf);
                if (bigData != nullptr) {
                    if (fragBufs[0] != nullptr && fragBufs[1] != nullptr) {
                        (void)DLI_SplitData(bigData, fragBufs, 2);
                    }
                    DLI_DataStruDestroy(bigData);
                } else {
                    SDF_BuffFree(bigBuf);
                }
                SDF_BuffFree(fragBufs[0]);
                SDF_BuffFree(fragBufs[1]);
            } else {
                SDF_BuffFree(bigBuf);
            }
        }
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    /* staking builds starburst connection */

    (void)SDF_ThreadInit(OHOS::TEST_DEFAULT_NUM);
    (void)SDF_EvcInit();
    DLI_LOGE("dlilayer_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzDliLayerAllApi(static_cast<uint8_t *>(fuzzData), size);
    free(fuzzData);
    return 0;
}
