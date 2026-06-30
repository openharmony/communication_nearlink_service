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
#ifndef NLSTK_SSAP_APP_SERVER_H
#define NLSTK_SSAP_APP_SERVER_H

#include "nlstk_public_define.h"
#include "ssap_type.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 服务描述符参数结构体
 * 该结构体用于描述服务的参数信息，包含权限、描述符类型、操作类型和变量长度值
 */
typedef struct NLSTK_SsapServiceDescriptorParam {
    NLSTK_SsapPermission_S permission;  ///< 权限信息
    NLSTK_SsapDescriptorType_E type;    ///< 描述符类型
    NLSTK_SsapOperation_S operation;    ///< 操作类型
    NLSTK_VariableData_S val;   ///< 变量长度值
} NLSTK_SsapServiceDescriptorParam_S;

/**
 * @brief 服务声明参数结构体
 * 该结构体用于声明服务，包含服务UUID、服务类型、描述符数量和描述符指针
 */
typedef struct NLSTK_SsapServiceStatementParam {
    NLSTK_SsapUuid_S uuid;                            ///< 服务UUID
    NLSTK_SsapItemType_E serviceType;                 ///< 服务类型
    uint16_t descriptorNum;                          ///< 描述符数量
    NLSTK_SsapServiceDescriptorParam_S *descriptors;  ///< 描述符指针
} NLSTK_SsapServiceStatementParam_S;

/**
 * @brief 服务引用参数结构体
 * 该结构体用于引用服务，包含服务UUID和服务类型
 */
typedef struct NLSTK_SsapServiceReferenceParam {
    NLSTK_SsapUuid_S uuid;             ///< 服务UUID
    NLSTK_SsapItemType_E serviceType;  ///< 服务类型
} NLSTK_SsapServiceReferenceParam_S;

/**
 * @brief 服务属性参数结构体
 * 该结构体用于描述服务属性，包含服务UUID、权限、操作类型、变量长度值、描述符数量和描述符指针
 */
typedef struct NLSTK_SsapServicePropertyParam {
    NLSTK_SsapItemType_E type;                        ///< 属性类型
    NLSTK_SsapUuid_S uuid;                            ///< 服务UUID
    NLSTK_SsapPermission_S permission;                ///< 权限信息
    NLSTK_SsapOperation_S operation;                  ///< 操作类型
    NLSTK_VariableData_S val;                 ///< 变量长度值
    uint16_t descriptorNum;                          ///< 描述符数量
    NLSTK_SsapServiceDescriptorParam_S *descriptors;  ///< 描述符指针
} NLSTK_SsapServicePropertyParam_S;

/**
 * @brief 服务方法参数结构体
 * 该结构体用于描述服务方法，包含方法类型、服务UUID和权限信息
 */
typedef struct NLSTK_SsapServiceMethodParam {
    NLSTK_SsapItemType_E type;          ///< 方法类型
    NLSTK_SsapUuid_S uuid;              ///< 服务UUID
    NLSTK_SsapPermission_S permission;  ///< 权限信息
} NLSTK_SsapServiceMethodParam_S;

/**
 * @brief 服务事件参数结构体
 * 该结构体用于描述服务事件，包含事件类型和服务UUID
 */
typedef struct NLSTK_SsapServiceEventParam {
    NLSTK_SsapItemType_E type;  ///< 事件类型
    NLSTK_SsapUuid_S uuid;      ///< 服务UUID
} NLSTK_SsapServiceEventParam_S;

/**
 * @brief 属性更新回调参数
 * 该结构体用于serivce调用NLSTK_SsapServerUpdatePropertyValue更新属性时，更新结果通知回调函数
 */
typedef struct NLSTK_SsapServerOnSetPropertyParam {
    uint16_t handle;              ///< 服务句柄
    NLSTK_SsapUuid_S uuid;      ///< 服务UUID
    NLSTK_VariableData_S value;  ///< 属性值
} NLSTK_SsapServerOnSetPropertyParam_S;

