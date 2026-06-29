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
#ifndef NLSTK_SM_API_H
#define NLSTK_SM_API_H

#include <stddef.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"
#include "cm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_LINK_KEY_LEN         16
#define SM_GROUP_KEY_LEN        16
#define SM_DHKEY_LEN            32
#define SM_PRIVATE_KEY_LEN      32
#define SM_PUBLIC_KEY_LEN       64    /* X & Y */
#define SM_PSK_SEC_KEY_LEN      32

#define SM_OCTETS_32    32
#define SM_OCTETS_20    20
#define SM_OCTETS_16    16
#define SM_OCTETS_15    15
#define SM_OCTETS_14    14
#define SM_OCTETS_13    13
#define SM_OCTETS_12    12
#define SM_OCTETS_11    11
#define SM_OCTETS_10    10
#define SM_OCTETS_9     9
#define SM_OCTETS_8     8
#define SM_OCTETS_7     7
#define SM_OCTETS_6     6
#define SM_OCTETS_5     5
#define SM_OCTETS_4     4
#define SM_OCTETS_3     3
#define SM_OCTETS_2     2
#define SM_OCTETS_1     1
#define SM_OCTETS_0     0

/**
 * @brief 输入输出能力
 */
typedef enum {
    SM_IO_DISPLAY_ONLY          = 0x00,
    SM_IO_DISPLAY_YES_OR_NO     = 0x01,
    SM_IO_KEYBOARD_ONLY         = 0x02,
    SM_IO_NO_INPUT_AND_OUTPUT   = 0x03,
    SM_IO_KEYBOARD_DISPLAY      = 0x04,
    SM_IO_RESERVED,                             /* 0x05-0xFF Reserved */
} NLSTK_SmIoAbility_E;

/**
 * @brief 鉴权方式
 */
typedef enum {
    SM_AUTH_NUMBER_COMPARE = 0x00,              /* 数字比较 */
    SM_AUTH_NO_ENTRY       = 0x01,              /* 免输入 */
    SM_AUTH_PASSCODE       = 0x02,              /* 通行码输入 */
    SM_AUTH_PASSWORD_ENTRY = 0x03,              /* 口令验证 */
    SM_AUTH_OUT_OF_BAND    = 0x04,              /* 带外方式 */
    SM_AUTH_PSK            = 0x05,              /* 预配置密钥PSK */
    SM_AUTH_METHOD_MAX,
} NLSTK_SmAuthMethod_E;

/**
 * @brief 带外数据标识
 */
typedef enum {
    SM_WITHOUT_OOB_DATA = 0x00,                 /* 无带外数据 */
    SM_WITH_OOB_DATA    = 0x01,                 /* 有带外数据 */
} NLSTK_SmOobDataFlag_E;

/**
 * @brief 加密算法能力
 */
typedef enum {
    SM_ENCRYPTION_ALGORITHM_ABILITY_AC1 = 0x01,             /*!< SM4-CCM 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_ENCRYPTION_ALGORITHM_ABILITY_AC2 = 0x02,             /*!< AES-CCM 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_ENCRYPTION_ALGORITHM_ABILITY_EA1 = 0x03,             /*!< ZUC 密钥长度：128 比特 */
    SM_ENCRYPTION_ALGORITHM_ABILITY_EA2 = 0x04,             /*!< AES-CTR 密钥长度：128 比特 */
    SM_ENCRYPTION_ALGORITHM_ABILITY_RESERVED,
} NLSTK_SmEncAlgoAbility_E;

/**
 * @brief 完整性保护算法能力
 */
typedef enum {
    SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_AC1 = 0x01,   /*!< SM4-CCM 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_AC2 = 0x02,   /*!< AES-CCM 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_IA1 = 0x03,   /*!< ZUC 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_IA2 = 0x04,   /*!< AES-CMAC 密钥长度：128 比特；MIC 长度：32 比特 */
    SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_RESERVED,
} NLSTK_SmIntgProtectAlgoAbility_E;

/**
 * @brief 密钥派生算法能力
 */
typedef enum {
    SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA1 = 0x01,         /*!< HMAC-SM3 */
    SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2 = 0x02,         /*!< AES-CMAC 128 */
    SM_KEY_DERIVATION_ALGORITHM_ABILITY_RESERVED,
} NLSTK_SmKeyDerivAlgoAbility_E;

/**
 * @brief 密钥协商算法能力
 */
