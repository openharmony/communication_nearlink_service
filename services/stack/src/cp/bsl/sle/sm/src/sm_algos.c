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
#include "securec.h"

#include "nlstk_log.h"
#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_algos.h"

NLSTK_SmCryptoAlgoFuncs_S g_algoFuncs;

/*****************************************************************************************
                                SM Algorithm Interfaces
*****************************************************************************************/

bool SmGenRandNum(uint8_t *out, uint8_t len)
{
    NLSTK_CHECK_RETURN(out != NULL && len != 0, false, "rand num params invalid");
    if (g_algoFuncs.randNumFunc != NULL) {
        NLSTK_LOG_DEBUG("[SM] Start to generate rand number.");
        g_algoFuncs.randNumFunc(out, len);
        return true;
    }
    return false;
}

bool SmGenPubPriKey(NLSTK_SmKeyPair_S *keyPair)
{
    if (g_algoFuncs.pubPriKeyFunc != NULL) {
        NLSTK_LOG_INFO("[SM] Start to generate pubkey and prikey, algo %d.", keyPair->algo);
        g_algoFuncs.pubPriKeyFunc(keyPair);
        return true;
    }
    return false;
}

bool SmGenDhKey(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen)
{
    NLSTK_CHECK_RETURN(keyPair != NULL && secKey != NULL, false, "[SM] Dh key params invalid.");
    if (g_algoFuncs.secKeyFunc != NULL) {
        NLSTK_LOG_INFO("[SM] Start to generate security key, algo %d.", keyPair->algo);
        g_algoFuncs.secKeyFunc(keyPair, secKey, secKeyLen);
        return true;
    }
    return false;
}

bool SmCmacGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen)
{
    NLSTK_CHECK_RETURN(input != NULL && output != NULL && outputLen != 0, false, "[SM] Cmac input invalid.");
    if (g_algoFuncs.derivedKeyFunc != NULL) {
        NLSTK_LOG_DEBUG("[SM] Start to generate derived key, algo %d.", input->algo);
        g_algoFuncs.derivedKeyFunc(input, output, outputLen);
        return true;
    }
    return false;
}

bool SmGenSha256(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash)
{
    NLSTK_CHECK_RETURN(value != NULL && value->data != NULL && value->len != 0 && shaHash != NULL,
        false, "[SM] Sha input invalid.");
    if (g_algoFuncs.sha256HashFunc != NULL) {
        NLSTK_LOG_INFO("[SM] Start to generate sha256.");
        g_algoFuncs.sha256HashFunc(value, shaHash);
        return true;
    }
    return false;
}

NLSTK_SmCryptoAlgoFuncs_S* SmGetAlgoFuncs(void)
{
    return &g_algoFuncs;
}