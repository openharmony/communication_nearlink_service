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
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_log.h"

#include "nlstk_sm_algos.h"
#include "sm_numcmp.h"
#include "sm_noentry.h"
#include "sm_passcode.h"
#include "sm_password.h"
#include "sm_oob.h"
#include "sm_psk.h"
#include "sm_algos.h"
#include "sm_auth.h"

/*
 * 星闪协议提供了六种鉴权方式，根据协商阶段选择的鉴权方式，
 * 下面数组中对应的收包接口函数将被激活.
 * 如需修改或新增鉴权方式，应在下面的数组中填入对应的接口函数.
 */
SmSLinkPkgDispatcher_S SmGetAuthPkgDispatcher(NLSTK_SmAuthMethod_E authMethod)
{
    static SmSLinkPkgDispatcher_S smAuthPkgDispatcher[SM_AUTH_METHOD_MAX] = {
        [SM_AUTH_NUMBER_COMPARE] = SmNumCmpAuthPkgDispatcher,
        [SM_AUTH_NO_ENTRY] = SmNoEntryAuthPkgDispatcher,
        [SM_AUTH_PASSCODE] = SmPassCodeAuthPkgDispatcher,
        [SM_AUTH_PASSWORD_ENTRY] = SmPassWordAuthPkgDispatcher,
        [SM_AUTH_OUT_OF_BAND] = SmOobAuthPkgDispatcher,
        [SM_AUTH_PSK] = SmPskAuthPkgDispatcher,
    };
    if (authMethod >= SM_AUTH_METHOD_MAX) {
        return NULL;
    }
    return smAuthPkgDispatcher[authMethod];
}

/*
 * 类似地，鉴权方式的启动函数应当被填入下面的数组.
 * 当本端进入鉴权阶段时，启动函数将被调用
 */
SmAuthStartFunc_S SmGetAuthStartFunc(NLSTK_SmAuthMethod_E authMethod)
{
    static SmAuthStartFunc_S smAuthStartFunc[SM_AUTH_METHOD_MAX] = {
        [SM_AUTH_NUMBER_COMPARE] = SmNumCmpAuthStart,
        [SM_AUTH_NO_ENTRY] = SmNoEntryAuthStart,
        [SM_AUTH_PASSCODE] = SmPassCodeAuthStart,
        [SM_AUTH_PASSWORD_ENTRY] = SmPassWordAuthStart,
        [SM_AUTH_OUT_OF_BAND] = SmOobAuthStart,
        [SM_AUTH_PSK] = SmPskAuthStart,
    };
    if (authMethod >= SM_AUTH_METHOD_MAX) {
        return NULL;
    }
    return smAuthStartFunc[authMethod];
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/
/* Only for number compare, no entry and out of band */
bool SmGenConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink, SmConfirmNum_S *confirm)
{
    NLSTK_CHECK_RETURN(keyLen == SM_RANDOM_NUMBER_R_LEN, false, "[SM] Generate confirm num key len error.");
    uint8_t *buff = SDF_MemZalloc(sizeof(uint8_t) * (SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN)); // 运算结束后释放
    NLSTK_CHECK_RETURN(buff != NULL, false, "[SM] Generate confirm num malloc fail.");
    if ((memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK)) {
        SDF_MemFree(buff);
        return false;
    }
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
        .buff = buff,
        .buffSize = SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN,
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, key, keyLen);
    if (!SmCmacGenerate(&input, confirm->confirm, SM_CONFIRM_NUMBER_LEN)) {
        NLSTK_LOG_ERROR("[SM] Generate confirm code error.");
        SDF_MemFree(buff);
        return false;
    }
    SDF_MemFree(buff);
    return true;
}