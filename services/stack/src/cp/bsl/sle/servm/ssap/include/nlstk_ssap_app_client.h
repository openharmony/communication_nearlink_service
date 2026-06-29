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
#ifndef NLSTK_SSAP_APP_CLIENT_H
#define NLSTK_SSAP_APP_CLIENT_H

#include "nlstk_public_define.h"
#include "ssap_type.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NLSTK_SSAP_CLIENT_APP_MAX_NUM     50

typedef struct {
    uint16_t handle;
    NLSTK_SsapDescriptorType_E type;                // 描述符类型
    NLSTK_SsapOperation_S operation;                // 操作指示
    NLSTK_SsapPermission_S permission;              // 权限
} NLSTK_SsapDtor_S;

typedef struct {
    bool isNtf;
    bool isInd;
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapOperation_S operation;
    uint16_t descriptorNum;
    NLSTK_SsapDtor_S *descriptors;
    NLSTK_SsapPermission_S permission;
} NLSTK_SsapPrty_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    uint16_t endHandle;
    NLSTK_SsapUuid_S uuid;
    uint16_t propertyNum;
    NLSTK_SsapPrty_S *properties;                    // 属性
    uint16_t methodNum;
    NLSTK_SsapPrty_S *methods;                      // 方法
    uint16_t eventNum;
    NLSTK_SsapPrty_S *events;                       // 事件
    NLSTK_SsapItemType_E serviceType;
} NLSTK_SsapServ_S;

typedef void (*NLSTK_SsapClientFreeFunc)(NLSTK_SsapServ_S *service, uint16_t serviceNum);

typedef struct {
    int32_t appId;
    NLSTK_SsapUuid_S *uuid;
    NLSTK_SsapServ_S **serv;
    uint16_t *num;
    NLSTK_SsapClientFreeFunc *func;
} NLSTK_SsapServObtain_S;

typedef struct {
    int32_t appId;
    NLSTK_SsapUuid_S *uuid;
} NLSTK_SsapServObtainAsyn_S;

typedef struct {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    uint8_t errorCode;                                 // 错误码
    NLSTK_VariableData_S value;
} NLSTK_SsapClientReadPropertyInfo_S, NLSTK_SsapClientWritePropertyInfo_S,
  NLSTK_SsapClientCallMethodResult_S, NLSTK_SsapClientEventInfo_S;

typedef struct {
    uint16_t handle;
    uint8_t type;
    NLSTK_SsapUuid_S uuid;
    uint8_t errorCode;                                 // 错误码
    NLSTK_VariableData_S value;
} NLSTK_SsapClientReadDescriptorInfo_S, NLSTK_SsapClientWriteDescriptorInfo_S;

/*
 * 写命令或请求的回调结:w构体
 */
typedef struct {
    int32_t appId;
    uint16_t handle;
    NLSTK_VariableData_S *value;
    uint8_t type;
} NLSTK_SsapClientWriteBaseInfo_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    bool isNtf;
    bool enable;
} NLSTK_SsapClientSetPropertyInfo_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    bool isNtf;
} NLSTK_SsapClientGetPropertyInfo_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    NLSTK_VariableData_S value;
} NLSTK_SsapClientCallMethodInfo_S;

/**
 * @brief 客户端MTU变化回调函数
 * @details 当客户端应用调用NLSTK_SsapSendExchangeInfoReq函数时，操作结果会通过此回调函数返回
 * @param appId 应用客户端的唯一标识符，用于标识触发回调的应用
 * @param mtu 最大传输单元（MTU）值，表示新的MTU大小
 * @param ret 操作结果，0表示成功，非零表示失败（具体错误码需参考文档）
 */
typedef void (*NLSTK_SsapClientMtuChangedCb)(int32_t appId, uint16_t mtu, NLSTK_Errcode_E ret);

/**
 * @brief 服务发现结果回调函数
 * @details 当客户端应用调用NLSTK_SsapFindService函数时，服务发现的结果会通过此回调函数返回。
 *          该回调函数仅返回操作结果，不包含具体的服务信息。
 *          服务信息可以通过调用NLSTK_GetServices函数获取。
 * @param ret 操作结果，0表示成功，非零表示失败（具体错误码需参考协议中对于错误码的描述）
 */
