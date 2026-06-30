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
#ifndef HADM_API_H
#define HADM_API_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HADM_SOUNDING_STOP 0        // 正在停止测距或者处于测距未启动状态
#define HADM_SOUNDING_START 1       // 正在启动测距过程或者已经启动测距状态
#define HADM_MEASURE_PM_BAND_24G_LEN 10  // 2.4GHz频带长度

typedef struct HadmConnectionParam {
    uint8_t version;             /*!< 原语版本号，当前标准版本设置为0 */
    uint16_t localIndex;         /*!< 本地索引 */
    uint16_t intervalMin;        /*!< 链路调度最小间隔，单位slot */
    uint16_t intervalMax;        /*!< 链路调度最大间隔，单位slot */
    uint8_t eventIfs;            /*!< 链路调度间隔tx和rx间隔，单位us */
    uint16_t eventEfs;           /*!< 链路调度事件间隔，单位us */
    uint16_t maxLatency;         /*!< 延迟周期，单位slot */
    uint16_t supervisionTimeout; /*!< 超时时间，单位10ms */
    uint8_t systemTimeUint;      /*!< 系统调度时隙,枚举值 */
    uint8_t txRxFlag; /*!< 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} __attribute__((packed)) HadmConnectionParam_S;

typedef struct HadmSoundingParam {
    uint8_t configId;                   /* 位置测量信号配置索引 */
    uint8_t schedulingTimeslot;         /* 系统调度时隙 */
    uint8_t rttPhy;                     /* 测量信号带宽指示 */
    uint8_t freqHoppingMode;            /* 跳频方式指示 */
    uint16_t fmFreq;                    /* 初始化阶段的频点 */
    uint16_t fmInteractionType;         /* 初始化阶段交互类型指示, 和glp有关 stepmode */
    uint16_t occurrenceGroupPeriod;     /* 事件组周期 */
    uint16_t fmOccurrenceGroupInterval; /* 初始化阶段事件间间隔 */
    uint16_t pmMeasureType1Interval;    /* 测量帧类型1 事件间间隔 */
    uint16_t pmMeasureType2Interval;    /* 测量帧类型2 事件间间隔 */
    uint16_t fmTIp1Time;                /* 初始化阶段事件内间隔 timeIp1 120 */
    uint16_t pmTIp2Time;                /* 事件内间隔 timeIp2 */
    uint8_t fmTGuard;                   /* 初始化阶段的测量帧的内部切换间隔, guardTime 默认配10 */
    uint8_t fmSignal2Length;            /* 初始化阶段的测量信号2的长度 */
    uint8_t pmInitAntCount;             /* 先发节点天线数量指示 */
    uint8_t pmInitSignal2Tone;          /* 先发链路测量信号2 多音指示 */
    uint8_t pmReflAntCount;             /* 后发节点天线数量指示 */
    uint8_t pmReflSignal2Tone;          /* 后发链路测量信号2 多音指示 */
    uint8_t pmFreqHoppingBand;          /* 跳频频带 */
    uint8_t pm2400mBand[HADM_MEASURE_PM_BAND_24G_LEN]; /* 2.4GHz 跳频信道指示 */
    uint8_t glpMode;
    uint8_t sleHadmMode;  /* slehadm的模式 */
    uint8_t isCsParamChg; /* 是否需要更改测距连接参数，0：不需要更改，1：需要更改（首次设置参数需置0）[车钥匙应用] */
    uint8_t freqSpace;    /* 频率间隔， 0：1M， 1：2M */
    uint8_t conAnchorNum; /* 需要连接的锚点数量 */
    uint8_t refreshRate;  /* 刷新率, 1：高频刷新，2：中频刷新；4：低频刷新 */
    uint16_t acbInterval; /* 根据参数计算的acb周期 */
    uint16_t csInterval;  /* 根据参数计算的测距周期 */
} __attribute__((packed)) HadmSoundingParam_S;

typedef struct HadmReportIqData {
    uint16_t dutIData;
    uint16_t rtdIData;
    uint16_t dutQData;
    uint16_t rtdQData;
} HadmReportIqData_S;

typedef struct HadmSoundingIqData {
    uint8_t dutRssi;
    uint8_t rtdRssi;
    uint16_t dutTof;
    uint16_t rtdTof;
    uint32_t timeStampSn;

    uint16_t iqChnlNum;
    HadmReportIqData_S *iqData;

    uint8_t isMultiTone;
    uint8_t dutIqBitLen;
    uint8_t rtdIqBitLen;
    uint8_t dutSlemChmap[HADM_MEASURE_PM_BAND_24G_LEN];
    uint8_t rtdSlemChmap[HADM_MEASURE_PM_BAND_24G_LEN];
    uint16_t localNvOffset;
    uint16_t remoteNvOffset;
    uint16_t localTofOffset;
    uint16_t remoteTofOffset;
} __attribute__((packed)) HadmSoundingIqData_S;

