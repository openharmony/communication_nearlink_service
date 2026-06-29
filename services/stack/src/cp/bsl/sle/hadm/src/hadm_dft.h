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
#ifndef HADM_DFT_H
#define HADM_DFT_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_dft.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************************
                                    HADM模块打点数据定义
*****************************************************************************************/

/**
 * @brief HADM事件打点字段
 */
typedef enum {
    HADM_DFT_DEVICE_ADDR = 1,
    HADM_DFT_READ_LOCAL_MEASURE_TIME = 3,
    HADM_DFT_READ_LOCAL_MEASURE_END_TIME,
    HADM_DFT_READ_REMOTE_MEASURE_TIME,
    HADM_DFT_READ_REMOTE_MEASURE_END_TIME,
    HADM_DFT_ENABLE_TIME,
    HADM_DFT_ENABLE_END_TIME,
    HADM_DFT_RES,
} HadmDftExcepParam_E;

/**
 * @brief HADM打点异常类型
 */
typedef enum {
    HADM_DFT_EVT_RES_NULL = 0X100,
    HADM_DFT_EVT_READ_LOCAL_MEASURE_ERR,
    HADM_DFT_IQ_DATA_SIZE_ERR,
    HADM_DFT_IQ_REBUILD_DATA_ERR,
} HadmDftErrCode_E;

typedef enum {
    HADM_DFT_RES_SUCC = 0,
    HADM_DFT_RES_FAIL = 1,
} HadmDftRes_E;

void HadmDftCacheTimestamp(NLSTK_DftEventId_E eventId, uint16_t paramId);
void HadmDftReport(uint16_t errVal);

#ifdef __cplusplus
}
#endif

#endif /* HADM_DFT_H */