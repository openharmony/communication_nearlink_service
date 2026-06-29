/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef NLSTK_MCP_MEDIA_SERVER_H
#define NLSTK_MCP_MEDIA_SERVER_H

#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// 通用的枚举，应该放到SSAP的头文件中
// 用于授权回调 NLSTK_McpAuthorize 表示客户端的具体操作
typedef enum {
    NLSTK_SSAP_PROPERTY_READ = 0x01,
    NLSTK_SSAP_PROPERTY_WRITE = 0x02,
} NLSTK_ServicePropertyOpType_E;

// 每个属性的操作权限，用来设置 NLSTK_McpMediaInfo_S 中的 propertyRights 字段
#define NLSTK_MCP_READ_AUTHEN 0x01    // 认证读取权限
#define NLSTK_MCP_READ_ENCRYPT 0x02   // 加密读取权限
#define NLSTK_MCP_READ_AUTHOR 0x04    // 授权读取权限

// NLSTK_McpCreateMediaInstance 的第一个入参
typedef enum {
    NLSTK_MCP_COMMON_SERVICE = 0,  // 表示 <<通用媒体播放控制>>
    NLSTK_MCP_SERVICE = 1,         // 表示 <<媒体播放控制>>
} NLSTK_McpServiceType_E;

typedef enum {
    NLSTK_MEDIA_AUDIO = 0x00,         // 音频媒体
    NLSTK_MEDIA_VIDEO = 0x01,         // 视频媒体
    NLSTK_MEDIA_AUDIO_VIDEO = 0x02,   // 音视频媒体
    NLSTK_MEDIA_UNSPECIFIED = 0xFF,   // 未指定媒体
} NLSTK_McpMediaType_E;

typedef enum {
    NLSTK_MCP_STATE_UNINITIALIZED = 0x00,  // 未就绪
    NLSTK_MCP_STATE_READY = 0x01,          // 就绪
    NLSTK_MCP_STATE_PLAYING = 0x02,        // 播放
    NLSTK_MCP_STATE_SEEKING = 0x03         // 寻道
} NLSTK_McpReadyState_E;

typedef enum {
    NLSTK_MCP_FEATURE_TYPE_PLAY_CTL = 0x01,   // 媒体控制的控制类型
    NLSTK_MCP_FEATURE_TYPE_PLAY_MODE = 0x02,  // 媒体控制的播放顺序控制类型
} NLSTK_McpFeaturesSupportedType_E;

// 媒体控制的控制类型，这个枚举用来设置 NLSTK_McpMediaInfo_S 结构体中的 playCtlFeaturesSupported 字段
// eg: playCtlFeaturesSupported = MCP_PLAY | MCP_STOP | MCP_PAUSE;
typedef enum {
    NLSTK_MCP_PLAY = 0x00000001,
    NLSTK_MCP_STOP = 0x00000002,
    NLSTK_MCP_PAUSE = 0x00000004,
    NLSTK_MCP_FAST_FORWARD = 0x00000008,
    NLSTK_MCP_REWIND = 0x00000010,
    NLSTK_MCP_MEDIA_POSITION_MOVE = 0x00000020,
    NLSTK_MCP_MEDIA_POSITION_RELATIVE_MOVE = 0x00000040,
    NLSTK_MCP_PREVIOUS_MEDIA_SEGMENT = 0x00000080,
    NLSTK_MCP_NEXT_MEDIA_SEGMENT = 0x00000100,
    NLSTK_MCP_MEDIA_SEGMENT_JUMP = 0x00000200,
    NLSTK_MCP_PREVIOUS_MEDIA = 0x00000400,
    NLSTK_MCP_NEXT_MEDIA = 0x00000800,
    NLSTK_MCP_MEDIA_JUMP = 0x00001000,
    NLSTK_MCP_PREVIOUS_MEDIA_GROUP = 0x00002000,
    NLSTK_MCP_NEXT_MEDIA_GROUP = 0x00004000,
    NLSTK_MCP_MEDIA_GROUP_JUMP = 0x00008000,
    NLSTK_MCP_PLAYBACK_ORDER_CHANGE = 0x00010000,
    NLSTK_MCP_PLAYBACK_SPEED_CHANGE = 0x00020000,
    NLSTK_MCP_OPERATION_RESERVED = 0xFFFFF000
} NLSTK_McpOperationType_E;