typedef enum {
    SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1 = 0x01,        /*!< SM2 */
    SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2 = 0x02,        /*!< ECDH P-256 */
    SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_RESERVED,
} NLSTK_SmKeyNegoAlgoAbility_E;

/**
 * @brief 安全属性
 */
typedef enum {
    SM_PAIRING_MODE_NO_BINDING  = 0,            /* 不绑定模式配对 */
    SM_PAIRING_MODE_BINDING     = 1,            /* 绑定模式配对 */
    SM_PAIRING_MODE_RESERVED    = 2,            /* 预留 */
} NLSTK_SmSecurityAttribute_E;

/**
 * @brief 防中间人攻击
 */
typedef enum {
    SM_MITM_DEFEND_UNSUPPORT   = 0,             /* 不需要支持防中间人攻击的配对方式 */
    SM_MITM_DEFEND_SUPPORT     = 1,             /* 需要支持防中间人攻击的配对方式 */
} NLSTK_SmMitmDefend_E;

/**
 * @brief 按键提示
 */
typedef enum {
    SM_KEYPRESS_NOTIF_NOT_USED = 0,             /* 不采用按键提示方式配对 */
    SM_KEYPRESS_NOTIF_USED     = 1,             /* 采用按键提示方式配对 */
} NLSTK_SmKeypressNotif_E;

/**
 * @brief 鉴权请求
 */
typedef struct {
    uint8_t secAttribute   : 2;     /* 安全属性 2 比特 参考 NLSTK_SmSecurityAttribute_E */
    uint8_t mitmDefend     : 1;     /* 防中间人攻击指示位 1 比特 参考 NLSTK_SmMitmDefend_E */
    uint8_t kpressNotif    : 1;     /* 按键提示 1 比特 参考 NLSTK_SmKeypressNotif_E */
    uint8_t reserved       : 4;
} NLSTK_SmAuthReq_S;

/**
 * @brief  设备本端参数
 */
typedef struct {
    uint8_t ioAbility;                          /* 输入输出能力 参考 NLSTK_SmIoAbility_E */
    uint8_t authMethodMask;                     /* bitmap, 每位含义参考 NLSTK_SmAuthMethod_E */
    uint8_t oobDataFlag;                        /* 带外数据标识 参考 NLSTK_SmOobDataFlag_E */
    uint8_t secKeyMaxLen;                       /* 安全密钥最大长度 */
    uint8_t distIrkFlag;                        /* 分发IRK标志 secInfoDis的第0位：
                                                            当值为 0 时，表示不分发；当值为 1 时，表示分发 */
    uint8_t distAddrFlag;                       /* 分发真实地址标志 secInfoDis的第1位：
                                                            当值为 0 时，表示不分发；当值为 1 时，表示分发 */
    uint8_t codeAlgoCap[SM_OCTETS_4];           /* 协商后密码算法能力 参考
                                                    NLSTK_SmEncAlgoAbility_E
                                                    NLSTK_SmIntgProtectAlgoAbility_E
                                                    NLSTK_SmKeyDerivAlgoAbility_E
                                                    NLSTK_SmKeyNegoAlgoAbility_E */
    uint8_t pskFlag;                            /* PSK指示 */
    NLSTK_SmAuthReq_S authReq;                       /* 鉴权请求 */
} NLSTK_SmLocalParams_S;

/**
 * @brief  通行码鉴权passcode
 */
typedef struct {
    SLE_Addr_S addr;
    uint32_t passCode;
} NLSTK_SmPassCode_S;

/**
 * @brief  口令鉴权password
 */
typedef struct {
    SLE_Addr_S addr;
    uint8_t passWordLen;
    char passWord[0];
} NLSTK_SmPassWord_S;

/**
 * @brief  PSK共享密钥
 */
typedef struct {
    SLE_Addr_S addr;
    uint8_t psk[SM_PSK_SEC_KEY_LEN];
} NLSTK_SmPsk_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t linkKey[SM_LINK_KEY_LEN];
    uint8_t cryptoAlgo;
    uint8_t keyDerivAlgo;
    uint8_t intgChkInd;
} NLSTK_SmRecoverKeyParam_S;
 
typedef struct {
    uint8_t num;
    NLSTK_SmRecoverKeyParam_S *smKey;
} NLSTK_SmRecoverKeyParamVector_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t imgId;
    uint8_t groupKey[SM_OCTETS_16];
    uint8_t cryptoAlgo;
    uint8_t keyDerivAlgo;
    uint8_t intgChkInd;
    uint64_t giv;
} NLSTK_SmImgSecuConfig_S;

typedef struct {
    uint8_t imgHandle;
    uint8_t cryptoAlgo;
    uint64_t giv;
    uint8_t groupKey[SM_OCTETS_16];
} NLSTK_SmImgEncpParam_S;

/*****************************************************************************************
                                Crypto Algorithm Structure
*****************************************************************************************/

typedef struct {
    uint8_t algo;
    uint8_t priKey[SM_PRIVATE_KEY_LEN];
    uint8_t localPubKey[SM_PUBLIC_KEY_LEN];
    uint8_t remotePubKey[SM_PUBLIC_KEY_LEN];
} NLSTK_SmKeyPair_S;

typedef struct {
    uint8_t algo;
    uint8_t key[SM_OCTETS_16];
    uint8_t *buff;
    size_t buffSize;
} NLSTK_SmDerivedMac_S;

/*****************************************************************************************
                                Callback Structure Parameters
*****************************************************************************************/

/**
 * @brief 安全管理配对状态
 */
typedef enum {
    SM_PAIR_OK,             /*!< 配对成功 */
    SM_PAIR_ERROR,          /*!< 配对失败 */
    SM_KEY_MISSING,         /*!< 密钥丢失 */
    SM_LINK_DISCONNCTED,    /*!< 链接已断开 */
} NLSTK_SmPairStatus_E;

/**
 * @brief  安全管理模块启动配对回调参数
 */
typedef struct {
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t bondStatus;
} NLSTK_SmPairingStart_S;

/**
 * @brief  安全管理模块移除配对回调参数
 */
typedef struct {
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t removeStatus;
} NLSTK_SmPairingRemove_S;

/**
 * @brief  安全管理模块配对请求回调参数
 */
typedef struct {
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t requestType;
    char sixDigits[SM_OCTETS_7];
} NLSTK_SmPairingRequest_S;

/**
 * @brief  安全管理模块鉴权完成回调参数
 */
typedef struct {
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t authStatus;
    uint8_t linkKey[SM_LINK_KEY_LEN];
    uint8_t cryptoAlgo;
    uint8_t keyDerivAlgo;
    uint8_t intgChkInd;
    uint8_t isBond;
    uint8_t groupKey[SM_GROUP_KEY_LEN];
    uint64_t giv;
} NLSTK_SmAuthComplete_S;

/**
 * @brief  安全管理模块加密完成回调参数
 */
typedef struct {
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t encStatus;
} NLSTK_SmEncComplete_S;

/**
 * @brief 安全管理组播操作结果
 */
typedef enum {
    SM_IMG_OK = 0,         /*!< 组播操作成功 */
    SM_IMG_ERROR,          /*!< 组播操作失败 */
} NLSTK_SmImgResult_E;

typedef struct {
    SLE_Addr_S addr;
    uint8_t sendStatus;
} NLSTK_SmSendImgMsgCmpl_S;

typedef struct {
    uint8_t imgHandle;
    uint8_t encpStatus;
} NLSTK_SmImgEncpCmpl_S;

/**
 * @brief  安全管理模块回调函数类型
 */
typedef void (*NLSTK_SmStartEventCbk)(NLSTK_SmPairingStart_S *params);
typedef void (*NLSTK_SmRemoveEventCbk)(NLSTK_SmPairingRemove_S *params);
typedef void (*NLSTK_SmRequestEventCbk)(NLSTK_SmPairingRequest_S *params);
typedef void (*NLSTK_SmAuthEventCbk)(NLSTK_SmAuthComplete_S *params);
typedef void (*NLSTK_SmEncEventCbk)(NLSTK_SmEncComplete_S *params);
typedef void (*NLSTK_SmImgSendMsgCbk)(NLSTK_SmSendImgMsgCmpl_S *params);
typedef void (*NLSTK_SmImgEncpCbk)(NLSTK_SmImgEncpCmpl_S *params);

/**
 * @brief  安全管理模块回调函数
 */
typedef struct {
    NLSTK_SmStartEventCbk startCbk;
    NLSTK_SmRemoveEventCbk removeCbk;
    NLSTK_SmRequestEventCbk requestCbk;
    NLSTK_SmAuthEventCbk authCbk;
    NLSTK_SmEncEventCbk encCbk;
    NLSTK_SmImgSendMsgCbk imgMsgCbk;
} NLSTK_SmCallbacks_S;

