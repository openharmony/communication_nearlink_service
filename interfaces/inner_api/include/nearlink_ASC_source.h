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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines audio stream, including audio stream ctrl and callbacks, and audio stream functions.
 *
 * @since 6
 */

/**
 * @file nearlink_ASC_source.h
 *
 * @brief Audio Stream common functions.
 *
 * @since 6
 */


#ifndef NEARLINK_ASC_SOURCE_H
#define NEARLINK_ASC_SOURCE_H

#include <vector>
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_uuid.h"
#include "nearlink_remote_device.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief audio stream encode type.
 *
 * @since 6.0
 */
enum AudioStreamType : uint32_t {
    AUDIO_STREAM_NONE = 0x00000000,
    AUDIO_STREAM_UNDEFINED = 0x00000001, //未定义
    AUDIO_STREAM_MUSIC = 0x00000002, // 媒体音乐，指的是播放歌曲
    AUDIO_STREAM_VOICE_CALL = 0x00000004, // 移动网络通话
    AUDIO_STREAM_VOICE_ASSISTANT = 0x00000008, // 语音助手
    AUDIO_STREAM_RING = 0x00000010, // 铃声
    AUDIO_STREAM_VOIP = 0x00000020, // IP通话
    AUDIO_STREAM_GAME = 0x00000040, // 低时延：游戏
    AUDIO_STREAM_RECORD = 0x00000080, // 录音
    AUDIO_STREAM_ALERT = 0x00000100, // 提示音
    AUDIO_STREAM_VIDEO = 0x00000200, // 视频声
    AUDIO_STREAM_GUID = 0x00000400, // 导航声
    AUDIO_STREAM_ALARM = 0x00000800, // 告警声
    AUDIO_STREAM_SING = 0x00001000, // K歌
};

/**
 * @brief audio stream available status.
 *
 * @since 6.0
 */
enum AudioStreamState : uint8_t {
    AUDIO_STREAM_STATE_INVALID = 0x00,       // 无效值
    AUDIO_STREAM_STATE_AVAILABLE = 0x01,     // 可用
    AUDIO_STREAM_STATE_NOT_AVAILABLE = 0x02, // 不可用
};

struct AudioStreamInfo {
    // audio stream type
    AudioStreamType streamType;
    // audio stream state
    AudioStreamState streamState;
};

/**
 * @brief Audio Stream API callback function.
 *
 * @since 6.0
 */
class SleAudioStreamObserver {
public:
    /**
     * @brief A destructor used to delete the Audio Stream Observer instance.
     *
     * @since 6.0
     */
    virtual ~SleAudioStreamObserver() = default;

    /**
     * @brief The callback function after sle device audio profile connected.
     *
     * @param device the remote nearlink device.
     * @param streamType the supoport audio stream type.
     * @param mediaVolume the media volume.
     * @param callVolume the call volume.
     * @since 6.0
     */
    virtual void OnAddSleAudioDevice(const NearlinkRemoteDevice &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume)
    {}

    /**
     * @brief The callback function after sle device audio profile disconnected.
     *
     * @param device the remote nearlink device.
     * @since 6.0
     */
    virtual void OnDeleteSleAudioDevice(const NearlinkRemoteDevice &device)
    {}

    /**
     * @brief The callback function after sle virtual device online.
     *
     * @param device the remote nearlink device.
     * @param streamType the supoport audio stream type.
     * @since 6.0
     */
    virtual void OnAddSleVirtualAudioDevice(const NearlinkRemoteDevice &device, uint32_t streamType)
    {}

    /**
     * @brief The callback function after sle virtual device offline.
     *
     * @param device the remote nearlink device.
     * @since 6.0
     */
    virtual void OnDeleteSleVirtualAudioDevice(const NearlinkRemoteDevice &device)
    {}

    /**
     * @brief The callback function after start device's playing.
     *
     * @param device the remote nearlink device.
     * @param streamType the start playing stream type.
     * @param result the start result.
     * @since 6.0
     */
    virtual void OnStartPlayingResult(const NearlinkRemoteDevice &device, AudioStreamType streamType, int result)
    {}

    /**
     * @brief The callback function after stop device's playing.
     *
     * @param device the remote nearlink device.
     * @param streamType the stop playing stream type.
     * @param result the start result.
     * @since 6.0
     */
    virtual void OnStopPlayingResult(const NearlinkRemoteDevice &device, AudioStreamType streamType, int result)
    {}

