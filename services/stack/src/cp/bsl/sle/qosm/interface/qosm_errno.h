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
 * this file contains errno of QOSM module.
 *
 ***************************************************************************/

#ifndef QOSM_ERRNO_H
#define QOSM_ERRNO_H

#include "cp_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_SUCCESS 0

#define QOSM_FAIL CP_MAKE_NCP_QOSM_ERRNO(1)
#define QOSM_NULL_PTR_ERR CP_MAKE_NCP_QOSM_ERRNO(2)
#define QOSM_MALLOC_ERR CP_MAKE_NCP_QOSM_ERRNO(3)
#define QOSM_POST_TASK_ERR CP_MAKE_NCP_QOSM_ERRNO(4)
#define QOSM_MAP_CREATE_ERR CP_MAKE_NCP_QOSM_ERRNO(5)
#define QOSM_LOGIC_LINK_FOUND_ERR CP_MAKE_NCP_QOSM_ERRNO(6)
#define QOSM_INIT_DUPLICATE_ERR CP_MAKE_NCP_QOSM_ERRNO(7)
#define QOSM_FLOW_MAP_DELETE_ERR CP_MAKE_NCP_QOSM_ERRNO(8)
#define QOSM_INVALID_QFI_ERR CP_MAKE_NCP_QOSM_ERRNO(9)
#define QOSM_QOS_FLOW_CREATE_ERR CP_MAKE_NCP_QOSM_ERRNO(10)
#define QOSM_TRANS_CHANN_CREATE_ERR CP_MAKE_NCP_QOSM_ERRNO(11)
#define QOSM_BIND_QOS_FLOW_AND_TRANS_CHANN_ERR CP_MAKE_NCP_QOSM_ERRNO(12)
#define QOSM_FIND_TRANS_CHANN_ERR CP_MAKE_NCP_QOSM_ERRNO(13)
#define QOSM_INVALID_SLQI_ERR CP_MAKE_NCP_QOSM_ERRNO(14)
#define QOSM_INVALID_COUNT_ERR CP_MAKE_NCP_QOSM_ERRNO(15)
#define QOSM_INVALID_PARAM_ERR CP_MAKE_NCP_QOSM_ERRNO(16)
#define QOSM_REGISTER_CBK_ERR CP_MAKE_NCP_QOSM_ERRNO(17)
#define QOSM_UNREGISTER_CBK_ERR CP_MAKE_NCP_QOSM_ERRNO(18)
#define QOSM_MEMCPY_ERR CP_MAKE_NCP_QOSM_ERRNO(19)
#define QOSM_NOT_FOUND_ERR CP_MAKE_NCP_QOSM_ERRNO(20)
#define QOSM_REPEATED_ERR CP_MAKE_NCP_QOSM_ERRNO(21)
#define QOSM_GET_CAP_INFO_ERR CP_MAKE_NCP_QOSM_ERRNO(22)
#define QOSM_EXCEED_MAX_NUM_ERR CP_MAKE_NCP_QOSM_ERRNO(23)
#define QOSM_UNSUPPORTED_ERR CP_MAKE_NCP_QOSM_ERRNO(24)
#define QOSM_INVALID_MTU_ERR CP_MAKE_NCP_QOSM_ERRNO(25)

#ifdef __cplusplus
}
#endif
#endif