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
#include "SleSecurity.h"

#include <iostream>

#include <charconv>
#include "SleAdapter.h"
#include "SleConfig.h"
#include "SleProperties.h"
#include "SleUtils.h"
#include "sle_crypto.h"
#include "Compat.h"
#include "log.h"
#include "securec.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_ue.h"
#include "SleInterfaceProfile.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileCdsm.h"
#include "nlstk_sm_api.h"

namespace OHOS {
namespace Nearlink {
enum AuthTypeToNapi {
    AUTH_NO_ENTRY = 0,
    AUTH_PASSCODE,
    AUTH_NUMBER_COMPARE,
    AUTH_PASSWORD_ENTRY,
    AUTH_OUT_OF_BAND,
    AUTH_PSK,
    AUTH_MAX,
};

static SleSecurity *g_sleSecurityImpl = nullptr;
struct SleSecurity::impl {
public:
    uint8_t pairMethod_ = 0;
};
SleSecurity::SleSecurity(
    SleInterfaceAdapterSub &sleAdapter)
    : sleAdapter_(&sleAdapter),
      pimpl(std::make_unique<SleSecurity::impl>())
{
    g_sleSecurityImpl = this;
    int ret = RegisterCallbackToSm();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleSecurity]");
    }
    ret = RegisterCryptoFunctionsToSm();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleSecurity]");
    }
}

SleSecurity::~SleSecurity()
{
    int ret = DeregisterCallbackToGap();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleSecurity]");
    }
}

bool SleSecurity::PairStartStatusChange(const NLSTK_SmPairingStart_S &param) const
{
    if (sleAdapter_ != nullptr) {
        RawAddress addr = RawAddress::ConvertToString(param.addr.addr);
        (static_cast<SleAdapter *>(sleAdapter_))->PairStartChanged(addr);
    }
    return true;
}

void SleSecurity::PairStartChanged(NLSTK_SmPairingStart_S *param)
{
    NL_CHECK_RETURN(param, "[SleSecurity] param is null");
    LOG_INFO("[SleSecurity] bondStatus=%{public}hhu", param->bondStatus);
    NL_CHECK_RETURN(g_sleSecurityImpl, "[SleSecurity] sleSecurity is null");

    NLSTK_SmPairingStart_S pairStartParam;
    (void)memset_s(&pairStartParam, sizeof(pairStartParam), 0x00, sizeof(NLSTK_SmPairingStart_S));
    (void)memcpy_s(&pairStartParam, sizeof(pairStartParam), param, sizeof(NLSTK_SmPairingStart_S));
    DoInAdapterThread([sleSecurityImplPtr = g_sleSecurityImpl, pairStartParam]()-> void {
        sleSecurityImplPtr->PairStartStatusChange(pairStartParam);
    });
}

void SleSecurity::AuthComplete(NLSTK_SmAuthComplete_S *param)
{
    NL_CHECK_RETURN(param, "[SleSecurity] param is null");
    LOG_INFO("[SleSecurity] auth_status=%{public}hhu", param->authStatus);
    NL_CHECK_RETURN(g_sleSecurityImpl, "sleSecurity is null");

    NLSTK_SmAuthComplete_S smCallbackParam;
    (void)memset_s(&smCallbackParam, sizeof(smCallbackParam), 0x00, sizeof(NLSTK_SmAuthComplete_S));
    (void)memcpy_s(&smCallbackParam, sizeof(smCallbackParam), param, sizeof(NLSTK_SmAuthComplete_S));
    DoInAdapterThread([sleSecurityImplPtr = g_sleSecurityImpl, smCallbackParam]() -> void {
        sleSecurityImplPtr->SmpAuthComplete(smCallbackParam);
    });
    (void)memset_s(&smCallbackParam, sizeof(smCallbackParam), 0x00, sizeof(NLSTK_SmAuthComplete_S));
}