/**
 * @brief 描述符更新回调参数
 * 该结构体用于serivce调用NLSTK_SsapServerUpdateDescriptorValue更新描述符时，更新结果通知回调函数
 */
typedef struct NLSTK_SsapServerOnSetDescriptorParam {
    uint16_t handle;              ///< 服务句柄
    NLSTK_SsapUuid_S uuid;      ///< 服务UUID
    uint8_t type;                 ///< 描述符类型
    NLSTK_VariableData_S value;  ///< 描述符值
} NLSTK_SsapServerOnSetDescriptorParam_S;

/**
 * @brief 属性更新回调参数
 * 该结构体用于serivce调用NLSTK_SsapServerUpdateAndNotifyProperty更新描述符时，更新结果通知回调函数
 */
typedef struct NLSTK_SsapServerOnNotifyPropertyParam {
    SLE_Addr_S addr;              ///< 请求来源地址，即客户端地址
    uint16_t handle;              ///< 描述符对应的句柄
    NLSTK_SsapUuid_S uuid;          ///< 属性的UUID
    NLSTK_VariableData_S value;  ///< 描述符值
} NLSTK_SsapServerOnNotifyPropertyParam_S;

/**
 * @brief 客户端读取属性回调函数参数
 * 该结构体用于服务端收到客户端的属性读取请求时，通知service读取请求的回调函数
 */
typedef struct NLSTK_SsapServerReadPropertyInfo {
    SLE_Addr_S addr;  ///< 请求来源地址，即客户端地址
    NLSTK_SsapUuid_S uuid;          ///< 属性的UUID
    uint16_t handle;  ///< 客户端期望读取的属性句柄
} NLSTK_SsapServerReadPropertyInfo_S;

/**
 * @brief 客户端读取描述符回调函数参数
 * 该结构体用于服务端收到客户端的描述符读取请求时，通知service读取请求的回调函数
 */
typedef struct NLSTK_SsapServerReadDescriptorInfo {
    SLE_Addr_S addr;  ///< 请求来源地址，即客户端地址
    uint16_t handle;  ///< 客户端期望读取的描述符所在句柄
    NLSTK_SsapUuid_S uuid;          ///< 属性的UUID
    uint8_t type;     ///< 客户端期望读取的描述符类型
} NLSTK_SsapServerReadDescriptorInfo_S;

/**
 * @brief 客户端写入属性回调函数参数
 * 该结构体用于服务端收到客户端的属性写入请求时，通知service写入请求的回调函数
 */
typedef struct NLSTK_SsapServerWritePropertyInfo {
    SLE_Addr_S addr;              ///< 请求来源地址，即客户端地址
    uint16_t handle;              ///< 客户端期望写入的属性句柄
    NLSTK_SsapUuid_S uuid;          ///< 属性的UUID
    NLSTK_VariableData_S value;  ///< 客户端期望写入的属性值
} NLSTK_SsapServerWritePropertyInfo_S;

/**
 * @brief 客户端写入描述符回调函数参数
 * 该结构体用于服务端收到客户端的描述符写入请求时，通知service写入请求的回调函数
 */
typedef struct NLSTK_SsapServerWriteDescriptorInfo {
    SLE_Addr_S addr;              ///< 请求来源地址，即客户端地址
    uint16_t handle;              ///< 客户端期望写入的描述符所在句柄
    NLSTK_SsapUuid_S uuid;          ///< 属性的UUID
    uint8_t type;                 ///< 客户端期望写入的描述符类型
    NLSTK_VariableData_S value;  ///< 客户端期望写入的描述符的值
} NLSTK_SsapServerWriteDescriptorInfo_S;

/**
 * @brief 客户端方法调用请求回调参数
 * 该结构体用于服务端收到客户端的CallMethod请求时，通知service处理CallMethod请求的回调函数
 */
