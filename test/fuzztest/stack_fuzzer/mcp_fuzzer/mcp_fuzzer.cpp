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

static void MuteStatusChangeEventImpl(SLE_Addr_S *addr, uint8_t mute)
{
    (void)addr;
    (void)mute;
}

static void NotifyVolumeChangeImpl(SLE_Addr_S *addr, uint8_t volume)
{
    (void)addr;
    (void)volume;
}

static void GetVolumeRspImpl(SLE_Addr_S *addr, uint8_t volume, NLSTK_Errcode_E ret)
{
    (void)addr;
    (void)volume;
    (void)ret;
}

static void SetVolumeRspImpl(SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    (void)addr;
    (void)ret;
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

static uint16_t FillBytes(FuzzedDataProvider &fdp, uint8_t *buf, uint16_t maxLen)
{
    uint16_t len = fdp.ConsumeIntegral<uint8_t>() % maxLen;
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    return len;
}

/* 按合法 property 构造对应类型的载荷，供创建后的实例更新复用 */
static void FillPropertyPayloadAndApply(FuzzedDataProvider &fdp, int32_t instanceId,
    NLSTK_McpPropertyType_E property)
{
    uint8_t byteBuf[FUZZ_MCP_NAME_MAX_LEN] = {0};
    switch (property) {
        case NLSTK_MCP_MEDIA_INSTANCE_NAME:
        case NLSTK_MCP_MEDIA_EXTENDED_INFO:
        case NLSTK_MCP_MEDIA_SEGMENT_INFO: {
            NLSTK_VariableData_S var = {0};
            var.len = FillBytes(fdp, byteBuf, FUZZ_MCP_NAME_MAX_LEN);
            var.data = byteBuf;
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &var);
            break;
        }
        case NLSTK_MCP_MEDIA_INSTANCE_ICON: {
            NLSTK_McpInstanceIcon_S icon = {0};
            icon.iconType = fdp.ConsumeIntegral<uint8_t>();
            icon.iconLen = FillBytes(fdp, byteBuf, FUZZ_MCP_NAME_MAX_LEN);
            icon.iconValue = byteBuf;
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &icon);
            break;
        }
        case NLSTK_MCP_MEDIA_BASIC_INFO: {
            NLSTK_McpMediaBaseInfo_S baseInfo = {0};
            baseInfo.mediaType = fdp.ConsumeIntegral<uint8_t>();
            baseInfo.duration = fdp.ConsumeIntegral<uint32_t>();
            baseInfo.mediaNameLen = FillBytes(fdp, byteBuf, FUZZ_MCP_NAME_MAX_LEN);
            baseInfo.mediaName = byteBuf;
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &baseInfo);
            break;
        }
        case NLSTK_MCP_MEDIA_IDENTIFIER_INFO: {
            NLSTK_McpMediaIdInfo_S idInfo = {0};
            idInfo.mediaId = fdp.ConsumeIntegral<uint32_t>();
            idInfo.mediaGroupId = fdp.ConsumeIntegral<uint32_t>();
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &idInfo);
            break;
        }
        case NLSTK_MCP_MEDIA_PLAYBACK_POSITION: {
            uint32_t position = fdp.ConsumeIntegral<uint32_t>();
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &position);
            break;
        }
        case NLSTK_MCP_PLAYBACK_SPEED:
        case NLSTK_MCP_SEEK_SPEED: {
            uint16_t speed = fdp.ConsumeIntegral<uint16_t>();
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &speed);
            break;
        }
        case NLSTK_MCP_FEATURE_SUPPORT: {
            NLSTK_McpFeaturesSupported_S features = {0};
            features.FeatureType = fdp.ConsumeIntegral<uint8_t>();
            features.playCtl = fdp.ConsumeIntegral<uint32_t>();
            features.playMode = fdp.ConsumeIntegral<uint16_t>();
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &features);
            break;
        }
        case NLSTK_MCP_PLAYBACK_ORDER:
        case NLSTK_MCP_PLAYBACK_STATE:
        case NLSTK_MCP_MEDIA_INSTANCE_ID: {
            uint8_t value = fdp.ConsumeIntegral<uint8_t>();
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, &value);
            break;
        }
        default:
            (void)NLSTK_McpUpdateMediaProperty(instanceId, property, nullptr);
            break;
    }
}