typedef struct HadmSoundingStateInfo {
    uint8_t status;
    uint8_t posMeasureSigConfigIdx;
    uint8_t measureState;
} __attribute__((packed)) HadmSoundingStateInfo_S;

/**
 * @brief 用户操作枚举类型
 * @details 定义了链路管理模块中用户可能执行的操作类型
 */
typedef enum HadmUserOperate {
    HADM_USER_INVALID_OPERATE = 0, /**< 无效操作 */
    HADM_USER_START_SOUNDING,      /**< 用户启动声呐操作 */
    HADM_USER_STOP_SOUNDING        /**< 用户停止声呐操作 */
} HadmUserOperate_E;

/**
 * @brief 测距探测结果回调函数
 * @param addr 设备地址信息
 * @param args 芯片的测距探测结果信息
 */
typedef void (*HadmSoundingIqCbk)(SLE_Addr_S *addr, HadmSoundingIqData_S *args);
typedef void (*HadmSoundingControlResultCbk)(SLE_Addr_S *addr, HadmUserOperate_E ctrlType, NLSTK_Errcode_E errorcode);
typedef void (*HadmSoundingStateReport)(HadmSoundingStateInfo_S *state);

typedef struct HadmSoundingCbk {
    HadmSoundingIqCbk reportIqDataCbk;
    HadmSoundingControlResultCbk controlResultCbk;
    HadmSoundingStateReport soundingStateReportCbk;
} __attribute__((packed)) HadmSoundingCbk_S;

/**
 * @brief 注册星闪测距探测结果回调函数，多次注册会覆盖前一次的回调函数
 * 在使用测距功能之前，service需要调用此函数注册回调函数，当测距探测完成后，会通过此函数注册的回调函数通知service。
 * @param addr 设备地址信息
 * @param cbk 回调函数指针，指向 `HadmSoundingCbk_S` 类型的函数
 * @return NLSTK_Errcode_E 返回错误码，表示注册操作是否成功
 */
NLSTK_Errcode_E HadmRegCbk(HadmSoundingCbk_S *cbk);

/**
 * @brief 启动测距的函数，规格校验属于同步，状态逻辑属于异步
 * Service会调用此接口完成测距逻辑的启动，当service调用此接口后，协议栈会下发一系列指令到芯片，由芯片完成测距的逻辑；
 * @param addr 设备地址信息
 * @param param 连接参数结构体，星闪芯片的测距功能依赖链接的相关参数如广播周期等，service需要在调用此接口前填充好参数
 * @param soundParam 测距参数结构体，包含测距相关的配置信息
 * @return NLSTK_Errcode_E 返回错误码，表示启动操作是否成功
 */
NLSTK_Errcode_E HadmStartSounding(SLE_Addr_S *addr, HadmConnectionParam_S *param, HadmSoundingParam_S *soundParam);

/**
 * @brief 停止星闪测距，异步处理接口
 * Service会调用此接口完成停止测距的逻辑，当service调用此接口后，协议栈会下发一系列指令到芯片，由芯片完成测距的停止；
 * @param addr 设备地址信息
 * @return NLSTK_Errcode_E 返回错误码，表示停止操作是否成功
 */
NLSTK_Errcode_E HadmStopSounding(SLE_Addr_S *addr);

/**
 * @brief 获取星闪测距状态，同步接口
 * @param addr 设备地址信息
 * @param state 测距状态，返回测距的当前状态
 * @return NLSTK_Errcode_E 返回错误码，表示获取状态操作是否成功
 */
NLSTK_Errcode_E HadmGetSoundingState(SLE_Addr_S *addr, uint8_t *state);

/**
 * @brief 获取正在测距的地址信息，同步接口
 * @param addr 设备地址信息，协议栈会将正在测距的地址填入addr中
 * @param maxNum 表示传入的addr的长度
 * @return uint32_t 返回正在测距的地址数量,返回值大于maxNum则表示addr数组长度不够
 */
uint32_t HadmGetSoundingAddrInfo(SLE_Addr_S *addr, uint32_t maxNum);

#ifdef __cplusplus
}
#endif

#endif /* HADM_API_H */