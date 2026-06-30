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

/*
 * 本文件提供配对过程中使用的密码学算法.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "openssl/kdf.h"
#include "openssl/param_build.h"
#include <openssl/params.h>
#include "securec.h"

#include "nai_log.h"
#include "sle_crypto.h"

#define PUBKEY_LEN_UNCONV_256       65      /* ECDH 256 椭圆曲线的公钥长度(未压缩公钥) */
#define PUBKEY_LEN_CONV_256         33      /* ECDH 256 椭圆曲线的公钥长度(压缩公钥) */
#define ECDH_MODULE_LEN             32      /* ECDH 椭圆曲线模长 */
#define PUBKEY_CONV_FORM            0x03    /* 压缩公钥标志位 */
#define CRYPTO_KEY_GEN_MAX_TRY_CNT  100     /* SECP-256椭圆曲线生成公私钥对重试最大次数 */
#define CRYPTO_AES_CMAC_KEY_LEN     16      /* AES-CMAC 密钥长度 */
#define CRYPTO_PRIVATE_KEY_LEN      32      /* 私钥长度 */
#define CRYPTO_PUBLIC_KEY_LEN       64      /* 公钥长度 */
#define CRYPTO_SEC_KEY_LEN          32      /* 密钥长度 */
#define CRYPTO_HALF_PUBLIC_KEY_LEN  32      /* 公钥半长 */
#define DIVIDED_TWO                 2       /* 除数定义 */

static void EndianReverseOctets(uint8_t *data, uint32_t length);
static bool CheckConvForm(EVP_PKEY *pkey, uint8_t form);
static bool GenKeyPairToBuff(EVP_PKEY *pkey, uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN],
    uint8_t pubKey[CRYPTO_PUBLIC_KEY_LEN]);
static EVP_PKEY *RawKeyPairToPkey(uint8_t pri[CRYPTO_PRIVATE_KEY_LEN], uint8_t pub[CRYPTO_PUBLIC_KEY_LEN]);
static EVP_PKEY *RawPubkeyTopkey(uint8_t pub[CRYPTO_PUBLIC_KEY_LEN]);
static bool DeriveDhKey(EVP_PKEY *keyPair, EVP_PKEY *remoteKey, uint8_t secKey[CRYPTO_SEC_KEY_LEN]);

static void RandNumGenerate(uint8_t *out, uint8_t len);
static void ECDH_PubPriKeyGenerate(uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN], uint8_t pubKey[CRYPTO_PUBLIC_KEY_LEN]);
static void ECDH_SecKeyGenerate(uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN], uint8_t localPubKey[CRYPTO_PUBLIC_KEY_LEN],
                                uint8_t remotePubKey[CRYPTO_PUBLIC_KEY_LEN], uint8_t secKey[CRYPTO_SEC_KEY_LEN]);
static void AES_CMAC_KeyGenerate(uint8_t *key, uint8_t *buff, size_t buffSize, uint8_t *mac);
static void GenSha256Hash(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash);

/*****************************************************************************************
                                    Interfaces
*****************************************************************************************/

void Crypto_RandNumGenerate(uint8_t *out, uint8_t len)
{
    RandNumGenerate(out, len);
}

void Crypto_PubPriKeyPairGenerate(NLSTK_SmKeyPair_S *keyPair)
{
    switch (keyPair->algo) {
        case KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1:
            break;
        case KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2:
            ECDH_PubPriKeyGenerate(keyPair->priKey, keyPair->localPubKey);
            break;
        default:
            NAI_LOG_ERROR("Algo input is invalid.");
            break;
    }
}

void Crypto_SecKeyGenerate(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen)
{
    NAI_CHECK_LOG_RETURN_VOID(secKeyLen == CRYPTO_SEC_KEY_LEN, "security key generate failed.");
    switch (keyPair->algo) {
        case KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1:
            break;
        case KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2:
            ECDH_SecKeyGenerate(keyPair->priKey, keyPair->localPubKey, keyPair->remotePubKey, secKey);
            break;
        default:
            NAI_LOG_ERROR("Algo input is invalid.");
            break;
    }
}

