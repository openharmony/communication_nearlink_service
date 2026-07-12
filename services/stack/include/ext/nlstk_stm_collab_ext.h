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

#ifndef NLSTK_REG_STM_COLLAB_EXT_H
#define NLSTK_REG_STM_COLLAB_EXT_H

#include <stdint.h>
#include "sdf_stm.h"
#include "sdf_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t (*collabScanStmInit)(StateMachine *stm);      /*!< 需要在状态机先分配完成后调用 */
    uint32_t (*collabGetScannerMaxNum)(void);
    uint32_t (*collabScanStmCtorAfter)(void);              /*!< 需要在状态机构造完成后调用 */
    void (*collabScanStmDeInit)(void);
    bool (*isNeedTryStopScanCollab)(State *state);
    void (*notifyScanCollabResult)(State *state, bool stopped);
    State* (*createStartCollabState)(StateMachine *stm);
    State* (*createStopCollabState)(StateMachine *stm);
    bool (*isNeedStartScanCollabReqAndTransState)(State *state);
    bool (*isNeedStopScanCollabReqAndTransState)(State *state);
} NLSTK_DevdCollabScanFunc_S;

/**
 * @param  [in] scanners base data: NLSTK_DevdScanner_S
 * @param  [in] filters base data: NLSTK_DevdScanFilter_S, 可能为空
 */
typedef bool (*NLSTK_DevdCollabSetFilters)(const SDF_Vector_S *scanners, uint32_t scannerId, SDF_Vector_S *filters);

typedef struct {
    NLSTK_DevdCollabSetFilters setFilters;
} NLSTK_DevdCollabScanFilterFunc_S;

typedef struct {
    bool (*isNeedLinkCollabReq)(void);
    /**
     * @param connInitiateType 当前发起建链类型
     * @param addr 主动建链时，发起建链的地址，允许为空；不为空时，则表示需要断开协同侧若存在已完成连接的地址
     */
    bool (*startLinkCollabReq)(uint8_t connInitiateType, const SLE_Addr_S *addr);
    bool (*notifyLinkCollabResult)(void);
} CM_LinkCollabFunc_S;

#ifdef __cplusplus
}
#endif
#endif
