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

#ifndef ASC_DEFINES_H
#define ASC_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"
#include "actm_l2hc.h"
#include "ASCCodec.h"
#include "nearlink_ASC_source.h"

namespace OHOS {
namespace Nearlink {
// Notify available bitrates to DSP
constexpr int ASC_AVAILABLE_BITRATE_MAX           = 8;
// ASC co-set max number
constexpr int ASC_COSET_MAX_NUM                   = 2;
// Invalid Stream Id
constexpr uint8_t ASC_INVALID_STREAM_ID           = 0xFF;
// ASC event
// Service start/stop
constexpr int ASC_SERVICE_STARTUP_EVT             = 1;
constexpr int ASC_SERVICE_SHUTDOWN_EVT            = 2;
// Device open/close
constexpr int ASC_CONNECT_START_EVT               = 3;
constexpr int ASC_CONNECT_CMPL_EVT                = 4;
constexpr int ASC_DISCONNECT_CMPL_EVT             = 5;
// Audio Control
constexpr int ASC_START_PLAYING_EVT               = 6;
constexpr int ASC_STOP_PLAYING_EVT                = 7;
// Stack event Callback
constexpr int ASC_STACK_EVENT_CBK_EVT             = 8;
// Stack prop Callback
constexpr int ASC_STACK_PROP_CBK_EVT              = 9;
// Set active sink device
constexpr int ASC_SET_ACTIVE_SINK_DEVICE_EVT      = 10;
// Update virtual device
constexpr int ASC_UPDATE_VIRTUAL_DEVICE_EVT       = 11;
// Update Device Role
constexpr int ASC_UPDATE_DEVICE_ROLE_EVT          = 12;
// AutoRate
constexpr int ASC_BITRATE_CBK_EVT                 = 13;
// Stop Delay
constexpr int ASC_STOP_DELAY_TIMEOUT_EVT          = 14;

constexpr int ASC_SPATIAL_AUDIO_EVT               = 15;
constexpr int ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT     = 16;
// Connection Report Delay
constexpr int ASC_CONN_RPT_DELAY_TIMEOUT_EVT = 17;
// Send Play Or Pause By Wear Detection
constexpr int ASC_SEND_PLAY_OR_PAUSE_EVT          = 18;
// Device close
constexpr int ASC_DISCONNECT_START_EVT            = 19;
// Audio scene Change
constexpr int ASC_AUDIO_SCENE_CHANGE_EVT          = 20;
// Stop sink
constexpr int ASC_STOP_SINK_EVT                   = 21;
// Subrate changed
constexpr int ASC_SUBRATE_CHANGED_EVT             = 22;
// Subrate change Req
constexpr int ASC_SUBRATE_CHANGE_REQ_EVT          = 23;
// Stack Location Callback
constexpr int ASC_STACK_LOCATION_CBK_EVT          = 24;
// Tws nature update
constexpr int ASC_UPDATE_NATURE_EVT               = 25;
// Spatial audio source update
constexpr int ASC_SPATIAL_AUDIO_SOURCE_EVT        = 26;
// Spatial audio adaptive switch update
constexpr int ASC_SPATIAL_AUDIO_ADAPTIVE_SWITCH_RENDER_EVT = 27;
// Stack Stream Type Callback
constexpr int ASC_STACK_STREAM_TYPE_CBK_EVT       = 28;
// Voice call autorate msg
constexpr int ASC_STACK_AUTORATE_MSG_CBK          = 29;
// Frame type changed
constexpr int ASC_FRAME_CHANGED_EVT               = 30;
// Collaboration audio switch change
constexpr int ASC_COL_AUDIO_SWITCH_CHANGE_EVT = 31;

// 不透传
constexpr uint8_t ASC_TRANSPORT_TYPE_IMPERMEABLE  = 0x00;
// sle透传
constexpr uint8_t ASC_TRANSPORT_TYPE_SLE_TRANSPARENT = 0x02;
// 扩展sle qosid
constexpr uint8_t ASC_TRANSPORT_TYPE_SLE_QOSID = 0x03;
// 单播
constexpr uint8_t ASC_COMM_TYPE_UNICAST           = 0x00;
// 数据组播
constexpr uint8_t ASC_COMM_TYPE_MULTICAST         = 0x01;
// 音频流持续时长: 长时间
constexpr uint8_t ASC_DURATION_LONG               = 0x01;
// 短时间
constexpr uint8_t ASC_DURATION_SHORT              = 0x02;

// stack callback event type
// 音频属性获取失败
constexpr uint8_t ASC_STACK_CBK_READ_PROP_FAIL    = 0;
// 音频流打开
constexpr uint8_t ASC_STACK_CBK_OPEN_STREAM       = 1;
// 音频属性配置
constexpr uint8_t ASC_STACK_CBK_CFG_STREAM        = 2;
// 音频流开始
constexpr uint8_t ASC_STACK_CBK_START_STREAM      = 3;
// 音频流停止
constexpr uint8_t ASC_STACK_CBK_STOP_STREAM       = 4;
// 音频流释放
constexpr uint8_t ASC_STACK_CBK_RELEASE_STREAM    = 5;
// 断连事件
constexpr uint8_t ASC_STACK_CBK_DISCONNECT        = 6;
// 对端码率更新
constexpr uint8_t ASC_STACK_CBK_BITRATE_CHANGED   = 7;
// 断连事件
constexpr uint8_t ASC_STACK_CBK_CREATE_STREAM     = 8;

// 合作集设备最大数目
constexpr uint8_t COSET_MAX_NUM         = 2;

// 语音助手起播等待时延
constexpr uint32_t VOICE_ASSISTANT_WAIT_TIME = 500 * 1000;

// 同步链路数目
constexpr uint8_t ASC_ICB_NUM_ONE       = 1;
constexpr uint8_t ASC_ICB_NUM_TWO       = 2;

// 切帧格式请求
constexpr uint8_t ASC_CHANGE_FRAME_TYPE_REQ             = 0;
// 切帧格式回复
constexpr uint8_t ASC_CHANGE_FRAME_TYPE_RSP             = 1;
// 双端切码率请求
constexpr uint8_t ASC_CHANGE_PEER_BITRATE_REQ           = 2;
// 双端切码率回复
constexpr uint8_t ASC_CHANGE_PEER_BITRATE_RSP           = 3;
// 切同步链路请求
constexpr uint8_t ASC_CHANGE_LABEL_ID_REQ               = 4;
// 切同步链路回复
constexpr uint8_t ASC_CHANGE_LABEL_ID_RSP               = 5;
// 通话码率自适应能力通知
constexpr uint8_t ASC_NTF_VOICE_CALL_AUTORATE_ABILITY   = 6;

// 定时器
constexpr int START_PLAYING_TIMEOUT_MS  = 4500;
constexpr int STOP_PLAYING_TIMEOUT_MS   = 4000;
constexpr int STOP_DELAY_TIMEOUT_MS     = 3500;
constexpr int CONN_RPT_DELAY_TIMEOUT_MS = 2000;
constexpr int SET_SUBRATE_TIMEOUT_MS    = 5000;

// 上一次停流时低码率记录的有效时间范围
constexpr int ASC_FORMER_RECORD_VALID_SECOND = 1800;

// 耳机临时拒绝本次建链错误码0xC
constexpr uint8_t ASC_REJECT_REASON_ONCE         = 0xC;

// 无效lable id
constexpr uint8_t ASC_INVALID_LABLE_ID          = 255;

// 耳机侧对该值有长度校验(为12), 不允许扩展新增类型
constexpr AudioStreamType ASC_AUDIO_STREAM_TYPE_LIST[] = {
    AUDIO_STREAM_UNDEFINED,       // 未定义
    AUDIO_STREAM_MUSIC,           // 媒体音乐，指的是播放歌曲
    AUDIO_STREAM_VOICE_CALL,      // 移动网络通话
    AUDIO_STREAM_VOICE_ASSISTANT, // 语音助手
    AUDIO_STREAM_RING,            // 铃声
    AUDIO_STREAM_VOIP,            // IP通话
    AUDIO_STREAM_GAME,            // 低时延：游戏
    AUDIO_STREAM_RECORD,          // 录音
    AUDIO_STREAM_ALERT,           // 提示音
    AUDIO_STREAM_VIDEO,           // 视频声
    AUDIO_STREAM_GUID,            // 导航声
    AUDIO_STREAM_ALARM,           // 告警声
};

enum ASCState {
    NL_SLE_ASC_DISABLED                 = 0,
    NL_SLE_ASC_DISABLING                = 1,
    NL_SLE_ASC_ENABLED                  = 2,
    NL_SLE_ASC_ENABLING                 = 3,
    NL_SLE_ASC_CONNECTED                = 4,
    NL_SLE_ASC_CONNECTING               = 5,
    NL_SLE_ASC_DISCONNECTED             = 6,
    NL_SLE_ASC_DISCONNECTING            = 7,
    NL_SLE_ASC_CONFIGED                 = 8,
    NL_SLE_ASC_CONFIGURING              = 9,
    NL_SLE_ASC_OPENED                   = 10,
    NL_SLE_ASC_OPENING                  = 11,
    NL_SLE_ASC_STARTED                  = 12,
    NL_SLE_ASC_STARTING                 = 13,
    NL_SLE_ASC_STOPPED                  = 14,
    NL_SLE_ASC_STOPPING                 = 15,
    NL_SLE_ASC_RELEASED                 = 16,
    NL_SLE_ASC_RELEASING                = 17,
    NL_SLE_ASC_RECONFIG_STOPPING        = 18,
    NL_SLE_ASC_RECONFIG_STOPPED         = 19,
    NL_SLE_ASC_RECONFIGED               = 20,
    NL_SLE_ASC_RECONFIGURING            = 21,
    NL_SLE_ASC_CREATED                  = 22,
    NL_SLE_ASC_CREATING                 = 23,
    NL_SLE_ASC_CONFIG_SUBRATE_CHANGED   = 24,
    NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED = 25,
    NL_SLE_ASC_BUTT                     = 26,
};

// ASCService内部错误码(1~100:下层协议栈 100以上:service)
enum ASCErrorCode : uint8_t {
    NL_SLE_ASC_ERROR_GET_PROP_STATE_ERROR         = 100,
    NL_SLE_ASC_ERROR_CFG_QOS_ERROR                = 101,
    NL_SLE_ASC_ERROR_JUDGE_RECFG_STATE_ERROR      = 102,
    NL_SLE_ASC_ERROR_START_PLAY_STATE_ERROR       = 103,
    NL_SLE_ASC_ERROR_STOP_STREAM_NOT_STARTED      = 104,
    NL_SLE_ASC_ERROR_STOP_PLAY_STATE_ERROR        = 105,
    NL_SLE_ASC_ERROR_RELEASE_STREAMTYPE_ERROR     = 106,
    NL_SLE_ASC_ERROR_CBK_GET_PROP_STATE_ERROR     = 107,
    NL_SLE_ASC_ERROR_CBK_CFG_STATE_ERROR          = 108,
    NL_SLE_ASC_ERROR_CBK_OPEN_STATE_ERROR         = 109,
    NL_SLE_ASC_ERROR_CBK_START_STATE_ERROR        = 110,
    NL_SLE_ASC_ERROR_CBK_STOP_STATE_ERROR         = 111,
    NL_SLE_ASC_ERROR_CBK_RELEASE_STATE_ERROR      = 112,
    NL_SLE_ASC_ERROR_DISCONN_STATE_ERROR          = 113,
    NL_SLE_ASC_ERROR_CONN_MAX_LIMIT_ERROR         = 114,
    NL_SLE_ASC_ERROR_STREAMTYPE_ERROR             = 115,
    NL_SLE_ASC_ERROR_REJECT_ONCE                  = 201,    /* 耳机多连接通话抢占耳机侧拒绝 */
};

// 连接命令：连接/断开连接
enum ASCConnCmd : uint8_t {
    NL_SLE_ASC_CONN_CMD_CONN                      = 1,
    NL_SLE_ASC_CONN_CMD_DISCONN                   = 2,
};

// 连接上报状态：连接/断开连接
enum ASCConnState : uint8_t {
    NL_SLE_ASC_STATE_NOT_CONNECTED                = 0,
    NL_SLE_ASC_STATE_CONNECTED                    = 1,
    NL_SLE_ASC_STATE_DISCONNECTED                 = 3,
};

// 结果：成功/失败
enum ASCConnResult : uint8_t {
    NL_SLE_ASC_RESULT_SUCC                        = 0,
    NL_SLE_ASC_RESULT_FAIL                        = 1,
};
// 当前运行类型：单耳(包含单切双)/双耳同启同停
enum RunningType : uint8_t {
    NL_SLE_ASC_RUNNING_TYPE_SINGLE                = 0,
    NL_SLE_ASC_RUNNING_TYPE_DOUBLE                = 1,
};
// 空间音频音源类型
enum ASCSpatialAudioSourceType : uint8_t {
    NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_STEREO = 0,
    NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_AUDIO_VIVID  = 1,
    NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_MULTI_CHANNEL = 2,
};
enum ASCSpatialAudioState : uint8_t {
    NL_SLE_ASC_SPATIAL_AUDIO_MODE_DISABLED = 0,
    NL_SLE_ASC_SPATIAL_AUDIO_MODE_ENABLED = 1,
    NL_SLE_ASC_SPATIAL_AUDIO_HEAD_TRACKING_ENABLED = 2,
    NL_SLE_ASC_SPATIAL_AUDIO_SOURCE_TYPE_ENABLED = 4,
    NL_SLE_ASC_SPATIAL_AUDIO_ADAPTIVE_ENABLED = 8,
};
// subrate的设置状态
enum ASCSubRateState : uint8_t {
    NL_SLE_ASC_INIT = 0,
    NL_SLE_ASC_SETTING = 1,
    NL_SLE_ASC_SETTED = 2,
};

enum ASCVoiceCallAutoRateDirection : uint8_t {
    NL_SLE_ASC_VOICE_CALL_BITRATE_NONE = 0,
    NL_SLE_ASC_VOICE_CALL_BITRATE_UP = 1,
    NL_SLE_ASC_VOICE_CALL_BITRATE_DOWN = 2,
};

enum ASCVoiceCallAutoRateDSPStatus : uint8_t {
    NL_SLE_ASC_VOICE_CALL_PEER_BITRATE_CHANED = 1,
    NL_SLE_ASC_VOICE_CALL_LOCAL_BITRATE_NTF = 2,
};

// 音频属性结构定义
typedef struct {
    uint8_t version;                    // version
    uint16_t rate;                      // 采样率
    uint8_t depth;                      // 采样深度
    uint8_t channel;                    // 音频通道类型
    uint16_t frame;                     // 帧长模式
    uint64_t bps;                       // 编码速率
} AscL2HCParam;

typedef struct {
    uint16_t rate;                      // 采样率
    uint8_t depth;                      // 采样深度
    uint16_t channel;                   // 音频通道
    uint8_t dataType;                   // 数据类型
    uint8_t order;                      // 字节序
    uint16_t sduLen;                    // 音频SDU长度
} AscPCMParam;

typedef union {
    AscPCMParam pcmParam;               // PCM编解码
    AscL2HCParam l2hcParam;             // L2HC编解码
} AscCodecParam;

typedef struct {
    uint8_t codecId;                    // 编解码器标识, 0x0: PCM(暂不支持), 0x1: L2HC, 0xFF: L2HC5.0
    uint16_t companyId;                 // 厂商标识
    uint16_t vendorId;                  // 厂商编解码器标识
    AscCodecParam param;                // 编解码器参数, AscCodecParam
} AscCodecConfig;

typedef struct {
    uint8_t codecNum;
    AscCodecConfig codec[ASC_CODEC_NUM_MAX];
    uint8_t comm;                       /* 通信方式, 0x0: 单播, 0x1: 数据组播 */
    uint32_t supportType;               // 支持的音频流类型
    uint32_t acceptType;                // 可接收的音频流类型
} AscAbility;

typedef struct {
    uint8_t pointType;                  // 访问点类型，参考NLSTK_ActmPointType_E
    AscAbility ability;                 // AscAbility
} AscProp;

typedef struct {
    uint16_t groupId;
    uint8_t direction;
    uint8_t labelId;
    uint16_t downBitrate;
    uint16_t upBitrate;
    uint8_t qosIndex;
    uint8_t qosLevel;
    uint8_t dutyCycle;
    uint8_t availableBitratesCnt;
    uint16_t availableBitrates[ASC_AVAILABLE_BITRATE_MAX];
} AscBitrateChange;

typedef struct {
    std::string addr;
    uint8_t frameChangeState;
} AscPhyStatus;

typedef struct {
    AscBitrateChange ascBitrate;
    uint8_t changeDSPBitState;
    bool isGroupFrameChangeSucc;
    std::vector<AscPhyStatus> phyStatusList;
} AscAutorateInfo;

typedef struct {
    uint8_t streamId;
    uint8_t pointType;
    uint8_t commType;
} AscStreamInfo;

typedef struct {
    uint8_t groupId;
    uint8_t streamId;
    bool isLevelUp;
    uint8_t targetFrameType;
    uint8_t targetPhyType;
    uint8_t targetLabelId;
    uint16_t downwardBitrate;
    uint16_t upwardBitrate;
    uint8_t frameChangeState;
    uint8_t changeDSPBitState;
} AscLinkChangeInfo;

typedef struct {
    ASCState        state;                    // 音频流状态
    AudioStreamType streamType;               // 当前状态机流类型
    bool            isSync;                   // 当前运行类型：是否单切双
    bool            isStopDelaying;           // 延迟断同步链路等待中
    bool            isStopDoing;              // 断同步链路执行中
    uint16_t        autoRateBps;              // 播放速率
    bool            isSpatialConfiguring;     // 开/关空间音频改配同步链路执行中
    bool            isConnRptDelaying;        // 上报延迟中
    bool            isCalling;                // 是否通话中
    bool            isNeedDisconnect;         // 是否需要Disconnect断连接
    uint64_t        bpsRange;                 // 码率范围
    AscStreamInfo   unicastStream;            // 单播流信息
    AscStreamInfo   multicastStream;          // 组播流信息
    uint8_t         multicastFrameType;           // 组播acb帧格式
    uint8_t         multicastPhyType;             // 组播acb带宽
} ASCStatus;

typedef struct {
    ASCSubRateState      subrateState;               // subrate当前设置值的状态
    bool                 isStartStrChangeSubrate;    // 起流设置subrate当前状态NL_SLE_ASC_SETTING,需要待上一次完成后执行
} ASCSubRateInfo;
// ASC模块通知DSP编解码信息
typedef struct {
    uint8_t encodeDspVersion;            // 下行映射DSP版本号
    uint8_t encodeBitDepth;              // 下行位深
    uint32_t encodeSampleRate;           // 下行采样率Hz
    uint16_t encodeBps;                  // 下行起播码率kbps
    uint8_t encodeFrame;                 // 下行帧长ms
    uint16_t sduInterval;                // sdu间隔
    uint8_t frameNumPerSdu;              // 每个sdu携带帧数
    uint8_t twsNum;                      // 当前合作集中设备数目
    uint16_t maxSdu;                     // sdu最大长度
    uint16_t buffNum;                    // 同步链路buffer数
    uint8_t downChannelNum;              /* 下行通道数, 0表示不需要编码,通话组播为1,音箱通话\媒体为1,耳机媒体单播为2,
                                            不超过合作集中设备数目 */
    uint8_t encodeCodecClan;             // 下行编码器家族
    uint8_t encodeChannelNum;            // 编码输出声道数,0单声道,1双声道
    uint8_t upChannelNum;                /* 上行通道数,0表示不需要解码,
                                            双耳录音2,三方录音1,通话1,通话联合降噪2,不超过合作集中设备数目 */
    uint8_t decodeCodecClan;             // 上行编码器家族
    uint8_t decodeChannelNum;            // 解码输入声道数,0单声道,1双声道
    uint8_t decodeBitDepth;              // 上行位深
    uint32_t decodeSampleRate;           // 上行采样率Hz
    uint8_t ft;                          /* G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位,
                                            取值范围[0x01,0xFF] */
    uint8_t bn;                          // G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数
} ASCToDSPInfo;

} // namespace Sle
} // namespace OHOS
#endif // ASC_DEFINES_H