void Crypto_DerivedKeyGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen)
{
    NAI_CHECK_LOG_RETURN_VOID(outputLen == CRYPTO_AES_CMAC_KEY_LEN, "Derived key generate failed.");
    switch (input->algo) {
        case KEY_DERIVATION_ALGORITHM_ABILITY_HA1:
            break;
        case KEY_DERIVATION_ALGORITHM_ABILITY_HA2:
            AES_CMAC_KeyGenerate(input->key, input->buff, input->buffSize, output);
            break;
        default:
            NAI_LOG_ERROR("Algo input is invalid.");
            break;
    }
}

void Crypto_Sha256(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash)
{
    GenSha256Hash(value, shaHash);
}

/*****************************************************************************************
                                Algorithm implementation
*****************************************************************************************/

/** 用于生成指定长度的随机数，并将其写入输出缓冲区 out。具体实现通过从系统的 /dev/urandom 或 /dev/random 设备文件读取数据来完成。
 *  \param  out  指向输出缓冲区的指针，用于存放生成的随机数。
 *  \param  len  指定需要生成的随机数的长度（字节数）。
 */
static void RandNumGenerate(uint8_t *out, uint8_t len)
{
    size_t left = len;
    int fd;
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        fd = open("/dev/random", O_RDONLY);
        if (fd < 0) {
            NAI_LOG_ERROR("Failed to generate random num, fail to open file.");
            return;
        }
    }
    while (left > 0) {
        ssize_t ret;
        ret = read(fd, out, left);
        if (ret <= 0) {
            close(fd);
            NAI_LOG_ERROR("Failed to generate random num, fail to read file.");
            return;
        }
        left -= (size_t)ret;
        out += ret;
    }
    close(fd);
}

/** 此函数用于生成一对公私钥，并将其分别存储到 priKey 和 pubKey 缓冲区。
 *  该函数实现了基于 P-256 椭圆曲线的公私钥对生成，并通过 OpenSSL API 进行密钥对的生成、格式检查以及字节转换。
 *  \param  priKey  存储生成的私钥。
 *  \param  pubKey  存储生成的公钥。
 */
static void ECDH_PubPriKeyGenerate(uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN], uint8_t pubKey[CRYPTO_PUBLIC_KEY_LEN])
{
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    uint8_t findKey = 0;

    do {
        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        if (!ctx) {
            NAI_LOG_ERROR("OPENSSSL: EVP EC CTX init failed.");
            break;
        }
        /* 获取随机公私钥对，重复获取直到：1.公钥满足压缩格式03, 或2. 重复次数到达上限 */
        for (size_t i = 0; i < CRYPTO_KEY_GEN_MAX_TRY_CNT; i++) {
            pkey = EVP_EC_gen("P-256");
            if (pkey != NULL && CheckConvForm(pkey, PUBKEY_CONV_FORM)) {
                findKey = 1;
                break;
            } else {
                // 不满足压缩格式要求，删除密钥
                EVP_PKEY_free(pkey);
            }
        }
        /* 重复到达上限仍然没有找到符合要求的随机数，报错退出。（大概率原因是随机数异常） */
        if (findKey == 0) {
            NAI_LOG_ERROR("Fail to get proper key pair.");
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return;
        }
        /* 获取随机公私钥对公钥和私钥原文 */
        if (!GenKeyPairToBuff(pkey, priKey, pubKey)) {
            NAI_LOG_ERROR("Fail to generate proper key pair to buff.");
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            return;
        }
    } while (0);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
}

/** 此函数用于生成 Diffie-Hellman 密钥（dhKey），其中包含本地私钥和公钥、远程公钥的输入，并最终计算出共享的密钥。
 *  该过程包括密钥格式转换、密钥对生成、密钥派生等步骤。
 *  \param  priKey          本端私钥。
 *  \param  localPubKey     本端公钥。
 *  \param  remotePubKey    对端公钥。
 *  \param  dhKey           最终生成的 Diffie-Hellman 共享密钥。
 */
