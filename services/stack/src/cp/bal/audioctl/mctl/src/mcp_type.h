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
#ifndef MCP_TYPE_H
#define MCP_TYPE_H

#include <stdbool.h>
#include "sdf_vector.h"
#include "nlstk_public_define.h"
#include "nlstk_mcp_media_server.h"
#include "nlstk_mcp_volume_client.h"

#ifdef __cplusplus
extern "C" {
#endif

// 媒体控制相关
#define MCP_INVALID_UUID 0
#define MCP_INVALID_APP_ID (-1)
#define MCP_MEDIA_METHOD_NUM 1
#define MCP_MEDIA_REQUIRED_PROPERTY_NUM 6
#define MCP_READ_PERMISSION 0x10
#define MCP_MEDIA_ICON_TYPE_LEN 1
#define MCP_MEDIA_TYPE_LEN 1
#define MCP_MEDIA_DURATION_LEN 4
#define MCP_MEDIA_NAME_LEN_OFFSET 5
#define MCP_MEDIA_NAME_OFFSET 6
#define MCP_CTRL_FEATURES_OFFSET 1
#define MCP_MODEL_FEATURES_OFFSET 5
#define MCP_FEATURE_SUPPORT_LEN 7
#define MCP_DESC_CLIENT_CONFIG_LEN 2
#define MCP_MEDIA_METHOD_RES_LEN 2
#define MCP_MEDIA_INVALID_OPCODE 0

// 音量控制相关
#define MCP_VOLUME_MUTE_PARAM_LEN 3
#define MCP_VOLUME_CHANGE_PARAM_LEN 2
#define MCP_VOLUME_SET_PARAM_LEN 4
#define MCP_STREAM_VOLUME_SET_MAX_NUM 2
#define MCP_STREAM_VOLUME_SET_BASE_LEN 3
#define MCP_STREAM_VOLUME_SET_EXTRA_LEN 2
#define MCP_VOLUME_STATUS_LEN 4
#define MCP_VOLUME_EVENT_PARAM_LEN 2
#define MCP_VOLUME_CHANGE_EVENT 1
#define MCP_MUTE_CHANGE_EVENT 2
#define MCP_MUTIL_CHANNEL_LEN 4
#define MCP_RECT_ARRAY_LEN 9
#define MCP_RING_ARRAY_LEN 3
#define MCP_VOLUME_CALL_RES_MIN_LEN 2
#define MCP_VOLUME_CALL_RES_CODE_POS 1
#define MCP_VOLUME_OPCODE_POS 0
#define MCP_VOLUME_CHANGE_ID_POS 1
#define MCP_VOLUME_SET_CHANGE_ID_POS 2
#define MCP_VOLUME_REQ_MAX_SIZE 4
#define MCP_STREAM_VOLUME_BASE_LEN 2
#define MCP_STREAM_VOLUME_EXTRA_LEN 3
#define MCP_VOLUME_TIME_OUT 3000
#define MCP_TIMER_NO_USED_HANDLE (-1)
#define MCP_VOLUME_LOG_INFO_LEN 200
#define MCP_VOLUME_LOG_PER_ITEM_LEN 100

// <<媒体播放控制>>服务通用唯一标识
typedef enum {
    MCP_COMMON_MEDIA_SERVICE_UUID = 0x0614,
    MCP_MEDIA_SERVICE_UUID = 0x0615,
    MCP_MEDIA_INSTANCE_NAME_UUID = 0x1063,
    MCP_MEDIA_INSTANCE_ICON_UUID = 0x1064,
    MCP_MEDIA_BASIC_INFO_UUID = 0x1065,
    MCP_MEDIA_EXTENDED_INFO_UUID = 0x1066,
    MCP_MEDIA_IDENTIFIER_INFO_UUID = 0x1067,
    MCP_MEDIA_PLAYBACK_POSITION_UUID = 0x1068,
    MCP_MEDIA_SEGMENT_INFO_UUID = 0x1069,
    MCP_PLAYBACK_SPEED_UUID = 0x106A,
    MCP_SEEK_SPEED_UUID = 0x106B,
    MCP_FEATURE_SUPPORT_UUID = 0x106C,
    MCP_PLAYBACK_ORDER_UUID = 0x106D,
    MCP_PLAYBACK_STATE_UUID = 0x106E,
    MCP_MEDIA_INSTANCE_ID_UUID = 0x106F,
    MCP_MEDIA_PLAYBACK_CONTROL_POINT_UUID = 0x1070,
    MCP_MEDIA_BROWSER_CONTROL_POINT_UUID = 0x1071,
} McpMediaUuid_E;

// <<音量控制服务>>通用唯一标识
typedef enum {
    MCP_VOLUME_SERVICE_UUID = 0x0616,
    MCP_EFFECTIVE_TIME_SUPPORT_UUID = 0x1072,
    MCP_VOLUME_STATUS_UUID = 0x1073,
    MCP_CHANNEL_VOLUME_STATUS_UUID = 0x1074,
    MCP_VOLUME_CONTROL_POINT_UUID = 0x1075,
    MCP_VOLUME_SYNC_EVENT_UUID = 0x1076,
    MCP_STREAM_VOLUME_STATUS_UUID = 0x107B,
} McpVolumeUuid_E;

// 媒体播放控制点操作码
typedef enum {
    MCP_PLAY_OPCODE = 0x01,
    MCP_STOP_OPCODE = 0x02,
    MCP_PAUSE_OPCODE = 0x03,
    MCP_FAST_FORWARD_OPCODE = 0x04,
    MCP_REWIND_OPCODE = 0x05,
    MCP_MEDIA_POSITION_MOVE_OPCODE = 0x10,
    MCP_MEDIA_POSITION_RELATIVE_MOVE_OPCODE = 0x11,
    MCP_PREVIOUS_MEDIA_SEGMENT_OPCODE = 0x20,
    MCP_NEXT_MEDIA_SEGMENT_OPCODE = 0x21,
    MCP_MEDIA_SEGMENT_JUMP_OPCODE = 0x22,
    MCP_PREVIOUS_MEDIA_OPCODE = 0x30,
    MCP_NEXT_MEDIA_OPCODE = 0x31,
    MCP_MEDIA_JUMP_OPCODE = 0x32,
    MCP_PREVIOUS_MEDIA_GROUP_OPCODE = 0x40,
    MCP_NEXT_MEDIA_GROUP_OPCODE = 0x41,
    MCP_MEDIA_GROUP_JUMP_OPCODE = 0x42,
    MCP_PLAYBACK_ORDER_CHANGE_OPCODE = 0x50,
    MCP_PLAYBACK_SPEED_CHANGE_OPCODE = 0x51,
    MCP_OPERATION_RESERVED_OPCODE = 0x52,
} McpPlayControlOpCode_E;

// 音量控制点操作码
typedef enum {
    MCP_VOLUME_MUTE_OPCODE = 0x01,
    MCP_VOLUME_UNMUTE_OPCODE,
    MCP_INCREASE_FIRST_VOLUME_OPCODE,
    MCP_DECREASE_FIRST_VOLUME_OPCODE,
    MCP_INCREASE_SECOND_VOLUME_OPCODE,
    MCP_DECREASE_SECOND_VOLUME_OPCODE,
    MCP_SET_VOLUME_OPCODE,
    MCP_SET_CHANNEL_VOLUME_OPCODE,
    MCP_SET_STREAM_VOLUME_OPCODE,
} McpVolumeControlOpCode_E;

// 时间类型
typedef enum {
    MCP_VOLUME_WORK_IMMEDIATELY = 0x00,
    MCP_VOLUME_SLE_CLOCK = 0x01,
} McpVolumeTimeType_E;

// 媒体控制服务端结构体
typedef struct {
    uint16_t handle;
    uint16_t uuid;
    NLSTK_McpPropertyType_E type;
    NLSTK_VariableData_S value;   // 当前不保存具体值，暂不使用
} McpMediaPropertyCache_S;

typedef struct {
    uint16_t srvHandle;
    uint16_t endHandle;
    uint16_t uuid;
    uint16_t playControlHandle;
    uint16_t browseControlHandle;
    SDF_Vector_S *properties;
} McpMediaServiceCache_S;

typedef struct {
    bool used;
    uint8_t instanceId;
    uint8_t playState;
    int32_t appId;
    NLSTK_McpMediaInfo_S *basicInfo;
    McpMediaServiceCache_S serviceCache;
    NLSTK_McpPlayControl_S playControl;
    NLSTK_McpStartMediaInst startMediaInst;
    NLSTK_McpMediaAuthorize authorize;
    SDF_Vector_S *playReqQue;
} McpMediaService_S;

typedef struct {
    int32_t instanceId;
    uint16_t requestId;
    uint8_t errorCode;
} McpPlayControlResultParam_S;

typedef struct {
    int32_t instanceId;
    NLSTK_McpPropertyType_E type;
    NLSTK_VariableData_S *value;
} McpUpdateMediaPropertyParam_S;

typedef struct {
    int32_t instanceId;
    uint16_t requestId;
    NLSTK_McpPropertyType_E type;
    uint8_t errorCode;
} McpMediaAuthorizeResultParam_S;

typedef struct {
    uint16_t requestId;
    uint8_t opCode;
} McpMethodCallRequest_S;

// 音量控制客户端结构体
typedef struct {
    NLSTK_McpVolumePropertyType_E property;
    SLE_Addr_S addr;
} NLSTK_McpGetVolumeParam_S;

typedef struct {
    uint8_t volume;
    SLE_Addr_S addr;
} McpSetVolumeParam_S;

typedef struct {
    NLSTK_McpSetStreamVolume_S *volumeArray;
    uint8_t num;
    SLE_Addr_S addr;
} McpSetStreamVolumeParam_S;

typedef struct {
    bool ability;
    SLE_Addr_S addr;
} McpGetAbilityParam_S;

typedef struct {
    uint8_t volume;
    uint8_t accessPoint;
    uint8_t additionalInfo;
} McpStreamVolumeStatus_S;

typedef struct {
    int32_t appId;
    SLE_Addr_S addr;
    uint16_t volumeHandle;
    uint16_t streamVolumeHandle;
    uint16_t channelVolumeHandle;
    uint16_t volumeControlHandle;
    uint16_t volumeSyncEventHandle;
    uint8_t volumeChangeId;
    uint8_t streamVolumeChangeId;
    uint8_t channelVolumeChangeId;
    uint8_t mediaPoint;
    uint8_t callPoint;
    NLSTK_McpVolumeState_E connState;
    bool update;    // 表示当前音量状态的变更标记是否为最新状态
    bool updateV2;  // 表示当前音频流音量状态的变更标记是否为最新状态
    int timer;
    int timerV2;
    SDF_Vector_S *volumeReq;
    SDF_Vector_S *streamVolumeReq;
    SDF_Vector_S *streamVolumeStatus;
} McpVolumeDevice_S;

#ifdef __cplusplus
}
#endif

#endif