bool SleSecurity::SmpAuthComplete(const NLSTK_SmAuthComplete_S &param) const
{
    LOG_DEBUG("[SleSecurity]");

    RawAddress addr = RawAddress::ConvertToString(param.addr.addr);

    if (param.authStatus == SM_ERR_OK) {
        DftCacheConnInfoTime(addr.GetAddress(), AUTH_COMP_TIME);
        SaveSlePairKey(addr, param);
        SleConfig::GetInstance().Save();
    } else if (param.authStatus == SM_ERR_ACTIVE_CANCEL) {
        HILOGI("[SmpAuthComplete]: SM_ERR_ACTIVE_CANCEL.");
        uint8_t addrType = param.addr.type;
        CdsmService *cdsmService = CdsmService::GetService();
        if (cdsmService != nullptr) {
            std::vector<NearlinkCdsmInfo> cdsmInfo = {};
            if (cdsmService->CdsmGetAllMemberInfo(addr, cdsmInfo) != NL_NO_ERROR) {
                HILOGE("get cdsm info by member addr failed!");
                return false;
            }
            for (const auto &member : cdsmInfo) {
                CancelPairing(member.addr_, addrType);
            }
        }
        HILOGE("[SleSecurity]:get cdsm service error!");
        return false;
    } else {
        if (sleAdapter_ != nullptr) {
            (static_cast<SleAdapter *>(sleAdapter_))->PairComplete(addr, param.authStatus);
        }
    }

    LOG_DEBUG("[SleSecurity] Sle pair comelete event result = %{public}u", param.authStatus);
    return true;
}

/* 共用link_key：构造合作集其他地址配对记录 */
void SleSecurity::SaveCdsmOtherAddressKey(const RawAddress &addr,
    EncryptedLinkKey &sharedKeys, EncryptedLinkKey &sharedGroupkeys, const NLSTK_SmAuthComplete_S &param) const
{
    if (SleRemoteDeviceAdapter::GetInstance()->GetManufacturerBusinessType(addr) != SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        HILOGI("[cdsm pair]:get business type not expected.");
        return;
    }

    /* 获取集合地址列表 */
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    NL_CHECK_RETURN(cdsmService->CdsmCheckIsCooperationDevice(addr),
                    "[cdsm pair]:paired addr:%{public}s not cdsm member.", GET_ENCRYPT_ADDR(addr));
    RawAddress otherAddr = {};
    NL_CHECK_RETURN(cdsmService->CdsmGetOtherAddr(addr, otherAddr),
                    "[cdsm pair]:get other addr failed,addr:%{public}s", GET_ENCRYPT_ADDR(addr));
    /* 协议栈添加配对记录 */
    NLSTK_SmRecoverKeyParam_S pairedDevice;
    (void)memset_s(&pairedDevice, sizeof(pairedDevice), 0x00, sizeof(pairedDevice));
    pairedDevice.addr.type = 0;
    otherAddr.ConvertToUint8(pairedDevice.addr.addr, SLE_ADDR_LEN);
    NL_CHECK_RETURN(memcpy_s(&(pairedDevice.linkKey), SM_LINK_KEY_LEN, param.linkKey, OCTET16_LEN) == EOK,
                    "[cdsm pair]:add other addr:%{public}s pair record fail,memcpy fail.", GET_ENCRYPT_ADDR(otherAddr));
    pairedDevice.cryptoAlgo  = param.cryptoAlgo;
    pairedDevice.keyDerivAlgo = param.keyDerivAlgo;
    pairedDevice.intgChkInd = param.intgChkInd;
    NLSTK_ERRCODE ret = NLSTK_SmRecoverKey(&pairedDevice, 1); // 此处只恢复一个设备的配对记录
    (void)memset_s(&pairedDevice, sizeof(pairedDevice), 0x00, sizeof(pairedDevice));
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "[cdsm pair]:add other addr:%{public}s pair record fail:%{public}u",
                    GET_ENCRYPT_ADDR(otherAddr), ret);

    /* link_key 存储到xml */
    std::string sharedGroupkeysStr = SleUtils::ConvertIntToHexString(sharedGroupkeys.data(), sharedGroupkeys.size());
    bool cfgRet = SleConfig::GetInstance().SetLinkKey(otherAddr.GetAddress(),
        SleUtils::ConvertIntToHexString(sharedKeys.data(), sharedKeys.size()));
    cfgRet &= SleConfig::GetInstance().SetGroupkey(otherAddr.GetAddress(), sharedGroupkeysStr);
    cfgRet &= SleConfig::GetInstance().SetCryptoAlgo(otherAddr.GetAddress(), param.cryptoAlgo);
    cfgRet &= SleConfig::GetInstance().SetKeyDerivAlgo(otherAddr.GetAddress(), param.keyDerivAlgo);
    cfgRet &= SleConfig::GetInstance().SetIntegrChk(otherAddr.GetAddress(), param.intgChkInd);
    cfgRet &= SleConfig::GetInstance().SetGiv(otherAddr.GetAddress(), param.giv);
    NL_CHECK_RETURN(cfgRet, "[cdsm pair]:save other addr:%{public}s pair xml fail", GET_ENCRYPT_ADDR(otherAddr));

    /* 私有设备信息存储 */
    RawAddress reportAddr;
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService->CdsmGetReportAddr(addr, reportAddr) &&
        cdsmService->CdsmGetAllMemberInfo(addr, cdsmInfo) == NL_NO_ERROR) {
        std::vector<std::string> memberList = {};
        for (auto &member : cdsmInfo) {
            memberList.push_back(member.addr_.GetAddress());
        }
        SleConfig::GetInstance().SetCdsmMemberList(reportAddr.GetAddress(), memberList);
    }
    SleConfig::GetInstance().Save();

    HILOGI("[cdsm pair]:add other addr:%{public}s pair record ok", GET_ENCRYPT_ADDR(otherAddr));
}