static void ECDH_SecKeyGenerate(uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN], uint8_t localPubKey[CRYPTO_PUBLIC_KEY_LEN],
                                uint8_t remotePubKey[CRYPTO_PUBLIC_KEY_LEN], uint8_t secKey[CRYPTO_SEC_KEY_LEN])
{
    uint8_t tempPri[CRYPTO_PRIVATE_KEY_LEN];
    uint8_t tempLocalPub[CRYPTO_PUBLIC_KEY_LEN];
    uint8_t tempRemotePub[CRYPTO_PUBLIC_KEY_LEN];

    (void)memcpy_s(tempPri, CRYPTO_PRIVATE_KEY_LEN, priKey, CRYPTO_PRIVATE_KEY_LEN);
    (void)memcpy_s(tempLocalPub, CRYPTO_PUBLIC_KEY_LEN, localPubKey, CRYPTO_PUBLIC_KEY_LEN);
    (void)memcpy_s(tempRemotePub, CRYPTO_PUBLIC_KEY_LEN, remotePubKey, CRYPTO_PUBLIC_KEY_LEN);

    EndianReverseOctets(tempPri, CRYPTO_PRIVATE_KEY_LEN);
    EndianReverseOctets(tempLocalPub, CRYPTO_HALF_PUBLIC_KEY_LEN);
    EndianReverseOctets(tempLocalPub + CRYPTO_HALF_PUBLIC_KEY_LEN, CRYPTO_HALF_PUBLIC_KEY_LEN);
    EndianReverseOctets(tempRemotePub, CRYPTO_HALF_PUBLIC_KEY_LEN);
    EndianReverseOctets(tempRemotePub + CRYPTO_HALF_PUBLIC_KEY_LEN, CRYPTO_HALF_PUBLIC_KEY_LEN);

    EVP_PKEY *local = RawKeyPairToPkey(tempPri, tempLocalPub);
    NAI_CHECK_LOG_RETURN_VOID(local, "Raw local key pair error.");
    EVP_PKEY *remote = RawPubkeyTopkey(tempRemotePub);
    if (remote == NULL) {
        NAI_LOG_ERROR("remote pubkey to pkey error.");
        EVP_PKEY_free(local);
        return;
    }

    uint8_t tempDhKey[CRYPTO_SEC_KEY_LEN] = {0};
    NAI_CHECK_LOG_RETURN_VOID(DeriveDhKey(local, remote, tempDhKey), "Derive dhkey failed.");

    EndianReverseOctets(tempDhKey, CRYPTO_SEC_KEY_LEN);
    (void)memcpy_s(secKey, CRYPTO_SEC_KEY_LEN, tempDhKey, CRYPTO_SEC_KEY_LEN);

    EVP_PKEY_free(local);
    EVP_PKEY_free(remote);
}

/** 该函数实现了基于 AES-CMAC (Cipher-based Message Authentication Code) 算法的消息认证码（MAC）计算。
 *  该函数的核心步骤包括密钥反转、输入数据反转、CMAC 计算和反转结果。
 *  \param  key         AES-CMAC 的密钥。
 *  \param  buff        输入数据，计算 CMAC 时需要的消息。
 *  \param  buffSize    输入数据的大小（字节数）。
 *  \param  mac         输出的消息认证码，计算得到的 128 位结果。
 */
static void AES_CMAC_KeyGenerate(uint8_t *key, uint8_t *buff, size_t buffSize, uint8_t *mac)
{
    uint8_t keyReversed[CRYPTO_AES_CMAC_KEY_LEN];
    uint8_t *inputReversed = malloc(buffSize);
    NAI_CHECK_LOG_RETURN_VOID(inputReversed != NULL, "Memory allocation error.");

    (void)memcpy_s(keyReversed, CRYPTO_AES_CMAC_KEY_LEN, key, CRYPTO_AES_CMAC_KEY_LEN);
    EndianReverseOctets(keyReversed, CRYPTO_AES_CMAC_KEY_LEN);
    
    (void)memcpy_s(inputReversed, (uint32_t)buffSize, buff, (uint32_t)buffSize);
    EndianReverseOctets(inputReversed, buffSize);

    CMAC_CTX *ctx = CMAC_CTX_new();
    if (ctx == NULL) {
        NAI_LOG_ERROR("CMAC_CTX_new error.");
        free(inputReversed);
        return;
    }
    if (CMAC_Init(ctx, keyReversed, CRYPTO_AES_CMAC_KEY_LEN, EVP_aes_128_cbc(), NULL) != 1) {
        NAI_LOG_ERROR("CMAC_Init error.");
        CMAC_CTX_free(ctx);
        free(inputReversed);
        return;
    }
    if (CMAC_Update(ctx, inputReversed, buffSize) != 1) {
        NAI_LOG_ERROR("EVP_CMAC_Update error.");
        CMAC_CTX_free(ctx);
        free(inputReversed);
        return;
    }
    size_t macLen = 0;
    if (CMAC_Final(ctx, mac, &macLen) != 1) {
        NAI_LOG_ERROR("EVP_CMAC_Final error.");
        CMAC_CTX_free(ctx);
        free(inputReversed);
        return;
    }
    EndianReverseOctets(mac, macLen);
    CMAC_CTX_free(ctx);
    free(inputReversed);
}

