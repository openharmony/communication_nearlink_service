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

/****************************************************************************
 *
 * this file contains the CM errno definitions
 *
 ***************************************************************************/

#ifndef CM_ERRNO_H
#define CM_ERRNO_H

#include "cp_errno_base.h"

#define CM_SUCCESS              0x00

#define CM_INVALID_MODULE_TYPE   CP_MAKE_NCP_CM_ERRNO(101)
#define CM_FAIL                  CP_MAKE_NCP_CM_ERRNO(102)
#define CM_INVALID_PARAM_ERR     CP_MAKE_NCP_CM_ERRNO(103)
#define CM_NOT_IMPLEMENT         CP_MAKE_NCP_CM_ERRNO(104)
#define CM_MEM_ERR               CP_MAKE_NCP_CM_ERRNO(105)
#define CM_NOT_INITED            CP_MAKE_NCP_CM_ERRNO(106)
#define CM_NOT_FOUND             CP_MAKE_NCP_CM_ERRNO(107)
#define CM_NULL_POINTER          CP_MAKE_NCP_CM_ERRNO(108)
#define CM_MAP_INSERT_ERR        CP_MAKE_NCP_CM_ERRNO(109)
#define CM_TCID_NOT_SUPPOPRT_ERR CP_MAKE_NCP_CM_ERRNO(110)
#define CM_REPEAT_CALLED         CP_MAKE_NCP_CM_ERRNO(111)

#endif