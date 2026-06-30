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
#ifndef CDSM_CONTROL_H
#define CDSM_CONTROL_H

#include "sdf_addr.h"
#include "cdsm_tbl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 合作集客户端服务缓存去初始化
 *
 * 该函数用于对合作集服务缓存去初始化
 *
 * @param[in] void
 *
 * @return void
 */
void CdsmServiceDeInit(void);

/**
 * @brief 合作集连接profile开始函数
 *
 * 该函数用于向合作集成员读取服务属性信息，从而实现合作集profile的状态连接
 *
 * @param[in] addr 合作集成员地址
 *
 * @return void
 */
void CdsmConnectProfile(CdsmCoopSetMeb_S *setMeb);

/**
 * @brief 合作集发起定向连接广播邀请
 *
 * 该函数用于设置广播参数和数据，并发起定向连接广播邀请其他合作集成员连接
 *
 * @param[in] addr 合作集成员地址
 *
 * @return void
 */
void CdsmStartAdv(SLE_Addr_S *addr);

void CdsmStopAdv(CdsmCoopSet_S *coopSet);

/**
 * @brief 合作集profile注册ssap app
 *
 * 该函数用于向ssap client注册appId
 *
 * @param[in] meb 合作集成员
 *
 * @return NLSTK_OK: 成功 OTHER: 失败
 */
uint32_t CdsmRegisterSsapApp(CdsmCoopSetMeb_S *meb);

#ifdef __cplusplus
}
#endif
#endif