    /**
     * @brief The callback function after device action.
     *
     * @param device the remote nearlink device.
     * @param action on the device.
     * @since 6.0
     */
    virtual void OnSleAudioDeviceActionChanged(const NearlinkRemoteDevice &device,
        std::vector<struct AudioStreamInfo> &streamInfo, int action)
    {}
};

/**
 * @brief codec configuration information of audio stream
 *
 * @since 6.0
 */
struct AudioStreamCodecInfo {
    // Codec type
    uint64_t codecType;
    // Codec sample
    uint16_t sampleRate;
    // Codec bits per sample
    uint8_t bitsPerSample;
    // Codec channel mode
    uint8_t channelMode;
    // Codec frame length
    uint16_t frameDuration;
    // Codec encoding rate
    uint64_t bitRate;
    // Codec frame num per sdu
    uint8_t frameNumPerSdu;
};

/**
 * @brief Audio stream API.
 *
 * @since 6.0
 */
class NEARLINK_API SleAudioStream {
public:
    /**
     * @brief Get audio stream instance.
     *
     * @return Returns an instance of audio stream.
     * @since 6.0
     */
    static std::shared_ptr<SleAudioStream> CreateSleAudioStream(std::shared_ptr<SleAudioStreamObserver> callback);

    /**
     * get audio device list.
     *
     * @param devices virtual device list.
     * @since 12
     */
    void GetSleAudioDeviceList(std::vector<NearlinkRemoteDevice> &devices);

    /**
     * get audio device support codec.
     *
     * @param device audio device.
     * @return Returns codecInfo of audio stream type.
     * @since 12
     */
    std::map<AudioStreamType, AudioStreamCodecInfo> GetSleAudioDeviceCodecInfo(const NearlinkRemoteDevice &device);

    /**
     * get virtual audio device list.
     *
     * @param devices virtual device list.
     * @since 12
     */
    void GetSleVirtualAudioDeviceList(std::vector<NearlinkRemoteDevice> &devices);

    /**
     * @brief remote device inband ring is open or not.
     *
     * @param device The address of the peer nearlink device.
     * @return true:in band ring open, false:in band ring close;
     * @since 6.0
     */
    bool IsInBandRingOpen(const NearlinkRemoteDevice &device) const;

    /**
     * @brief Get device support stream type.
     *
     * @param device The address of the peer nearlink device.
     * @return Returns support stream type AudioStreamType;
     * @since 6.0
     */
    uint32_t GetSupportStreamType(const NearlinkRemoteDevice &device) const;

    /**
     * @brief Set target device as active device in target stream type.
     *
     * @param device The address of the peer nearlink device.
     * @param streamType audio stream type as AudioStreamType
     * @return Returns the status code for this function called.
     * @since 6.0
     */
    NlErrCode SetActiveSinkDevice(const NearlinkRemoteDevice &device, uint64_t supportStreamType);

    /**
     * @brief Audio start streaming.
     *
     * @param device The address of the nearlink device.
     * @param streamType The audio stream type.
     * @return Returns the status code for this function called.
     * @since 6.0
     */
    NlErrCode StartPlaying(const NearlinkRemoteDevice &device, AudioStreamType streamType);

    /**
     * @brief Audio stop streaming.
     *
     * @param device The address of the nearlink device.
     * @param streamType The audio stream type.
     * @return Returns the status code for this function called.
     * @since 6.0
     */
    NlErrCode StopPlaying(const NearlinkRemoteDevice &device, AudioStreamType streamType);

    /**
     * @brief Get local and target device whether support dual record or not.
     *
     * @param device The address of the peer nearlink device.
     * @return true:local and remote support, false:local or remote not support;
     * @since 6.0
     */
    bool GetDualRecordAbility(const NearlinkRemoteDevice &device);

    /**
     * @brief Get local and target device whether support dual karaoke or not.
     *
     * @param device The address of the peer nearlink device.
     * @return true:local and remote support, false:local or remote not support;
     * @since 6.0
     */
    bool GetKaraokeAbility(const NearlinkRemoteDevice &device);

    ~SleAudioStream();
private:
    explicit SleAudioStream(std::shared_ptr<SleAudioStreamObserver> callback);

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SleAudioStream);

    NEARLINK_DECLARE_IMPL();

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use CreateSleAudioStream interface to create objects.
    explicit SleAudioStream(Pattern, std::shared_ptr<SleAudioStreamObserver> callback) :
        SleAudioStream(callback) {};

};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_ASC_SOURCE_H