bool SleSecurity::SaveSlePairKey(const RawAddress &addr, const NLSTK_SmAuthComplete_S &param) const
{
    LOG_DEBUG("[SleSecurity]");

    bool ret = true;
    LinkKey sleLinkkey;
    for (int i = 0; i < SM_LINK_KEY_LEN; i++) {
        sleLinkkey[i] = param.linkKey[i];
    }
    EncryptedLinkKey sleEncryptedLinkkey;
    if (SleHksTool::GetInstance().SleLinkKeyEncrypt(sleLinkkey, sleEncryptedLinkkey) == HKS_SUCCESS) {
        ret &= SleConfig::GetInstance().SetLinkKey(addr.GetAddress(),
            SleUtils::ConvertIntToHexString(sleEncryptedLinkkey.data(), sleEncryptedLinkkey.size()));
        HILOGI("SaveSlePairKey size(%{public}lu)", sleEncryptedLinkkey.size());
    } else {
        HILOGE("SleLinkKeyEncrypt FAIL!");
    }
    (void)memset_s(&sleLinkkey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
    LinkKey sleGroupkey;
    for (int i = 0; i < SM_GROUP_KEY_LEN; i++) {
        sleGroupkey[i] = param.groupKey[i];
    }
    EncryptedLinkKey sleEncryptedGroupkey;
    std::string sleEncryptedGroupkeyStr = "";
    if (SleHksTool::GetInstance().SleLinkKeyEncrypt(sleGroupkey, sleEncryptedGroupkey) == HKS_SUCCESS) {
        sleEncryptedGroupkeyStr =
            SleUtils::ConvertIntToHexString(sleEncryptedGroupkey.data(), sleEncryptedGroupkey.size());
        ret &= SleConfig::GetInstance().SetGroupkey(addr.GetAddress(), sleEncryptedGroupkeyStr);
        HILOGI("Save Groupkey size(%{public}lu)", sleEncryptedGroupkey.size());
    } else {
        HILOGE("SleGroupkeyEncrypt FAIL!");
    }
    (void)memset_s(&sleGroupkey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
    ret &= SleConfig::GetInstance().SetCryptoAlgo(addr.GetAddress(), param.cryptoAlgo);
    ret &= SleConfig::GetInstance().SetKeyDerivAlgo(addr.GetAddress(), param.keyDerivAlgo);
    ret &= SleConfig::GetInstance().SetIntegrChk(addr.GetAddress(), param.intgChkInd);
    ret &= SleConfig::GetInstance().SetGiv(addr.GetAddress(), param.giv);
    ret &= SleRemoteDeviceAdapter::GetInstance()->SetPairAlgoInfo(addr, param.cryptoAlgo, param.keyDerivAlgo,
        param.intgChkInd);
    ret &= SleRemoteDeviceAdapter::GetInstance()->SetGroupAndGiv(addr, sleEncryptedGroupkeyStr, param.giv);
    NL_CHECK_RETURN_RET(ret, false, "save addr:%{public}s pair xml fail", GET_ENCRYPT_ADDR(addr));
    SaveCdsmOtherAddressKey(addr, sleEncryptedLinkkey, sleEncryptedGroupkey, param);

    return ret;
}

void SleSecurity::EncryptionComplete(NLSTK_SmEncComplete_S *param)
{
    NL_CHECK_RETURN(param, "[SleSecurity] param is null");
    HILOG_COMM_INFO("[%{public}s:%{public}d][SleSecurity] encStatus=%{public}hhu",
        __FUNCTION__, __LINE__, param->encStatus);
    NL_CHECK_RETURN(g_sleSecurityImpl, "[SleSecurity] sleSecurity is null");

    NLSTK_SmEncComplete_S encryptParam;
    (void)memset_s(&encryptParam, sizeof(encryptParam), 0x00, sizeof(NLSTK_SmEncComplete_S));
    (void)memcpy_s(&encryptParam, sizeof(encryptParam), param, sizeof(NLSTK_SmEncComplete_S));
    DoInAdapterThread([sleSecurityImplPtr = g_sleSecurityImpl, encryptParam]() -> void {
        sleSecurityImplPtr->SmpEncComplete(encryptParam);
    });
}

bool SleSecurity::SmpEncComplete(const NLSTK_SmEncComplete_S &param) const
{
    RawAddress addr = RawAddress::ConvertToString(param.addr.addr);
    if (sleAdapter_ != nullptr) {
        DftCacheConnInfoTime(addr.GetAddress(), ENCRY_COMP_TIME);
        (static_cast<SleAdapter *>(sleAdapter_))->EncryptionComplete(addr, param.encStatus);
        (static_cast<SleAdapter *>(sleAdapter_))->PairComplete(addr, param.encStatus);
    }
    return true;
}

void SleSecurity::PairCancelComplete(NLSTK_SmPairingRemove_S *param)
{
    NL_CHECK_RETURN(param, "[SleSecurity] param is null");
    LOG_INFO("[SleSecurity] removeStatus=%{public}hhu", param->removeStatus);
    NL_CHECK_RETURN(g_sleSecurityImpl, "[SleSecurity] sleSecurity is null");

    NLSTK_SmPairingRemove_S cancelPairParam;
    (void)memset_s(&cancelPairParam, sizeof(cancelPairParam), 0x00, sizeof(NLSTK_SmPairingRemove_S));
    (void)memcpy_s(&cancelPairParam, sizeof(cancelPairParam), param, sizeof(NLSTK_SmPairingRemove_S));
    DoInAdapterThread([sleSecurityImplPtr = g_sleSecurityImpl, cancelPairParam]() -> void {
        sleSecurityImplPtr->SmpPairCancelComplete(cancelPairParam);
    });
}

bool SleSecurity::SmpPairCancelComplete(const NLSTK_SmPairingRemove_S &param) const
{
    RawAddress addr = RawAddress::ConvertToString(param.addr.addr);
    if (sleAdapter_ != nullptr) {
        (static_cast<SleAdapter *>(sleAdapter_))->CancelPairComplete(
            addr, param.removeStatus, static_cast<uint8_t>(PairingStateChangeReason::PAIRING_AUTH_FAILED));
    }
    LOG_DEBUG("[SleSecurity] Sle SmpPairCancelComplete success!");
    return true;
}

void SleSecurity::PairingReq(NLSTK_SmPairingRequest_S *param)
{
    NL_CHECK_RETURN(param != nullptr, "[SleSecurity] param is null");
    HILOGI("[SleSecurity] requestType=%{public}d", param->requestType);
    NL_CHECK_RETURN(g_sleSecurityImpl != nullptr, "[SleSecurity] sleSecurity is null");
    NLSTK_SmPairingRequest_S pairingReqParam;
    (void)memset_s(&pairingReqParam, sizeof(pairingReqParam), 0x00, sizeof(NLSTK_SmPairingRequest_S));
    (void)memcpy_s(&pairingReqParam, sizeof(pairingReqParam), param, sizeof(NLSTK_SmPairingRequest_S));
    DoInAdapterThread([sleSecurityImplPtr = g_sleSecurityImpl, pairingReqParam]() -> void {
        sleSecurityImplPtr->SmpPairingReq(pairingReqParam);
    });
}

bool SleSecurity::SmpPairingReq(const NLSTK_SmPairingRequest_S &param) const
{
    RawAddress addr = RawAddress::ConvertToString(param.addr.addr);
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, addr, PAIRINGREQ, PAIRCFMINVALID, "");
    if (sleAdapter_ != nullptr) {
        std::string passKey(param.sixDigits);
        uint8_t requestType = ConvertRequestType(param.requestType);
        (static_cast<SleAdapter *>(sleAdapter_))->PairingRequest(addr, passKey, requestType);
    }
    HILOGD("[SleSecurity] Sle SmpPairingReq success!");
    return true;
}

uint8_t SleSecurity::ConvertRequestType(uint8_t type) const
{
    HILOGD("the auth type input is %{public}d", type);
    if (type > AUTH_NUMBER_COMPARE) { // No other auth method is defined for Napi.
        return AUTH_NO_ENTRY;
    }
    uint8_t authTypeMap[AUTH_MAX] = {
        AUTH_NUMBER_COMPARE, AUTH_NO_ENTRY, AUTH_PASSCODE, AUTH_PASSWORD_ENTRY, AUTH_OUT_OF_BAND, AUTH_PSK
    };
    return authTypeMap[type];
}

int SleSecurity::RegisterCallbackToSm()
{
    HILOGD("[SleSecurity]");
    NLSTK_SmCallbacks_S smCallbacks {};
    smCallbacks.startCbk = &SleSecurity::PairStartChanged;
    smCallbacks.authCbk = &SleSecurity::AuthComplete;
    smCallbacks.encCbk = &SleSecurity::EncryptionComplete;
    smCallbacks.removeCbk = &SleSecurity::PairCancelComplete;
    smCallbacks.requestCbk = &SleSecurity::PairingReq;
    return NLSTK_SmRegExternalCbks(&smCallbacks);
}

int SleSecurity::RegisterCryptoFunctionsToSm()
{
    HILOGD("[SleSecurity]");
    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        .sha256HashFunc = Crypto_Sha256,
    };
    return NLSTK_SmRegAlgoFuncs(&algoFuncs);
}

