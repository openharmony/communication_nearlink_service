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

#ifndef NEARLINK_DFT_EXCEPTION_H
#define NEARLINK_DFT_EXCEPTION_H

#include "nearlink_dft_database.h"
#include <string>
#include <cstdint>
#include "nearlink_dft_manager.h"
#include "nearlink_utils.h"
#include "nearlink_dft_device_data.h"

#define STATE_FLOW_MARK "-"
#define STATE_FLOW_MAX_LEN 20

// DFT SWITCH INFO MSG
#define DFT_MSG_SWITCH_SUCCESS "Nearlink Switch operation succeed."
#define DFT_MSG_DISABLE_TIME_OUT "Closing Nearlink operation is time out."
#define DFT_MSG_ENABLE_TIME_OUT "Opening Nearlink operation is time out."
#define DFT_MSG_HALF_TIME_OUT "Turning Nearlink to half-disable is time out."
#define DFT_MSG_ALREADY_DISABLE "Nearlink Switch is already close."
#define DFT_MSG_ALREADY_ENABLE "Nearlink Switch is already open."
#define DFT_MSG_ALREADY_HALF "Nearlink Switch is already half-disabled."
#define DFT_MSG_TURNING_STATE "Nearlink Switch is in TURNING state so operation failed."
#define DFT_MSG_SWITCH_REASON "Nearlink Switch operation reason is: "

// DFT DISCONN INFO MSG
#define DFT_DISCONN_SLESECURITY "[SleAdapter] sleSecurity is null."
#define DFT_DISCONN_PAIRFAIL "pair complete fail."
#define DFT_DISCONN_ENCRYFAIL "[SleAdapter] encry fail."
#define DFT_DISCONN_DISCOVERFAIL "[SleAdapter] DiscoveryServices faild."
#define DFT_DISCONN_PROFILEFLAG "SetSupportedProfileFlags failed."
#define DFT_DISCONN_NOPROFILE "no supported profile."
#define DFT_DISCONN_PROFILECONNTIMEOUT "profile conn timeout."
#define DFT_DISCONN_DISCONNALLPROFILES "disconn all profiles."
#define DFT_DISCONN_NOAVAILABLEPROFILE "no available profile"

// DFT PAIR MSG
#define PAIR_SSAP_TIMEOUT "SleAdapterProfConnInst Timeout!"
#define PAIR_DIS_TIMEOUT "Dis Connect Timeout"
#define PAIR_HID_HOST_CLOSE "Hid Host Close Event"
#define PAIR_HID_CONN_TIMEOUT "His Connect Timeout"

constexpr uint32_t AUDIO_EXCEP_SCENE_MASK = 0x0F;
constexpr uint32_t AUDIO_EXCEP_ERROR_MASK = 0xFF0;

constexpr uint16_t AUDIO_RSSI_SOURCE_MASK = 0xFF;
constexpr uint16_t AUDIO_RSSI_EAR_MASK = 0xFF00;

constexpr uint32_t AUDIO_RETRANS_CRC_MASK = 0xFF;
constexpr uint32_t AUDIO_RETRANS_RXSEQ_MASK = 0xFF00;
constexpr uint32_t AUDIO_RETRANS_NOSYNC_MASK = 0xFF0000;
constexpr uint32_t AUDIO_EXCEP_BAND_MASK = 0xFF000000;

constexpr uint32_t AUDIO_SCENE_LNA_MASK = 0x03;
constexpr uint32_t AUDIO_SCENE_DTN_MASK = 0x0C;
constexpr uint32_t AUDIO_SCENE_CODEC_MASK = 0x3F0;
constexpr uint32_t AUDIO_SCENE_BIT_MASK = 0xFFFC00;
constexpr uint32_t AUDIO_SCENE_USERSCENE_MASK = 0xF000000;
constexpr uint32_t AUDIO_SCENE_DEVICE_MASK = 0x30000000;
constexpr uint32_t AUDIO_SCENE_CENTRAL_MASK = 0x80000000;

constexpr uint32_t AUDIO_AFH_STRONG_MASK = 0xFF;
constexpr uint32_t AUDIO_AFH_MID_MASK = 0xFF00;
constexpr uint32_t AUDIO_AFH_WEAK_MASK = 0xFF0000;
constexpr uint32_t AUDIO_AFH_NOSISE_MASK = 0xFF000000;

