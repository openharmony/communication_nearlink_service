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
#ifndef SSAP_TYPE_H
#define SSAP_TYPE_H

#include <stdint.h>
#include "sdf_vector.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SSAP_SERVICE_CHANGE_EVENT_HANDLE 0x000E
#define SSAP_HASH_HANDLE 0x000F

#define SSAP_START_HANDLE 0x01
#define SSAP_END_HANDLE 0xFFFF

#define SSAP_STACK_SUCCESS 0
#define SSAP_STACK_FAILED 1

#define SSAP_UUID16_LEN 2
#define SSAP_UUID128_LEN 16
#define SSAP_UUID_STD_BASE_OFFSET 14

// 最大的可变长度字符串，因此属性和描述符的最大长度均限制为1K
#define SSAP_MAX_VALUE_LENTH 1024

#define SSAP_INTERACTION_MIN_TIMEOUT 1000
#define SSAP_INTERACTION_MAX_TIMEOUT 30000
#define SSAP_INTERACTION_DEFAULT_TIMEOUT SSAP_INTERACTION_MAX_TIMEOUT

#define SSAP_APP_INVALID_ID  (-1)

// 通知的类型，属性或事件
#define SSAP_PROPERTY_NTF 0
#define SSAP_EVENT_NTF 1

typedef struct {
    uint8_t uuid[SSAP_UUID128_LEN];
} NLSTK_SsapUuid_S;

typedef enum SSAP_UuidType {
    UUID_TYPE_SERVICE,
    UUID_TYPE_PROPERTY,
    UUID_TYPE_METHOD,
    UUID_TYPE_EVENT,
    UUID_TYPE_UNKNOWN,
} SSAP_UuidType_E;

typedef enum SSAP_ServiceType {
    SSAP_SERVICE_TYPE_BLE,
    SSAP_SERVICE_TYPE_GLE,
} SSAP_ServiceType_E;

/**
 * @enum NLSTK_SsapItemType_E
 * @brief 条目类别
 */
typedef enum {
    ITEM_TYPE_STD_PRIMARY_SERVICE = 0x00,           // 标准首要服务
    ITEM_TYPE_STD_SECONDARY_SERVICE,                // 标准次要服务
    ITEM_TYPE_STD_PROPERTY,                         // 标准属性
    ITEM_TYPE_STD_METHOD,                           // 标准方法
    ITEM_TYPE_STD_EVENT,                            // 标准事件
    ITEM_TYPE_STD_SERVICE_REFERENCE,                // 标准服务引用
    ITEM_TYPE_STD_DESCRIPTOR = 0x06,                // 标准服务属性描述，协议无此定义
    ITEM_TYPE_VENDOR_PRIMARY_SERVICE = 0x08,        // 自定义首要服务
    ITEM_TYPE_VENDOR_SECONDARY_SERVICE,             // 自定义次要服务
    ITEM_TYPE_VENDOR_PROPERTY,                      // 自定义属性
    ITEM_TYPE_VENDOR_METHOD,                        // 自定义方法
    ITEM_TYPE_VENDOR_EVENT,                         // 自定义事件
    ITEM_TYPE_VENDOR_SERVICE_REFERENCE,             // 自定义服务引用
    ITEM_TYPE_VENDOR_DESCRIPTOR = 0x0E,             // 自定义服务属性描述，协议无此定义
    ITEM_TYPE_MAX,
} NLSTK_SsapItemType_E;

typedef struct SSAP_Operation {
    union {
        struct {
            uint32_t read : 1;                              // 读取
            uint32_t writeNoRsp : 1;                        // 无响应写入
            uint32_t writeRsp : 1;                          // 写入
            uint32_t notify : 1;                            // 通知
            uint32_t indicate : 1;                          // 指示
            uint32_t advertise : 1;                         // 广播
            uint32_t resv1 : 2;                             // 保留
            uint32_t descInstr : 1;                         // 描述符可写
            uint32_t clientdescInstr : 1;                   // 客户端描述符可写
            uint32_t serverdescInstr : 1;                   // 服务端描述符可写
            uint32_t resv2 : 21;                            // 保留
        } operation;
        uint32_t operationValue;
    };
} NLSTK_SsapOperation_S;

