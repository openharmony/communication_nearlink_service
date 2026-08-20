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
    HADM_DFT_EXCEP_DEVICE_ADDR = 1,
    HADM_DFT_EXCEP_READ_LOCAL_CS_TIME = 3,
    HADM_DFT_EXCEP_READ_LOCAL_CS_END_TIME,
    HADM_DFT_EXCEP_READ_REMOTE_CS_TIME,
    HADM_DFT_EXCEP_READ_REMOTE_CS_END_TIME,
    HADM_DFT_EXCEP_ENABLE_TIME,
    HADM_DFT_EXCEP_ENABLE_END_TIME,
    HADM_DFT_EXCEP_ERR_CODE,
    HADM_DFT_EXCEP_RES,
} HadmDftStackExcepParam_E;

/**
 * @brief HADM打点异常类型
 */
typedef enum {
    HADM_DFT_EVT_RES_NULL = 0X100,
    HADM_DFT_EVT_READ_LOCAL_MEASURE_ERR,
    HADM_DFT_IQ_DATA_SIZE_ERR,
    HADM_DFT_IQ_REBUILD_DATA_ERR,
    HADM_DFT_EVT_INVALID_STATE_WHEN_IQ,
} HadmDftErrCode_E;

void HadmDftReportExcep(SLE_Addr_S *addr, uint16_t errCode, uint16_t res);

#ifdef __cplusplus
}
#endif

#endif /* HADM_DFT_H */