// 媒体控制的播放顺序控制类型，用来设置 NLSTK_McpMediaInfo_S 结构体中的 playModeFeaturesSupported 字段
// eg: playModeFeaturesSupported = MCP_SINGLE_PLAY | MCP_SINGLE_LOOP | MCP_SINGLE_LIST | MCP_LOOP_LIST;
typedef enum {
    NLSTK_MCP_SINGLE_PLAY = 0x0001,       // 单次播放
    NLSTK_MCP_SINGLE_LOOP = 0x0002,       // 单曲循环
    NLSTK_MCP_SINGLE_LIST = 0x0004,       // 单次列表
    NLSTK_MCP_LOOP_LIST = 0x0008,         // 循环列表
    NLSTK_MCP_RANDOM_PLAY = 0x0010,       // 随机播放
    NLSTK_MCP_PLAYMODE_RESERVED = 0xFFFF  // 其他保留
} NLSTK_McpPlayMode_E;

// 媒体播放控制请求响应的结果码
typedef enum {
    NLSTK_MCP_CONTROL_SUCCESS = 0x00,         // 操作成功
    NLSTK_MCP_CONTROL_UNSUPPORTED = 0x01,     // 请求类型不支持
    NLSTK_MCP_CONTROL_UNEXECUTABLE = 0x02,    // 请求无法执行
    NLSTK_MCP_MEDIA_NOT_READY = 0x03,         // 媒体播放实例未就绪
    NLSTK_MCP_CONTROL_FAILED = 0x04,          // 操作失败
} NLSTK_McpPlayControlRes_E;

// MCP定义的媒体控制服务属性，是 NLSTK_McpMediaInfo_S 中的 propertyRights数组的下标
typedef enum {
    NLSTK_MCP_MEDIA_INSTANCE_NAME = 0,  // 媒体实例名称
    NLSTK_MCP_MEDIA_INSTANCE_ICON,      // 媒体实例图标
    NLSTK_MCP_MEDIA_BASIC_INFO,         // 媒体基本信息
    NLSTK_MCP_MEDIA_EXTENDED_INFO,      // 媒体扩展信息
    NLSTK_MCP_MEDIA_IDENTIFIER_INFO,    // 媒体标识信息
    NLSTK_MCP_MEDIA_PLAYBACK_POSITION,  // 媒体播放位置
    NLSTK_MCP_MEDIA_SEGMENT_INFO,       // 媒体片段信息
    NLSTK_MCP_PLAYBACK_SPEED,           // 播放速度
    NLSTK_MCP_SEEK_SPEED,               // 快进快退速度
    NLSTK_MCP_FEATURE_SUPPORT,          // 特性支持
    NLSTK_MCP_PLAYBACK_ORDER,           // 播放顺序
    NLSTK_MCP_PLAYBACK_STATE,           // 播放状态
    NLSTK_MCP_MEDIA_INSTANCE_ID,        // 媒体实例标识
    NLSTK_MCP_MEDIA_MAX_PROPERTY        // 属性最大值
} NLSTK_McpPropertyType_E;

/**
 * @brief 授权回调函数
 *
 * 该回调函数用于处理客户端的属性权限请求。
 *
 * 如果属性权限是授权，但服务未注册回调函数，SSAP会默认按照“客户端未授权”处理，并返回给客户端。
 *
 * @param[in] requestId 请求ID，用于标识具体的请求
 * @param[in] instanceId 实例ID，标识具体的媒体播放控制实例
 * @param[in] property 属性类型
 * @param[in] operation 操作类型（读或写）
 */
typedef void (*NLSTK_McpMediaAuthorize)(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                      NLSTK_ServicePropertyOpType_E operation);

/**
 * @brief 添加媒体播放控制服务实例回调函数
 *
 * 该回调函数用于处理添加媒体播放控制服务实例回调结果。
 *
 * @param[in] instanceId 实例ID，标识具体的媒体播放控制实例
 * @param[in] ret 媒体播放控制实例添加结果
 */
