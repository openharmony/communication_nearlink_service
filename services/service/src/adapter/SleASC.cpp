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

#include "SleASC.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "log_util.h"
#include "actm_api.h"
#include "actm_l2hc.h"

namespace OHOS {
namespace Nearlink {
// 地址转换为stack接口类型
#define CONVERT_DEV_ADDR(device, addr) \
do { \
    (addr) = {0, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; \
    (device).ConvertToUint8((addr).addr); \
} while (0)

int SleASC::GetAudioProperty(const RawAddress& device)
{
    HILOGD("[SleASC]GetAudioProperty in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmReadRemoteProp(&addr);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]GetAudioProperty ret %{public}d", ret);
    }

    return ret;
}

int SleASC::ConfigStream(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara)
{
    HILOGI("[SleASC]ConfigStream in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmConfigAudioStream(&addr, &cfgPara);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]ConfigStream ret %{public}d", ret);
    }
    return ret;
}

int SleASC::ConfigStreamCustomize(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara)
{
    HILOGI("[SleASC]ConfigStream in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmStartAudioStream(&addr, &cfgPara);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]ConfigStream ret %{public}d", ret);
    }
    return ret;
}

int SleASC::OpenStream(const RawAddress &device, uint8_t streamId)
{
    HILOGI("[SleASC]OpenStream in %{public}s streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamId);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);

    NLSTK_ActmOpenParam_S param {};
    param.streamId = streamId;
    uint32_t ret = NLSTK_ActmOpenAudioStream(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]OpenStream ret %{public}d", ret);
    }

    return ret;
}

int SleASC::StartStream(const RawAddress &device, uint8_t streamId)
{
    HILOGI("[SleASC]StartStream in %{public}s streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamId);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);

    NLSTK_ActmChangeParam_S param {};
    param.streamId = streamId;
    param.op = NLSTK_ACTM_STREAM_TRANS;
    uint32_t ret = NLSTK_ActmChangeAudioStream(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]StartStream ret %{public}d", ret);
    }

    return ret;
}

int SleASC::StopStream(const RawAddress &device, uint8_t streamId)
{
    HILOGI("[SleASC]StopStream in %{public}s streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamId);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    NLSTK_ActmChangeParam_S param {};
    param.streamId = streamId;
    param.op = NLSTK_ACTM_STREAM_STOP;
    uint32_t ret = NLSTK_ActmChangeAudioStream(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]StopStream ret %{public}d", ret);
    }
    
    return ret;
}

int SleASC::ReleaseStream(const RawAddress &device, uint8_t streamId)
{
    HILOGI("[SleASC]ReleaseStream in %{public}s streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), streamId);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    NLSTK_ActmReleaseParam_S param {};
    param.streamId = streamId;
    uint32_t ret = NLSTK_ActmReleaseAudioStream(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]ReleaseStream ret %{public}d", ret);
    }

    return ret;
}

int SleASC::SetDirection(const RawAddress &device, uint8_t direction)
{
    HILOGI("[SleASC]SetDirection in %{public}s direction %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), direction);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmSetDirection(&addr, direction);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]SetDirection ret %{public}d", ret);
    }

    return ret;
}

int SleASC::Disconnect(const RawAddress &device)
{
    HILOGI("[SleASC]Disconnect in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmDisconnect(&addr);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]Disconnect ret %{public}d", ret);
    }

    return ret;
}

int SleASC::UpdateBitRate(const RawAddress &device, uint64_t bps, uint8_t streamId)
{
    HILOGI("[SleASC]UpdateBitRate in %{public}s bps %{public}ld streamId %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), bps, streamId);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    NLSTK_ActmBitrateParam_S param {};
    param.streamId = streamId;
    param.bitrate = bps;
    uint32_t ret = NLSTK_ActmUpdateBitrate(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]UpdateBitRate ret %{public}d", ret);
    }

    return ret;
}

int SleASC::CreateStream(const RawAddress &device, uint8_t pointType, uint8_t commType)
{
    HILOGD("[SleASC]CreateStream in %{public}s pointType %{public}d commType %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), pointType, commType);

    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    NLSTK_ActmStreamParam_S param {};
    param.pointType = pointType;
    param.commType = commType;
    uint32_t ret = NLSTK_ActmCreateStream(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]CreateStream ret %{public}d", ret);
    }
    return ret;
}

int SleASC::SendVoiceCallAutoRateMsg(const RawAddress &device, NLSTK_ActmAutoRateRecvMsg_S &param)
{
    HILOGD("[SleASC]SendVoiceCallAutoRateMsg in %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    SLE_Addr_S addr {};
    CONVERT_DEV_ADDR(device, addr);
    uint32_t ret = NLSTK_ActmRecvAutoRateMsg(&addr, &param);
    if (ret != NL_NO_ERROR) {
        HILOGE("[SleASC]SendVoiceCallAutoRateMsg ret %{public}d", ret);
    }
    return ret;
}

} // namespace Nearlink
} // namespace OHOS