constexpr uint32_t AUDIO_FREQ_BAND_FREQ_MASK = 0x03;
constexpr uint32_t AUDIO_FREQ_BAND_ISO_PWR_MASK = 0x1C;
constexpr uint32_t AUDIO_FREQ_BAND_ACB_PWR_MASK = 0xE0;

constexpr uint8_t AUDIO_TWO_SHIFT = 2;
constexpr uint8_t AUDIO_FOUR_SHIFT = 4;
constexpr uint8_t AUDIO_FIVE_SHIFT = 5;
constexpr uint8_t AUDIO_EIGHT_SHIFT = 8;
constexpr uint8_t AUDIO_TEN_SHIFT = 10;
constexpr uint8_t AUDIO_SIXTEEN_SHIFT = 16;
constexpr uint8_t AUDIO_TWENTY_FOUR_SHIFT = 24;
constexpr uint8_t AUDIO_TWENTY_EIGHT_SHIFT = 28;
constexpr uint8_t AUDIO_THIRTY_ONE_SHIFT = 31;

constexpr int32_t COMM_PROTOCOL_NEARLINK = 2; // 2代表星闪

typedef enum DftSwitchTypeEnum :int {
    INVALIDVALUE = -1,
    SWITCH_CLOSE,
    SWITCH_OPEN,
    SWITCH_RESET,
    SWITCH_FACTORY_RESET,
    SWITCH_HALF,
    SWITCH_SELF_RECOVERY
} DftSwitchTypeEnum;

typedef enum DftSwitchOperRes :int {
    SWITCH_OPER_SUCCESS = 0,
    SWITCH_OPER_FAIL,
} DftSwitchOperRes;

typedef enum DftSwitchErrCode :int {
    SWITCH_SUCCESS = 0,
    DISABLE_TIME_OUT,
    ENABLE_TIME_OUT,
    TURNING_STATE,
    HALF_TIME_OUT,
} DftSwitchErrCode;

typedef enum DftDisconnScene :int {
    DISCONN_INVALIDVALUE = 0,
    DISCONN_CLICK_DISCONN,
    DISCONN_CLICK_UNPAIR,
    DISCONN_CLICK_SWITCH,
    DISCONN_EXCEP = 100,
} DftDisconnScene;

typedef enum DftAccurateSearchPath :int {
        DFT_SCAN = 10,
        DFT_ACB_CONN = 20,
        DFT_SSAP_CONN = 30,
        DFT_MEASURE = 40,
} DftAccurateSearchPath;

typedef enum DftAccurateSearchConnResult :int {
        DFT_CONN_SUCCESS = 0,
        DFT_CONN_FAILED,
        DFT_CONN_TIME_OUT,
} DftAccurateSearchConnResult;

typedef enum DftAccurateSearchMeasureResult :int {
        DFT_MEASURE_SUCCESS = 0,
        DFT_MEASURE_FAILED,
        DFT_MEASURE_TIME_OUT,
        DFT_MEASURE_FINISHED,
} DftAccurateSearchMeasureResult;

typedef enum DftTaskName :int {
        DFT_DEFAULT = 0,
        DFT_NORMAL_TASK,
        DFT_ACCURATESEARCH_TASK,
}DftTaskName;

typedef enum DftPairConnPath :int {
    PAIR_CONN_PATH_SCAN = 10,
    PAIR_CONN_PATH_ACB = 20,
    PAIR_CONN_PATH_AUTH = 30,
    PAIR_CONN_PATH_ENCYP = 40,
    PAIR_CONN_PATH_SSAP = 50,
    PAIR_CONN_PATH_DISCOVER = 60,
    PAIR_CONN_PATH_DIS = 70,
    PAIR_CONN_PATH_HID = 80,
    PAIR_CONN_PATH_ICCE = 90,
    PAIR_CONN_PATH_PORT = 100,
    PAIR_CONN_PATH_BAS = 110,
    PAIR_CONN_PATH_ALL_PROFILE = 200,
} DftPairConnPath;

typedef enum DftPairType :int {
    NO_PAIR = 0,
    FIRST_TIME_CONN,
    CLOUD_PAIR,
} DftPairType;

typedef enum DftConnType :int {
    NO_CONN = 0,
    CLICK_CONN,
    BG_CONN,
} DftConnType;

typedef enum DftDeviceServiceState :int {
    NO_SERVICE = 0,
    DATA_TRANSPORT = 3,
} DftDeviceServiceState;

