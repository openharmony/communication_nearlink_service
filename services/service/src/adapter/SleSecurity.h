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
#ifndef SLE_SECURITY_H
#define SLE_SECURITY_H

#include <map>
#include "BaseDef.h"

#include "BaseObserverList.h"
#include "SleDefs.h"
#include "SleInterfaceAdapterSub.h"
#include "nlstk_sm_api.h"
#include "slem.h"
#include "SleHuksTool.h"
#include "sm_errcode.h"
/*
 * @brief The nearlink system.
 */
namespace OHOS {
namespace Nearlink {
/*
 * @brief SLE filter.
 */
class SleSecurity {
public:
    /**
     * @brief Constructor.
     */
    explicit SleSecurity(SleInterfaceAdapterSub &sleAdapter);

    /**
     * @brief Destructor.
     */
    virtual ~SleSecurity();

    static bool StartPair(const RawAddress &device, uint8_t peerAddrType =
        static_cast<int>(SLE_ADDR_TYPE::SLE_PUBLIC_ADDRESS_TYPE));
    int GapSleRequestSecurity(uint16_t connectionHandle, const SLE_Addr_S &addr, uint8_t role);
    bool CancelPairing(const RawAddress &device, uint8_t peerAddrType) const;
    bool SetPairingPassCode(const RawAddress &device, const std::string &passCode, uint8_t peerAddrType);
    bool SetPairingConfirmation(const RawAddress &device, uint8_t peerAddrType);
    bool PairRequestReply(const RawAddress &addr, uint8_t addrType, bool accept) const;

    /**
     * @brief Register avertising callback to sm
     *
     * @return @c status.
     */
    int RegisterCallbackToSm();

    /**
     * @brief Register cryptp functions to sm
     *
     * @return @c status.
     */
    int RegisterCryptoFunctionsToSm();

    /**
     * @brief Deregister avertising callback to gap
     *
     * @return @c status.
     */
    int DeregisterCallbackToGap() const;
    bool SendImgSecuConfig(const RawAddress &device, uint32_t groupId);
    bool GetCdsmImgSecuConfig(const RawAddress &device, std::string &encryptGroupKeyStr, uint64_t &giv,
        NLSTK_SmImgSecuConfig_S &config);

private:
    // smp callback

    static void PairStartChanged(NLSTK_SmPairingStart_S *param);
    static void AuthComplete(NLSTK_SmAuthComplete_S *param);
    bool SmpAuthComplete(const NLSTK_SmAuthComplete_S &param) const;
    bool SaveSlePairKey(const RawAddress &addr, const NLSTK_SmAuthComplete_S &param) const;
    void SaveCdsmOtherAddressKey(const RawAddress &addr, EncryptedLinkKey &sharedKeys,
        EncryptedLinkKey &sharedGroupkeys, const NLSTK_SmAuthComplete_S &param) const;
    static void EncryptionComplete(NLSTK_SmEncComplete_S *param);
    bool SmpEncComplete(const NLSTK_SmEncComplete_S &param) const;
    static void PairCancelComplete(NLSTK_SmPairingRemove_S *param);
    bool SmpPairCancelComplete(const NLSTK_SmPairingRemove_S &param) const;

    static void PairingReq(NLSTK_SmPairingRequest_S *param);
    uint8_t ConvertRequestType(uint8_t type) const;
    bool SmpPairingReq(const NLSTK_SmPairingRequest_S &param) const;
    bool PairStartStatusChange(const NLSTK_SmPairingStart_S &param) const;
    SleInterfaceAdapterSub *sleAdapter_ = nullptr;

    SLE_DISALLOW_COPY_AND_ASSIGN(SleSecurity);
    DECLARE_IMPL();
};
}  // namespace Sle
}  // namespace OHOS

#endif  // SLE_SECURITY_H