typedef enum SSAP_OperationIndication {
    SSAP_OPERATE_INDICATION_READ            = 0x01,   // 数据值可被读取
    SSAP_OPERATE_INDICATION_WRITE_NO_RSP    = 0x02,   // 数据值可被写入，写入后无响应
    SSAP_OPERATE_INDICATION_WRITE           = 0x04,   // 数据值可被写入，写入后有响应给客户端
    SSAP_OPERATE_INDICATION_NOTIFY          = 0x08,   // 数据值通过通知方式传递给客户端
    SSAP_OPERATE_INDICATION_INDICATE        = 0x10,   // 数据值通过指示方式传递给客户端
    SSAP_OPERATE_INDICATION_BROADCAST       = 0x20,   // 数据值可携带在广播中
    SSAP_OPERATE_INDICATION_DESCRITOR_WRITE = 0x100,  // 数据值说明描述符可被写入
    SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE = 0x200,
                                                      // 客户端描述符可被写入
    SSAP_OPERATE_INDICATION_DESCRIPTOR_SERVER_CONFIGURATION_WRITE = 0x400
                                                      // 服务端描述符可被写入
} SSAP_OperationIndication_E;

/**
 * @brief 描述符类型
 */
typedef enum {
    DESC_TYPE_PROPERTY_RESERVE = 0x00,              // 预留不做定义
    DESC_TYPE_PROPERTY_INSTRUCTION,                 // 属性说明描述符
    DESC_TYPE_CLIENT_CONFIG,                        // 客户端属性值配置描述符
    DESC_TYPE_SERVER_CONFIG,                        // 服务端属性值配置描述符
    DESC_TYPE_PROPERTY_FORMAT,                      // 属性值格式描述符

    DESC_TYPE_MAX 
} NLSTK_SsapDescriptorType_E;

/**
 * @brief 权限控制
 */
typedef struct {
    union {
        struct {
            uint8_t authentication : 1;                     // 认证
            uint8_t encryption : 1;                         // 加密
            uint8_t authorization : 1;                      // 授权
            uint8_t resv : 5;
        } permission;
        uint8_t permissionValue;
    };
} NLSTK_SsapPermission_S;

typedef enum SSAP_Permission_Indication {
    SSAP_PERMISSION_NO_NEED             = 0x00,
    SSAP_PERMISSION_AUTHENTICATION_NEED = 0x01,     // 需要认证
    SSAP_PERMISSION_ENCRYPTION_NEED     = 0x02,     // 需要加密
    SSAP_PERMISSION_AUTHORIZATION_NEED  = 0x04,     // 需要授权
} SSAP_Permission_Indication_E;

typedef struct SSAP_LengthValue {
    uint16_t len;
    uint8_t value[0];
} SSAP_LengthValue_S;

/**
 * @brief 客户端属性配置描述符
 */
typedef struct SSAP_ClientPropertyConfigDescriptor {
    SLE_Addr_S addr;
    uint8_t type;
    SSAP_LengthValue_S *val;
} SSAP_ClientPropertyConfigDescriptor_S;

/**
 * @brief 描述符
 */
typedef struct SSAP_Descriptor {
    NLSTK_SsapDescriptorType_E type;                      // 描述符类型
    NLSTK_SsapOperation_S operation;                     // 操作指示
    NLSTK_SsapPermission_S permission;                   // 权限
    SDF_Vector_S *clientConfigs;                    // 当描述符为客户端属性值配置描述符时, 需要为不同的客户端准备不同的 SSAP_ClientConfigCharactor */
    SSAP_LengthValue_S *val;                        // 描述符值
} SSAP_Descriptor_S;

/**
 * @brief 属性
 */
typedef struct SSAP_Property {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapOperation_S operation;
    SDF_Vector_S *descriptors;
    NLSTK_SsapPermission_S permission;
    SSAP_LengthValue_S *val;
} SSAP_Property_S;

/**
 * @brief 方法
 */
typedef struct SSAP_Method {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapPermission_S permission;
} SSAP_Method_S;

typedef void (*SSAP_ServiceRegisterCallback)(void *context, uint8_t evt, void *arg);

typedef struct SSAP_Service {
    int32_t appId;
    uint16_t handle;
    uint16_t endHandle;
    NLSTK_SsapUuid_S uuid;
    SDF_Vector_S *references;                     // 服务引用，暂未实现
    SDF_Vector_S *properties;                      // 属性
    SDF_Vector_S *methods;                        // 方法，暂未实现
    SDF_Vector_S *events;                         // 事件，暂未实现
    SDF_Vector_S *descriptors;                    // 服务描述符，未用到，暂置为NULL
    NLSTK_SsapItemType_E serviceType;
    SSAP_ServiceType_E protocol;
    SSAP_ServiceRegisterCallback callback;
} SSAP_Service_S, NLSTK_SsapService_S;