typedef enum DftDeviceConnState :int {
    MUIDEVICE_CONNECTED = 1,
    MUIDEVICE_DISCONNECTED = 3,
} DftDeviceConnState;

typedef enum DftDtfrScene :int {
    DTFR_CREATE_PORT = 1,
    DTFR_DESTROY_PORT = 2,
    DTFR_CONNECT = 3,
    DTFR_DISCONNECT = 4,
} DftDtfrScene;

typedef enum DftDtfrSubScene :int {
    DTFR_SUCC = 1,
    DTFR_FAILED = 2,
} DftDtfrSubScene;

typedef enum DftDtfrExcepScene :int {
    EXCEP_CREATE_PORT = 1,
    EXCEP_CONNECT = 2,
    EXCEP_WRITE = 3,
} DftDtfrExcepScene;

typedef enum DftDtfrType :int {
    DTFR_ICCE = 1,
    DTFR_PROXY = 2,
} DftDtfrType;

typedef enum DftDtfrExcepPortSubScene :int {
    CREATE_PORT_FAIL = 1,
} DftDtfrExcepPortSubScene;

typedef enum DftDtfrExcepConnSubScene :int {
    GET_ICCE_PORT_FAIL = 1,
    CONNECT_REMOTE_FAIL = 2,
    CONNECT_REMOTE_TIMEOUT = 3,
    GET_PEER_PORT_FAIL = 4,
    CONNECT_GET_TCID_ERR = 5,
} DftDtfrExcepConnSubScene;

typedef enum DftDtfrExcepWriteSubScene :int {
    WRITE_DISCONNECTED_ERR = 1,
    WRITE_EXCEEDED = 2,
    WRITE_PORT_CHANNEL_ERR = 3,
    WRITE_RETRY_MAX_FAIL = 4,
} DftDtfrExcepWriteSubScene;

typedef enum DftDtfrChState :uint8_t {
    DTFR_CHANNEL_ESTABLISHED,
    DTFR_CHANNEL_ESTABLISH_FAIL,
    DTFR_CHANNEL_RELEASED,
    DTFR_CHANNEL_RELEASE_FAIL,
} DftDtfrChState;

typedef enum DftStreamState : int {
    STREAM_ASC_DISABLED                 = 0,
    STREAM_ASC_DISABLING                = 1,
    STREAM_ASC_ENABLED                  = 2,
    STREAM_ASC_ENABLING                 = 3,
    STREAM_ASC_CONNECTED                = 4,
    STREAM_ASC_CONNECTING               = 5,
    STREAM_ASC_DISCONNECTED             = 6,
    STREAM_ASC_DISCONNECTING            = 7,
    STREAM_ASC_CONFIGED                 = 8,
    STREAM_ASC_CONFIGURING              = 9,
    STREAM_ASC_OPENED                   = 10,
    STREAM_ASC_OPENING                  = 11,
    STREAM_ASC_STARTED                  = 12,
    STREAM_ASC_STARTING                 = 13,
    STREAM_ASC_STOPPED                  = 14,
    STREAM_ASC_STOPPING                 = 15,
    STREAM_ASC_RELEASED                 = 16,
    STREAM_ASC_RELEASING                = 17,
    STREAM_ASC_RECONFIG_STOPPING        = 18,
    STREAM_ASC_RECONFIG_STOPPED         = 19,
    STREAM_ASC_RECONFIGED               = 20,
    STREAM_ASC_RECONFIGURING            = 21,
    STREAM_ASC_BUTT                     = 22,
} DftStreamState;

typedef enum DftAudioProfileErrorScene : uint8_t {
    ERROR_NEARLINK_INNER = 1,
    ERROR_AUDIO_FWK = 2,
    ERROR_TELEPHONY = 3,
    ERROR_VOICE_ASSISTANT = 4,
    ERROR_ASC = 5,
} DftAudioProfileErrorScene;

typedef enum NearlinkCommonSubErrCode : uint8_t {
    SUB_ERRCODE_CASE1 = 1,
    SUB_ERRCODE_CASE2,
    SUB_ERRCODE_CASE3,
    SUB_ERRCODE_CASE4,
    SUB_ERRCODE_CASE5,
} NearlinkCommonSubErrCode;