/*****************************************************************************************
                                    Help Functions
*****************************************************************************************/

/** 用于翻转数组，保证大小端序一致
 *  \param  data  数组
 *  \param  length  要翻转的字节长度
 */
static void EndianReverseOctets(uint8_t *data, uint32_t length)
{
    NAI_CHECK_LOG_RETURN_VOID(data != NULL, "Data null pointer failure.");
    uint32_t lastIndex = length - 1;
    for (uint32_t i = 0; i < length / DIVIDED_TWO; i++) {
        uint8_t temp = data[i];
        data[i] = data[lastIndex - i];
        data[lastIndex - i] = temp;
    }
}

/** 检查EVP_PKEY格式公私钥对中公钥的压缩类型是否满足要求
 *  \param  pkey  指定公私钥对
 *  \param  form  指定压缩类型
 *  \return 0:是，1：否
 */
static bool CheckConvForm(EVP_PKEY *pkey, uint8_t form)
{
    bool ret = false;
    uint8_t pub[PUBKEY_LEN_UNCONV_256] = {0};
    size_t len = PUBKEY_LEN_UNCONV_256;

    do {
        if (EVP_PKEY_get_octet_string_param(pkey, "pub", pub, PUBKEY_LEN_UNCONV_256, &len) != 1) {
            NAI_LOG_ERROR("Fail to get public key from key pair.");
            break;
        }
        if (len == PUBKEY_LEN_CONV_256 && pub[0] == form) {
            ret = true;
        } else if (len == PUBKEY_LEN_UNCONV_256 && (0x02 + (pub[len - 1] & 0x01) == form)) {
            ret = true;
        }
    } while (0);

    return ret;
}

/** 此函数将生成的公私钥对转换为字节数组，并存储到 priKey 和 pubKey 缓冲区中。
 *  \param  pkey    包含公私钥对的 EVP_PKEY 对象。
 *  \param  priKey  用于存储私钥的缓冲区。
 *  \param  pubKey  用于存储公钥的缓冲区。
 *  \return 0:是，1：否
 */
static bool GenKeyPairToBuff(EVP_PKEY *pkey, uint8_t priKey[CRYPTO_PRIVATE_KEY_LEN],
    uint8_t pubKey[CRYPTO_PUBLIC_KEY_LEN])
{
    bool ret = false;
    size_t pubLen = PUBKEY_LEN_UNCONV_256;
    uint8_t pub[PUBKEY_LEN_UNCONV_256] = {0};
    BIGNUM *pri = NULL;

    do {
        if (EVP_PKEY_get_bn_param(pkey, "priv", &pri) != 1) {
            NAI_LOG_ERROR("Failed to get private key BIGNUM from key pair.");
            break;
        }
        int priActualLen = BN_num_bytes(pri);
        if (priActualLen > CRYPTO_PRIVATE_KEY_LEN) {
            NAI_LOG_ERROR("Unexpected prikey length: expected %zu, got %d.\n", CRYPTO_PRIVATE_KEY_LEN, priActualLen);
            break;
        }
        uint8_t tempPriKey[CRYPTO_PRIVATE_KEY_LEN] = {0};
        if (BN_bn2bin(pri, tempPriKey + (CRYPTO_PRIVATE_KEY_LEN - priActualLen)) != priActualLen) {
            NAI_LOG_ERROR("Failed to convert BIGNUM to binary for private key.");
            break;
        }
        EndianReverseOctets(tempPriKey, CRYPTO_PRIVATE_KEY_LEN);
        (void)memcpy_s(priKey, CRYPTO_PRIVATE_KEY_LEN, tempPriKey, CRYPTO_PRIVATE_KEY_LEN);

        if (EVP_PKEY_get_octet_string_param(pkey, "pub", pub, PUBKEY_LEN_UNCONV_256, &pubLen) != 1) {
            NAI_LOG_ERROR("Fail to get public key from key pair.");
            break;
        }
        if (memcpy_s(pubKey, CRYPTO_PUBLIC_KEY_LEN, pub + 1, CRYPTO_PUBLIC_KEY_LEN) == EOK) {
            EndianReverseOctets(pubKey, CRYPTO_HALF_PUBLIC_KEY_LEN);
            EndianReverseOctets(pubKey + CRYPTO_HALF_PUBLIC_KEY_LEN, CRYPTO_HALF_PUBLIC_KEY_LEN);
            ret = true;
        }
    } while (0);

    BN_clear_free(pri);
    return ret;
}