#define SSAP_MAX_REQUEST_FOR_ONE_SERVICE 16

typedef struct {
    SLE_Addr_S addr;                    // 客户端的地址
    uint32_t requestId;                 // 请求ID
    uint16_t handle;                    // 需要操作的句柄
    uint8_t requestType;                // 请求类型读、写、方法等
} SSAP_RequestInfo_S;

typedef struct SSAP_ServiceHandle {
    uint16_t handle;
    uint16_t endHandle;
    SSAP_RequestInfo_S request[SSAP_MAX_REQUEST_FOR_ONE_SERVICE];
} SSAP_ServiceHandle_S;

typedef enum SSAP_FindType {
    FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE = 0x00,
    FIND_STRUCTURE_TYPE_PRIMARY_SERVICE = 0x01,
    FIND_STRUCTURE_TYPE_REFERENCE_SERVICE = 0x02,
    FIND_STRUCTURE_TYPE_PROPERTY = 0x03,
    FIND_STRUCTURE_TYPE_METHOD = 0x04,
    FIND_STRUCTURE_TYPE_EVENT = 0x05,
} SSAP_FindType_E;

typedef enum SSAP_FindItemType {
    FIND_ITEM_TYPE_STANDARD = 0x00,
    FIND_ITEM_TYPE_CUSTOMIZE = 0x01,
    FIND_ITEM_TYPE_MIX = 0x02,
} SSAP_FindItemType_E;

typedef enum SSAP_FindRspMode {
    FIND_RSP_MODE_SINGLE_RSP = 0x00,
    FIND_RSP_MODE_MULTI_RSP = 0x01,
} SSAP_FindRspMode_E;

typedef struct SSAP_ParamFind {
    int32_t appId;
    SLE_Addr_S addr;
    SSAP_FindType_E type;
    uint16_t startHandle;
    uint16_t endHandle;
} SSAP_ParamFind_S, NLSTK_ParamFind_S;

typedef struct SSAP_ParamFindByUuid {
    int32_t appId;
    SLE_Addr_S addr;
    SSAP_FindType_E type;
    NLSTK_SsapUuid_S uuid;
    uint16_t startHandle;
    uint16_t endHandle;
} SSAP_ParamFindByUuid_S, NLSTK_ParamFindByUuid_S;

typedef enum SSAP_ServiceMemberType {
    MEMBER_TYPE_REFERENCE = 0x01,
    MEMBER_TYPE_PROPERTY = 0x02,
    MEMBER_TYPE_METHOD = 0x04,
    MEMBER_TYPE_EVENT = 0x08,
} SSAP_ServiceMemberType_E;

typedef struct SSAP_FindServiceInfo {
    uint16_t startHandle;
    uint16_t endHandle;
    NLSTK_SsapUuid_S uuid;
    bool isStdUuid;
    union {
        struct {
            uint8_t reference : 1;      // 是否存在服务引用
            uint8_t property : 1;       // 是否存在属性
            uint8_t method : 1;         // 是否存在方法
            uint8_t event : 1;          // 是否存在事件
            uint8_t resv : 4;
        } member;
        uint8_t memberValue;
    };
} SSAP_FindServiceInfo_S;

typedef struct SSAP_FindPropertyInfo {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    bool isStdUuid;
    NLSTK_SsapOperation_S operation;
    uint8_t descriptorCount;
    uint8_t *descriptors;
} SSAP_FindPropertyInfo_S;

typedef struct SSAP_FindMethodInfo {
    uint16_t handle;
    NLSTK_SsapUuid_S uuid;
    bool isStdUuid;
} SSAP_FindMethodInfo_S;

typedef struct SSAP_BufferedOperation {
    int32_t appId;
    uint8_t msgCode;
    uint8_t errCode;
    uint8_t controlCode;
    SLE_Addr_S addr;
    uint16_t requestId;
    uint16_t serviceHandle;
    NLSTK_SsapUuid_S serviceUuid;
    uint16_t propertyHandle;
    NLSTK_SsapUuid_S propertyUuid;
    uint16_t beginHandle;
    uint16_t endHandle;
    uint8_t dataType;
    bool needRsp;
    bool needAuth;
    SSAP_LengthValue_S value;
} SSAP_BufferedOperation_S;

