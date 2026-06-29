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
/**
 * @file SleInterfaceProfile.h
 *
 * @brief basic profile interface.
 *
 * @since 6
 */

#ifndef SLE_INTERFACE_PROFILE_H
#define SLE_INTERFACE_PROFILE_H

#include <list>

#include "nearlink_def.h"
#include "raw_address.h"
#include "context.h"

/**
 * @brief forward declaration for class Context in namespace utility
 */
namespace OHOS {
namespace Nearlink {
/**
 * @brief profile service name Define
 */
const std::string PROFILE_NAME_SSAP_CLIENT = "SsapClientService";
const std::string PROFILE_NAME_SSAP_SERVER = "SsapServerService";
const std::string PROFILE_NAME_MAP_MSE = "MapMseService";
const std::string PROFILE_NAME_HID_HOST = "HidHostService";
const std::string PROFILE_NAME_DIS = "DisService";
const std::string PROFILE_NAME_BAS = "BasService";
const std::string PROFILE_NAME_LIS = "LisService";
const std::string PROFILE_NAME_ICCE = "IcceService";
const std::string PROFILE_NAME_PORT = "PortService";
const std::string PROFILE_NAME_CDSM = "CdsmService";
const std::string PROFILE_NAME_MCP_SERVER = "McpServerService";
const std::string PROFILE_NAME_ASC = "ASCService";
const std::string PROFILE_NAME_CCP = "CcpService";
const std::string PROFILE_NAME_TWS = "TwsService";
const std::string PROFILE_NAME_VCP = "VcpService";
const std::string PROFILE_NAME_VAS = "VasService";
const std::string PROFILE_NAME_MIC = "MicService";

/**
 * @brief profile connect state define, using to GetConnectState()...
 */
const uint8_t PROFILE_STATE_CONNECTED = 0x08;
const uint8_t PROFILE_STATE_CONNECTING = 0x04;
const uint8_t PROFILE_STATE_DISCONNECTING = 0x02;
const uint8_t PROFILE_STATE_DISCONNECTED = 0x01;

/**
 * @brief Represents basic profile for each profile service, including the common functions.
 *
 * @since 6
 */
class SleInterfaceProfile {
public:
    /**
     * @brief A destructor used to delete the <b>SleInterfaceProfile</b> instance.
     *
     * @since 6
     */
    virtual ~SleInterfaceProfile() = default;

    /**
     * @brief Connect with device.
     *
     * @param device Remote device address.
     * @return Returns Result for connect operation.
     * @since 6
     */
    virtual int Connect(const RawAddress &device) = 0;

    /**
     * @brief Disconnect with device.
     *
     * @param device Remote device address.
     * @return Returns Result for disconnect operation.
     * @since 6
     */
    virtual int Disconnect(const RawAddress &device) = 0;

    /**
     * @brief Get connected devices.
     *
     * @return Returns List for connected devices.
     * @since 6
     */
    virtual std::list<RawAddress> GetConnectDevices() = 0;

    /**
     * @brief Get connect state.
     *
     * @return Returns connect state for profile service.
     * @since 6
     */
    virtual int GetConnectState() = 0;

    /**
     * @brief Get utility::Context pointer for each profile service.
     *
     * @return Returns the pointer for adapter.
     * @since 6
     */
    virtual utility::Context *GetContext() = 0;
};
}  // namespace sle
}  // namespace OHOS

#endif  // SLE_INTERFACE_PROFILE_H