/** 该函数的作用是将原始的私钥和公钥二进制数据转换为 OpenSSL 使用的 EVP_PKEY 结构。EVP_PKEY 结构用于存储和处理加密算法中的密钥对。
 *  \param  pri  私钥的原始二进制数据。
 *  \param  pub  公钥的原始二进制数据。
 *  \return 转换为EVP PKEY结构的公私钥对
 */
static EVP_PKEY *RawKeyPairToPkey(uint8_t pri[CRYPTO_PRIVATE_KEY_LEN], uint8_t pub[CRYPTO_PUBLIC_KEY_LEN])
{
    bool ret = false;
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    BIGNUM *priKey = NULL;
    OSSL_PARAM_BLD *paramBld = NULL;
    OSSL_PARAM *param = NULL;
    uint8_t pubKey[PUBKEY_LEN_CONV_256] = {0};

    do {
        if ((priKey = BN_bin2bn(pri, CRYPTO_PRIVATE_KEY_LEN, NULL)) == NULL ||
            memcpy_s(pubKey + 1, ECDH_MODULE_LEN, pub, ECDH_MODULE_LEN) != EOK) {
            break;
        }
        pubKey[0] = PUBKEY_CONV_FORM;
        if ((paramBld = OSSL_PARAM_BLD_new()) == NULL ||
            !OSSL_PARAM_BLD_push_utf8_string(paramBld, "group", "prime256v1", 0) ||
            !OSSL_PARAM_BLD_push_BN(paramBld, "priv", priKey) ||
            !OSSL_PARAM_BLD_push_octet_string(paramBld, "pub", pubKey, sizeof(pubKey))) {
            NAI_LOG_ERROR("Failed to build evp pkey ossl params.");
            break;
        }
        if ((param = OSSL_PARAM_BLD_to_param(paramBld)) == NULL) {
            NAI_LOG_ERROR("param is null.");
            break;
        }
        if ((ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL)) == NULL ||
            EVP_PKEY_fromdata_init(ctx) <= 0 ||
            EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_KEYPAIR, param) <= 0) {
            unsigned long errCode = ERR_get_error();
            char errMsg[120];
            ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
            NAI_LOG_ERROR("Failed to get evp pkey, ret = %d, OpenSSL Error Message: %s.", ret, errMsg);
            break;
        }
        ret = true;
    } while (0);

    EVP_PKEY_CTX_free(ctx);
    OSSL_PARAM_free(param);
    OSSL_PARAM_BLD_free(paramBld);
    BN_clear_free(priKey);
    if (ret) {
        NAI_LOG_INFO("Raw key pair to pkey succeed.");
        return pkey;
    } else {
        NAI_LOG_INFO("Raw key pair to pkey failed.");
        return NULL;
    }
}

/** 将公钥原始二进制串转换为 EVP PKEY 结构，不含私钥
 *  \param  pub    输入对端公钥
 *  \return 转换为EVP PKEY结构的对端公钥
 */