typedef void (*NLSTK_McpStartMediaInst)(int32_t instanceId, NLSTK_Errcode_E ret);

// 播放、停止、暂停、快进、上一个媒体、下一个媒体
typedef void (*NLSTK_McpPlay_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
typedef void (*NLSTK_McpStop_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
typedef void (*NLSTK_McpPause_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
typedef void (*NLSTK_McpFastForward_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
typedef void (*NLSTK_McpPreviousMedia_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
typedef void (*NLSTK_McpNextMedia_S)(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);

// NLSTK_McpPlayControl_S 结构体中的函数指针，用来设置各个操作的回调函数
typedef struct {
    NLSTK_McpPlay_S play;
    NLSTK_McpStop_S stop;
    NLSTK_McpPause_S pause;
    NLSTK_McpFastForward_S fastForward;
    NLSTK_McpPreviousMedia_S previousMedia;
    NLSTK_McpNextMedia_S nextMedia;
} NLSTK_McpPlayControl_S;                     // 媒体播放控制点

typedef void *NLSTK_McpBrowseControl_S;       // 媒体浏览控制点

typedef struct {
    uint8_t iconType;                   // 媒体实例图标类型
    uint16_t iconLen;                   // 媒体实例图标类型值长度
    uint8_t *iconValue;                 // 媒体实例图标类型值
} NLSTK_McpInstanceIcon_S;

typedef struct {
    uint8_t mediaType;                          // 媒体类型
    uint32_t duration;                          // 媒体总时长
    uint8_t mediaNameLen;                       // 媒体名称长度
    uint8_t *mediaName;                         // 媒体名称
} NLSTK_McpMediaBaseInfo_S;

typedef struct {
    uint8_t FeatureType;                        // 特性类型
    uint32_t playCtl;                           // 媒体控制请求
    uint16_t playMode;                          // 播放顺序
} __attribute__((packed)) NLSTK_McpFeaturesSupported_S;

typedef struct {
    uint32_t mediaId;                           // 媒体标识
    uint32_t mediaGroupId;                      // 媒体组标识
} NLSTK_McpMediaIdInfo_S;

typedef union {
    uint8_t flagByte;
    struct {
        uint8_t instanceIconFlag : 1;
        uint8_t mediaExtendedInfoFlag : 1;
        uint8_t mediaIdInfoFlag : 1;
        uint8_t segmentInfoFlag : 1;
        uint8_t playbackSpeedFlag : 1;
        uint8_t seekSpeedFlag : 1;
        uint8_t playbackOrderFlag : 1;
        uint8_t browseControlPointFlag : 1;
    } flagBit;
} NLSTK_McpOptionalFlag_S;

typedef struct {
    NLSTK_McpOptionalFlag_S flags;
    NLSTK_McpInstanceIcon_S instanceIcon;                     // 媒体实例图标
    NLSTK_VariableData_S mediaExtendedInfo;            // 媒体扩展信息
    // ID3v2.4
    NLSTK_McpMediaIdInfo_S mediaIdInfo;                       // 媒体标识信息
    // 若当前媒体的媒体类型为未指定媒体，媒体标识信息的属性值为空
    NLSTK_VariableData_S segmentInfo;                  // 媒体段信息
    uint16_t playbackSpeed;                                 // 播放速度
    // 当前媒体类型为未指定媒体， 播放速度属性的属性值为空
    uint16_t seekSpeed;                                     // 寻道速度
    // 无效值 0，当媒体播放实例未进行寻道操作时，寻道速度为0
    uint8_t playbackOrder;                                  // 播放顺序
    // 媒体播放实例默认的播放顺序由实现决定
    NLSTK_McpBrowseControl_S browseControlPoint;              // 媒体浏览控制点
} NLSTK_McpOptionalItem_S;

typedef struct {
    NLSTK_McpServiceType_E type;                              // 媒体播放控制服务类型
    NLSTK_VariableData_S instanceName;                 // 媒体实例名称(必选)
    NLSTK_McpMediaBaseInfo_S mediaBaseInfo;                   // 媒体基本信息(必选)
    // 媒体类型 NLSTK_MEDIA_UNSPECIFIED = 0xFF : 未指定媒体
    uint32_t playbackLocation;                              // 媒体播放位置(必选)
    // 当前媒体类型为未指定媒体时，媒体播放位置属性的属性值为空；若当前媒体的媒体播放位置不能确定，媒体播放位置属性的属性值为无效值全F
    NLSTK_McpFeaturesSupported_S featuresSupported;           // 特性支持(必选)
    uint8_t playbackState;                                  // 播放状态(必选)
    // 未指定当前媒体的状态，播放状态为未就绪 NLSTK_MCP_STATE_UNINITIALIZED = 0x00
    uint8_t mediaInstanceId;                                // 媒体实例标识(必选)
    NLSTK_McpPlayControl_S playbackControlPoint;              // 媒体播放控制点(必选)
    NLSTK_McpOptionalItem_S optionalItem;                     // 媒体控制可选条目
    NLSTK_McpStartMediaInst startMediaInst;                   // 添加媒体播放控制服务实例回调
    NLSTK_McpMediaAuthorize authorize;                        // 授权回调函数
    uint8_t propertyRights[NLSTK_MCP_MEDIA_MAX_PROPERTY];     // 针对每个属性的操作权限，目前MCP不支持设置写属性
} NLSTK_McpMediaInfo_S;

/**
 * @brief 创建一个媒体播放控制服务实例
 *
 * 该函数用于创建一个媒体播放控制服务实例，并根据提供的媒体信息进行初始化；
 * 媒体播放控制服务含有可选属性，用户可以根据需求添加，用户需维护所有添加的媒体播放控制服务属性的更新确保媒体状态一致。
 *
 * @param[in] basicInfo 媒体播放控制服务信息结构体指针
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpCreateMediaInstance(NLSTK_McpMediaInfo_S *basicInfo);

/**
 * @brief 通知媒体播放控制执行结果
 *
 * 该函数用于在业务处理完 NLSTK_McpPlayControl_S 回调后，通知MCP媒体播放控制执行的结果；
 * 若方法调用涉及到媒体属性的更新，应该先回复方法调用响应，然后再通过 NLSTK_McpUpdateMediaProperty 更新媒体属性。
 *
 * @param[in] requestId 请求标识，用于标识具体的请求序号，该标识由 NLSTK_McpPlayControl_S 回调函数传入
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] errorCode 操作结果，0表示成功，其他值表示失败，表示 NLSTK_McpPlayControl_S 回调函数的操作结果
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpPlayControlResult(uint16_t requestId, int32_t instanceId, uint8_t errorCode);

/**
 * @brief 返回读取媒体播放控制服务属性授权结果
 *
 * 该函数用于用户处理完读取媒体播放控制服务属性授权请求后返回授权结果，媒体播放控制服务实例根据错误码决定后续操作。
 *
 * @param[in] requestId 请求标识，标识当前操作的唯一请求
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] property 属性类型，指定操作的目标属性
 * @param[in] errorCode 错误码，决定后续处理逻辑
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpMediaAuthorizeResult(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                     uint8_t errorCode);

/**
 * @brief 更新媒体播放控制服务属性值
 *
 * 该函数用于用户更新媒体播放控制服务中的属性值，若媒体控制端订阅了相应属性描述符，SSAP将发送属性变更通知给媒体控制端。
 *
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] property 属性类型，指定需要更新的属性
 * @param[in] value 属性值指针，指向需要更新的新值
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpUpdateMediaProperty(int32_t instanceId, NLSTK_McpPropertyType_E property, void *value);

/**
 * @brief 删除一个媒体播放控制服务实例
 *
 * 该函数用于删除一个媒体播放控制服务实例，销毁相应的SSAP服务和缓存资源。
 *
 * @param[in] instanceId 实例标识，标识需要清理的服务实例
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpDeleteMediaInstance(int32_t instanceId);

#ifdef __cplusplus
}
#endif

#endif