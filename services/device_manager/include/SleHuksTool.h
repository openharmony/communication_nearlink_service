/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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

#ifndef OHOS_SLE_HKS_TOOL_H
#define OHOS_SLE_HKS_TOOL_H

#include <string>
#include <array>
#include "hks_api.h"
#include "hks_type.h"
#include "hks_param.h"
#include "SleUtils.h"
namespace OHOS {
namespace Nearlink {
constexpr uint32_t COMMON_SIZE = 512;
constexpr int OCTET16_LEN = 16;
constexpr int OCTET32_LEN = 32;
constexpr int OCTET128_LEN = 128;
constexpr int OCTET160_LEN = 160;
/* Link Key */
using LinkKey = std::array<uint8_t, OCTET16_LEN>;
/* Encrypted Link Key */
using EncryptedLinkKey = std::array<uint8_t, OCTET128_LEN>;
/* Cloud Device Token */
using CloudDeviceToken = std::array<uint8_t, OCTET32_LEN>;
/* Encrypted Cloud Device Token */
using EncryptedCloudDeviceToken = std::array<uint8_t, OCTET160_LEN>;
class SleHksTool final {
public:
    static SleHksTool& GetInstance();

    int32_t SleLinkKeyEncrypt(const LinkKey &linkKey, EncryptedLinkKey &encryptedLinkKey);

    int32_t SleLinkKeyDecrypt(const EncryptedLinkKey &encryptedLinkKey, LinkKey &linkKey);

    int32_t SleCloudDeviceTokenEncrypt(const CloudDeviceToken &token, EncryptedCloudDeviceToken &encryptedToken);

    int32_t SleCloudDeviceTokenDecrypt(const EncryptedCloudDeviceToken &encryptedToken, CloudDeviceToken &token);

    /**
    * @Description  delete key
    * @return HKS_SUCCESS - delete hks key success, others - delete hks key failed
    */
    int32_t SleDeleteHksKey();

private:
    /**
    * @Description  Generate new or get existed RSA key based on input encryptionInfo and genParamSet
    * @param keyAlias - keyAlias info
    * @param genParamSet - generate params
    * @return HKS_SUCCESS - find key, others - find key failed
    */
    int32_t SleGenerateKey(const HksBlob &keyAlias, const HksParamSet *genParamSet);

    /**
    * @Description  Clear up genParamSet, encryptParamSet and delete Key if fail
    * @param genParamSet - generate params
    * @param encryptParamSet - generate encrypted params
    * @param ret - return the result
    * @return HKS_SUCCESS - find key, others - find key failed
    */
    int32_t CleanUpResources(HksParamSet *genParamSet, HksParamSet *encryptParamSet, int32_t ret);

    /**
    * @Description  Encrypt inputString using RSA based on input encryptionInfo
    * @param plainText - Plaintext
    * @param encryptedText - Encrypted text
    * @param plainTextLength - Length of plaintext
    * @param plainTextLength - Length of encrypted text
    * @return HKS_SUCCESS - encryption success, others - encryption failed. Please refer to "hks_type.h"
    */
    template<typename T,typename S>
    int32_t SleEncrypt(const T &plainText, S &encryptedText, int plainTextLength, int encryptedTextLength);

    /**
    * @Description  Decrypt encryptedData using RSA based on input encryptionInfo
    * @param cipherText - Ciphertext
    * @param decryptedText - Decrypted text
    * @param cipherTextLength - Length of ciphertext
    * @param decryptedTextLength - Length of decrypted text
    * @return HKS_SUCCESS - decryption success, others - decryption failed
    */
    template<typename T,typename S>
    int32_t SleDecrypt(const T &cipherText, S &decryptedText, int cipherTextLength, int decryptedTextLength);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif