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
 * @file         devd_tbl.h
 * @brief        sle广播扫描数据
 */
#ifndef DEVD_TBL_H
#define DEVD_TBL_H

#include <stdint.h>
#include "sdf_dlist.h"
#include "sdf_addr.h"
#include "nlstk_devd_def.h"

#ifdef __cplusplus
extern "C" {
#endif
#define DEVD_LEGACY_ADV_HANDLE 0x00
#define DEVD_INVALID_ADV_HANDLE 0xFF

#define DEFAULT_MAX_ADV_DATA_LEN 0x07FF
#define LOW_BITS_2 0x03
#define LOW_BITS_5 0x1F
#define SHIFT_BITS_5 5
/* 广播使能状态 */
#define ADV_DISABLED 0
#define ADV_ENABLED  1
#define SCAN_PHY_MAX_NUM 3

/**
 * @brief  扫描当前状态
 */
typedef enum {
    DEVD_SLE_SCAN_STATUS_DISABLED,           /*!< 未使能的扫描态 */
    DEVD_SLE_SCAN_STATUS_ENABLED,            /*!< 已使能的扫描态 */
} DevdSleScanStatus_S;

/**
 * @brief  扫描管理节点
 */
typedef struct {
    uint8_t status;
    NLSTK_DevdSleScanParams_S *scanParams;
    NLSTK_DevdSleScanParams_S *tmpScanParams;
} DevdMgrScanItem_S;

/**
 * @brief  设备发现模块状态机
 */
typedef enum {
    DEVD_SLE_STATUS_PENDING,                          /*!< 待处理状态 */
    DEVD_SLE_STATUS_EXECUTING,                        /*!< 执行状态 */
    DEVD_SLE_STATUS_IDLE,                             /*!< 空闲状态 */
    DEVD_SLE_STATUS_SET_RANDOM_ADDR,                  /*!< 设置随机地址状态 */
    DEVD_SLE_STATUS_SET_ADV_PARAMS,                   /*!< 设置广播参数状态 */
    DEVD_SLE_STATUS_SET_ADV_DATA,                     /*!< 设置广播数据状态 */
    DEVD_SLE_STATUS_SET_SCAN_RSP_DATA,                /*!< 设置扫描响应数据状态 */
    DEVD_SLE_STATUS_DISABLE_ADV,                      /*!< 广播关闭状态 */
    DEVD_SLE_STATUS_ENABLE_ADV,                       /*!< 广播使能状态 */
    DEVD_SLE_STATUS_REMOVE_ADV,                       /*!< 移除广播状态 */
    DEVD_SLE_STATUS_CLEAR_ADV,                        /*!< 清除广播状态 */
    DEVD_SLE_STATUS_SET_SCAN_PARAMS,                  /*!< 设置扫描参数状态 */
    DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE,          /*!< 扫描由使能到关闭状态 */
    DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE,          /*!< 扫描由关闭到使能状态 */
    DEVD_SLE_STATUS_COMPLETE,                         /*!< 完成状态 */
} DevdSleStatus_S;

/**
 * @brief 设备发现模块回调事件
 */
typedef enum {
    DEVD_CBK_EVENT_SET_ADV_PARAMS,                        /*!< 设置广播参数 */
    DEVD_CBK_EVENT_SET_ADV_DATA,                          /*!< 设置广播数据 */
    DEVD_CBK_EVENT_SET_SCAN_RSP_DATA,                     /*!< 设置扫描响应数据 */
    DEVD_CBK_EVENT_ENABLE_ADV,                            /*!< 使能特定广播 */
    DEVD_CBK_EVENT_DISABLE_ADV,                           /*!< 禁用特定广播 */
    DEVD_CBK_EVENT_REMOVE_ADV,                            /*!< 移除特定广播 */
    DEVD_CBK_EVENT_CLEAR_ADV,                             /*!< 清除所有广播 */
    DEVD_CBK_EVENT_TERMINATED_ADV,                        /*!< 底层终止广播 */

    DEVD_CBK_EVENT_SET_SCAN_PARAMS,                       /*!< 设置扫描参数 */
    DEVD_CBK_EVENT_ENABLE_SCAN,                           /*!< 使能扫描 */
    DEVD_CBK_EVENT_DISABLE_SCAN,                          /*!< 禁用扫描 */
    DEVD_CBK_EVENT_ADV_REPORT,                            /*!< 扫描数据上报 */
    DEVD_CBK_EVENT_SET_SCAN_FILTER,                       /*!< 设置扫描过滤器 */

    DEVD_CBK_EVENT_GET_PUBLIC_ADDR,                      /*!< 获取public地址 */

    DEVD_CBK_EVENT_MAX
} DevdCbkEventType_E;

/**
 * @brief  广播数据分片操作
 */
typedef enum {
    ADV_OPERATION_FRAGMENT_COMPLETE     = 0x00,
    ADV_OPERATION_FRAGMENT_FIRST        = 0x01,
    ADV_OPERATION_FRAGMENT_INTERMEDIATE = 0x02,
    ADV_OPERATION_FRAGMENT_LAST         = 0x03,
    ADV_OPERATION_FRAGMENT_INVALID      = 0x04,
} DevdAdvDataOperation_E;

/**
 * @brief 广播基础参数
 */
typedef struct {
    uint8_t advMode;            /* 广播类型 */
    uint8_t gtRole;             /* G/T角色协商指示 */
    uint8_t channelMap;         /* 广播信道 */
    uint32_t advMinInterval;    /* 最小广播间隔 */
    uint32_t advMaxInterval;    /* 最大广播间隔 */
    SLE_Addr_S ownAddr;         /* 本端地址 */
    SLE_Addr_S peerAddr;        /* 对端地址 */
    uint8_t primaryFrameType;   /* 基础广播无线帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
} DevdAdvBasicParam_S;

/**
 * @brief 广播连接参数
 */
typedef struct {
    uint16_t connMinInterval;    /* 最小连接间隔 */
    uint16_t connMaxInterval;    /* 最大连接间隔 */
    uint16_t maxLatency;         /* 最大休眠连接间隔 */
    uint16_t supervisionTimeout; /* 最大超时事件 */
    uint16_t minCeLength;        /* 推荐的连接事件的最小取值 */
    uint16_t maxCeLength;        /* 推荐的连接事件的最大取值 */
} DevdAdvConnectParam_S;

/**
 * @brief 广播信道参数
 */
typedef struct {
    uint8_t primAdvFrameFormat; /* 基础广播无线帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
    uint8_t primaryPhy;       /* 基础广播带宽 */
    uint8_t secondaryFrame;   /* 扩展广播帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
    uint8_t secondaryPhy;     /* 数据广播带宽 */
    uint8_t secondaryPilot;   /* 数据广播pilot码率 */
    uint8_t secondaryMcs;     /* 数据广播编码模式 */
    uint8_t secondaryMaxSkip; /* 0:优先发送数据广播 0x1~0xff:发送数据广播前可跳过的最大evt数 */
} DevdAdvPhyParam_S;

/**
 * @brief 广播扫描参数
 */
typedef struct {
    uint8_t notifyEnable;   /* 是否上报收到的扫描请求 */
    uint8_t rxMaxNumber;    /* 一个广播周期内扫描请求最大接收数目 */
    uint16_t rxMaxDuration; /* 一个广播周期内扫描请求最大接收时间 */
} DevdAdvScanParam_S;

/**
 * @brief 广播参数
 */
typedef struct {
    uint8_t handle;                 /* 广播句柄 */
    uint8_t discoveryLevel;         /* 发现等级 */
    uint8_t filterPolicy;           /* 广播过滤 */
    uint8_t sid;                    /* 广播分组 */
    int8_t txPower;                /* 广播发送功率 */
    DevdAdvBasicParam_S basic;     /* 广播基础参数 */
    DevdAdvConnectParam_S connect; /* 广播连接参数 */
    DevdAdvPhyParam_S phy;         /* 广播信道参数 */
    DevdAdvScanParam_S scan;       /* 广播扫描参数 */
} DevdAdvParam_S;

/**
 * @brief 广播参数设置集
 */
typedef struct {
    uint8_t accessMode;     /* 接入层类型 */
    DevdAdvParam_S param;  /* 广播参数 */
    NLSTK_DevdAdvData_S data;    /* 广播数据 */
} DevdSetAdvParams_S;

typedef struct {
    uint16_t sendLen;
    uint16_t dataOffset;
} DevdAdvDataOp_S;

/**
 * @brief 广播节点
 */
typedef struct {
    SDF_DListEntry_S entry;     /* 链表节点 */
    uint8_t handle;             /* 广播句柄 */
    uint8_t status;             /* 广播节点状态 */
    uint8_t advStatus;          /* 广播使能状态 */
    bool isUpdateData;          /* 是否为更新广播数据操作 */
    DevdAdvParam_S *param;     /* 正式广播参数 */
    DevdAdvParam_S *tempParam; /* 临时广播参数 */
    NLSTK_DevdAdvData_S *data;       /* 正式广播数据 */
    NLSTK_DevdAdvData_S *tempData;   /* 临时广播数据 */
    DevdAdvDataOp_S dataOp;    /* 数据发送操作 */
    NLSTK_DevdAdvEventCbk cbk;       /* 广播回调 */
} DevdAdvNode_S;

typedef struct {
    uint8_t status;
    uint8_t advHandle;
    uint16_t connHandle;
} DevdAdvTerminatedEvent_S;

void DevdFreeAdvData(NLSTK_DevdAdvData_S *data);
void DevdFreeSetAdvData(void *arg);
void DevdFreeSetAdvParams(void *arg);
void DevdFreeAdvNode(DevdAdvNode_S *node);

DevdAdvNode_S *DevdCreateAdvNode(uint8_t handle, NLSTK_DevdAdvEventCbk cbk, SDF_DListHead_S *list);
DevdAdvNode_S *DevdGetAdvNode(uint8_t handle, SDF_DListHead_S *list);
void DevdRemoveAdvNode(uint8_t handle, SDF_DListHead_S *list);

uint32_t DevdSaveAdvParamToTbl(DevdAdvNode_S *node, DevdAdvParam_S *param);
uint32_t DevdSaveAdvDataToTbl(DevdAdvNode_S *node, NLSTK_DevdAdvData_S *data);

/**
 * @brief  扫描结果报告
 */
typedef struct {
    uint8_t deviceNum;                             /*!< 设备数量 */
    NLSTK_DevdAdvReportInfo_S *deviceList[0];  /*!< 设备信息列表 */
} DevdAdvReport_S;

/**
 * @brief  扫描结果报告单设备信息
 */
typedef struct {
    uint8_t addrType;                   /*!< 地址类型，参考NLSTK_AddrType_E定义 */
    uint8_t addr[SLE_ADDR_LEN];        /*!< 地址 */
    uint8_t eventType;                  /*!< 上报event类型 */
} DevdMpcAdvReport_S;

#ifdef __cplusplus
}
#endif
#endif