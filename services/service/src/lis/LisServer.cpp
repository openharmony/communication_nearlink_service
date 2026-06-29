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
#include "LisServer.h"
#include <string>
#include "SleServiceFfrtLog.h"
#include "SleUtils.h"
#include "SleInterfaceProfileManager.h"
#include "ssap_server_service.h"
#include "ssap_type.h"
#include "LisService.h"
#include "param_wrapper.h"
#include "ThreadUtil.h"
#include "SleFeature.h"

namespace OHOS {
namespace Nearlink {

LisServer &LisServer::GetInstance()
{
    static LisServer instance;
    return instance;
}

LisServer::LisServer()
{}

LisServer::~LisServer()
{}

InterfaceProfileSsapServer *LisServer::GetSsapServerService()
{
    return static_cast<InterfaceProfileSsapServer *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_SERVER));
}

class LisServer::LisSsapServerCallback : public InterfaceSsapServerCallback {
public:
    LisSsapServerCallback() {};
    ~LisSsapServerCallback() {};
    void OnMtuChanged(const RawAddress &addr, uint8_t transport, uint16_t mtu) override {}
    void OnAddService(Service &service, int ret) override
    {
        HILOGI("[LisServer] ret: %{public}d, service property num = %{public}d",
            ret, static_cast<int>(service.properties_.size()));
    }
    void OnSetPropertyValue(Property &property, int ret) override {}
    void OnSetDescriptorValue(Descriptor &descriptor, int ret) override {}
    void OnReadPropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) override {}
    void OnReadDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) override {}
    void OnWritePropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) override {}
    void OnWriteDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) override {}
    void OnReadProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnReadDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override {}
    void OnWriteProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnWriteDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override {}
    void OnNotifyProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnIndicateProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnNotifyEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) override {}
    void OnIndicateEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) override {}
    void OnCallMethod(const RawAddress &addr, uint8_t transport, Method &method,
                    std::vector<uint8_t> &value, bool needReturn) override {}
    void OnConnectionStateChanged(const RawAddress &addr, uint8_t transport, uint8_t state,
        int ret, int reason) override {}
};

bool LisServer::LoadDeviceTypeInd()
{
    int32_t res = OHOS::system::GetStringParameter("const.nearlink.lis.device_type_ind",
        deviceTypeInd_, LIS_DEVICE_TYPE_IND);
    HILOGI("[LisServer]:: deviceTypeInd = %{public}s, res = %{public}d", deviceTypeInd_.c_str(), res);
    return res == 0;
}

int LisServer::RegisterLisServerApplication()
{
    HILOGI("[LisServer] enter");
    DoInLisThread([this]() { this->RegisterLisServerApplicationInterface(); });
    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

void LisServer::RegisterLisServerApplicationInterface()
{
    if (!SleFeature::GetInstance().IsRangSupported()) {
        HILOGI("[LisServer] no need register, not support");
        return;
    }
    if (serviceCallback_ == nullptr) {
        serviceCallback_ = std::make_shared<LisSsapServerCallback>();
    }
    HILOGI("[LisServer] register previous appId_=%{public}d ", appId_);
    // -1 is default value
    if (appId_ >= 0) {
        HILOGI("[LisServer] have register");
        return;
    }
    InterfaceProfileSsapServer *ssapServerService_ = GetSsapServerService();
    NL_CHECK_RETURN(ssapServerService_, "ssapServerService_ is null.");
    if (ssapServerService_ != nullptr) {
        appId_ = ssapServerService_->RegisterApplication(serviceCallback_);
        HILOGI("[LisServer] register appId_=%{public}d finish", appId_);
        if (appId_ < 0) {
            HILOGE("[LisServer] Register err appId_");
            return;
        }
        LoadDeviceTypeInd();
        instance_ = BuildService();
        int result = ssapServerService_->AddService(appId_, *instance_);
        HILOGE("[LisServer] add service appId_=%{public}d, result=%{public}d", appId_, result);
    }
}

int LisServer::DeregisterLisServerApplication()
{
    HILOGI("[LisServer] enter");
    DoInLisThread([this]() { this->DeregisterLisServerApplicationInterface(); });
    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

void LisServer::DeregisterLisServerApplicationInterface()
{
    HILOGI("[LisServer] enter");
    // -1 is default value
    if (appId_ < 0) {
        HILOGI("[LisServer] have unregister");
        return;
    }
    InterfaceProfileSsapServer *ssapServerService_ = GetSsapServerService();
    NL_CHECK_RETURN(ssapServerService_, "ssapServerService_ is null.");
    if (ssapServerService_ != nullptr) {
        ssapServerService_->DeregisterApplication(appId_);
        HILOGI("[LisServer] deregister finish");
        appId_ = -1;
    }
}

std::unique_ptr<Service> LisServer::BuildService()
{
    std::unique_ptr<Service> svc =
        std::make_unique<Service>(0, 0, 0, Uuid::ConvertFrom16Bits(UUID_LOCAL_MANAGER_SERVICE));
    svc->isPrimary_ = true;
    std::string value = deviceTypeInd_;
    svc->properties_.push_back(
        Property(0,
            Uuid::ConvertFrom16Bits(LIS_UUID_SSAP_DEVICE_TYPE_IND),
            std::vector<uint8_t>(value.begin(), value.end()),
            SSAP_OP_IND_READ | SSAP_OP_IND_WRITE_CMD | SSAP_OP_IND_WRITE_REQ |
            SSAP_OP_IND_ADV | SSAP_OP_IND_NTF | SSAP_OP_IND_IND,
            static_cast<int>(SSAP_Permission_Indication_E::SSAP_PERMISSION_NO_NEED)));
    return svc;
}


} // namespace Sle
} // namespace OHOS