typedef void (*NLSTK_SsapClientFindServiceCb)(int32_t appId, NLSTK_Errcode_E ret);
typedef void (*NLSTK_SsapClientFindServicesByUuidCb)(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientReadPropertyCb)
(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientReadPropertiesCb)
(int32_t appId, uint8_t num, NLSTK_SsapClientReadPropertyInfo_S *properties, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientReadDescriptorCb)
(int32_t appId, NLSTK_SsapClientReadDescriptorInfo_S *descriptor, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientReadPropertyByUuidCb)
    (int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, uint8_t propertyNum, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientConnectionStateChangedCb)(int32_t appId, uint8_t state, NLSTK_Errcode_E ret,
    int32_t reason);

typedef void (*NLSTK_SsapClientGetPropertyNtfCb)
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable, NLSTK_Errcode_E ret);
typedef void (*NLSTK_SsapClientGetPropertyIndCb)
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable, NLSTK_Errcode_E ret);
typedef void (*NLSTK_SsapClientSetPropertyNtfCb)
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable, NLSTK_Errcode_E ret);
typedef void (*NLSTK_SsapClientSetPropertyIndCb)
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientPropertyChangedCb)(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property);
typedef void (*NLSTK_SsapClientEventCb)(int32_t appId, NLSTK_SsapClientEventInfo_S *event);

typedef void (*NLSTK_SsapClientWritePropertyCb)
(int32_t appId, NLSTK_SsapClientWritePropertyInfo_S *property, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientWriteDescriptorCb)
(int32_t appId, NLSTK_SsapClientWriteDescriptorInfo_S *descriptor, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientCallMethodCb)(int32_t appId, NLSTK_SsapClientCallMethodResult_S *result,
    NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientRegisterAppCb)(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret);

typedef void (*NLSTK_SsapClientGetServicesCb)
(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service, uint16_t serviceNum, NLSTK_SsapClientFreeFunc func);

typedef void (*NLSTK_SsapClientCleanAppResultCb)(void);

typedef void (*NLSTK_SsapClientServiceChangeCb)(int32_t appId, uint16_t handle, NLSTK_SsapUuid_S *uuid);

// service内存有stack管理，不需要上层释放
typedef void (*NLSTK_SsapClientServiceRediscoverCb)(int32_t appId, NLSTK_SsapServ_S *service, uint16_t serviceNum);
/**
 * 回调结构体
 */
typedef struct {
    NLSTK_SsapClientMtuChangedCb onMtuChanged;
    NLSTK_SsapClientFindServiceCb onFindService;
    NLSTK_SsapClientFindServicesByUuidCb onFindServiceByUuid;
    NLSTK_SsapClientConnectionStateChangedCb onConnectionStateChanged;
    NLSTK_SsapClientReadPropertyCb onReadProperty;
    NLSTK_SsapClientReadPropertiesCb onReadProperties;
    NLSTK_SsapClientReadDescriptorCb onReadDescriptor;
    NLSTK_SsapClientReadPropertyByUuidCb onReadPropertyByUuid;
    NLSTK_SsapClientGetPropertyNtfCb onGetPropertyNtf;
    NLSTK_SsapClientGetPropertyIndCb onGetPropertyInd;
    NLSTK_SsapClientSetPropertyNtfCb onSetPropertyNtf;
    NLSTK_SsapClientSetPropertyIndCb onSetPropertyInd;
    NLSTK_SsapClientPropertyChangedCb onPropertyChanged;
    NLSTK_SsapClientWritePropertyCb onWriteProperty;
    NLSTK_SsapClientWriteDescriptorCb onWriteDescriptor;
    NLSTK_SsapClientCallMethodCb onCallMethod;
    NLSTK_SsapClientRegisterAppCb onRegisterApp;
    NLSTK_SsapClientGetServicesCb onGetServices;
    NLSTK_SsapClientEventCb onEvent;
    NLSTK_SsapClientServiceChangeCb onServiceChange;
    NLSTK_SsapClientServiceRediscoverCb onServiceRediscover;
} NLSTK_SsapAppClientCb_S;

/**
 * @brief 注册应用客户端
 * @details 该函数用于注册一个应用客户端，将回调函数传递给系统，以便在后续处理中使用。
 * @param [in] cb 指向回调函数结构体的指针
 * @param [in] addr 指向对端服务端设备地址的指针
 * @param [out] appId 返回注册的appId，返回<0 注册失败，具体错误信息可通过错误码查看
 * @return 错误码信息
 */
