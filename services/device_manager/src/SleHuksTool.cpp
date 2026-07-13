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

#include <iterator>
#include <sstream>
#include <cstring>
#include "SleHuksTool.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
const char NEARLINK_KEY[] = "NearlinkKeyAlias";
const HksBlob NEARLINK_KEY_ALIAS = { static_cast<uint32_t>(strlen(NEARLINK_KEY)),
    const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(NEARLINK_KEY)) };
const HksParam GEN_PARAM[] = {
    { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_SM2 },
    { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
    { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_SM2_KEY_SIZE_256 },
    { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
};
const HksParam ENCRYPT_PARAM[] = {
    { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_SM2 },
    { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
    { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_SM2_KEY_SIZE_256 },
    { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_OAEP },
    { .tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_SM3 },
    { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
};
const HksParam DENCRYPT_PARAM[] = {
    { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_SM2 },
    { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
    { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_SM2_KEY_SIZE_256 },
    { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_OAEP },
    { .tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_SM3 },
    { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
};
} // namespace

static inline int32_t InitParamSet(HksParamSet **paramSet, const HksParam *params, uint32_t paramCount)
{
    int32_t ret = HksInitParamSet(paramSet);
    NL_CHECK_RETURN_RET(ret == HKS_SUCCESS, ret, "HksInitParamSet failed");

    ret = HksAddParams(*paramSet, params, paramCount);
    if (ret != HKS_SUCCESS) {
        HILOGE("HksAddParams failed");
        HksFreeParamSet(paramSet);
        return ret;
    }

    ret = HksBuildParamSet(paramSet);
    if (ret != HKS_SUCCESS) {
        HILOGE("HksBuildParamSet failed!");
        HksFreeParamSet(paramSet);
        return ret;
    }
    return ret;
}

static inline int32_t IsHksKeyExist(const HksBlob *keyAlias)
{
    HksParam keyExistParams[] = {
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    HksParamSet *keyExistParamSet = nullptr;
    int32_t ret = InitParamSet(&keyExistParamSet, keyExistParams, sizeof(keyExistParams) / sizeof(HksParam));
    NL_CHECK_RETURN_RET(ret == HKS_SUCCESS, ret, "Init keyExist ParamSet failed");

    ret = HksKeyExist(keyAlias, keyExistParamSet);
    HksFreeParamSet(&keyExistParamSet);
    return ret;
}

SleHksTool& SleHksTool::GetInstance()
{
    static SleHksTool instance;
    return instance;
}

int32_t SleHksTool::SleGenerateKey(const HksBlob &keyAlias, const HksParamSet *genParamSet)
{
    int32_t keyExist = IsHksKeyExist(&keyAlias);
    if (keyExist == HKS_ERROR_NOT_EXIST) {
        int32_t ret = HksGenerateKey(&keyAlias, genParamSet, nullptr);
        HILOGI("HksGenerateKey key ret(%{public}d)", ret);
        return ret;
    }
    HILOGI("HksGenerateKey key keyExist(%{public}d)", keyExist);
    return keyExist;
}

int32_t SleHksTool::CleanUpResources(HksParamSet *genParamSet, HksParamSet *encryptParamSet, int32_t ret)
{
    if (ret == HKS_SUCCESS){
        HksFreeParamSet(&genParamSet);
        HksFreeParamSet(&encryptParamSet);
        return ret;
    }
    HksDeleteKey(&NEARLINK_KEY_ALIAS, genParamSet);
    HksFreeParamSet(&genParamSet);
    HksFreeParamSet(&encryptParamSet);
    return ret;
}

template<typename T,typename S>
int32_t SleHksTool::SleEncrypt(const T &plainText, S &encryptedText, int plainTextLength, int encryptedTextLength)
{
    // Initializes the ParamSet of the key.
    HksParamSet *genParamSet = nullptr;
    int32_t ret = InitParamSet(&genParamSet, GEN_PARAM, sizeof(GEN_PARAM) / sizeof(HksParam));
    NL_CHECK_RETURN_RET(ret == HKS_SUCCESS, ret, "InitParamSet failed");

    // Initializes the encrypted paramSet.
    HksParamSet *encryptParamSet = nullptr;
    ret = InitParamSet(&encryptParamSet, ENCRYPT_PARAM, sizeof(ENCRYPT_PARAM) / sizeof(HksParam));
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&genParamSet);
        return ret;
    }

    // Generate Key
    ret = SleGenerateKey(NEARLINK_KEY_ALIAS, genParamSet);
    if (ret != HKS_SUCCESS) {
        HILOGE("generate key failed");
        HksFreeParamSet(&genParamSet);
        HksFreeParamSet(&encryptParamSet);
        return ret;
    }

    // Convert the link key to HksBlob.
    T hksBlobStr;
    if (memcpy_s(&hksBlobStr[0], plainTextLength, &plainText[0], plainTextLength) != EOK) {
        LOG_ERROR("memcpy_s failed!");
        HksFreeParamSet(&genParamSet);
        HksFreeParamSet(&encryptParamSet);
        return HKS_FAILURE;
    }
    HksBlob inData = { plainTextLength, hksBlobStr.data() };

    // encrypt
    uint8_t cipherBuf[COMMON_SIZE] = {0};
    HksBlob cipherData = {
        .size = COMMON_SIZE,
        .data = cipherBuf
    };
    ret = HksEncrypt(&NEARLINK_KEY_ALIAS, encryptParamSet, &inData, &cipherData);
    if (ret != HKS_SUCCESS) {
        HILOGE("hks encryption failed");
        (void)memset_s(&hksBlobStr, sizeof(hksBlobStr), 0x00, sizeof(hksBlobStr));
        return CleanUpResources(genParamSet, encryptParamSet, ret);
    }
    (void)memset_s(&hksBlobStr, sizeof(hksBlobStr), 0x00, sizeof(hksBlobStr));
    // Convert the ciphertext into a link key.
    errno_t memRet = memcpy_s(encryptedText.data(), encryptedTextLength, cipherData.data, cipherData.size);
    if (memRet != EOK) {
        HILOGE("memcpy_s ret error, ret: %{public}d", memRet);
        return CleanUpResources(genParamSet, encryptParamSet, HKS_FAILURE);
    }
    return CleanUpResources(genParamSet, encryptParamSet, ret);
}

template<typename T,typename S>
int32_t SleHksTool::SleDecrypt(const T &cipherText, S &decryptedText, int cipherTextLength, int decryptedTextLength)
{
    T empty_cipher_text {};
    NL_CHECK_RETURN_RET(cipherText != empty_cipher_text, HKS_SUCCESS, "encryptedStr is empty");

    // Initialize the decrypted paramSet.
    HksParamSet *decryptParamSet = nullptr;
    int32_t ret = InitParamSet(&decryptParamSet, DENCRYPT_PARAM, sizeof(DENCRYPT_PARAM) / sizeof(HksParam));
    NL_CHECK_RETURN_RET(ret == HKS_SUCCESS, ret, "init decryptParamSet failed");

    // Check whether the key exists
    ret = IsHksKeyExist(&NEARLINK_KEY_ALIAS);
    if (ret != HKS_SUCCESS) {
        HILOGE("key not exist");
        HksFreeParamSet(&decryptParamSet);
        return ret;
    }

    // Convert the ciphertext linkKey to HksBlob.
    T hksBlobEncryptedStr;
    if (memcpy_s(&hksBlobEncryptedStr[0], cipherTextLength, &cipherText[0], cipherTextLength) != EOK) {
        LOG_ERROR("memcpy_s failed!");
        HksFreeParamSet(&decryptParamSet);
        return HKS_FAILURE;
    }
    HksBlob cipherData = { cipherTextLength, hksBlobEncryptedStr.data() };

    // decrypt
    uint8_t plainBuff[COMMON_SIZE] = {0};
    HksBlob plainText = {
        .size = COMMON_SIZE,
        .data = plainBuff
    };
    ret = HksDecrypt(&NEARLINK_KEY_ALIAS, decryptParamSet, &cipherData, &plainText);
    if (ret != HKS_SUCCESS) {
        HILOGE("hks decryption failed");
        (void)memset_s(plainBuff, sizeof(plainBuff), 0x00, sizeof(plainBuff));
        HksFreeParamSet(&decryptParamSet);
        return ret;
    }

    // Convert the decryption result into a link key.
    errno_t memRet = memcpy_s(decryptedText.data(), decryptedTextLength, plainText.data, decryptedTextLength);
    if (memRet != EOK) {
        HILOGE("memcpy_s ret error, ret: %{public}d", memRet);
        (void)memset_s(plainBuff, sizeof(plainBuff), 0x00, sizeof(plainBuff));
        HksFreeParamSet(&decryptParamSet);
        return HKS_FAILURE;
    };
    (void)memset_s(plainBuff, sizeof(plainBuff), 0x00, sizeof(plainBuff));
    HksFreeParamSet(&decryptParamSet);
    return ret;
}

int32_t SleHksTool::SleLinkKeyEncrypt(const LinkKey &linkKey, EncryptedLinkKey &encryptedLinkKey)
{
    return SleEncrypt(linkKey, encryptedLinkKey, OCTET16_LEN, OCTET128_LEN);
}

int32_t SleHksTool::SleLinkKeyDecrypt(const EncryptedLinkKey &encryptedLinkKey, LinkKey &linkKey)
{
    return SleDecrypt(encryptedLinkKey, linkKey, OCTET128_LEN, OCTET16_LEN);
}

int32_t SleHksTool::SleCloudDeviceTokenEncrypt(const CloudDeviceToken &token, EncryptedCloudDeviceToken &encryptedToken)
{
    return SleEncrypt(token, encryptedToken, OCTET32_LEN, OCTET160_LEN);
}

int32_t SleHksTool::SleCloudDeviceTokenDecrypt(const EncryptedCloudDeviceToken &encryptedToken, CloudDeviceToken &token)
{
    return SleDecrypt(encryptedToken, token, OCTET160_LEN, OCTET32_LEN);
}

int32_t SleHksTool::SleDeleteHksKey()
{
    HksParam keyExistParams[] = {
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };
    HksParamSet *keyExistParamSet = nullptr;
    int32_t ret = InitParamSet(&keyExistParamSet, keyExistParams, sizeof(keyExistParams) / sizeof(HksParam));
    NL_CHECK_RETURN_RET(ret == HKS_SUCCESS, ret, "Init keyExist ParamSet failed");

    ret = HksKeyExist(&NEARLINK_KEY_ALIAS, keyExistParamSet);
    if (ret != HKS_SUCCESS) {
        HILOGE("ret(%{public}d), key not exist.", ret);
        HksFreeParamSet(&keyExistParamSet);
        return ret;
    }

    ret = HksDeleteKey(&NEARLINK_KEY_ALIAS, keyExistParamSet);
    if (ret != HKS_SUCCESS) {
        HILOGE("ret(%{public}d), delete key failed.", ret);
    }
    HksFreeParamSet(&keyExistParamSet);
    return ret;
}
}  // namespace Nearlink
}  // namespace OHOS