typedef struct SSAP_CallMethodRequestInfo {
    SLE_Addr_S addr;              ///< 请求来源地址，即客户端地址
    uint16_t handle;              ///< 客户端期望调用的方法句柄
    NLSTK_SsapUuid_S uuid;          ///< 客户端期望调用的方法句柄对应的UUID
    NLSTK_VariableData_S param;  ///< 客户端发送CallMethod消息中所携带的参数
} NLSTK_SsapServerCallMethodRequestInfo_S;

/**
 * @brief 服务参数综合结构体
 * 该结构体用于综合描述服务的所有参数信息，包括服务声明、服务引用、服务属性、服务方法和服务事件
 */
typedef struct NLSTK_ServiceParam {
    NLSTK_SsapServiceStatementParam_S serviceStatement;  ///< 服务声明参数

    uint16_t serviceReferenceNum;                       ///< 服务引用数量
    NLSTK_SsapServiceReferenceParam_S *serviceReference;  ///< 服务引用指针

    uint16_t servicePropertyNum;               ///< 服务属性数量
    NLSTK_SsapServicePropertyParam_S *property;  ///< 服务属性指针

    uint16_t serviceMethodNum;             ///< 服务方法数量
    NLSTK_SsapServiceMethodParam_S *method;  ///< 服务方法指针

    uint16_t serviceEventNum;            ///< 服务事件数量
    NLSTK_SsapServiceEventParam_S *event;  ///< 服务事件指针
} NLSTK_ServiceParam_S;

/**
 * @brief 释放NLSTK_ServiceParam_S对象及相关资源
 */
void NLSTK_SsapFreeServiceParam(void *ptr);

/**
 * @brief MTU变化通知回调
 * 服务端收到了对端过来的exchangeinfo请求的时候，会通过这个接口通知上层APP
 */
typedef void (*NLSTK_SsapServerMtuChanged)(int32_t appId, SLE_Addr_S *addr, uint16_t mtu);

/**
 * @brief 服务添加结果通知回调
 * 当服务端用户调用NLSTK_SsapServerAddService接口添加服务时，会通过此回调通知上层应用添加的结果
 */
typedef void (*NLSTK_SsapServerAddServiceResult)(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret);

/**
 * @brief 属性值设置结果通知回调
 * 当服务端用户调用NLSTK_SsapServerUpdatePropertyValue接口更新属性值时，会通过此回调通知上层应用更新的结果
 */
typedef void (*NLSTK_SsapServerSetPropertyValue)(int32_t appId, NLSTK_SsapServerOnSetPropertyParam_S *param,
                                               NLSTK_Errcode_E ret);

/**
 * @brief 描述符值设置结果通知回调
 * 当服务端用户调用NLSTK_SsapServerUpdateDescriptorValue接口更新描述符值时，会通过此回调通知上层应用更新的结果
 */
typedef void (*NLSTK_SsapServerSetDescriptorValue)(int32_t appId, NLSTK_SsapServerOnSetDescriptorParam_S *descriptor,
                                                 NLSTK_Errcode_E ret);

/**
 * @brief 属性通知回调
 * 当服务端用户调用NLSTK_NotifyProperty接口通知属性值变化时，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerNotifyProperty)(int32_t appId, NLSTK_SsapServerOnNotifyPropertyParam_S *property,
                                             NLSTK_Errcode_E ret);

/**
 * @brief 读属性授权请求回调
 * 当服务端收到客户端的读属性请求时，若需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerReadPropertyAuthorizeRequest)(int32_t appId, uint16_t requestId,
                                                           NLSTK_SsapServerReadPropertyInfo_S *property);

/**
 * @brief 读属性请求回调
 * 当服务端收到客户端的读属性请求时，若不需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerReadProperty)(int32_t appId, NLSTK_SsapServerReadPropertyInfo_S *property);

/**
 * @brief 读描述符授权请求回调
 * 当服务端收到客户端的读描述符请求时，若需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerReadDescriptorAuthorizeRequest)(int32_t appId, uint16_t requestId,
                                                             NLSTK_SsapServerReadDescriptorInfo_S *descriptor);

/**
 * @brief 读描述符请求回调
 * 当服务端收到客户端的读描述符请求时，若不需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerReadDescriptor)(int32_t appId, NLSTK_SsapServerReadDescriptorInfo_S *descriptor);

/**
 * @brief 写属性授权请求回调
 * 当服务端收到客户端的写属性请求时，若需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerWritePropertyAuthorizeRequest)(int32_t appId, uint16_t requestId,
                                                            NLSTK_SsapServerWritePropertyInfo_S *property);

/**
 * @brief 写属性请求回调
 * 当服务端收到客户端的写属性请求时，若不需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerWriteProperty)(int32_t appId, NLSTK_SsapServerWritePropertyInfo_S *property);

/**
 * @brief 写描述符授权请求回调
 * 当服务端收到客户端的写描述符请求时，若需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerWriteDescriptorAuthorizeRequest)(int32_t appId, uint16_t requestId,
                                                              NLSTK_SsapServerWriteDescriptorInfo_S *descriptor);

/**
 * @brief 写描述符请求回调
 * 当服务端收到客户端的写描述符请求时，若不需要授权，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerWriteDescriptor)(int32_t appId, NLSTK_SsapServerWriteDescriptorInfo_S *descriptor);

/**
 * @brief 方法调用回调
 * 当服务端收到客户端的调用方法请求时，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerCallMethod)(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerCallMethodRequestInfo_S *method, bool needReturn, bool needAuth);

/**
 * @brief 连接状态变化回调
 * 当服务端感知到底层链路状态发生变化时，会通过此回调通知上层应用
 */
typedef void (*NLSTK_SsapServerConnectionStateChanged)(int32_t appId, const SLE_Addr_S *addr, uint8_t state,
                                                     NLSTK_Errcode_E ret, int32_t reason);

typedef void (*NLSTK_SsapServerRegisterApp)(int32_t appId, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapServerCleanAppResultCb)(void);

// 上层APP注册服务实例，同步接口，返回值表示appId，返回-1表示失败
typedef struct {
    // 服务端收到了对端过来的exchangeinfo请求的时候，会通过这个接口通知上层APP
    NLSTK_SsapServerMtuChanged onMtuChanged;

    // 服务端用户调用NLSTK_SsapServerAddService接口添加服务时，会通过这个回调通知上层APP添加的结果
    NLSTK_SsapServerAddServiceResult onAddService;

    // 服务端用户调用NLSTK_SsapServerUpdatePropertyValue接口更新属性值时，会通过这个回调通知上层APP更新的结果
    NLSTK_SsapServerSetPropertyValue onSetPropertyValue;

    // 服务端用户调用NLSTK_SsapServerUpdateDescriptorValue接口更新描述符值时，会通过这个回调通知上层APP更新的结果
    NLSTK_SsapServerSetDescriptorValue onSetDescriptorValue;

    // 服务端收到客户端的读属性请求时，若存在需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerReadPropertyAuthorizeRequest onReadPropertyAuthorizeRequest;

    // 服务端收到客户端的读取描述符请求时，若存在需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerReadDescriptorAuthorizeRequest onReadDescriptorAuthorizeRequest;

    // 服务端收到客户端的写属性请求时，若存在需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerWritePropertyAuthorizeRequest onWritePropertyAuthorizeRequest;

    // 服务端收到客户端的写描述符请求时，若存在需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerWriteDescriptorAuthorizeRequest onWriteDescriptorAuthorizeRequest;

    // 服务端收到客户端的读属性请求时，不需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerReadProperty onReadProperty;

    // 服务端收到客户端的读描述符请求时，不需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerReadDescriptor onReadDescriptor;

    // 服务端收到客户端的写属性请求时，不需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerWriteProperty onWriteProperty;

    // 服务端收到客户端的写描述符请求时，不需要授权时，会通过这个回调通知上层APP
    NLSTK_SsapServerWriteDescriptor onWriteDescriptor;

    // 服务端收到客户端的调用方法请求时，会通过这个回调通知上层APP，当前未实现
    NLSTK_SsapServerCallMethod onCallMethod;  // 未使用

    // service调用 NLSTK_NotifyProperty 触发此回调
    NLSTK_SsapServerNotifyProperty onNotifyProperty;

    // NLSTK_OnIndicateProperty onIndicateProperty;  // 未使用
    // NLSTK_OnNotifyEvent onNotifyEvent;            // 未使用
    // NLSTK_OnIndicateEvent onIndicateEvent;        // 未使用

    // 在服务端，当感知到底层链路发生变化时，会通过这个接口通知上层APP？
    // 问题： 服务端并不会和对端发送建链，如何建立appId和addr的关联？所有链路发生变化，都会通知所有的appId？
    NLSTK_SsapServerConnectionStateChanged onConnectionStateChanged;
    NLSTK_SsapServerRegisterApp onRegisterApp;
} NLSTK_SsapAppServerCb_S;