NLSTK_Errcode_E NLSTK_SsapClientRegApp(int32_t *appId, NLSTK_SsapAppClientCb_S *cb, SLE_Addr_S *addr);

/**
 * @brief 注册应用客户端异步接口
 * @details 该函数用于注册一个应用客户端，将回调函数传递给系统，AppId通过回调返回，请确保注册的接收AppId的回调函数不为空。
 * @param [in] addr 指向对端服务端设备地址的指针
 * @param [in] connParam 连接参数，用于指定帧类型等信息
 * @param [in] cb 指向回调函数结构体的指针
 * @return 错误码信息
 */
NLSTK_Errcode_E NLSTK_SsapClientRegAppAsyn(SLE_Addr_S *addr, NLSTK_ConnParam_S *connParam, NLSTK_SsapAppClientCb_S *cb);

/**
 * @brief 注销应用的异步接口
 * @details 该函数用于注销一个已注册的应用客户端，释放相关资源并停止与该应用相关的客户端服务。
 * @param [in] appId 应用的唯一标识符
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用客户端将无法再使用
 */
void NLSTK_SsapClientDeregAppAsync(int32_t appId);

/**
 * @brief 注销应用
 * @details 该函数用于注销一个已注册的应用客户端，释放相关资源并停止与该应用相关的客户端服务。
 * @param [in] appId 应用的唯一标识符
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用客户端将无法再使用
 */
void NLSTK_SsapClientDeregApp(int32_t appId);

NLSTK_Errcode_E NLSTK_SsapClientSetInteractionTimeout(int32_t appId, int64_t timeout);

NLSTK_Errcode_E NLSTK_SsapClientExchangeMtu(int32_t appId, uint16_t mtu);

NLSTK_Errcode_E NLSTK_SsapClientDiscoverServices(int32_t appId, uint16_t handleStart, uint16_t handleEnd, uint8_t type);
NLSTK_Errcode_E NLSTK_SsapClientDiscoverServicesByUuid
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handleStart, uint16_t handleEnd, uint8_t type);

NLSTK_Errcode_E NLSTK_SsapClientGetServices
(int32_t appId, NLSTK_SsapServ_S **service, uint16_t *serviceNum, NLSTK_SsapClientFreeFunc *func);
NLSTK_Errcode_E NLSTK_SsapClientGetServicesAsyn(int32_t appId);
NLSTK_Errcode_E NLSTK_SsapClientGetServicesByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid,
    NLSTK_SsapServ_S **service, uint16_t *serviceNum, NLSTK_SsapClientFreeFunc *func);
NLSTK_Errcode_E NLSTK_SsapClientGetServicesByUuidAsyn(int32_t appId, NLSTK_SsapUuid_S *uuid);

NLSTK_Errcode_E NLSTK_SsapClientReadProperty(int32_t appId, uint16_t handle);
NLSTK_Errcode_E NLSTK_SsapClientReadProperties(int32_t appId, uint16_t *handles, uint8_t num);
NLSTK_Errcode_E NLSTK_SsapClientReadPropertyByUuid
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handleStart, uint16_t handleEnd);
NLSTK_Errcode_E NLSTK_SsapClientReadDescriptor(int32_t appId, uint16_t handle, uint8_t type);

NLSTK_Errcode_E NLSTK_SsapClientGetPropertyNtf(int32_t appId, uint16_t handle);
NLSTK_Errcode_E NLSTK_SsapClientGetPropertyInd(int32_t appId, uint16_t handle);
NLSTK_Errcode_E NLSTK_SsapClientSetPropertyNtf(int32_t appId, uint16_t handle, bool enable);
NLSTK_Errcode_E NLSTK_SsapClientSetPropertyInd(int32_t appId, uint16_t handle, bool enable);

NLSTK_Errcode_E NLSTK_SsapClientWriteProperty
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, bool withoutRsp);

NLSTK_Errcode_E NLSTK_SsapClientWriteDescriptor
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, uint8_t type, bool withoutRsp);

NLSTK_Errcode_E NLSTK_SsapClientCallMethod
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, bool withoutRsp);

NLSTK_Errcode_E NLSTK_SsapCleanClientApp(NLSTK_SsapClientCleanAppResultCb clientCleanAppResultCb);
#ifdef __cplusplus
}
#endif
#endif