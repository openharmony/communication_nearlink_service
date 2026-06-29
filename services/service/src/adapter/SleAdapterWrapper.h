/*
 * Copyright (C) 2026-2026 Huawei Device Co., Ltd.
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

#ifndef SLE_ADAPTER_WRAPPER_H
#define SLE_ADAPTER_WRAPPER_H

#include "IServiceManagerPlugin.h"

namespace OHOS {
namespace Nearlink {
/**
 *  @brief SLE Adpter implementation wrapper class
 */
class SleAdapterWrapper : public SleInterfaceAdapterWrapper {
public:

    SleAdapterWrapper();
    virtual ~SleAdapterWrapper() = default;

    std::vector<RawAddress> GetConnectedDevices() const override;
    uint16_t GetLcidByAddress(const RawAddress &device) override;
    void AddBgConnDevice(const std::string &address) override;
    bool IsAudioDevice(const std::string &address) override;
    bool GetCdsmOtherAddr(const RawAddress &member, RawAddress &other) override;
    void SendBgConnList(NearlinkSafeList<RawAddress> &bgList) override;
    void SendDirectConnList(NearlinkSafeList<RawAddress> &directList) override;
    std::string GetAliasName(const std::string &address) const override;
    bool WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) override;
    uint8_t GetPeerDeviceAddrType(const RawAddress& address) const override;
    void GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType) const override;
    int HidSendData(const HidReportInfo &reportInfo) const override;
    int GetDeviceAppearance(const RawAddress &device) const override;
    bool DisconnectAllProfile(const RawAddress &device) override;
};
} // namespace Nearlink
} // namespace OHOS
#endif  // SLE_ADAPTER_WRAPPER_H