namespace OHOS {
namespace Nearlink {

typedef struct {
    std::string audioDeviceAddr;
    int32_t audioDeviceAppear;
    std::string audioDeviceName;
    uint16_t freqBand;
    uint16_t audioSceneCode;
    uint16_t userSceneCode;
    uint16_t errorCode;
    int32_t audioSourceRssi;
    int32_t otherEarRssi;
    uint16_t wrongPackRate;
    uint16_t misorderCntRxseq;
    uint16_t nosyncRate;
    uint16_t lnaSwitch;
    uint16_t doubleTunedState;
    uint16_t codecType;
    uint16_t bitrateInfo;
    uint16_t deviceNumInLink;
    uint16_t audioRole;
    uint16_t strongChannelNum;
    uint16_t midChannelNum;
    uint16_t weakChannelNum;
    int32_t noiseAvgValue;
} AudioHeadsetExcep;

#pragma pack(1) // 开始使用1字节对齐
typedef struct {
    uint32_t audioExceptionType;
    uint16_t audioQualityRssi;
    uint32_t audioQualityRetrans;
    uint32_t audioQualityScene;
    uint32_t audioQualityAfh;
} DftAudioExceptionData;
#pragma pack() // 恢复默认对齐

typedef struct {
    std::string addr;
    int32_t state;
    uint8_t runningType;
    uint8_t pointType;
    std::string reportaddr;
    std::string startBuff;
    std::string configparam;
} DftAudioStreamInfo;

void DftCacheAcbStartConn(const std::string &addr);
void DftCacheAcbDisConn(const std::string &addr, const std::string &callingName);
void DftCacheAcbFinishConn(const std::string &addr, uint32_t result, int32_t reason, uint32_t count, uint16_t lcid);
void DftCacheBgStartConn(const std::string &addr, int32_t deviceAppearance);

// Nearlink switch event
void DftCacheSwitchInfo(DftSwitchTypeEnum switchType, DftSwitchOperRes operRes, const std::string &app = "");
void DftCacheStateFlow(int sleState);
void DftReportSwitchInfo(DftSwitchErrCode errCode, const std::string &infoMsg,
    DftSwitchTypeEnum switchType = INVALIDVALUE, DftSwitchOperRes operRes = SWITCH_OPER_SUCCESS,
    const std::string &app = "");

// PEER_INFO子事件
void DftCachePeerName(const std::string &addr, const std::string &name);
void DftCachePeerAppearance(const std::string &addr, int32_t appearance);
void DftCacheHidState(const std::string &addr, int state);
void DftCachePeerInfoTime(const std::string &addr, DftPeerInfoParamEnum paramId);
void DftCachePeerManufacturer(const std::string &addr, const std::string &manufacturer);

// CONN_INFO子事件
void DftCacheConnInfoTime(const std::string &addr, DftConnInfoParamEnum paramId);
void DftConnStateChange(const std::string &addr, uint8_t state, uint32_t resaon);

// NEARLINK_DISCONN_EXCEPTION
void DftCacheDisconnRssi(const std::string &addr, int32_t rssi);
void DftCacheDisconnInfoMsg(const std::string &addr, const std::string &infoMsg, uint16_t secne = DISCONN_EXCEP);

// NEARLINK_ACCURATESEARCH
void DftSetMeasureResultMap(const std::string &addr, bool value);
void DftSetAccurateSearchTaskMap(const std::string &addr);
void DftCacheRssiValueInfo(const std::string &addr, int32_t rssiValue);
void DftDealAccurateSearchScanInfo(const std::string &name, DftAccurateSearchPath path);
void DftCacheAccurateSearchConnInfo(const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchConnResult connResult);
void DftReportAccurateSearchConnFailInfo(const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchConnResult connResult, int32_t errorCode);
void DftDealAccurateSearchConnInfo(const std::string &addr, uint16_t connResult, int32_t discReason);
void DftCacheAccurateSearchMeasureInfo(const std::string &name, const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchMeasureResult measureResult);
void DftReportAccurateSearchMeasureInfo(const std::string &name, const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchMeasureResult measureResult, int32_t measureErrorCode);
void DftDealAccurateSearchMeasureInfo(const std::string &name, const std::string &addr);

// NEARLINK_PAIR_EXCEPTION
void DftCachePairConnPath(const std::string &addr, uint32_t connPath);
void DftCachePairConnTime(const std::string &addr, uint32_t currentConnPath, uint32_t pairTimeType);
void DftCachePairDeviceCount(const std::string &addr, uint32_t count);
void DftCachePairConnType(const std::string &addr, uint32_t pairType, uint32_t connType);
void DftCachePairAppearance(const std::string &addr, int32_t deviceAppearance);
void DftCachePairName(const std::string &addr, const std::string &name);
void DftCachePairAcbFinish(const std::string &addr, uint32_t result, int32_t reason, uint32_t count);
void DftCacheSsapStart(const std::string &addr);
void DftCacheSecurityPairType(const std::string &addr, bool isSecurityPairType);
void DftCacheHidStart(const std::string &addr);
void DftCacheHidFinish(const std::string &addr, int32_t reason);
void DftCacheDisStart(const std::string &addr);
void DftCacheDisFinish(const std::string &addr);
void DftCacheIcceStart(const std::string &addr);
void DftCacheIcceFinish(const std::string &addr);
void DftReportPairInfo(const std::string &addr, uint32_t currentConnPath, int32_t reason,
    const std::string &detail = "");
void DftCachePairRssiAndChannelNoise(const std::string &addr, int32_t rssi, std::string noise);
void DftCacheDisconChipInfo(const std::string &addr, int32_t rssi, std::string channelNoise);
void DftCacheCallingName(const std::string &addr, const std::string &callingName);
void DftSetConnectProfileTaskFlag(const std::string &addr);
void DftDealEncryptFailEvent(const std::string &addr, int32_t result);
void DftReportPairSuccess(const std::string &addr, uint16_t connType);
void DftCacheCdsmInfo(const std::string &device, const std::string &reportAddr, const std::string &otherAddr);

// NEARLINK_AUDIO_PROFILE_EXCEPTION
void DftReportAudioError(const std::string &device, const uint16_t errorCode, const uint16_t subErrorCode,
    const std::string &extraParam = "");

// Nearlink unpair event
void DftCacheUnPairInfo(const std::string &addr, int32_t deviceAppearance,
    const std::string &callingName, int32_t state, int32_t pairState);
void DftDealUnPairLinkKeyInfo(const std::string &addr, bool ret);
void DftReportUnPairFailInfo(const std::string &addr, int32_t status, uint16_t result);
void DftReportUnPairSuccessInfo(const std::string &addr, int32_t status, uint16_t result);

// Multidevice conn event
void DftCacheMultideviceInfo(const std::string &addr, const std::string &callingName, uint16_t serviceState);
void DftDealMultideviceInfo(const std::string &addr, uint32_t result);

void DftCachePeerInfo(const std::string &addr, const std::string &name, int32_t appearance);
void DftCacheAppearance(const std::string &addr, int32_t appearance);
void DftCacheName(const std::string &addr, const std::string &name);

// Data transfer event
void DftDtfrRecordCallName(const std::string &addr, const std::string &callName);
void DftReportDtfrStatisInfo(uint8_t state, const std::string &addr, const std::string &uuid);
void DftReportDtfrStatisPortInfo(const std::string &callName, int32_t scene);
void DftReportDtfrExcepInfo(const std::string &addr, const std::string &uuid, int32_t scene, int32_t errCode);

// Audio headSet event
void DftHandleAudioExcep(const RawAddress &addr, DftAudioExceptionData &dftAudioExcepData,
    const std::string &localAddr = "");
void DftReportAudioHeadSet(AudioHeadsetExcep &audioHeadsetData);

// NEARLINK_AUDIO_STREAM_EXCEP
void DftCacheStreamTime(const std::string &addr, int32_t timeType);
void DftCacheStreamConfig(const std::string &addr, uint16_t reconfig, const std::string &configParam = "");
void DftCacheStreamOpen(const std::string &addr);
void DftCacheStreamStart(const std::string &addr);
void DftCacheStreamStop(const std::string &addr);
void DftCacheStreamRelease(const std::string &addr);
void DftCacheStreamInfo(DftAudioStreamInfo &info);
void DftReportStreamSuccess(const std::string &addr, bool cbkdiscon = false);
void DftReportStreamFail(const std::string &addr, int32_t state, int32_t reason);
void DftReportStreamASC(const std::string &addr, int32_t state, uint8_t result, int32_t reason);
void DftCacheStreamASC(DftAudioStreamInfo info);

// Set Airplane Mode From ASC
void DftSetAirplaneMode(bool isOn);
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_DFT_EXCEPTION_H