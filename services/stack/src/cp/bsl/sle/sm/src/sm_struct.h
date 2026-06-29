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
#ifndef SM_STRUCT_H
#define SM_STRUCT_H

#include <stdint.h>
#include "sdf_dlist.h"
#include "sdf_addr.h"
#include "nlstk_sm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_CONFIRM_NUMBER_LEN     16
#define SM_DHKEY_AUTHCODE_LEN     16
#define SM_FEATURES_PARAM_LEN     10
#define SM_IV_LEN                 4     /* IV长度 */
#define SM_KEY_DIV_LEN            8     /* 会话密钥分散器长度 */
#define SM_KEY_RAND_LEN           8     /* 会话密钥随机值长度 */
#define SM_MAX_KEY_MIN_LEN        7
#define SM_RANDOM_NUMBER_R_LEN    16
#define SM_SALT_LEN               16
#define SM_MIX_PASSCODE_LEN       16
#define SM_MIX_PASSWORD_LEN       16

#define SM_FILTER_DATA_LENGTH 52

#define SM_SHIFT_BITS_8 8
#define SM_SHIFT_BITS_6 6
#define SM_SHIFT_BITS_5 5
#define SM_SHIFT_BITS_4 4
#define SM_SHIFT_BITS_1 1

#define SM_LAST_ONE_BITS_MASK 0x01
#define SM_LAST_TWO_BITS_MASK 0x03

#define SM_BYTE_MAX_VAL 0xFF

#define EXTEND_ASCII_CHAR_LITTLE_D 100
#define EXTEND_ASCII_CHAR_LITTLE_K 107
#define EXTEND_ASCII_CHAR_LITTLE_L 108

#define SM_ERR_STATUS_KEY_MISSING 0x05

#define LOW_SIX_DIGIT 1000000

#define MAX_SIX_DIGIT 900000
#define MIN_SIX_DIGIT 100000

/**
 * @brief 报文消息类型索引
 */
typedef enum {
    SM_NEGO_PAIRING_START       = 0x0133,   /* 配对发起消息 */
    SM_NEGO_PAIRING_REQUEST     = 0x0134,   /* 配对请求消息 */
    SM_NEGO_PAIRING_RESPONSE    = 0x0135,   /* 配对回应消息 */
    SM_NEGO_PAIRING_CONFIRM     = 0x0136,   /* 配对确认消息 */
    SM_NEGO_PAIRING_INIT_INFO   = 0x0137,   /* 配对初始消息 */
    SM_AUTH_T_NODE_CFM          = 0x0138,   /* T 节点确认码 */
    SM_AUTH_RAND_NUM_RA         = 0x0139,   /* 随机数 Ra */
    SM_AUTH_RAND_NUM_RB         = 0x013A,   /* 随机数 Rb */
    SM_AUTH_G_NODE_CFM_WITH_RA  = 0x013B,   /* 携带Ra的G节点确认码 */
    SM_AUTH_T_NODE_CFM_WITH_RB  = 0x013C,   /* 携带Rb的T节点确认码 */
    SM_IMG_SECU_CONFIG          = 0x013D,   /* 组播安全配置消息 */
    SM_AUTH_G_NODE_CFM          = 0x013F,   /* G 节点确认码  */
    SM_AUTH_G_NODE_DHKEY        = 0x0141,   /* G 节点 DHKey 验证码 */
    SM_AUTH_T_NODE_DHKEY        = 0x0142,   /* T 节点 DHKey 验证码 */
    SM_PAIR_FAIL_MESSAGE_OPCODE = 0x0147,  /* 配对失败消息 */
} SmOpcode_E;


/*****************************************************************************************
                                Negotiation Procedure
*****************************************************************************************/

/************************************ 配对发起消息 ***************************************/

/**
 * @brief  配对发起消息结构体
 */
typedef struct {
    NLSTK_SmAuthReq_S authReq;           /* 鉴权请求 1 字节 */
} __attribute__((packed)) SmPairStartMsg_S;

/********************************* 配对请求及回复消息 ***************************************/

/**
 * @brief 安全信息分发信息
 */
typedef enum {
    SM_SECURITY_INFORMATION_DISTRIBUTION_IRK    = 0b0001,
    SM_SECURITY_INFORMATION_DISTRIBUTION_ADDR   = 0b0010,
} SmSecInfoDist_E;

/**
 * @brief 算法能力标识
 */
typedef enum {
    SM_ENC_ALGO_ABILITY             = 0,                    /* NLSTK_SmEncAlgoAbility_E */
    SM_INTG_PROTECT_ALGO_ABILITY    = 1,                    /* NLSTK_SmIntgProtectAlgoAbility_E */
    SM_KEY_DERIV_ALGO_ABILITY       = 2,                    /* NLSTK_SmKeyDerivAlgoAbility_E */
    SM_KEY_NEGO_ALGO_ABILITY        = 3,                    /* NLSTK_SmKeyNegoAlgoAbility_E */
} SmCodeAlgoCap_E;

