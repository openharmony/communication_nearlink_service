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
#include "VasService.h"
#include <cstring>

#include "ASCService.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "want.h"
#include "ability_manager_ipc_interface_code.h"
#include "ClassCreator.h"
#include "nearlink_dft_exception.h"
#include "nearlink_errorcode.h"
#include "SleInterfaceProfileManager.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "VasStackAdapter.h"
#include "CdsmService.h"
#include "ServiceManagerPluginLoader.h"
#include "nearlink_dft_ue.h"

namespace OHOS {
namespace Nearlink {
namespace {
    // Wake up voice recognition param
    const std::string VOICE_RECOGNITION_ABILITY_NAME = "WakeUpExtAbility";
    const std::string VOICE_RECOGNITION_LANUCH_TYPE = "launch_type";
    const std::string VOICE_RECOGNITION_NEARLINK = "nearlink_key";

    const std::u16string ABILITY_MGR_DESCRIPTOR = u"ohos.aafwk.AbilityManager";
    constexpr int DEFAULT_INVALID_VALUE  = -1;
}

struct VasService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    VasStackAdapter stackAdapter_;
};

VasService::VasService() : utility::Context(PROFILE_NAME_VAS, "1.0.0"), pimpl(std::make_unique<impl>())
{
    HILOGI("[VasService]%{public}s Create", PROFILE_NAME_VAS.c_str());
}

VasService::~VasService()
{
    HILOGI("[VasService]%{public}s Destroy", PROFILE_NAME_VAS.c_str());
}

utility::Context *VasService::GetContext()
{
    return this;
}

VasService *VasService::GetService()
{
    return static_cast<VasService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VAS));
}

void VasService::Enable()
{
    DoInVasThread([this]() {
        HILOGI("[VasService] start enable vas service");
        if (pimpl->isEnabled_) {
            HILOGW("[VasService]has already been started before.");
            return;
        }
        // 调用协议栈进行初始化
        pimpl->stackAdapter_.CreateVasInstance();

        GetContext()->OnEnable(PROFILE_NAME_VAS, true);
        pimpl->isEnabled_ = true;
        HILOGI("[VasService]VasService enabled");
    });
}

void VasService::Disable()
{
    DoInVasThread([this]() {
        HILOGI("[VasService] start disable ccp service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_VAS, true);
            HILOGW("[VasService]has already been shutdown before.");
            return;
        }
        // 调用协议栈去初始化
        pimpl->stackAdapter_.DeleteVasInstance();

        GetContext()->OnDisable(PROFILE_NAME_VAS, true);
        pimpl->isEnabled_ = false;
        HILOGI("[VasService]VasService disabled");
    });
}

void VasService::OpenVoiceAssistant(const RawAddress &device)
{
    HILOGI("[VasService]open voice assistant device=%{public}s", GET_ENCRYPT_ADDR(device));
    DoInVasThread([this] {
        pimpl->stackAdapter_.SetVoiceAssistantOpened();
    });
}

void VasService::CloseVoiceAssistant(const RawAddress &device)
{
    HILOGI("[VasService]close voice assistant device=%{public}s", GET_ENCRYPT_ADDR(device));
    DoInVasThread([this] {
        pimpl->stackAdapter_.SetVoiceAssistantClosed();
    });
}

void VasService::DftVoiceAssistantSubScene(const RawAddress &device, int subSceneCode, int32_t controlResult)
{
    RawAddress reportAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(device, reportAddr);
    }
    NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, VOICE_ASSISTANT_SCENE_SCENE,
        subSceneCode, UPDATE_VOICE_STACK_REASON_INVALID, static_cast<NlErrCode>(controlResult));
}

void VasService::HandleActivateVoiceAssistant(const RawAddress &device, uint32_t requestId)
{
    HILOGI("[VasService]try wake up voice assistant device=%{public}s, requestId=%{public}d",
        GET_ENCRYPT_ADDR(device), requestId);
    DoInVasThread([this, requestId, device] {
        int32_t ret = WakeUpVoiceRecognition();
        DftVoiceAssistantSubScene(device, VOICE_ASSISTANT_OPEN, ret);
        if (ret != NL_NO_ERROR) {
            pimpl->stackAdapter_.OpenVoiceAssistantFail(requestId);
            return;
        }
        pimpl->stackAdapter_.OpenVoiceAssistantSuccess(requestId);
    });
}

void VasService::HandleCloseVoiceAssistant(const RawAddress &device, uint32_t requestId)
{
    HILOGI("[VasService]try close voice assistant device=%{public}s, requestId=%{public}d",
        GET_ENCRYPT_ADDR(device), requestId);
    DoInVasThread([this, device, requestId] {
        ASCService *ascService = ASCService::GetService();
        NL_CHECK_RETURN(ascService, "ascService is null.");
        ascService->AudioControl(device, AUDIO_STREAM_VOICE_ASSISTANT, NL_SLE_ASC_CONTROL_CMD_STOP);
        pimpl->stackAdapter_.CloseVoiceAssistantSuccess(requestId);
        DftVoiceAssistantSubScene(device, VOICE_ASSISTANT_CLOSE, NL_NO_ERROR);
    });
}

int32_t VasService::WakeUpVoiceRecognition()
{
    HILOGI("[VasService]Enter");
    AAFwk::Want want;
    std::string bundleName =
        ServiceManagerPluginInterface::GetInstance()->GetBundleName(BundleNameType::BUNDLE_NAME_AIBASE);
    want.SetElementName(bundleName, VOICE_RECOGNITION_ABILITY_NAME);
    want.SetParam(VOICE_RECOGNITION_LANUCH_TYPE, VOICE_RECOGNITION_NEARLINK);

    sptr<ISystemAbilityManager> systemAbilityManager =
         SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(systemAbilityManager != nullptr, NL_ERR_INTERNAL_ERROR,
        "[VasService]SystemAbilityManager is nullptr");

    sptr<IRemoteObject> remote = systemAbilityManager->GetSystemAbility(ABILITY_MGR_SERVICE_ID);
    NL_CHECK_RETURN_RET(remote != nullptr, NL_ERR_INTERNAL_ERROR, "[VasService]Remote is nullptr");

    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(ABILITY_MGR_DESCRIPTOR), NL_ERR_INTERNAL_ERROR,
        "[VasService]Write interface token error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&want), NL_ERR_INTERNAL_ERROR, "[VasService]Write Want error");
    NL_CHECK_RETURN_RET(data.WriteInt32(DEFAULT_INVALID_VALUE ), NL_ERR_INTERNAL_ERROR,
        "[VasService]Write UserId error");
    NL_CHECK_RETURN_RET(data.WriteInt32(DEFAULT_INVALID_VALUE ), NL_ERR_INTERNAL_ERROR,
        "[VasService]Write RequestCode error");

    MessageParcel reply;
    MessageOption option;
    uint32_t task =  static_cast<uint32_t>(AAFwk::AbilityManagerInterfaceCode::START_ABILITY);
    int ret = remote->SendRequest(task, data, reply, option);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "[VasService]Send request error");
    return reply.ReadInt32();
}

REGISTER_CLASS_CREATOR(VasService);

}  // namespace Nearlink
}  // namespace OHOS