/**
 * @brief 注册应用服务器，同步接口
 * @details 该函数用于注册一个应用服务器，将回调函数传递给系统，以便在后续处理中使用。
 * @param [in] cb 指向回调函数结构体的指针
 * @param [out] appId 返回注册的appId，返回<0 注册失败，具体错误信息可通过错误码查看
 */
NLSTK_Errcode_E NLSTK_SsapServerRegApp(NLSTK_SsapAppServerCb_S *cb, int32_t *appId);

/**
 * @brief 注册应用服务器，异步接口
 * @details 该函数用于注册一个应用服务器，将回调函数传递给系统，以便在后续处理中使用。
 * @param [in] cb 指向回调函数结构体的指针
 */
NLSTK_Errcode_E NLSTK_SsapServerRegAppAsyn(NLSTK_SsapAppServerCb_S *cb);

/**
 * @brief 注销应用，同步接口
 * @details 该函数用于注销一个已注册的应用，释放相关资源并停止与该应用相关的服务。
 * @param [in] appId 应用的唯一标识符
 * @return void
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用将无法再使用
 */
void NLSTK_SsapServerDeregisterApplication(int32_t appId);

/**
 * @brief 注销应用，异步接口
 * @details 该函数用于注销一个已注册的应用，释放相关资源并停止与该应用相关的服务。
 * @param [in] appId 应用的唯一标识符
 * @return void
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用将无法再使用
 */
void NLSTK_SsapServerDeregisterApplicationAsync(int32_t appId);

/**
 * @brief 设置MTU，异步接口
 * @details 该函数将修改服务端的MTU值，通过此函数修改后，服务端收到客户端的exchangeinfo请求的时，
 *          将使用此MTU值和请求中的MTU进行比较，并取最小值进行回复。
 * @param [in] mtu 要设置的MTU值
 */
NLSTK_Errcode_E NLSTK_SsapServerSetMtu(uint16_t mtu);

/**
 * @brief 添加服务，异步接口
 * @details 该函数用于向指定应用添加一个服务，服务添加的结果会通过回调函数NLSTK_OnAddService返回。
 * @param [in] appId 应用的唯一标识符
 * @param [in] service 指向服务参数结构体的指针
 * @note 请确保service参数有效且已正确初始化
 */
NLSTK_Errcode_E NLSTK_SsapServerAddService(int32_t appId, const NLSTK_ServiceParam_S *service);

/**
 * @brief 删除服务，异步接口
 * @details 该函数用于删除指定应用的一个服务。
 * @param [in] appId 应用的唯一标识符
 * @param [in] handle 要删除的服务句柄，属于该服务起始句柄到结束句柄之间的任意一个值
 * @return NLSTK_Errcode_E 表示删除任务是否已经加入到任务队列中，0表示成功，其他值表示失败
 * @note 请确保handle有效，否则可能导致意外行为
 */
NLSTK_Errcode_E NLSTK_SsapServerRemoveService(int32_t appId, uint16_t handle);

/**
 * @brief 清除指定应用的所有服务，异步接口
 * @details 该函数用于删除指定应用ID下所有的服务，操作是异步进行的。
 * @param [in] appId 应用的唯一标识符
 * @return NLSTK_Errcode_E 表示删除任务是否已经加入到任务队列中，0表示成功，其他值表示失败
 * @note 请确保appId有效，否则可能导致意外行为。清除服务后，所有相关联的服务将被删除，无法再使用。
 */