static EVP_PKEY *RawPubkeyTopkey(uint8_t pub[CRYPTO_PUBLIC_KEY_LEN])
{
    bool ret = false;
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    OSSL_PARAM_BLD *paramBld = NULL;
    OSSL_PARAM *para = NULL;
    uint8_t pubKey[PUBKEY_LEN_CONV_256] = {0};

    do {
        if (memcpy_s(pubKey + 1, ECDH_MODULE_LEN, pub, ECDH_MODULE_LEN) != EOK) {
            break;
        }
        pubKey[0] = PUBKEY_CONV_FORM;
        if ((paramBld = OSSL_PARAM_BLD_new()) == NULL ||
            !OSSL_PARAM_BLD_push_utf8_string(paramBld, "group", "prime256v1", 0) ||
            !OSSL_PARAM_BLD_push_octet_string(paramBld, "pub", pubKey, sizeof(pubKey))) {
            NAI_LOG_ERROR("Failed to build evp pkey ossl params.");
            break;
        }
        if ((para = OSSL_PARAM_BLD_to_param(paramBld)) == NULL) {
            NAI_LOG_ERROR("param is null.");
            break;
        }
        if ((ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL)) == NULL ||
            EVP_PKEY_fromdata_init(ctx) <= 0 ||
            EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_PUBLIC_KEY, para) <= 0) {
            unsigned long errCode = ERR_get_error();
            char errMsg[120];
            ERR_error_string_n(errCode, errMsg, sizeof(errMsg));
            NAI_LOG_ERROR("Failed to get evp pkey, ret = %d, OpenSSL Error Message: %s.", ret, errMsg);
            break;
        }
        ret = true;
    } while (0);

    EVP_PKEY_CTX_free(ctx);
    OSSL_PARAM_free(para);
    OSSL_PARAM_BLD_free(paramBld);

    if (ret) {
        NAI_LOG_INFO("Raw pubkey to pkey succeed.");
        return pkey;
    } else {
        NAI_LOG_INFO("Raw pubkey to pkey failed.");
        return NULL;
    }
}

/** 使用 模长为256比特的椭圆曲线密钥协商算法计算共享秘密信息。
 *  \param  keyPair    输入本端公私钥对
 *  \param  remoteKey  输入对端公钥
 *  \param  dhKey      输出共享秘密信息
 *  \return 0:成功，非0：失败
 */
static bool DeriveDhKey(EVP_PKEY *keyPair, EVP_PKEY *remoteKey, uint8_t dhKey[CRYPTO_SEC_KEY_LEN])
{
    bool ret = false;
    EVP_PKEY_CTX *dctx = NULL;
    size_t outLen = ECDH_MODULE_LEN;
    uint8_t out[ECDH_MODULE_LEN] = {0};

    if (!keyPair || !remoteKey || !dhKey || (outLen < CRYPTO_SEC_KEY_LEN)) {
        NAI_LOG_ERROR("Derive DhKey Input error.");
        return ret;
    }
    do {
        dctx = EVP_PKEY_CTX_new(keyPair, NULL);
        if (!dctx) {
            NAI_LOG_ERROR("No memory space for inner value.");
            break;
        }
        if (EVP_PKEY_derive_init(dctx) != 1 ||
            EVP_PKEY_derive_set_peer(dctx, remoteKey) != 1 ||
            EVP_PKEY_derive(dctx, NULL, &outLen) != 1 ||
            EVP_PKEY_derive(dctx, out, &outLen) != 1) {
            NAI_LOG_ERROR("Key deriven failed.");
            break;
        }
        if (memcpy_s(dhKey, CRYPTO_SEC_KEY_LEN, out, CRYPTO_SEC_KEY_LEN) != EOK) {
            NAI_LOG_ERROR("Build output failed.");
            break;
        }
        ret = true;
    } while (0);

    EVP_PKEY_CTX_free(dctx);
    return ret;
}

static void GenSha256Hash(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash)
{
    NAI_CHECK_LOG_RETURN_VOID(value != NULL && value->data != NULL && value->len != 0 && shaHash != NULL,
        "Gen Sha256 input invalid");
    SHA256_CTX sha256;
    if (SHA256_Init(&sha256) != 1) {
        NAI_LOG_ERROR("SHA256_Init failed.");
        return;
    }
    if (SHA256_Update(&sha256, value->data, value->len) != 1) {
        NAI_LOG_ERROR("SHA256_Update failed.");
        return;
    }
    if (SHA256_Final(shaHash->sha256Hash, &sha256) != 1) {
        NAI_LOG_ERROR("SHA256_Final failed.");
        return;
    }
}