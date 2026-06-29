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

#ifndef SLE_INTERFACE_PROFILE_ASC_H
#define SLE_INTERFACE_PROFILE_ASC_H

#include "SleInterfaceProfile.h"
#include "nearlink_ASC_source.h"
#include "nearlink_raw_address.h"
#include "nearlink_asc_audio_control_result.h"
#include "nearlink_asc_audio_stream_info.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {
class ASCObserver {
public:
    virtual ~ASCObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          new state
     * @param  oldState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

enum class UpdateOutputStackAction : uint8_t {
    ACTION_WEAR                   = 0, /* 已佩戴 */
    ACTION_UNWEAR                 = 1, /* 未佩戴 */
    ACTION_ENABLE_FROM_REMOTE     = 2, /* 对端设备使能音乐 */
    ACTION_DISABLE_FROM_REMOTE    = 3, /* 对端设备禁用音乐 */
    ACTION_ENABLE_WEAR_DETECTION  = 4, /* 开佩戴检测 */
    ACTION_DISABLE_WEAR_DETECTION = 5, /* 关佩戴检测 */
    ACTION_USER_OPERATION         = 6,
    ACTION_ROLE_TYPE_CHANGE       = 7, /* 主副切换 */
};

typedef struct {
    uint8_t qosIndex;
    uint8_t codecId;
    uint8_t version;                    // version
    uint8_t frame;                      // 帧长ms
    uint8_t bitSamp;                    // 位宽
    uint8_t channelMode;                // 通道类型
    uint8_t linkCnt;                    // 链路数量
    uint8_t sca;                        // 睡眠时钟精度
    uint8_t packing;                    // 当前仅支持交叉
    uint8_t framing;                    // 当前仅支持未切分
    uint8_t ft;                         // G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF]
    uint8_t rtn;                        // G到T每一个IMB数据PDU重传次数
    uint8_t nse;                        // 0x01-0x1F 在IMG中每一个IMB每个间隔内的子事件个数
    uint8_t bn;                         // G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数
    uint8_t phy;                        // 0:1M, 1:2M, 2:4M
    uint8_t mcs;                        // G到T方向上使用的调制方式和polar编码
    uint8_t pilot;                      // 0x00-0x07 标识Polar编码时插导频pilot的比例，代表[数据:导频]=[2^ratio:1]
    uint8_t frameType;                  // 无线电帧类型
    uint16_t companyId;                 // 厂商标识
    uint16_t vendorId;                  // 厂商编解码器标识
    uint16_t bps;                       // 码率kbps
    uint16_t gHandle;                   // 组播组句柄
    uint16_t connHandle;                // 链路句柄
    uint16_t sduInterval;               // sdu间隔
    uint16_t maxSdu;                    // sdu最大长度
    uint16_t maxPdu;                    // pdu最大长度
    uint16_t bufNum;                    // 同步链路buffer数
    uint16_t maxLatency;                // 最大传输延迟，ms
    uint16_t icbInterval;               // 连续两个IMB Anchor Point的间隔时间，取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s]
    uint32_t rate;                      // 采样率Hz
    bool     isUpdated;                 // 更新标记
} AscQosmInfo;

class InterfaceASCCallback {
public:
    virtual ~InterfaceASCCallback() = default;
    virtual void OnAudioControlComplete(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result);
    virtual void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume);
    virtual void OnDeleteSleAudioDevice(const NearlinkRawAddress &device);
    virtual void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device,
        const NearlinkASCAudioStreamInfo &streamInfo, int action);
    virtual void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType);
    virtual void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device);
};

class ProfileASC : public SleInterfaceProfile {
public:
    virtual int AudioControl(const RawAddress &device, AudioStreamType streamType, int cmd) = 0;
    virtual void StopSink() = 0;
    virtual int GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices) = 0;
    virtual int GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress>& devices) = 0;
    virtual int GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType) = 0;
    virtual int GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
        AudioStreamCodecInfo> &info) = 0;
    virtual int SetActiveSinkDevice(const NearlinkRawAddress &device, uint64_t supportStreamType) = 0;
    virtual int UpdateDeviceRole(const RawAddress &device, uint8_t devRole) = 0;

    virtual int RegisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) = 0;
    virtual int DeregisterApplication() = 0;
    virtual int DeregisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) = 0;
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(ASCObserver &observer) = 0;
    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(ASCObserver &observer) = 0;

    /**
     * @brief Get active sink device.
     *
     * @param None
     * @return device The address of the active sink device.
     * @since 6.0
     */
    virtual const NearlinkRawAddress GetActiveSinkDevice() const = 0;

    /**
     * @brief Get whether the device is on calling.
     *
     * @return Returns true if device is on calling, false otherwise.
     * @since 6.0
     */
    virtual bool IsCalling() = 0;

    /**
     * @brief Get whether the device is playing.
     *
     * @param device The address of the peer device.
     * @return Returns true if device is playing, false otherwise.
     * @since 6.0
     */
    virtual bool IsPlaying(const RawAddress &device) = 0;

    /**
     * @brief Get device audio stream qos info.
     *
     * @param device The address of the device.
     * @return Returns true if found, false otherwise.
     * @since 6.0
     */
    virtual bool GetAscQosmInfo(const RawAddress& device, AscQosmInfo& qosmInfo) = 0;

    /**
     * @brief Audio device action change notify.
     *
     * @param device The address of the active sink device.
     * @param action action.
     * @return None
     * @since 6.0
     */
    virtual void SleAudioDeviceActionChanged(const NearlinkRawAddress &device, int action) = 0;

    virtual bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) = 0;

    virtual void SetIsCallingFlag(bool isCalling) = 0;

    /* 上报音频流类型变化 */
    virtual void SleAudioDeviceActionChanged(const NearlinkRawAddress &device,
        const std::vector<struct AudioStreamInfo> &streamData, int action) = 0;

    virtual void SendPlayOrPauseByWearDetection(const RawAddress &devAddr, uint8_t playOrPauseKey) = 0;

    virtual void AcbSubrateChanged(const RawAddress &device, uint32_t subrate) = 0;

    virtual void AcbSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &subrateParam) = 0;
    /* 查找本端&对端设备是否支持双耳录音 */
    virtual bool GetDualRecordAbility(const RawAddress &device) = 0;

    virtual void PhyChanged(RawAddress device, uint8_t frameType, uint8_t phyType, uint8_t status) = 0;
    /* 查找本端&对端设备是否支持双耳K歌 */
    virtual bool GetKaraokeAbility(const RawAddress &device) = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_ASC_H