NLSTK_Errcode_E NLSTK_SsapServerClearServices(int32_t appId);

/**
 * @brief 处理授权结果的回调函数，异步接口
 * @details 该函数用于处理授权请求的结果，根据`allow`参数决定授权结果。
 * @param [in] appId 应用的唯一标识符
 * @param [in] requestId 请求的唯一标识符
 * @param [in] allow 是否允许该请求
 * @return NLSTK_Errcode_E
 * @note 请确保`appId`和`requestId`的有效性
 */
NLSTK_Errcode_E NLSTK_SsapServerAuthorizeResult(int32_t appId, uint16_t requestId, bool allow);

/**
 * @brief 异步设置属性值，异步接口
 * @details 该函数用于根据应用ID和属性句柄设置属性值，是一个异步接口。
 * @param [in] appId 应用的唯一标识符
 * @param [in] handle 属性的句柄
 * @param [in] value 用于存储属性值的结构体指针
 * @return NLSTK_Errcode_E
 * @note 该函数为异步接口，调用后需通过回调处理结果
 */
NLSTK_Errcode_E NLSTK_SsapServerUpdatePropertyValue(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value);

/**
 * @brief 设置描述符值, 异步接口
 * @details 该函数用于根据应用ID和描述符句柄设置描述符值。
 * @param [in] appId 应用的唯一标识符
 * @param [in] handle 描述符的句柄
 * @param [in] type 描述符的类型（需确保与目标描述符匹配）
 * @param [in] value 用于存储描述符值的结构体指针
 * @return NLSTK_Errcode_E
 */
NLSTK_Errcode_E NLSTK_SsapServerUpdateDescriptorValue(int32_t appId, uint16_t handle, uint8_t type,
                                                  NLSTK_VariableData_S *value);

/**
 * @brief 更新属性值并向指定客户端通知变化，异步接口
 * @details 该函数用于更新指定应用和属性句柄的属性值，并将变化通知给相关方。
 * @param [in] appId 应用的唯一标识符
 * @param [in] handle 属性的句柄
 * @param [in] value 用于存储属性值的结构体指针
 * @param [in] addr 通知的目标地址结构体指针，addr不能为NULL，且地址不能时全零；
 * @return NLSTK_Errcode_E
 * @note 请确保`appId`、`handle`和`addr`的有效性，以及`value`指针不为空
 */
NLSTK_Errcode_E NLSTK_SsapServerUpdateAndNotifyProperty(int32_t appId, uint16_t handle, SLE_Addr_S *addr,
                                                    NLSTK_VariableData_S *value);

/**
 * @brief 根据服务的uuid检查服务是否已经在appId中注册，同步接口
 * @details 根据服务的uuid检查服务是否已经在appId中注册，同步接口
 * @param [in] appId 应用的唯一标识符
 * @param [in] uuid  待查询的服务的UUID的值
 */
NLSTK_Errcode_E NLSTK_SsapServerCheckServiceExistByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid, bool *isExist);

/**
 * @brief 异步返回方法调用结果，异步接口
 * @details 该函数用于根据requestID和结果参数返回方法调用结果，是一个异步接口。
 * @param [in] appId 应用的唯一标识符
 * @param [in] requestId 方法调用请求标识符
 * @param [in] value 用于存储结果参数的结构体指针
 * @return NLSTK_Errcode_E
 * @note 该函数为异步接口，调用后需通过回调处理结果
 */
NLSTK_Errcode_E NLSTK_SsapServerSendMethodCallRes(int32_t appId, uint16_t requestId, NLSTK_VariableData_S *value);

NLSTK_Errcode_E NLSTK_SsapCleanServerApp(NLSTK_SsapServerCleanAppResultCb serverCleanAppResultCb);

#ifdef __cplusplus
}
#endif
#endif