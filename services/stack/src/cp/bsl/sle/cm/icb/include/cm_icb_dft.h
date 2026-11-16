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
#ifndef CM_ICB_DFT_H
#define CM_ICB_DFT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************************
                                    ICB模块打点数据定义
*****************************************************************************************/

/**
 * @brief ICB事件打点字段
 *
 * 取值与服务侧 services/dft/include/nearlink_dft_database.h 的DftStackIcbExcepParamEnum（ICB_EXCEP_*）一一对应
 * 禁止单侧增删枚举项，否则事件字段会错位
 */
typedef enum {
    ICB_DFT_EXCEP_DEVICE_ADDR = 2,
    ICB_DFT_EXCEP_DEVICE_NAME = 3,
    ICB_DFT_EXCEP_DEVICE_APPEARANCE = 4,
    ICB_DFT_EXCEP_SCENE = 5,
    ICB_DFT_EXCEP_ID = 6,
    ICB_DFT_EXCEP_ERRORCODE = 7,
} CMIcbDftStackExcepParam_E;

/**
 * @brief ICB打点异常场景
 */
typedef enum {
    ICB_DFT_SCENE_IOG_SET_PARAM = 0,
    ICB_DFT_SCENE_IMG_SET_PARAM,
} CMIcbDftScene_E;

void CMIcbDftReportExcep(uint16_t scene, uint8_t id, uint16_t status);

#ifdef __cplusplus
}
#endif

#endif /* CM_ICB_DFT_H */
