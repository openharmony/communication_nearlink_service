/**
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
 * this file defines transport module error code.
 *
 ***************************************************************************/

#ifndef TRANSPORT_ERRNO_H
#define TRANSPORT_ERRNO_H

#include "dpfwk_errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRANS_SUCCESS 0
#define TRANS_BUSY 1
#define TRANS_FAIL (-1)

/* 注册回调错误码 */
#define TRANS_REGISTER_CBKS_NULL_PTR DP_MAKE_TRANS_FWK_ERRNO(101)

/* 初始化失败错误码 */
#define TRANS_INIT_REGISTER_FAILED DP_MAKE_TRANS_FWK_ERRNO(201)
#define TRANS_INIT_DUPLICATE_FAILED DP_MAKE_TRANS_FWK_ERRNO(202)

/* 发送数据错误码 */
#define TRANS_SEND_DATA_INVALID_PARAMS DP_MAKE_TRANS_FWK_ERRNO(301)
#define TRANS_SEND_DATA_MALLOC_FAILED DP_MAKE_TRANS_FWK_ERRNO(302)
#define TRANS_SEND_DATA_APPEND_FAILED DP_MAKE_TRANS_FWK_ERRNO(303)
#define TRANS_SEND_DATA_POST_FAILED DP_MAKE_TRANS_FWK_ERRNO(304)
#define TRANS_SEND_DATA_OVERFLOW DP_MAKE_TRANS_FWK_ERRNO(305)

#ifdef __cplusplus
}
#endif

#endif