int SleSecurity::DeregisterCallbackToGap() const
{
    LOG_DEBUG("[SleSecurity]");
    return NLSTK_ERRCODE_SUCCESS;
}

int SleSecurity::GapSleRequestSecurity(uint16_t connectionHandle, const SLE_Addr_S &addr, uint8_t role)
{
    LOG_DEBUG("[SleSecurity]");
    SLE_Addr_S paramAdd = {};
    (void)memcpy_s(&paramAdd, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    return NLSTK_SmStartPairing(&paramAdd);
}

bool SleSecurity::PairRequestReply(const RawAddress &addr, uint8_t addrType, bool accept) const
{
    LOG_DEBUG("[SleSecurity]");

    if (accept) {
        LOG_DEBUG("[SleSecurity] accept pair");
        return false;
    } else {
        return CancelPairing(addr, addrType);
    }
}

bool SleSecurity::StartPair(const RawAddress &device, uint8_t peerAddrType)
{
    DftCachePairConnPath(device.GetAddress(), PAIR_CONN_PATH_AUTH);
    LOG_INFO("[SleSecurity] addr: %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    SLE_Addr_S stackAddr = {peerAddrType, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    device.ConvertToUint8(stackAddr.addr, SLE_ADDR_LEN);
    int ret = NLSTK_SmStartPairing(&stackAddr);
    return (ret == NLSTK_ERRCODE_SUCCESS);
}

bool SleSecurity::CancelPairing(const RawAddress &device, uint8_t peerAddrType) const
{
    HILOGI("[SleSecurity] addr: %{public}s", GET_ENCRYPT_ADDR(device));
    SLE_Addr_S stackAddr = {peerAddrType, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    device.ConvertToUint8(stackAddr.addr, SLE_ADDR_LEN);
    int ret = NLSTK_SmRemovePairing(&stackAddr);
    return (ret == NLSTK_ERRCODE_SUCCESS);
}

bool SleSecurity::SetPairingPassCode(const RawAddress &device, const std::string &passCode, uint8_t peerAddrType)
{
    HILOGI("[SleSecurity] addr: %{public}s", GET_ENCRYPT_ADDR(device));
    uint32_t passCodeNum = 0;
    auto [ptr, ec] = std::from_chars(passCode.data(), passCode.data() + passCode.size(), passCodeNum);
    if (!(ec == std::errc{} && ptr == passCode.data() + passCode.size())) {
        HILOGE("[SleSecurity]: passcode Converting failed");
        return false;
    }
    SLE_Addr_S stackAddr = {peerAddrType, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    device.ConvertToUint8(stackAddr.addr, SLE_ADDR_LEN);
    NLSTK_SmPassCode_S passCodeIn = {stackAddr, passCodeNum};
    int ret = NLSTK_SmSetPassCode(&passCodeIn);
    return (ret == NLSTK_ERRCODE_SUCCESS);
}

bool SleSecurity::SetPairingConfirmation(const RawAddress &device, uint8_t peerAddrType)
{
    HILOGI("[SleSecurity] addr: %{public}s", GET_ENCRYPT_ADDR(device));
    SLE_Addr_S stackAddr = {peerAddrType, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    device.ConvertToUint8(stackAddr.addr, SLE_ADDR_LEN);
    int ret = NLSTK_SmSetConfirm(&stackAddr);
    return (ret == NLSTK_ERRCODE_SUCCESS);
}

bool SleSecurity::SendImgSecuConfig(const RawAddress &device, uint32_t groupId)
{
    SLE_Addr_S stackAddr = {};
    device.ConvertToUint8(stackAddr.addr);

    NLSTK_SmImgSecuConfig_S config;
    std::string encryptGroupKeyStr = "";
    uint64_t giv = 0;
    bool ret = GetCdsmImgSecuConfig(device, encryptGroupKeyStr, giv, config);
    NL_CHECK_RETURN_RET(ret, false, "GetGroupAndGiv or GetPairAlgoInfo fail");

    LinkKey sleGroupkey;
    EncryptedLinkKey sleEncryptedGroupkey;
    ret = SleUtils::ConvertHexStringToInt(encryptGroupKeyStr, sleEncryptedGroupkey.data(), sleEncryptedGroupkey.size());
    NL_CHECK_RETURN_RET(ret, false, "ConvertHexStringToInt fail");
    int32_t hksret = SleHksTool::GetInstance().SleLinkKeyDecrypt(sleEncryptedGroupkey, sleGroupkey);
    NL_CHECK_RETURN_RET(hksret == HKS_SUCCESS, false, "SleLinkKeyDecrypt fail");

    config.addr = stackAddr;
    config.imgId = groupId;
    config.giv = giv;
    if (memcpy_s(&(config.groupKey), SM_LINK_KEY_LEN, &sleGroupkey[0], OCTET16_LEN) != EOK) {
        LOG_ERROR("memcpy_s failed!");
        return false;
    }
    int sendRet = NLSTK_SmSendImgSecuConfig(&config);
    (void)memset_s(&sleGroupkey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
    return (sendRet == NLSTK_ERRCODE_SUCCESS);
}

bool SleSecurity::GetCdsmImgSecuConfig(const RawAddress &device, std::string &encryptGroupKeyStr, uint64_t &giv,
    NLSTK_SmImgSecuConfig_S &config)
{
    uint8_t cryptoAlgo = 0;
    uint8_t keyDerivAlgo = 0;
    uint8_t integrChkInd = 0;

    /* 获取集合地址列表 */
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN_RET(cdsmService, false, "cdsmService is null");

    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    // 1. 先从reportAddr获取groupKey
    bool ret = SleRemoteDeviceAdapter::GetInstance()->GetGroupAndGiv(reportAddr, encryptGroupKeyStr, giv);
    ret = ret && SleRemoteDeviceAdapter::GetInstance()->GetPairAlgoInfo(reportAddr, cryptoAlgo,
        keyDerivAlgo, integrChkInd);
    // 2. reportAddr获取失败，从otherAddr获取groupKey
    if (!ret || encryptGroupKeyStr.empty() || giv == 0) {
        RawAddress otherAddr(device);
        cdsmService->CdsmGetOtherAddr(reportAddr, otherAddr);
        ret = SleRemoteDeviceAdapter::GetInstance()->GetGroupAndGiv(otherAddr, encryptGroupKeyStr, giv);
        ret = ret && SleRemoteDeviceAdapter::GetInstance()->GetPairAlgoInfo(otherAddr, cryptoAlgo,
            keyDerivAlgo, integrChkInd);
    }
    NL_CHECK_RETURN_RET(ret && !encryptGroupKeyStr.empty() && giv != 0, false, "encryptGroupKeyStr is empty or giv=0");
    config.cryptoAlgo = cryptoAlgo;
    config.keyDerivAlgo = keyDerivAlgo;
    config.intgChkInd = integrChkInd;
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS