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
#ifndef NLSTK_CFGDB_API_H
#define NLSTK_CFGDB_API_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *data;
    uint32_t len;
} NLSTK_CfgdbChanInfo_S;

typedef void (*NLSTK_CfgdbConfigListenerFunc)(void *args);

// 订阅CFGDB的模块
typedef enum {
    NLSTK_CFGDB_MODULE_NEARLINK_SERVICE = 0,
    NLSTK_CFGDB_MODULE_QOSM = 1,
    NLSTK_CFGDB_MODULE_MAX,
} NLSTK_CfgdbModule_E;

// CFGDB可订阅的配置数据
typedef enum {
    NLSTK_CFGDB_CONFIG_PUBLIC_ADDRESS = 0,
    NLSTK_CFGDB_CONFIG_LOCAL_VERSION,
    NLSTK_CFGDB_CONFIG_LOCAL_FEATURE,
    NLSTK_CFGDB_CONFIG_RSSI,
    NLSTK_CFGDB_CONFIG_POWER_LEVEL,
    NLSTK_CFGDB_CONFIG_CHIP_RESET,
    NLSTK_CFGDB_CONFIG_MAX,
} NLSTK_CfgdbConfig_E;

#define NLSTK_CFGDB_LOCAL_FEATURE_LEN 10

// 本地特性定义
typedef struct {
    uint16_t status;
    uint8_t feats[NLSTK_CFGDB_LOCAL_FEATURE_LEN]; /*!< 特性表格式，按协议bit定义 */
} NLSTK_CfgdbLocalFeatures_S;

#define NLSTK_MANUFACTURER_ABILITY_LEN 16

typedef struct {
    uint8_t ability[NLSTK_MANUFACTURER_ABILITY_LEN];
} NLSTK_ManufacturerAbility_S;

typedef struct {
    NLSTK_CfgdbConfigListenerFunc rssiCbk;
    NLSTK_CfgdbConfigListenerFunc powerLevelCbk;
    NLSTK_CfgdbConfigListenerFunc localFeatureCbk;
    NLSTK_CfgdbConfigListenerFunc chipResetNotifyCbk;
} NLSTK_CfgdbCbk_S;

/**
 * @brief 订阅配置
 *
 * 不可重入函数，仅在单线程环境下使用
 *
 * @param[in] func 订阅的函数回调
 * @param[in] module 当前模块
 * @param[in] config 订阅的配置
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_CfgdbRegisterConfigListener(NLSTK_CfgdbConfigListenerFunc func, NLSTK_CfgdbModule_E module,
    NLSTK_CfgdbConfig_E config);

/**
 * @brief 协议栈配置对外注册监听接口
 *
 * @param[in] func 订阅的函数回调
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_CfgdbRegisterCbks(NLSTK_CfgdbCbk_S *cbkIn);

/**
 * @brief 读取设备公共地址
 *
 * 必须在协议栈初始化并使能完成后才能调用
 * @param[out] addr 设备公共地址
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR: 失败
 */
uint32_t NLSTK_CfgdbGetPublicAddress(SLE_Addr_S *addr);

/**
 * @brief 抓取芯片日志
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR: 失败
 */
uint32_t NLSTK_CfgdbGetChipLog(void);

/**
 * @brief 设置设备厂商自定义能力
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR: 失败
 */
uint32_t NLSTK_CfgdbSetManufacturerAbility(SLE_Addr_S *addr, NLSTK_ManufacturerAbility_S *ability);

/**
 * @brief 移除设备厂商自定义能力
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR: 失败
 */
uint32_t NLSTK_CfgdbRemoveManufacturerAbility(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif
#endif