typedef struct SSAP_SendResponseValue {
    uint16_t status;              // 用户授权结果
    uint16_t requestId;           // 执行命令内部编码
    uint16_t len;                 // 星闪属性值长度
    uint8_t data[0];              // 星闪属性值
} SSAP_SendResponseValue_S;

typedef struct SSAP_MethodCallResValue {
    uint16_t requestId;
    uint16_t len;
    uint8_t data[0];
} SSAP_MethodCallResValue_S;

typedef struct SSAP_WriteInfo {
    SLE_Addr_S addr;
    uint16_t handle;
    uint8_t type;
    SSAP_LengthValue_S value;
} SSAP_WriteReqInfo_S, SSAP_WriteCmdInfo_S;

typedef struct SSAP_ReadReqInfo {
    int32_t appId;
    SLE_Addr_S addr;
    uint16_t handle;
    uint8_t type;
} SSAP_ReadReqInfo_S, NLSTK_ReadReqInfo_S;

typedef struct SSAP_ReadPropsInfo {
    int32_t appId;
    SLE_Addr_S addr;
    uint8_t type;
    uint8_t num;
    uint16_t handles[0];
} SSAP_ReadPropsInfo_S, NLSTK_ReadPropsInfo_S;

typedef struct SSAP_ReadByUuidReqInfo {
    int32_t appId;
    SLE_Addr_S addr;
    NLSTK_SsapUuid_S uuid;
    uint16_t handleStart;
    uint16_t handleEnd;
    uint8_t dataType;
} SSAP_ReadByUuidReqInfo_S, NLSTK_ReadByUuidReqInfo_S;

typedef struct SSAP_ExchangeInfoReqInfo {
    int32_t appId;
    SLE_Addr_S addr;
    uint16_t mtu;
    uint16_t version;
} SSAP_ExchangeInfoReqInfo_S;

typedef struct SSAP_ReadByUuidReqPktInfo {
    uint8_t multiFlag;
    uint8_t errorFlag;
    uint16_t realRspDataLen;
} SSAP_ReadByUuidReqPktInfo_S;

typedef struct SSAP_ValuePkt {
    SLE_Addr_S addr;                                   // 连接地址
    uint8_t opCode;                                    // 消息码
    uint8_t controlCode;                               // 消息控制码
    uint16_t handle;                                   // 句柄
    uint8_t dataType;                                  // 数据类型
    uint8_t errorCode;                                 // 错误码
    SSAP_LengthValue_S value;                          // 数据
} SSAP_ValuePkt_S;

typedef struct SSAP_ValueInfo {
    SLE_Addr_S addr;
    uint8_t type;
    uint16_t handle;
    SSAP_LengthValue_S value;
} SSAP_ValueInfo_S;

typedef struct SSAP_ValueAckInfo {
    SLE_Addr_S addr;
    SSAP_LengthValue_S value;
} SSAP_ValueAckInfo_S;

typedef struct SSAP_MtuInfo {
    uint32_t serverId;
    uint16_t connId;
    uint16_t version;
    uint16_t mtuSize;
    uint8_t errCode;
} SSAP_MtuInfo_S;

typedef struct SSAP_CallMethodInfo {
    SLE_Addr_S addr;
    uint16_t handle;
    SSAP_LengthValue_S value;
} SSAP_CallMethodReqInfo_S, SSAP_CallMethodCmdInfo_S;

typedef struct SSAP_StructureInfo {
    uint16_t handle;
    NLSTK_SsapItemType_E serviceType;
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapOperation_S operation;
    uint8_t descriptorCount;
    uint8_t *descriptors;
} SSAP_StructureInfo_S;

typedef struct SSAP_FindStructureInfo {
    uint16_t handle;
    NLSTK_SsapItemType_E itemType;
    NLSTK_SsapUuid_S uuid;
    bool isStdUuid;
    uint16_t startHandle;
    uint16_t endHandle;
    NLSTK_SsapOperation_S operation;
    uint8_t descriptorCount;
    uint8_t *descriptors;
} SSAP_FindStructureInfo_S;

#ifdef __cplusplus
}
#endif

#endif