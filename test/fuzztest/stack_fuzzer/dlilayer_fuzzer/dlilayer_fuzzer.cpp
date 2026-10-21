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
    const uint16_t MIN_ACB_LEN = 3;
    const uint16_t TEST_DEFAULT_NUM = 10;
    const uint32_t DLI_FUZZ_TEST_SLEEP_TIME = 100;
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
        (void)buf;
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

        static const DLI_PbFlag pbStages[] = {
            DLI_FIRST_FRAGMENT, DLI_MIDDLE_FRAGMENT, DLI_LAST_FRAGMENT,
        };
        for (size_t s = 0; s < sizeof(pbStages) / sizeof(pbStages[0]); s++) {
            uint16_t payloadLen = (uint16_t)(provider.ConsumeIntegral<uint8_t>() + 1);
            SDF_Buff_S *buf = SDF_BuffNewWithReserve((uint32_t)(DLI_HEADER + payloadLen));
            if (buf == nullptr) {
                continue;
            }
            uint8_t *p = SDF_BuffAppend(buf, (uint32_t)(DLI_HEADER + payloadLen));
            if (p == nullptr) {
                SDF_BuffFree(buf);
                continue;
            }
            /* 帧头照 DLI_SplitData 写入格式：type(1) + lcid|pb<<12|ts<<14|prio<<15(2)
             * + length(2)，DLI_ENCODE2BYTE 为小端。 */
            p[0] = fragType;
            uint16_t flags = (uint16_t)(lcid | ((uint16_t)pbStages[s] << 12));
            DLI_ENCODE2BYTE(p + 1, flags);
            DLI_ENCODE2BYTE(p + 3, payloadLen);
            for (uint16_t i = 0; i < payloadLen; i++) {
                p[DLI_HEADER + i] = provider.ConsumeIntegral<uint8_t>();
            }
            DLI_AcbRecvHander(lcid, buf);
            SDF_BuffFree(buf);
        }

        /* 超限 DataSend → SplitData 真实分片（SleSendDliPacket 桩 >100B 拒发，
         * 分片与 DLI_TrySend 重试路径均已执行；buff 由 DataSendInner 接管）。 */
        uint16_t bigLen = (uint16_t)(DLI_GetFragmentMaxLen() + DLI_HEADER + 64);
        SDF_Buff_S *bigBuf = SDF_BuffNewWithReserve(bigLen);
        if (bigBuf != nullptr) {
            uint8_t *bp = SDF_BuffAppend(bigBuf, bigLen);
            if (bp != nullptr) {
                for (uint16_t i = 0; i < bigLen; i++) {
                    bp[i] = provider.ConsumeIntegral<uint8_t>();
                }
                DLI_DataStru *bigData = DLI_DefaultDataStruCreate(lcid, DLI_DATATYPE_ACB, 0, 0, bigBuf);
                if (bigData != nullptr) {
                    (void)DLI_DataSend(bigData);
                    SDF_ThreadSleep(DLI_FUZZ_TEST_SLEEP_TIME);
                } else {
                    SDF_BuffFree(bigBuf);
                }
            } else {
                SDF_BuffFree(bigBuf);
            }
        }

        /* 命令节点回收：发送命令后喂结构化 CMD_STATUS（opcode 对齐所发命令）。 */
        uint16_t cmdOpcode = provider.ConsumeIntegral<uint16_t>();
        DLI_CmdStru *cmd = DLI_DefaultCmdStruCreate(cmdOpcode, 0, data, size);
        if (cmd != nullptr) {
            (void)DLI_CmdSend(cmd);
            SDF_ThreadSleep(DLI_FUZZ_TEST_SLEEP_TIME);
            DLI_CommandStatus evt = {0};
            evt.cmdOpcode = cmdOpcode;
            evt.numDliCommandPackets = 1;
            evt.status = 0;
            RecvEventHandler(DLI_CMD_STATUS_EVT, nullptr, (uint8_t *)&evt, sizeof(evt));
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