typedef struct {
    NLSTK_SmImgEncpCbk imgEncpCbk;
} NLSTK_SmImgCallbacks_S;

#define SM_SHA_256_HASH_LEN 32

typedef struct {
    uint8_t sha256Hash[SM_SHA_256_HASH_LEN];
} NLSTK_SmSha256Hash;

/**
 * @brief  安全管理模块加密函数类型
 */
typedef void (*NLSTK_SmRandNumGenAlgo)(uint8_t *out, uint8_t len);
typedef void (*NLSTK_SmKeyPairGenAlgo)(NLSTK_SmKeyPair_S *keyPair);
typedef void (*NLSTK_SmSecKeyGenAlgo)(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen);
typedef void (*NLSTK_SmDerivedKeyGenAlgo)(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen);
typedef void (*NLSTK_SmSha256HashAlgo)(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash);

/**
 * @brief  安全管理模块加密函数注册
 */
typedef struct {
    NLSTK_SmRandNumGenAlgo randNumFunc;             /* 随机数生成函数指针 */
    NLSTK_SmKeyPairGenAlgo pubPriKeyFunc;           /* 公钥/私钥生成函数指针 */
    NLSTK_SmSecKeyGenAlgo secKeyFunc;               /* 协商密钥生成函数指针 */
    NLSTK_SmDerivedKeyGenAlgo derivedKeyFunc;       /* 派生密钥生成函数指针 */
    NLSTK_SmSha256HashAlgo sha256HashFunc;          /* SHA-256函数指针 */
} NLSTK_SmCryptoAlgoFuncs_S;

/* SM Api interface functions */

/**
 * @brief 启动配对
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param addrIn 对端设备地址
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmStartPairing(SLE_Addr_S *addrIn);

/**
 * @brief 移除配对
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param addrIn 对端设备地址
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmRemovePairing(SLE_Addr_S *addrIn);

/**
 * @brief 用户下发确认消息，用于数字比较鉴权方式和免输入鉴权方式
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param addrIn 对端设备地址
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSetConfirm(SLE_Addr_S *addrIn);

/**
 * @brief 用户设置通行码，用于通行码鉴权方式
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param passCodeIn 通行码信息
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSetPassCode(NLSTK_SmPassCode_S *passCodeIn);

/**
 * @brief 用户设置口令信息，用于口令验证鉴权方式
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param passWordIn 口令信息
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSetPassWord(NLSTK_SmPassWord_S *passWordIn);

/**
 * @brief 用户设置PSK密钥，用于PSK鉴权方式
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param pskIn PSK信息
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSetLocalPsk(NLSTK_SmPsk_S *pskIn);

/**
 * @brief 恢复秘钥，用于设备回连
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param recoverKey 恢复的秘钥信息，数组
 * @param keyNum 恢复的秘钥数目
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmRecoverKey(NLSTK_SmRecoverKeyParam_S *recoverKey, uint8_t keyNum);

/**
 * @brief 用户设置设置安全链路参数
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param paramsIn 安全链路参数
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSetSecurityParams(NLSTK_SmLocalParams_S *paramsIn);

/**
 * @brief 注册外部回调
 *
 * 同步调用，返回注册结果
 *
 * @param cbksIn 外部回调
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmRegExternalCbks(NLSTK_SmCallbacks_S *cbksIn);

/**
 * @brief 注册加密算法
 *
 * 同步调用，返回注册结果
 *
 * @param funsIn 加密算法
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmRegAlgoFuncs(NLSTK_SmCryptoAlgoFuncs_S *funsIn);

/**
 * @brief 发送组播安全配置消息
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param config 组播安全配置消息
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmSendImgSecuConfig(NLSTK_SmImgSecuConfig_S *config);

/**
 * @brief 组播加密使能
 *
 * 异步调用，返回异步队列添加结果
 *
 * @param config 加密参数
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmEnableImgEncp(NLSTK_SmImgEncpParam_S *param);

/**
 * @brief 注册组播加密回调
 *
 * 同步调用，返回注册结果
 *
 * @param cbksIn 外部回调
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_SmRegImgCbks(NLSTK_SmImgCallbacks_S *cbksIn);

#ifdef __cplusplus
}
#endif

#endif /* NLSTK_SM_API_H */