/* 单输入内状态一致性：instanceId 由调用方指定并贯穿 创建→播放结果→授权结果→
 * 属性更新→删除；音量侧同地址 Connect→Get/Set→SetStream→Disconnect。 */
void FuzzMcpConsistentSequence(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);

    /* 媒体实例：type 固定合法值，instanceId 保证非 0 */
    NLSTK_McpMediaInfo_S info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    uint8_t nameBuf[FUZZ_MCP_NAME_MAX_LEN] = {0};
    uint16_t nameLen = fdp.ConsumeIntegral<uint8_t>() % FUZZ_MCP_NAME_MAX_LEN;
    for (uint16_t i = 0; i < nameLen; i++) {
        nameBuf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    info.type = NLSTK_MCP_COMMON_SERVICE;
    info.instanceName.len = nameLen;
    info.instanceName.data = nameBuf;
    info.playbackLocation = fdp.ConsumeIntegral<uint32_t>();
    info.playbackState = fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MEDIA_MAX_PROPERTY;
    info.mediaInstanceId = static_cast<int32_t>(1 + fdp.ConsumeIntegral<uint8_t>() % 255);
    info.startMediaInst = StartMediaInstImpl;
    info.authorize = MediaAuthorizeImpl;
    for (int i = 0; i < NLSTK_MCP_MEDIA_MAX_PROPERTY; i++) {
        info.propertyRights[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    int32_t instanceId = info.mediaInstanceId;
    uint16_t requestId = fdp.ConsumeIntegral<uint16_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    NLSTK_McpPropertyType_E authorizeProperty = static_cast<NLSTK_McpPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MEDIA_MAX_PROPERTY);
    NLSTK_McpPropertyType_E updateProperty = static_cast<NLSTK_McpPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_MCP_MEDIA_MAX_PROPERTY);

    (void)NLSTK_McpCreateMediaInstance(&info);
    (void)NLSTK_McpPlayControlResult(requestId, instanceId, errorCode);
    (void)NLSTK_McpMediaAuthorizeResult(requestId, instanceId, authorizeProperty, errorCode);
    FillPropertyPayloadAndApply(fdp, instanceId, updateProperty);
    (void)NLSTK_McpDeleteMediaInstance(instanceId);

    /* 音量：同一地址上先建链再查询/设置，最后确定性断开 */
    SLE_Addr_S volAddr = {0};
    FillAddr(fdp, volAddr);
    uint8_t volume = fdp.ConsumeIntegral<uint8_t>();
    NLSTK_McpVolumePropertyType_E volumeProperty =
        (fdp.ConsumeIntegral<uint8_t>() % 2 == 0) ? NLSTK_MCP_VOLUME_STATUS
                                                  : NLSTK_MCP_STREAM_VOLUME_STATUS;
    uint8_t num = fdp.ConsumeIntegral<uint8_t>() % FUZZ_MCP_STREAM_VOLUME_MAX_NUM;
    NLSTK_McpSetStreamVolume_S volumeArray[FUZZ_MCP_STREAM_VOLUME_MAX_NUM] = {};
    for (uint8_t i = 0; i < num; i++) {
        volumeArray[i].volume = fdp.ConsumeIntegral<uint8_t>();
        volumeArray[i].streamType = static_cast<NLSTK_McpSetStreamVolume_E>(fdp.ConsumeIntegral<uint8_t>());
    }
    (void)NLSTK_McpVolumeConnect(&volAddr);
    (void)NLSTK_McpGetVolume(&volAddr, volumeProperty);
    (void)NLSTK_McpSetVolume(&volAddr, volume);
    (void)NLSTK_McpSetStreamVolume(&volAddr, volumeArray, num);
    (void)NLSTK_McpVolumeDisconnect(&volAddr);

    /* 保留非法参数路径：instanceId=0（产品侧判非法）与断开后的查询 */
    (void)NLSTK_McpUpdateMediaProperty(0, updateProperty, nullptr);
    (void)NLSTK_McpGetVolume(&volAddr, volumeProperty);
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
    OHOS::FuzzMcpConsistentSequence(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