/**
 * @brief PSK指示
 */
typedef enum {
    SM_PSK_OFF  = 0,
    SM_PSK_ON   = 1,
    SM_RESERVED,
} SmPskFlag_E;

/**
 * @brief  配对请求或回复消息结构体
 */
typedef struct {
    uint8_t ioAbility;                  /* 输入输出能力 1 字节 参考 NLSTK_SmIoAbility_E */
    uint8_t oobDataFlag;                /* 带外数据标识 1 字节 参考 NLSTK_SmOobDataFlag_E */
    NLSTK_SmAuthReq_S authReq;               /* 鉴权请求 1 字节 */
    uint8_t secKeyMaxLen;               /* 最大加密密钥长度 1 字节 */
    uint8_t secInfoDis;                 /* 安全信息分发信息 1 字节 */
    uint8_t codeAlgoCap[SM_OCTETS_4];   /* 密码算法能力 4 字节 参考
                                                NLSTK_SmEncAlgoAbility_E
                                                NLSTK_SmIntgProtectAlgoAbility_E
                                                NLSTK_SmKeyDerivAlgoAbility_E
                                                NLSTK_SmKeyNegoAlgoAbility_E */
    uint8_t pskFlag;                    /* PSK指示 1 字节 */
} __attribute__((packed)) SmPairReqRspMsg_S;

/************************************ 配对确认消息 ***************************************/

/**
 * @brief  配对确认消息结构体
 */
typedef struct {
    uint8_t secKeyLen;                          /* 密钥长度 1 字节 */
    uint8_t authMethod;                         /* 鉴权方式 1 字节 参考 NLSTK_SmAuthMethod_E */
    uint8_t codeAlgoCap[SM_OCTETS_4];           /* 密码算法能力 4 字节 参考
                                                    NLSTK_SmEncAlgoAbility_E
                                                    NLSTK_SmIntgProtectAlgoAbility_E
                                                    NLSTK_SmKeyDerivAlgoAbility_E
                                                    NLSTK_SmKeyNegoAlgoAbility_E */
    uint8_t gNodePubKey[SM_PUBLIC_KEY_LEN];     /* G节点公钥 (64 字节) */
} __attribute__((packed)) SmPairCfmMsg_S;

/************************************ 配对初始消息 ***************************************/

/**
 * @brief  配对初始消息结构体
 */
typedef struct {
    uint8_t tNodePubKey[SM_PUBLIC_KEY_LEN];     /* T节点公钥 (64 字节) */
} __attribute__((packed)) SmPairInitInfoMsg_S;

/*****************************************************************************************
                                Authentication Procedure
*****************************************************************************************/

/**
 * @brief  G\T节点确认码、Ra\Rb随机数消息、Dhkey消息
 */
typedef struct {
    uint8_t authData[SM_OCTETS_16];             /* 16 字节 */
} __attribute__((packed)) SmAuthGCfmMsg_S, SmAuthTCfmMsg_S,
                          SmAuthRandomAMsg_S, SmAuthRandomBMsg_S, SmAuthDhkeyMsg_S;

/**
 * @brief  携带Ra的G节点确认码
 */
typedef struct {
    uint8_t gNodeCfm[SM_OCTETS_16];             /* 16 字节 */
    uint8_t randomA[SM_OCTETS_16];              /* 16 字节 */
} __attribute__((packed)) SmAuthGCfmWithRaMsg_S;

/**
 * @brief  携带Rb的T节点确认码
 */
typedef struct {
    uint8_t tNodeCfm[SM_OCTETS_16];             /* 16 字节 */
    uint8_t randomB[SM_OCTETS_16];              /* 16 字节 */
} __attribute__((packed)) SmAuthTCfmWithRbMsg_S;

typedef struct {
    char code[SM_OCTETS_7];
} SmAuthUserCode_S;

typedef struct {
    uint8_t confirm[SM_CONFIRM_NUMBER_LEN];
} SmConfirmNum_S;

/*****************************************************************************************
                                Encrption Procedure
*****************************************************************************************/

/**
 * @brief 加密和完整性保护
 */
typedef enum {
    SM_ENCRYPTION_INTEGRITY_PROTECTION      = 0x00,
    SM_NOENCRYPTION_INTEGRITY_PROTECTION    = 0x01,
    SM_ENCRYPTION_NOINTEGRITY_PROTECTION    = 0x02,
    SM_NOENCRYPTION_NOINTEGRITY_PROTECTION  = 0x03,
} SmEncIntgProtection_E;

/*****************************************************************************************
                                Img Procedure
*****************************************************************************************/

/**
 * @brief  组播安全配置消息
 */
typedef struct {
    uint8_t imgId; /* 事件组集合标识 1 字节 */
    uint8_t rand[SM_OCTETS_16]; /* 随机数 16 字节 */
    uint8_t codeAlgoCap[SM_OCTETS_3]; /* 密码算法能力 3 字节 参考
                                            NLSTK_SmKeyDerivAlgoAbility_E
                                            NLSTK_SmEncAlgoAbility_E
                                            NLSTK_SmIntgProtectAlgoAbility_E */
    uint8_t c[SM_OCTETS_16]; /* 16 字节 */
    uint64_t giv; /* 组播初始化向量 8 字节 */
} __attribute__((packed)) SmImgSecuConfigMsg_S;

/*****************************************************************************************
                                Slink Structure Framework
*****************************************************************************************/

typedef enum {
    SM_G_NODE,
    SM_T_NODE,
} SmNodeType_E;

typedef enum {
    SM_UNPAIRED,
    SM_START_PAIRING,
    SM_START_AUTHENTICATION,
    SM_WAITING_USER_ENTRY,
    SM_WAITING_USER_CONFIRM,
    SM_WAITING_ENCRYPT,
    SM_ENCRYPTED,
} SmStatus_E;

/**
 * @brief 设备安全管理参数，本地公钥、随机数、确认码及IO、PSK能力支持
 */
typedef struct {
    uint8_t pubKey[SM_PUBLIC_KEY_LEN];
    uint8_t randomR[SM_RANDOM_NUMBER_R_LEN];
    uint8_t confirm[SM_OCTETS_16];
    uint8_t ioAbility;                          /* 输入输出能力 参考 NLSTK_SmIoAbility_E */
    uint8_t pskFlag;                            /* PSK指示 */
    uint32_t passCode;                          /* 通行码鉴权数字 */
    uint8_t mixCode[SM_MIX_PASSCODE_LEN];       /* 通行码混淆码 */
    uint8_t mixWord[SM_MIX_PASSWORD_LEN];       /* 口令码混淆码 */
    bool recvFlag;                              /* 通行码 口令验证 收到对端确认码确认 */
} SmDeviceParams_S;

typedef struct {
    uint8_t oobDataFlag;                        /* 带外数据标识 参考 NLSTK_SmOobDataFlag_E */
    uint8_t authMethod;                         /* 鉴权方式 参考 NLSTK_SmAuthMethod_E */
    uint8_t secAttribute;                       /* 安全属性 参考 NLSTK_SmSecurityAttribute_E */
    uint8_t kpressNotif;                        /* 按键指示标志 */
    uint8_t secKeyMaxLen;                       /* 安全密钥最大长度，默认16字节，不得低于7字节 */
    uint8_t distIrkFlag;                        /* 分发IRK标志 secInfoDis的第0位：
                                                            当值为 0 时，表示不分发；当值为 1 时，表示分发 */
    uint8_t distAddrFlag;                       /* 分发真实地址标志 secInfoDis的第1位：
                                                            当值为 0 时，表示不分发；当值为 1 时，表示分发 */
    uint8_t codeAlgoCap[SM_OCTETS_4];           /* 协商后密码算法能力 参考
                                                    NLSTK_SmEncAlgoAbility_E
                                                    NLSTK_SmIntgProtectAlgoAbility_E
                                                    NLSTK_SmKeyDerivAlgoAbility_E
                                                    NLSTK_SmKeyNegoAlgoAbility_E */
} SmPairingNegoParam_S;

/*****************************************************************************************
                                Callback Structure Parameters
*****************************************************************************************/

/**
 * @brief 安全管理模块回调事件
 */
typedef enum {
    SM_CBK_EVENT_PAIRING_START,             /*!< 启动配对 */
    SM_CBK_EVENT_PAIRING_REMOVE,            /*!< 移除配对 */
    SM_CBK_EVENT_PAIRING_REQUEST,           /*!< 配对请求 */
    SM_CBK_EVENT_AUTH_COMPLETE,             /*!< 鉴权完成 */
    SM_CBK_EVENT_ENCRYPT_COMPLETE,          /*!< 加密完成 */
    SM_CBK_EVENT_IMG_SEND_MESSAGE,          /*!< 组播消息 */
    SM_CBK_EVENT_IMG_ENCRYPT,               /*!< 组播加密 */
    SM_INVALID_EVENT                        /*!< 无效事件 */
} SmCallbackEvent_E;

#ifdef __cplusplus
}
#endif
#endif /* SM_STRUCT_H */