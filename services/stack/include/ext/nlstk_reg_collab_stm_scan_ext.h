
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

#ifndef NLSTK_REG_COLLAB_STM_SCAN_EXT_H
#define NLSTK_REG_COLLAB_STM_SCAN_EXT_H

#include <stdint.h>
#include "sdf_vector.h"
#include "sdf_addr.h"
#include "nlstk_public_define_ext.h"
#include "nlstk_devd_scan_type_ext.h"
#include "nlstk_stm_collab_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /**
     * @brief 创建一个扫描模块
     * @param scanCbk
     * @return moduleId
     */
    uint8_t (*createScanModule)(NLSTK_DevdScanCbk_S *scanCbk);

    /**
     * @brief 销毁一个扫描模块
     * @param moduleId
     */
    void (*removeScanModule)(uint8_t moduleId);

    /**
     * @brief 创建一个扫描器
     * @param moduleId
     * @return scannerId
     */
    uint32_t (*createScanner)(uint8_t moduleId);

    /**
     * @brief 销毁一个扫描器
     * @param scannerId
     */
    void (*destroyScanner)(uint32_t scannerId);

    /**
     * @brief 添加扫描参数
     * @param scannerId
     * @param setting 扫描设置参数
     * @param filters  base data: NLSTK_DevdScanFilter_S
     */
    NLSTK_Errcode_E (*addScanParam)(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting, SDF_Vector_S *filters);

    /**
     * @brief 移除扫描参数
     * @param scannerId
     */
    NLSTK_Errcode_E (*removeScanParam)(uint32_t scannerId);

    /**
     * @brief 下发过滤器
     * @param filter 过滤器参数
     */
    void (*setScanfilter)(NLSTK_DevdScanFilter_S *filter);
} COLLAB_StmDevdScanCbk_S;

typedef struct {
    NLSTK_Errcode_E (*regCollabFunc)(const NLSTK_DevdCollabScanFunc_S *cbks);
    void (*unRegCollabFunc)(void);
    NLSTK_Errcode_E (*regScanFilterFunc)(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc);
    void (*unRegScanFilterFunc)(void);
    NLSTK_Errcode_E (*setStmStateName)(uint8_t devdScanState, const char *scanStateName);
    const char* (*getStmStateName)(uint8_t devdScanState);
} COLLAB_DevdCollabStmFunc_S;

typedef struct {
    /**
     * @brief 在协议栈扫描状态机COLLAB模块初始化后调用
     */
    void (*initAfterFunc)(void);
} COLLAB_StmDevdScanInitAfterReg_Cbk_S;

#ifdef __cplusplus
}
#endif

#endif // NLSTK_REG_COLLAB_STM_SCAN_EXT_H