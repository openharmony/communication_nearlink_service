/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines Volume Controller interface and callbacks that provides basic nearlink telephony
 *
 * @since 6
 */

/**
 * @file nearlink_VCP_client.h
 *
 * @brief Volume Controller common functions.
 *
 * @since 6
 */

#ifndef NEARLINK_VCP_CLIENT_H
#define NEARLINK_VCP_CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include <list>

#include "nearlink_def.h"
#include "nearlink_remote_device.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace Nearlink {

enum VolumeStreamType : uint8_t {
    SLE_STREAM_MEDIA = 0, // 媒体
    SLE_STREAM_CALL,      // 通话
    SLE_STREAM_MAX,       // 流类型数目
};

/**
 * @brief Class for Volume Controller API.
 *
 * @since 6
 */
class NEARLINK_API VolumeControllerClient {
public:
    /**
     * @brief Get the instance of VolumeControllerClient object.
     *
     * @return Returns the pointer to the VolumeControllerClient instance.
     * @since 6
     */
    static VolumeControllerClient *GetProfile();

    /**
     * @brief audio set device absolute volume.
     *
     * @param[in] device The remote device.
     * @param[in] volumeLevel device absolute volume.
     * @param[in] streamType device stream type. (SLE_STREAM_MEDIA / SLE_STREAM_MAX)
     * @return error code
     * @since 6
     */
    NlErrCode SetDeviceAbsoluteVolume(const NearlinkRemoteDevice &device, int32_t volumeLevel, uint8_t streamType);

    /**
     * @brief audio get device media volume.
     *
     * @param[in] device The remote device.
     * @param[out] mediaVolume device media volume.
     * @return error code
     * @since 6
     */
    NlErrCode GetDeviceMediaVolume(const NearlinkRemoteDevice &device, int &mediaVolume);

    /**
     * @brief audio get device call volume.
     *
     * @param[in] device The remote device.
     * @param[out] callVolume device call volume.
     * @return error code
     * @since 6
     */
    NlErrCode GetDeviceCallVolume(const NearlinkRemoteDevice &device, int &callVolume);

private:
    VolumeControllerClient();
    ~VolumeControllerClient();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(VolumeControllerClient);
    NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_VCP_CLIENT_H
