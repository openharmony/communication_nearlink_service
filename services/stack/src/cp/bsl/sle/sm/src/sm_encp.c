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
#include "nlstk_log.h"
#include "cm_logic_link_api.h"
#include "dli_cmd.h"
#include "dli_errno.h"

#include "sm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "sm_dft.h"
#include "sm_encp.h"

// start encrypt process
void SmEncpEnable(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to security control process.");
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_ENCP_EXCEP, SM_DFT_ENCP_ENABLE_TIME);
    NLSTK_CHECK_RETURN_VOID(slink->lcid != CM_INVALID_LCID, "[SM] Lcid invalid on security control process.");
    DLI_EnableEncryptParam params = {0};
    params.connHandle = slink->lcid;
    if (memcpy_s(params.linkKey, SLE_SM_LINK_KEY_LEN, slink->linkKey, SM_LINK_KEY_LEN) != EOK) {
        NLSTK_LOG_ERROR("[SM] Enable encryption params link key memcpy failed.");
        return;
    }
    /*
     * 若加密算法和完整性算法都支持AC1或AC2，则HCI命令中加密算法可使用AC1或AC2
     * 若加密算法支持EA1且完整性算法支持IA1，则HCI命令中加密算法可使用EA1/IA1
     * 若加密算法支持EA2且完整性算法支持IA2，则HCI命令中加密算法可使用EA2/IA2
     */
    uint8_t *algoCap = slink->negoParams.codeAlgoCap;
    if (algoCap[SM_ENC_ALGO_ABILITY] == algoCap[SM_INTG_PROTECT_ALGO_ABILITY]) {
        params.cryptoAlgo = algoCap[SM_ENC_ALGO_ABILITY] - 1;
    } else {
        params.cryptoAlgo = 0xFF;
    }
    params.keyDerivAlgo = algoCap[SM_KEY_DERIV_ALGO_ABILITY] - 1;
    params.integrChkInd = slink->encIntgCheck;
    NLSTK_LOG_INFO("[SM] Enable encryption, lcid: %u, crypt: %u, keyDeriv: %u, integrChk: %u.",
                 params.connHandle, params.cryptoAlgo, params.keyDerivAlgo, params.integrChkInd);
    uint32_t ret = DLI_EnableEncryption(&params);
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send enable encryption command failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }
    (void)memset_s(&params, sizeof(DLI_EnableEncryptParam), 0x00, sizeof(DLI_EnableEncryptParam));
}

void SmEncpParamReqReplyProcess(SmSLink_S *slink)
{
    NLSTK_LOG_INFO("[SM] Start to encryption parameter request reply process.");
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_ENCP_EXCEP, SM_DFT_ENCP_REQ_PARAM_TIME);
    uint32_t ret;
    if (slink->role != SM_T_NODE) {
        DLI_ConnHandleStru params = {0};
        params.connHandle = slink->lcid;
        ret = DLI_EncryptionParamReqNegativeReply(&params);
        if (ret != DLI_SUCCESS) {
            NLSTK_LOG_ERROR("[SM] Send encryption negative reply command failure.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        }
        return;
    }
    DLI_EncryptReqReplyParam params = {0};
    params.connHandle = slink->lcid;
    if (memcpy_s(params.linkKey, SLE_SM_LINK_KEY_LEN, slink->linkKey, SM_LINK_KEY_LEN) != EOK) {
        NLSTK_LOG_ERROR("[SM] Encryption parameter request reply link key memcpy failed.");
        return;
    }
    /*
     * 若加密算法和完整性算法都支持AC1或AC2，则HCI命令中加密算法可使用AC1或AC2
     * 若加密算法支持EA1且完整性算法支持IA1，则HCI命令中加密算法可使用EA1/IA1
     * 若加密算法支持EA2且完整性算法支持IA2，则HCI命令中加密算法可使用EA2/IA2
     */
    params.cryptoAlgo = slink->negoParams.codeAlgoCap[SM_ENC_ALGO_ABILITY] - 1;
    params.keyDerivAlgo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] - 1;
    ret = DLI_EncryptionParamReqReply(&params);
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send encryption reply command failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }
    (void)memset_s(&params, sizeof(DLI_EncryptReqReplyParam), 0x00, sizeof(DLI_EncryptReqReplyParam));
}