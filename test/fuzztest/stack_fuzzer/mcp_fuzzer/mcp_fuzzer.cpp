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

#include "mcp_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nlstk_mcp_media_server.h"
#include "nlstk_mcp_volume_client.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

#define FUZZ_MCP_NAME_MAX_LEN 16
#define FUZZ_MCP_VALUE_BUF_LEN 64
#define FUZZ_MCP_STREAM_VOLUME_MAX_NUM 4

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

static void StartMediaInstImpl(int32_t instanceId, NLSTK_Errcode_E ret)
{
    (void)instanceId;
    (void)ret;
}

static void MediaAuthorizeImpl(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
    NLSTK_ServicePropertyOpType_E operation)
{
    (void)requestId;
    (void)instanceId;
    (void)property;
    (void)operation;
}

static void VolumeChangeEventImpl(SLE_Addr_S *addr, uint8_t volume)
{
    (void)addr;
    (void)volume;
}

static void MuteStatusChangeEventImpl(SLE_Addr_S *addr, uint8_t muteStatus)
{
    (void)addr;
    (void)muteStatus;
}

static void NotifyVolumeChangeImpl(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property, void *value)
{
    (void)addr;
    (void)property;
    (void)value;
}

static void GetVolumeRspImpl(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property, uint8_t errorCode,
    void *value)
{
    (void)addr;
    (void)property;
    (void)errorCode;
    (void)value;
}

static void SetVolumeRspImpl(SLE_Addr_S *addr, uint8_t errorCode)
{
    (void)addr;
    (void)errorCode;
}

static void VolumeStateChangeImpl(SLE_Addr_S *addr, uint8_t state, uint8_t preState)
{
    (void)addr;
    (void)state;
    (void)preState;
}

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

void FuzzMcpCreateMediaInstance(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_McpMediaInfo_S info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    uint8_t nameBuf[FUZZ_MCP_NAME_MAX_LEN] = {0};
    uint16_t nameLen = fdp.ConsumeIntegral<uint8_t>() % FUZZ_MCP_NAME_MAX_LEN;
    for (uint16_t i = 0; i < nameLen; i++) {
        nameBuf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    info.instanceName.len = nameLen;
    info.instanceName.data = nameBuf;
    info.playbackLocation = fdp.ConsumeIntegral<uint32_t>();
    info.playbackState = fdp.ConsumeIntegral<uint8_t>();
    info.mediaInstanceId = fdp.ConsumeIntegral<uint8_t>();
    info.startMediaInst = StartMediaInstImpl;
    info.authorize = MediaAuthorizeImpl;
    for (int i = 0; i < NLSTK_MCP_MEDIA_MAX_PROPERTY; i++) {
        info.propertyRights[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_McpCreateMediaInstance(&info);
}

void FuzzMcpPlayControlResult(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint16_t requestId = fdp.ConsumeIntegral<uint16_t>();
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_McpPlayControlResult(requestId, instanceId, errorCode);
}

void FuzzMcpMediaAuthorizeResult(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint16_t requestId = fdp.ConsumeIntegral<uint16_t>();
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    NLSTK_McpPropertyType_E property = static_cast<NLSTK_McpPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MEDIA_MAX_PROPERTY);
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_McpMediaAuthorizeResult(requestId, instanceId, property, errorCode);
}

void FuzzMcpUpdateMediaProperty(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    NLSTK_McpPropertyType_E property = static_cast<NLSTK_McpPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MEDIA_MAX_PROPERTY);
    uint8_t value[FUZZ_MCP_VALUE_BUF_LEN] = {0};
    for (int i = 0; i < FUZZ_MCP_VALUE_BUF_LEN; i++) {
        value[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_McpUpdateMediaProperty(instanceId, property, value);
}

void FuzzMcpDeleteMediaInstance(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    (void)NLSTK_McpDeleteMediaInstance(instanceId);
}

void FuzzMcpVolumeConnect(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_McpVolumeConnect(&addr);
    (void)NLSTK_McpVolumeDisconnect(&addr);
}

void FuzzMcpGetVolume(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_McpVolumePropertyType_E property = static_cast<NLSTK_McpVolumePropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MAX_PROPERTY);
    (void)NLSTK_McpGetVolume(&addr, property);
}

void FuzzMcpSetVolume(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    uint8_t volume = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_McpSetVolume(&addr, volume);
}

void FuzzMcpSetStreamVolume(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    uint8_t num = fdp.ConsumeIntegral<uint8_t>() % FUZZ_MCP_STREAM_VOLUME_MAX_NUM;
    NLSTK_McpSetStreamVolume_S volumeArray[FUZZ_MCP_STREAM_VOLUME_MAX_NUM] = {0};
    for (uint8_t i = 0; i < num; i++) {
        volumeArray[i].volume = fdp.ConsumeIntegral<uint8_t>();
        volumeArray[i].streamType = static_cast<NLSTK_McpSetStreamVolume_E>(fdp.ConsumeIntegral<uint8_t>() % 2);
    }
    (void)NLSTK_McpSetStreamVolume(&addr, volumeArray, num);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    NLSTK_McpVolumeClientCallBack_S callback = {0};
    callback.volumeChangeEvent = OHOS::VolumeChangeEventImpl;
    callback.muteStatusChangeEvent = OHOS::MuteStatusChangeEventImpl;
    callback.notifyVolumeChange = OHOS::NotifyVolumeChangeImpl;
    callback.getVolumeRsp = OHOS::GetVolumeRspImpl;
    callback.setVolumeRsp = OHOS::SetVolumeRspImpl;
    callback.stateChange = OHOS::VolumeStateChangeImpl;
    (void)NLSTK_McpRegVolumeClientCbk(&callback);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzMcpCreateMediaInstance(fuzzData, size);
    OHOS::FuzzMcpPlayControlResult(fuzzData, size);
    OHOS::FuzzMcpMediaAuthorizeResult(fuzzData, size);
    OHOS::FuzzMcpUpdateMediaProperty(fuzzData, size);
    OHOS::FuzzMcpDeleteMediaInstance(fuzzData, size);
    OHOS::FuzzMcpVolumeConnect(fuzzData, size);
    OHOS::FuzzMcpGetVolume(fuzzData, size);
    OHOS::FuzzMcpSetVolume(fuzzData, size);
    OHOS::FuzzMcpSetStreamVolume(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
