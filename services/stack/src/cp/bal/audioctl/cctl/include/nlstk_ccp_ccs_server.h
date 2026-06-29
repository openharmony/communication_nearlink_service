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
#ifndef NLSTK_CCP_CCS_SERVER_H
#define NLSTK_CCP_CCS_SERVER_H

#include "sdf_addr.h"
#include "nlstk_ccp_ccs_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NLSTK_CcpStartCallControlServiceInst)(int32_t instanceId, NLSTK_Errcode_E ret);

typedef void (*NLSTK_CcpCallControlServiceAuthorize)(uint32_t requestId, int32_t instanceId,
    NLSTK_CcpCcsPropertyType_E property, NLSTK_ServicePropertyOpType_E operation);

typedef void (*NLSTK_CcpAnswer)(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);
typedef void (*NLSTK_CcpHangUp)(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);

typedef struct {
    NLSTK_CcpAnswer answer;
    NLSTK_CcpHangUp hangUp;
} NLSTK_CcpCallControlPoint_S;

/**
 * @brief 通话控制相关信息结构体
 */
typedef struct {
    NLSTK_CcpCcsType_E type;                                          /**< 通话控制服务类型 */
    NLSTK_VariableData_S instanceName;                         /**< 通话控制实例名称 */
    uint8_t featureStatus;                                          /**< 特性状态 */
    NLSTK_VariableData_S protocolSupport;                      /**< 协议支持 */
    NLSTK_CcpCallInOutInfo_S callInOutInfo;                           /**< 呼入呼出信息 */
    NLSTK_CcpCallStatues_S callStatus;                                /**< 通话状态 */
    NLSTK_CcpCallTermination_S callTermination;                       /**< 通话终止 */
    uint8_t mediaInstanceId;                                        /**< 媒体实例标识 */
    uint16_t callRequestSupport;                                    /**< 通话请求支持 */
    NLSTK_CcpInstanceIcon_S instanceIcon;                             /**< 通话控制实例图标（可选） */
    bool instanceIconFlag;                                          /**< 通话控制实例图标可选标识 */
    NLSTK_VariableData_S networkSelection;                     /**< 网络选择（可选） */
    bool networkSelectionFlag;                                      /**< 网络选择可选标识 */
    NLSTK_CcpCallControlPoint_S callControlPoint;                     /**< 通话控制点 */
    NLSTK_CcpStartCallControlServiceInst startCcsInst;                /**< 添加通话控制服务实例回调 */
    NLSTK_CcpCallControlServiceAuthorize authorize;                   /**< 通话控制授权回调 */
    uint8_t propertyRights[NLSTK_CCP_CCS_MAX_PROPERTY];               /**< 针对每个属性的操作权限 */
} NLSTK_CcpCallControlInfo_S;

/**
 * @brief 创建一个通话控制服务实例
 *
 * 该函数用于创建一个通话控制服务实例，并根据提供的通话信息进行初始化；
 * 通话控制服务含有可选属性，用户可以根据需求添加，用户需维护所有添加的通话控制服务属性的更新确保通话状态一致。
 *
 * @param[in] baseInfo 通话控制服务信息结构体指针
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpCreateCcsInstance(NLSTK_CcpCallControlInfo_S *baseInfo);

/**
 * @brief 通知通话控制执行结果
 *
 * 该函数用于在业务处理完 NLSTK_CcpCallControlPoint_S 回调后，通知CCP通话控制执行的结果；
 * 若方法调用涉及到通话属性的更新，应该先回复方法调用响应，然后再通过 NLSTK_CcpUpdateCcsProperty 更新通话属性。
 *
 * @param[in] requestId 请求标识，用于标识具体的请求序号，该标识由 NLSTK_McpPlayControl_S 回调函数传入
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] errorCode 操作结果，0表示成功，其他值表示失败，表示 NLSTK_McpPlayControl_S 回调函数的操作结果
 *
 * @return uint32_t 返回调用结果
 *
 */
void NLSTK_CcpCallControlResult(uint32_t requestId, int32_t instanceId, uint8_t errorCode);

/**
 * @brief 返回读写通话控制服务属性授权结果
 *
 * 该函数用于用户处理完读写通话控制服务属性授权请求后返回授权结果，通话控制服务实例根据错误码决定后续操作。
 *
 * @param[in] requestId 请求标识，标识当前操作的唯一请求
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] property 属性类型，指定操作的目标属性
 * @param[in] errorCode 错误码，决定后续处理逻辑
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpCcsAuthorizeResult(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
    uint8_t errorCode);

/**
 * @brief 更新通话控制服务属性值
 *
 * 该函数用于用户更新通话控制服务中的属性值，若通话控制端订阅了相应属性描述符，SSAP将发送属性变更通知给通话控制端。
 *
 * @param[in] instanceId 实例标识，标识具体的服务实例
 * @param[in] property 属性类型，指定需要更新的属性
 * @param[in] value 属性值指针，指向需要更新的新值
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpUpdateCcsProperty(int32_t instanceId, NLSTK_CcpCcsPropertyType_E property, void *value);

/**
 * @brief 删除一个通话控制服务实例
 *
 * 该函数用于删除一个通话控制服务实例，销毁相应的SSAP服务和缓存资源。
 *
 * @param[in] instanceId 实例标识，标识需要清理的服务实例
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_CcpDeleteCcsInstance(int32_t instanceId);

#ifdef __cplusplus
}
#endif
#endif