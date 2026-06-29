/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "cJSON.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_exception_c.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_dft_utils.h"
#include "nearlink_dft_manager.h"
#include <mutex>
#include "nearlink_safe_map.h"
#include "nearlink_def.h"
#include "log.h"
#include "nearlink_dft_device_data.h"
#include "nearlink_dft_common_event_helper.h"
#include "nearlink_cemap_report_loader.h"

#define PARAMS_LEN(params) (sizeof(params) / sizeof(DftParamC))
#define CM_STATE_CONNECTED 0
#define CM_STATE_DISCONNECTED 2
#define MEASURE_SUCCESS 0
#define MEASURE_TIMEOUT 6
#define CONNECTION_TERMINATED_BY_LOCAL_HOST 0x11
#define UNPAIR_DELLINKKEY_ERROR 100
#define UNPAIR_SCENE_CLICK 0
#define UNPAIR_SCENE_OTHER 100
#define PAIR_RESULT_SUCCESS 0
#define PAIR_RESULT_FAILED 1
#define SLE_SUCCESS 0
#define STREAM_RESULT_SUCCESS 0
#define STREAM_RESULT_FAILED 1

namespace OHOS {
namespace Nearlink {

// protect g_stateFlow
static std::mutex g_mutex;
static std::mutex g_flagMutex;
static std::mutex g_countMutex;
static std::mutex g_modeMutex;
static std::string g_stateFlow = "";
const std::string ACCURATE_SEARCH_CALLING_NAME = "findnetwork";
const std::string UNPAIR_TASK_NAME = "cancelpairing";
const std::string NO_SECURITY_PAIR_TYPE = "NoneSecurityPair";
const std::string SECURITY_PAIR_TYPE = "SecurityPair";
const std::string DFT_UUID_ICCE = "060D";
constexpr const int32_t PAIRED_STATE = 3;
constexpr const uint16_t DFT_INVALID_LCID = 0xFFFF;
constexpr const int32_t DFT_CONNECTION_TIMEOUT = 0x07;
constexpr const int32_t INVALID_DISCONN_RSSI = -255; // 超时断连时使用该默认值进行大数据上报
static NearlinkSafeMap<std::string, bool> g_measureResultMap;
static NearlinkSafeMap<std::string, int> g_measurePathMap;
static NearlinkSafeMap<std::string, std::string> g_dftTaskMap;
static DftTaskName g_dftTaskName = DFT_DEFAULT;
static NearlinkSafeMap<std::string, uint32_t> g_pairFirstTimeMap;
static NearlinkSafeMap<std::string, uint32_t> g_pairConnPath;
static NearlinkSafeMap<std::string, std::string> g_dtfrCallNameMap;
static NearlinkSafeMap<std::string, bool> g_connectProfileTask; // <addr, whether call connectallprofile interface>
static bool g_sceneFlag = false;
static std::atomic<bool> g_pairFlag{false};
static bool g_disconnFlag = false;
static NearlinkSafeMap<std::string, std::string> g_dftUnpairTaskMap;
constexpr const uint16_t CDSM_AUDIO_DEVICE_NUM = 2;
static uint16_t g_streamCountSuccess = 0;
static bool g_airplaneModeFlag = false;

void DftCacheAcbStartConn(const std::string &addr)
{
    std::string time = GetMillTime(true);
    std::vector <DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(SLE_ACB_START_TIME, time));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCacheAcbDisConn(const std::string &addr, const std::string &callingName)
{
    DftCacheDisconnInfoMsg(addr, "", DISCONN_CLICK_SWITCH);
    DftCacheConnInfoTime(addr, UP_DISCONN_REQ_TIME);
}

void DftCacheAcbFinishConn(const std::string &addr, uint32_t result, int32_t reason, uint32_t count, uint16_t lcid)
{
    if (result == CM_STATE_DISCONNECTED && lcid == DFT_INVALID_LCID && reason == DFT_CONNECTION_TIMEOUT) {
        DftCacheDisconnRssi(addr, INVALID_DISCONN_RSSI);
    }

    DftConnStateChange(addr, result, reason);
    DftDealAccurateSearchConnInfo(addr, result, reason);
    DftCachePairAcbFinish(addr, result, reason, count);
    DftDealMultideviceInfo(addr, result);
}

void DftCacheBgStartConn(const std::string &addr, int32_t deviceAppearance)
{
    DftCacheConnInfoTime(addr, LAST_CONN_REQ_TIME);
    DftCachePairConnType(addr, NO_PAIR, BG_CONN);
    DftCacheAcbStartConn(addr);
    DftCachePairAppearance(addr, deviceAppearance);
}

static void DftCacheAppInfo(const std::string &app)
{
    std::string time = GetMillTime();
    DftSubEventRefC subFlow = {DFT_FLOW_STATE, nullptr, 0};
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    params.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    params.emplace_back(CreateRefParamC(SUB_STATE_FLOW, subFlow));
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), params.size());
}

static void DftCacheSwitchType(DftSwitchTypeEnum switchType)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateUi16ParamC(SWITCH_TYPE_INFO, switchType));
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), params.size());
}

static void DftCacheOperRes(DftSwitchOperRes operRes)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateUi16ParamC(SWITCH_OPER_RES, operRes));
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), params.size());
}

static void IsClearCacheStateFlow(void)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_stateFlow.size() > STATE_FLOW_MAX_LEN) {
        g_stateFlow.clear();
        g_stateFlow.shrink_to_fit();
    }
}

void DftCacheSwitchInfo(DftSwitchTypeEnum switchType, DftSwitchOperRes operRes, const std::string &app)
{
    DftCacheSwitchType(switchType);
    DftCacheOperRes(operRes);
    if (!app.empty()) {
        DftCacheAppInfo(app);
    }
}

void DftCacheStateFlow(int sleState)
{
    std::string reportstateFlow;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_stateFlow.empty()) {
            g_stateFlow.append(std::to_string(sleState));
        } else {
            g_stateFlow.append(STATE_FLOW_MARK).append(std::to_string(sleState));
        }
        reportstateFlow = g_stateFlow;
    }
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(STATE_FLOW, reportstateFlow));
    DftManagerCache(DFT_FLOW_STATE, params.data(), params.size());
}

void DftReportSwitchInfo(DftSwitchErrCode errCode, const std::string &infoMsg, DftSwitchTypeEnum switchType,
    DftSwitchOperRes operRes, const std::string &app)
{
    std::string time = GetMillTime();
    DftSubEventRefC subFlow = {DFT_FLOW_STATE, nullptr, 0};
    std::vector<DftParamC> params;
    params.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, errCode));
    params.emplace_back(CreateStrParamC(SWITCH_INFO_MSG, infoMsg));
    params.emplace_back(CreateRefParamC(SUB_STATE_FLOW, subFlow));
    params.emplace_back(CreateStrParamC(SWITCH_END_TIME, time));
    if (!app.empty()) {
        params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    }
    if (switchType != INVALIDVALUE) {
        params.emplace_back(CreateUi16ParamC(SWITCH_TYPE_INFO, switchType));
        params.emplace_back(CreateUi16ParamC(SWITCH_OPER_RES, operRes));
    }
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());
    IsClearCacheStateFlow();
}

void DftCachePeerName(const std::string &addr, const std::string &name)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateStrParamC(PEER_INFO_NAME, name));
    DftManagerCache(DFT_PEER_INFO, params.data(), params.size());
}

void DftCachePeerAppearance(const std::string &addr, int32_t appearance)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateI32ParamC(PEER_INFO_APPEARANCE, appearance));
    DftManagerCache(DFT_PEER_INFO, params.data(), params.size());
}

void DftCachePeerManufacturer(const std::string &addr, const std::string &manufacturer)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateStrParamC(PEER_INFO_MANUFACTURER, manufacturer));
    DftManagerCache(DFT_PEER_INFO, params.data(), params.size());
}

void DftCacheHidState(const std::string &addr, int state)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateUi16ParamC(PEER_HID_STATE, state));
    DftManagerCache(DFT_PEER_INFO, params.data(), params.size());
}

void DftCachePeerInfoTime(const std::string &addr, DftPeerInfoParamEnum paramId)
{
    std::string time = GetMillTime();
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateStrParamC(paramId, time));
    DftManagerCache(DFT_PEER_INFO, params.data(), params.size());
}

void DftCacheConnInfoTime(const std::string &addr, DftConnInfoParamEnum paramId)
{
    std::string time = GetMillTime();
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(CONN_INFO_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(CONN_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateStrParamC(paramId, time));
    DftManagerCache(DFT_CONN_INFO, params.data(), params.size());
}

static void DftReportDisconn(const std::string &addr, int32_t reason)
{
    std::vector<DftParamC> refKeys;
    refKeys.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
    refKeys.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    DftSubEventRefC peerInfoRef = {DFT_PEER_INFO, refKeys.data(), refKeys.size()};
    DftSubEventRefC connInfoRef = {DFT_CONN_INFO, refKeys.data(), refKeys.size()};

    std::string time = GetMillTime(true);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DISCONN_PEER_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(DISCONN_PEER_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateI32ParamC(DISCONN_ERRORCODE, reason));
    params.emplace_back(CreateStrParamC(DISCONN_TIME, time));
    params.emplace_back(CreateRefParamC(SUB_PEER_INFO, peerInfoRef));
    params.emplace_back(CreateRefParamC(SUB_CONN_INFO, connInfoRef));
    {
        std::lock_guard<std::mutex> lock(g_flagMutex);
        if (!g_sceneFlag) {
            params.emplace_back(CreateUi16ParamC(DISCONN_SCENE, DISCONN_EXCEP));
        }
        g_disconnFlag = false;
        g_sceneFlag = false;
    }
    DftManagerReport(DFT_DISCONN_EXCEP, params.data(), params.size());
}

void DftConnStateChange(const std::string &addr, uint8_t state, uint32_t resaon)
{
    if (state == CM_STATE_CONNECTED) {
        DftCacheConnInfoTime(addr, CONN_COMP_TIME);
    } else if (state == CM_STATE_DISCONNECTED) {
        DftCacheConnInfoTime(addr, DISCONN_COMP_TIME);
        DftReportDisconn(addr, resaon);
    }
}

void DftCacheDisconnRssi(const std::string &addr, int32_t rssi)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DISCONN_PEER_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(DISCONN_PEER_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    params.emplace_back(CreateI32ParamC(DISCONN_DEVICE_RSSI, rssi));
    DftManagerCache(DFT_DISCONN_EXCEP, params.data(), params.size());
}

void DftCacheDisconnInfoMsg(const std::string &addr, const std::string &infoMsg, uint16_t secne)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DISCONN_PEER_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(DISCONN_PEER_TYPE, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    if (!infoMsg.empty()) {
        params.emplace_back(CreateStrParamC(DISCONN_INFO_MSG, infoMsg));
    }
    {
        std::lock_guard<std::mutex> lock(g_flagMutex);
        if (!g_disconnFlag) {
            params.emplace_back(CreateUi16ParamC(DISCONN_SCENE, secne));
            g_sceneFlag = true;
        }
        if (secne == DISCONN_CLICK_UNPAIR) {
            g_disconnFlag = true;
        }
    }
    DftManagerCache(DFT_DISCONN_EXCEP, params.data(), params.size());
}

void DftSetMeasureResultMap(const std::string &addr, bool value)
{
    g_measureResultMap.EnsureInsert(addr, value);
}

void DftSetAccurateSearchTaskMap(const std::string &addr)
{
    if (g_dftTaskName == DFT_ACCURATESEARCH_TASK) {
        g_dftTaskMap.EnsureInsert(addr, ACCURATE_SEARCH_CALLING_NAME);
        g_dftTaskName = DFT_DEFAULT;
    }
}

bool DftAccurateSearchTaskFind(const std::string &addr)
{
    bool result = g_dftTaskMap.Find([&addr](const std::string &first, const std::string &second) -> bool {
        return (addr == first) && (second == ACCURATE_SEARCH_CALLING_NAME);
    });
    return result;
}

void DftCacheMeasureEndTime(const std::string &addr)
{
    std::string time = GetMillTime();
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
    params.emplace_back(CreateStrParamC(MEASURE_END_TIME, time));
    DftManagerCache(DFT_ACCURATESEARCH, params.data(), params.size());
}

void DftCacheRssiValueInfo(const std::string &addr, int32_t rssiValue)
{
    if (DftAccurateSearchTaskFind(addr)) {
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateI32ParamC(RSSI_VALUE, rssiValue));
        DftManagerCache(DFT_ACCURATESEARCH, params.data(), params.size());
    }
}

void DftDealAccurateSearchScanInfo(const std::string &name, DftAccurateSearchPath path)
{
    if (name == ACCURATE_SEARCH_CALLING_NAME) {
        g_dftTaskName = DFT_ACCURATESEARCH_TASK;
    }
}

void DftCacheAccurateSearchConnInfo(const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchConnResult connResult)
{
    if (DftAccurateSearchTaskFind(addr)) {
        g_measurePathMap.EnsureInsert(addr, path);
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateUi16ParamC(PATH, path));
        params.emplace_back(CreateUi16ParamC(CONN_RESULT, connResult));
        DftManagerCache(DFT_ACCURATESEARCH, params.data(), params.size());
    }
}

void DftReportAccurateSearchConnFailInfo(const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchConnResult connResult, int32_t errorCode)
{
    if (DftAccurateSearchTaskFind(addr)) {
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateUi16ParamC(PATH, path));
        params.emplace_back(CreateUi16ParamC(CONN_RESULT, connResult));
        params.emplace_back(CreateI32ParamC(CONN_FAIL_ERROR_CODE, errorCode));
        DftManagerReport(DFT_ACCURATESEARCH, params.data(), params.size());
        g_measurePathMap.Erase(addr);
        g_dftTaskMap.Erase(addr);
    }
}

void DftReportAccurateSearchDisConnInfo(const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchConnResult connResult, std::int32_t errorCode)
{
    if (DftAccurateSearchTaskFind(addr)) {
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateUi16ParamC(PATH, path));
        params.emplace_back(CreateUi16ParamC(CONN_RESULT, DFT_CONN_SUCCESS));  // 已在测距流程，连接流程必然成功
        params.emplace_back(CreateI32ParamC(DISCONN_ERROR_CODE, errorCode));
        DftManagerReport(DFT_ACCURATESEARCH, params.data(), params.size());
        g_measurePathMap.Erase(addr);
        g_dftTaskMap.Erase(addr);
    }
}

void DftDealAccurateSearchConnInfo(const std::string &addr, uint16_t connResult, int32_t discReason)
{
    if (DftAccurateSearchTaskFind(addr)) {
        if (connResult == CM_STATE_CONNECTED) {
            DftCacheAccurateSearchConnInfo(addr, DFT_ACB_CONN, DFT_CONN_SUCCESS);
        } else if (connResult == CM_STATE_DISCONNECTED) {
            int path;
            if (!g_measurePathMap.GetValue(addr, path)) {
                return;
            }
            if (path == DFT_MEASURE && discReason != CONNECTION_TERMINATED_BY_LOCAL_HOST) {
                DftReportAccurateSearchDisConnInfo(addr, DFT_MEASURE, DFT_CONN_FAILED, discReason);
            }
            if (path != DFT_MEASURE) {
                DftReportAccurateSearchConnFailInfo(addr, DFT_ACB_CONN, DFT_CONN_FAILED, discReason);
            }
        }
    }
}

void DftCacheAccurateSearchMeasureInfo(const std::string &name, const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchMeasureResult measureResult)
{
    if (name == ACCURATE_SEARCH_CALLING_NAME) {
        g_measurePathMap.EnsureInsert(addr, path);
        std::string time = GetMillTime();
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateUi16ParamC(PATH, path));
        params.emplace_back(CreateUi16ParamC(MEASURE_RESULT, measureResult));
        params.emplace_back(CreateStrParamC(MEASURE_START_TIME, time));
        DftManagerCache(DFT_ACCURATESEARCH, params.data(), params.size());
    }
}

void DftReportAccurateSearchMeasureInfo(const std::string &name, const std::string &addr, DftAccurateSearchPath path,
    DftAccurateSearchMeasureResult measureResult, int32_t measureErrorCode)
{
    if (name == ACCURATE_SEARCH_CALLING_NAME) {
        g_measurePathMap.EnsureInsert(addr, path);
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(PEER_ADDR, addr));
        params.emplace_back(CreateUi16ParamC(PATH, path));
        params.emplace_back(CreateUi16ParamC(MEASURE_RESULT, measureResult));
        params.emplace_back(CreateI32ParamC(MEASURE_FAIL_ERROR_CODE, measureErrorCode));
        DftManagerReport(DFT_ACCURATESEARCH, params.data(), params.size());
        g_measureResultMap.Erase(addr);
    }
}

void DftDealAccurateSearchMeasureInfo(const std::string &name, const std::string &addr)
{
    if (name == ACCURATE_SEARCH_CALLING_NAME) {
        bool measureResult = false;
        g_measureResultMap.GetValue(addr, measureResult);
        if (measureResult) {
            DftCacheMeasureEndTime(addr);
            DftReportAccurateSearchMeasureInfo(name, addr, DFT_MEASURE, DFT_MEASURE_FINISHED, MEASURE_SUCCESS);
        } else {
            DftReportAccurateSearchMeasureInfo(name, addr, DFT_MEASURE, DFT_MEASURE_TIME_OUT, MEASURE_TIMEOUT);
        }
    }
}

void DftCachePairConnPath(const std::string &addr, uint32_t connPath)
{
    g_pairConnPath.EnsureInsert(addr, connPath);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(SLE_CONN_PATH, connPath));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairConnTime(const std::string &addr, uint32_t currentConnPath, uint32_t pairTimeType)
{
    if (pairTimeType < SLE_ACB_START_TIME || pairTimeType > SLE_PROFILE_FINISH_TIME) {
        return;
    }
    uint32_t checkPath;
    uint16_t connType;
    if (pairTimeType != SLE_ACB_START_TIME && pairTimeType != SLE_ACB_FINISH_TIME) {
        if (!g_pairConnPath.GetValue(addr, checkPath)) {
            return;
        }
    }
    connType = (g_pairFlag.load()) ? SLE_CONN_RESULT : SLE_PAIR_RESULT;
    if (currentConnPath == PAIR_CONN_PATH_SSAP && pairTimeType == SLE_SSAP_START_TIME &&
        checkPath == PAIR_CONN_PATH_ACB) {
            DftReportPairSuccess(addr, connType);
            g_pairFirstTimeMap.Erase(addr);
            g_pairConnPath.Erase(addr);
            g_pairFlag.store(false);
            return;
    }
    DftCachePairConnPath(addr, currentConnPath);
    std::string time = GetMillTime(true);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(pairTimeType, time));
    if (g_pairFlag.load()) {
        params.emplace_back(CreateUi16ParamC(SLE_CONN_RESULT, PAIR_RESULT_SUCCESS));
    } else {
        params.emplace_back(CreateUi16ParamC(SLE_PAIR_RESULT, PAIR_RESULT_SUCCESS));
    }
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairDeviceCount(const std::string &addr, uint32_t count)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(SLE_CONN_DEV_COUNT, count));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCacheSecurityPairType(const std::string &addr, bool isSecurityPairType)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    if (isSecurityPairType) {
        params.emplace_back(CreateStrParamC(SLE_PAIR_PROCESS_TYPE, SECURITY_PAIR_TYPE));
    } else {
        params.emplace_back(CreateStrParamC(SLE_PAIR_PROCESS_TYPE, NO_SECURITY_PAIR_TYPE));
    }
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairConnType(const std::string &addr, uint32_t pairType, uint32_t connType)
{
    if (connType == CLICK_CONN) {
        uint32_t isFirstTimeConn;
        if (g_pairFirstTimeMap.GetValue(addr, isFirstTimeConn)) {
            return;
        }
    }

    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    if (pairType == NO_PAIR) {
        params.emplace_back(CreateUi16ParamC(SLE_CONN_TYPE, connType));
        g_pairFlag.store(true);
    } else {
        g_pairFirstTimeMap.EnsureInsert(addr, 0);
        params.emplace_back(CreateUi16ParamC(SLE_PAIR_TYPE, pairType));
    }
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairAppearance(const std::string &addr, int32_t deviceAppearance)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(SLE_DEVICE_TYPE, static_cast<uint16_t>(deviceAppearance)));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairName(const std::string &addr, const std::string &name)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(SLE_DEVICE_NAME, name));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCachePairAcbFinish(const std::string &addr, uint32_t result, int32_t reason, uint32_t count)
{
    if (result == CM_STATE_CONNECTED) {
        DftCachePairConnTime(addr, PAIR_CONN_PATH_ACB, SLE_ACB_FINISH_TIME);
        DftCachePairDeviceCount(addr, count);
    } else if (result == CM_STATE_DISCONNECTED) {
        DftReportPairInfo(addr, PAIR_CONN_PATH_ACB, reason);
    }
}

void DftCacheSsapStart(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_SSAP, SLE_SSAP_START_TIME);
}

void DftCacheHidStart(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_HID, SLE_HID_START_TIME);
}

void DftCacheHidFinish(const std::string &addr, int32_t reason)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_HID, SLE_HID_FINISH_TIME);
}

void DftCacheDisStart(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_DIS, SLE_DIS_START_TIME);
}

void DftCacheDisFinish(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_DIS, SLE_DIS_FINISH_TIME);
}

void DftCacheIcceStart(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_ICCE, SLE_ICCE_START_TIME);
}

void DftCacheIcceFinish(const std::string &addr)
{
    DftCachePairConnTime(addr, PAIR_CONN_PATH_ICCE, SLE_ICCE_FINISH_TIME);
}

void DftReportPairInfo(const std::string &addr, uint32_t currentConnPath, int32_t reason, const std::string &detail)
{
    uint32_t preConnPath;
    uint16_t connType;
    if (!g_pairConnPath.GetValue(addr, preConnPath)) {
        return;
    }
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateI32ParamC(SLE_CONN_ERROR_CODE, reason));
    if (!detail.empty()) {
        params.emplace_back(CreateStrParamC(SLE_CONN_ERROR_REASON, detail));
    }
    connType = (g_pairFlag.load()) ? SLE_CONN_RESULT : SLE_PAIR_RESULT;
    if (reason != SLE_SUCCESS) {
        params.emplace_back(CreateUi16ParamC(connType, PAIR_RESULT_FAILED));
    }
    if (preConnPath >= PAIR_CONN_PATH_SSAP) {
        if (g_connectProfileTask.FindIf(addr)) {
            DftManagerReport(DFT_PAIR_EXCEP, params.data(), params.size());
        } else {
            DftReportPairSuccess(addr, connType);
        }
    } else {
        if (preConnPath == PAIR_CONN_PATH_ENCYP) {
            DftReportPairSuccess(addr, connType);
        } else {
            DftManagerReport(DFT_PAIR_EXCEP, params.data(), params.size());
        }
    }
    g_pairFirstTimeMap.Erase(addr);
    g_pairConnPath.Erase(addr);
    g_pairFlag.store(false);
    g_connectProfileTask.Erase(addr);
}

void DftCachePairRssiAndChannelNoise(const std::string &addr, int32_t rssi, std::string noise)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateI32ParamC(SLE_DISCON_RSSI, rssi));
    params.emplace_back(CreateStrParamC(SLE_NOISE_VALUE, noise));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftCacheDisconChipInfo(const std::string &addr, int32_t rssi, std::string channelNoise)
{
    DftCachePairRssiAndChannelNoise(addr, rssi, channelNoise);
    DftCacheDisconnRssi(addr, rssi);
    DftCacheRssiValueInfo(addr, rssi);
}

void DftCacheCallingName(const std::string &addr, const std::string &callingName)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(SLE_APP_NAME, callingName));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftSetConnectProfileTaskFlag(const std::string &addr)
{
    g_connectProfileTask.EnsureInsert(addr, true);
}

void DftDealEncryptFailEvent(const std::string &addr, int32_t result)
{
    DftCacheDisconnInfoMsg(addr, DFT_DISCONN_ENCRYFAIL);
    DftReportPairInfo(addr, PAIR_CONN_PATH_ENCYP, result, DFT_DISCONN_ENCRYFAIL);
}

void DftReportPairSuccess(const std::string &addr, uint16_t connType)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, addr));
    params.emplace_back(CreateI32ParamC(SLE_CONN_ERROR_CODE, 0));
    params.emplace_back(CreateUi16ParamC(connType, 0));
    DftManagerReport(DFT_PAIR_EXCEP, params.data(), params.size());
}

void DftReportAudioError(const std::string &addr, const uint16_t errorCode, const uint16_t subErrorCode,
    const std::string &extraParam)
{
    RawAddress device(addr);
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(addr), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(AUDIO_PROFILE_EXCEP_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(AUDIO_PROFILE_EXCEP_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(AUDIO_PROFILE_EXCEP_DEVICE_APPEARANCE, static_cast<uint16_t>(appearance)));
    params.emplace_back(CreateUi16ParamC(AUDIO_PROFILE_EXCEP_ERROR_CODE, errorCode));
    params.emplace_back(CreateUi16ParamC(AUDIO_PROFILE_EXCEP_SUB_ERROR_CODE, subErrorCode));
    params.emplace_back(CreateStrParamC(AUDIO_PROFILE_EXCEP_EXTRA_PARAM, extraParam));
    DftManagerReport(DFT_AUDIO_PROFILE_EXCEP, params.data(), params.size());
}

void DftCacheUnPairInfo(const std::string &addr, int32_t deviceAppearance,
    const std::string &callingName, int32_t state, int32_t pairState)
{
    if (pairState != PAIRED_STATE) {
        HILOGE("current pair state is not paired");
        return;
    }
    std::string time = GetMillTime(true);
    std::vector<DftParamC> params;
    g_dftUnpairTaskMap.EnsureInsert(addr, UNPAIR_TASK_NAME);
    params.emplace_back(CreateUi16ParamC(UNPAIR_SCENE, UNPAIR_SCENE_OTHER));
    params.emplace_back(CreateStrParamC(UNPAIR_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(UNPAIR_START_TIME, time));
    params.emplace_back(CreateUi16ParamC(UNPAIR_DEVICE_TYPE, static_cast<uint16_t>(deviceAppearance)));
    params.emplace_back(CreateStrParamC(UNPAIR_APP_NAME, callingName));
    params.emplace_back(CreateUi16ParamC(UNPAIR_CONSTATE, static_cast<uint16_t>(state)));
    DftManagerCache(DFT_UNPAIR_EXCEP, params.data(), params.size());
}

bool DftUnpairTaskFind(const std::string &addr)
{
    bool result = g_dftUnpairTaskMap.Find([&addr](const std::string &first, const std::string &second) -> bool {
        return (addr == first) && (second == UNPAIR_TASK_NAME);
    });
    return result;
}

void DftReportUnPairFailInfo(const std::string &addr, int32_t status, uint16_t result)
{
    if (DftUnpairTaskFind(addr)) {
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(UNPAIR_DEVICE_ADDR, addr));
        params.emplace_back(CreateI32ParamC(UNPAIR_ERRORCODE, status));
        params.emplace_back(CreateUi16ParamC(UNPAIR_RESULT, result));
        DftManagerReport(DFT_UNPAIR_EXCEP, params.data(), params.size());
        g_dftUnpairTaskMap.Erase(addr);
    }
}

void DftReportUnPairSuccessInfo(const std::string &addr, int32_t status, uint16_t result)
{
    if (DftUnpairTaskFind(addr)) {
        std::string time = GetMillTime(true);
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(UNPAIR_DEVICE_ADDR, addr));
        params.emplace_back(CreateStrParamC(UNPAIR_END_TIME, time));
        params.emplace_back(CreateI32ParamC(UNPAIR_ERRORCODE, status));
        params.emplace_back(CreateUi16ParamC(UNPAIR_RESULT, result));
        DftManagerReport(DFT_UNPAIR_EXCEP, params.data(), params.size());
        g_dftUnpairTaskMap.Erase(addr);
    }
}

void DftDealUnPairLinkKeyInfo(const std::string &addr, bool ret)
{
    if (DftUnpairTaskFind(addr)) {
        std::string time = GetMillTime(true);
        std::vector<DftParamC> params;
        params.emplace_back(CreateStrParamC(UNPAIR_DEVICE_ADDR, addr));
        params.emplace_back(CreateStrParamC(UNPAIR_DELETEKEY_TIME, time));
        params.emplace_back(CreateUi16ParamC(UNPAIR_DELETEKEY_RESULT, static_cast<uint16_t>(!ret)));
        DftManagerCache(DFT_UNPAIR_EXCEP, params.data(), params.size());
        if (!ret) {
            DftReportUnPairFailInfo(addr, UNPAIR_DELLINKKEY_ERROR, 1);
        }
    }
}

void DftCacheMultideviceInfo(const std::string &addr, const std::string &callingName, uint16_t serviceState)
{
    std::string time = GetMillTime(true);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(CONN_MODULE_NAME, callingName));
    params.emplace_back(CreateStrParamC(DEVICE_UPDATE_TIME, time));
    params.emplace_back(CreateUi16ParamC(DEVICE_SERVICE_STATE, serviceState));
    DftManagerReport(DFT_MULTIDEVICE_CONN, params.data(), params.size());
}

void DftDealMultideviceInfo(const std::string &addr, uint32_t result)
{
    std::string time = GetMillTime(true);
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(DEVICE_UPDATE_TIME, time));
    if (result == CM_STATE_CONNECTED) {
        params.emplace_back(CreateStrParamC(DEVICE_CONN_LAST_TIME, time));
        params.emplace_back(CreateUi16ParamC(DEVICE_CONN_STATE, MUIDEVICE_CONNECTED));
        DftManagerReport(DFT_MULTIDEVICE_CONN, params.data(), params.size());
    } else if (result == CM_STATE_DISCONNECTED) {
        params.emplace_back(CreateStrParamC(DEVICE_DISCONN_LAST_TIME, time));
        params.emplace_back(CreateUi16ParamC(DEVICE_CONN_STATE, MUIDEVICE_DISCONNECTED));
        DftManagerReport(DFT_MULTIDEVICE_CONN, params.data(), params.size());
    }
}

void DftCachePeerInfo(const std::string &addr, const std::string &name, int32_t appearance)
{
    // PEER_INFO子事件
    DftCachePeerName(addr, name);
    DftCachePeerAppearance(addr, appearance);
    // NEARLINK_PAIR_EXCEPTION 事件
    DftCachePairAppearance(addr, appearance);
    DftCachePairName(addr, name);
    DftCacheSecurityPairType(addr, false);
}

void DftCacheAppearance(const std::string &addr, int32_t appearance)
{
    DftCachePeerAppearance(addr, appearance);   // PEER_INFO子事件
    DftCachePairAppearance(addr, appearance);   // NEARLINK_PAIR_EXCEPTION 事件
}

void DftCacheName(const std::string &addr, const std::string &name)
{
    DftCachePeerName(addr, name);   // PEER_INFO子事件
    DftCachePairName(addr, name);   // NEARLINK_PAIR_EXCEPTION 事件
}

void DftReportDtfrStatisPortInfo(const std::string &callName, int32_t scene)
{
    std::vector<DftParamC> params;
    std::string time = GetMillTime();
    params.emplace_back(CreateStrParamC(TIMESTAMP, time));
    params.emplace_back(CreateStrParamC(CALLING_NAME, callName));
    params.emplace_back(CreateI32ParamC(SCENE_CODE, scene));
    params.emplace_back(CreateI32ParamC(SUB_SCENE_CODE, DTFR_SUCC));
    DftManagerReport(DFT_DATATRANSFER_STATIS, params.data(), params.size());
}

void DftDtfrRecordCallName(const std::string &addr, const std::string &callName)
{
    g_dtfrCallNameMap.EnsureInsert(addr, callName);
}

void DftReportDtfrStatisInfo(uint8_t state, const std::string &addr, const std::string &uuid)
{
    int32_t scene;
    int32_t subScene;
    switch (state) {
        case DTFR_CHANNEL_ESTABLISHED:
            scene = DTFR_CONNECT;
            subScene = DTFR_SUCC;
            break;
        case DTFR_CHANNEL_ESTABLISH_FAIL:
            scene = DTFR_CONNECT;
            subScene = DTFR_FAILED;
            DftReportDtfrExcepInfo(addr, uuid, EXCEP_CONNECT, CONNECT_REMOTE_FAIL);
            break;
        case DTFR_CHANNEL_RELEASED:
            scene = DTFR_DISCONNECT;
            subScene = DTFR_SUCC;
            break;
        case DTFR_CHANNEL_RELEASE_FAIL:
            scene = DTFR_DISCONNECT;
            subScene = DTFR_FAILED;
            break;
        default:
            return;
    }
    int32_t appearance;
    std::string peerName;
    std::string callName;
    std::vector<DftParamC> params;
    std::string time = GetMillTime();
    RawAddress device(addr);
    g_dtfrCallNameMap.GetValue(addr, callName);
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerName, appearance);
    params.emplace_back(CreateStrParamC(DTFR_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(TIMESTAMP, time));
    params.emplace_back(CreateStrParamC(DEVICE_NAME, peerName));
    params.emplace_back(CreateI32ParamC(DEVICE_APPEARANCE, appearance));
    if (!callName.empty()) {
        params.emplace_back(CreateStrParamC(CALLING_NAME, callName));
    }
    params.emplace_back(CreateI32ParamC(SCENE_CODE, scene));
    params.emplace_back(CreateI32ParamC(SUB_SCENE_CODE, subScene));
    DftManagerReport(DFT_DATATRANSFER_STATIS, params.data(), params.size());
    g_dtfrCallNameMap.Erase(addr);
}

void DftReportDtfrExcepInfo(const std::string &addr, const std::string &uuid, int32_t scene, int32_t errCode)
{
    int32_t appearance;
    std::string name;
    std::string callName;
    std::vector<DftParamC> params;
    std::string time = GetMillTime();

    RawAddress device(addr);
    g_dtfrCallNameMap.GetValue(addr, callName);
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, name, appearance);
    int32_t type = (uuid == DFT_UUID_ICCE) ? DTFR_ICCE : DTFR_PROXY;
    params.emplace_back(CreateStrParamC(DTFREXCEP_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(DTFREXCEP_TIMESTAMP, time));
    params.emplace_back(CreateStrParamC(DTFREXCEP_DEVICE_NAME, name));
    params.emplace_back(CreateI32ParamC(DTFREXCEP_DEVICE_APPEARANCE, appearance));
    if (!callName.empty()) {
        params.emplace_back(CreateStrParamC(DTFREXCEP_CALLING_NAME, callName));
    }
    params.emplace_back(CreateI32ParamC(DTFREXCEP_TYPE, type));
    params.emplace_back(CreateI32ParamC(DTFREXCEP_ERROR_CODE, scene));
    params.emplace_back(CreateI32ParamC(DTFREXCEP_SUB_ERR_CODE, errCode));
    DftManagerReport(DFT_DATATRANSFER_EXCEP, params.data(), params.size());
    if (scene == EXCEP_CONNECT && errCode == CONNECT_REMOTE_FAIL) {
        return;
    }
    g_dtfrCallNameMap.Erase(addr);
}

static void DftHandleAudioExcepInner(AudioHeadsetExcep& audioHeadsetData, DftAudioExceptionData &dftAudioExcepData)
{
    audioHeadsetData.audioSceneCode = static_cast<uint16_t>(dftAudioExcepData.audioExceptionType &
        AUDIO_EXCEP_SCENE_MASK);
    audioHeadsetData.errorCode = static_cast<uint16_t>((dftAudioExcepData.audioExceptionType &
                                              AUDIO_EXCEP_ERROR_MASK) >> AUDIO_FOUR_SHIFT);

    audioHeadsetData.audioSourceRssi = static_cast<int32_t>(static_cast<int8_t>(dftAudioExcepData.audioQualityRssi &
                                                                                AUDIO_RSSI_SOURCE_MASK));
    audioHeadsetData.otherEarRssi = static_cast<int32_t>(static_cast<int8_t>((dftAudioExcepData.audioQualityRssi &
                                                         AUDIO_RSSI_EAR_MASK) >> AUDIO_EIGHT_SHIFT));

    audioHeadsetData.wrongPackRate = static_cast<uint16_t>(dftAudioExcepData.audioQualityRetrans &
        AUDIO_RETRANS_CRC_MASK);
    audioHeadsetData.misorderCntRxseq = static_cast<uint16_t>((dftAudioExcepData.audioQualityRetrans &
                                                     AUDIO_RETRANS_RXSEQ_MASK) >> AUDIO_EIGHT_SHIFT);
    audioHeadsetData.nosyncRate = static_cast<uint16_t>((dftAudioExcepData.audioQualityRetrans &
                                               AUDIO_RETRANS_NOSYNC_MASK) >> AUDIO_SIXTEEN_SHIFT);
    audioHeadsetData.freqBand = static_cast<uint16_t>((dftAudioExcepData.audioQualityRetrans &
                                                AUDIO_EXCEP_BAND_MASK) >> AUDIO_TWENTY_FOUR_SHIFT);

    audioHeadsetData.lnaSwitch = static_cast<uint16_t>(dftAudioExcepData.audioQualityScene & AUDIO_SCENE_LNA_MASK);
    audioHeadsetData.doubleTunedState = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                                     AUDIO_SCENE_DTN_MASK) >> AUDIO_TWO_SHIFT);
    audioHeadsetData.codecType = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                              AUDIO_SCENE_CODEC_MASK) >> AUDIO_FOUR_SHIFT);
    audioHeadsetData.bitrateInfo = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                                AUDIO_SCENE_BIT_MASK) >> AUDIO_TEN_SHIFT);
    audioHeadsetData.userSceneCode = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                                  AUDIO_SCENE_USERSCENE_MASK) >> AUDIO_TWENTY_FOUR_SHIFT);
    audioHeadsetData.deviceNumInLink = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                                    AUDIO_SCENE_DEVICE_MASK) >> AUDIO_TWENTY_EIGHT_SHIFT);
    audioHeadsetData.audioRole = static_cast<uint16_t>((dftAudioExcepData.audioQualityScene &
                                                    AUDIO_SCENE_CENTRAL_MASK) >> AUDIO_THIRTY_ONE_SHIFT);

    audioHeadsetData.strongChannelNum = static_cast<uint16_t>(dftAudioExcepData.audioQualityAfh &
        AUDIO_AFH_STRONG_MASK);
    audioHeadsetData.midChannelNum = static_cast<uint16_t>((dftAudioExcepData.audioQualityAfh &
                                                  AUDIO_AFH_MID_MASK) >> AUDIO_EIGHT_SHIFT);
    audioHeadsetData.weakChannelNum = static_cast<uint16_t>((dftAudioExcepData.audioQualityAfh &
                                                   AUDIO_AFH_WEAK_MASK) >> AUDIO_SIXTEEN_SHIFT);
    audioHeadsetData.noiseAvgValue = static_cast<int32_t>(static_cast<int8_t>((dftAudioExcepData.audioQualityAfh &
                                                          AUDIO_AFH_NOSISE_MASK) >> AUDIO_TWENTY_FOUR_SHIFT));
}

static void AssembleStutteringData(cJSON *root, const AudioHeadsetExcep &audioHeadsetData,
    const std::string &localAddr)
{
    cJSON_AddStringToObject(root, "mobileDeviceAddr", GetEncryptAddr(localAddr).c_str());
    cJSON_AddStringToObject(root, "mobileDeviceName", ""); // 涉及隐私，暂不上报
    cJSON_AddStringToObject(root, "mobileDeviceVersion", "");
    cJSON_AddStringToObject(root, "earbudDeviceAddr", GetEncryptAddr(audioHeadsetData.audioDeviceAddr).c_str());
    cJSON_AddStringToObject(root, "earbudDeviceName", ""); // 涉及隐私，暂不上报
    cJSON_AddStringToObject(root, "earbudDeviceVersion", "");
    cJSON_AddNumberToObject(root, "audioSceneCode", audioHeadsetData.audioSceneCode);
    cJSON_AddNumberToObject(root, "errorCode", audioHeadsetData.errorCode);
	cJSON_AddNumberToObject(root, "audioSourceSyncRssi", audioHeadsetData.audioSourceRssi);
	cJSON_AddNumberToObject(root, "audioSourceAsyncRssi", audioHeadsetData.audioSourceRssi); // echo44暂不支持区分同步异步的rssi
	cJSON_AddNumberToObject(root, "otherEarRssi", audioHeadsetData.otherEarRssi);
	cJSON_AddNumberToObject(root, "wrongPackRate", audioHeadsetData.wrongPackRate);
	cJSON_AddNumberToObject(root, "misorderCntRxseq", audioHeadsetData.misorderCntRxseq);
	cJSON_AddNumberToObject(root, "nosyncRate", audioHeadsetData.nosyncRate);
	cJSON_AddNumberToObject(root, "freqBand", audioHeadsetData.freqBand & AUDIO_FREQ_BAND_FREQ_MASK);
	cJSON_AddNumberToObject(root, "syncLinkTxPower",
        (audioHeadsetData.freqBand & AUDIO_FREQ_BAND_ISO_PWR_MASK) >> AUDIO_TWO_SHIFT);
	cJSON_AddNumberToObject(root, "asyncLinkTxPower",
        (audioHeadsetData.freqBand & AUDIO_FREQ_BAND_ACB_PWR_MASK) >> AUDIO_FIVE_SHIFT);
	cJSON_AddNumberToObject(root, "lnaSwitch", audioHeadsetData.lnaSwitch);
	cJSON_AddNumberToObject(root, "doubleTunedState", audioHeadsetData.doubleTunedState);
	cJSON_AddNumberToObject(root, "codecType", audioHeadsetData.codecType);
	cJSON_AddNumberToObject(root, "bitrateInfo", audioHeadsetData.bitrateInfo);
	cJSON_AddNumberToObject(root, "businessType", audioHeadsetData.userSceneCode);
	cJSON_AddNumberToObject(root, "deviceNumInLink", audioHeadsetData.deviceNumInLink);
	cJSON_AddNumberToObject(root, "twsMode", 0); // 星闪固定为0
	cJSON_AddNumberToObject(root, "audioRole", audioHeadsetData.audioRole);
	cJSON_AddNumberToObject(root, "strongChannelNum", audioHeadsetData.strongChannelNum);
	cJSON_AddNumberToObject(root, "midChannelNum", audioHeadsetData.midChannelNum);
	cJSON_AddNumberToObject(root, "weakChannelNum", audioHeadsetData.weakChannelNum);
	cJSON_AddNumberToObject(root, "noiseAvgValue", audioHeadsetData.noiseAvgValue);
	cJSON_AddNumberToObject(root, "commProtocol", COMM_PROTOCOL_NEARLINK);
	cJSON_AddNumberToObject(root, "wifiConnectionState", 0); // 与wifi模块耦合，暂不上报
	cJSON_AddNumberToObject(root, "wifiConnectionFreqBand", 0);
	cJSON_AddNumberToObject(root, "wifiConnectionBandWidth", 0);
}

void ReportStutteringData(AudioHeadsetExcep& audioHeadsetData, const std::string &localAddr)
{
    // 将 audioHeadsetData 转换为 cJSON 格式
    cJSON *root = cJSON_CreateObject();
    NL_CHECK_RETURN(root != nullptr, "[NEARLINK_CEMAP]Failed to create root cJSON object");
    AssembleStutteringData(root, audioHeadsetData, localAddr);

    // 将cJSON转为string
    char *jsonStr = cJSON_PrintUnformatted(root);
    if (jsonStr == nullptr) {
        HILOGE("[NEARLINK_CEMAP]Failed to print cJSON object");
        cJSON_Delete(root);
        return;
    }
    NearlinkCemapReportLoader::GetInstance().ReportCemapJsonPayloadAsync(jsonStr);
    cJSON_Delete(root);
    cJSON_free(jsonStr);
}

void DftHandleAudioExcep(const RawAddress &addr, DftAudioExceptionData &dftAudioExcepData,
    const std::string &localAddr)
{
    AudioHeadsetExcep audioHeadsetData;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(addr.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(addr, peerDeviceName, appearance);
    audioHeadsetData.audioDeviceAddr = addr.GetAddress();
    audioHeadsetData.audioDeviceAppear = appearance;
    audioHeadsetData.audioDeviceName = peerDeviceName;
    DftHandleAudioExcepInner(audioHeadsetData, dftAudioExcepData);
    NearlinkDftManager *nearlinkDftManager = NearlinkDftManager::GetInstance();
    if (nearlinkDftManager == nullptr) {
        HILOGE("NearlinkDftManager is nullptr");
        return;
    }
    NearlinkHelper::NearlinkDftCommonEventHelper::PublishSleAudioExcepEvent(audioHeadsetData.audioDeviceAddr,
        audioHeadsetData.codecType, audioHeadsetData.bitrateInfo, audioHeadsetData.freqBand);
    DftReportAudioHeadSet(audioHeadsetData);
    
    // 上报卡顿事件众包
    ReportStutteringData(audioHeadsetData, localAddr);
}

void DftReportAudioHeadSet(AudioHeadsetExcep &audioHeadsetData)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(AUDIO_DEVICE_ADDR, audioHeadsetData.audioDeviceAddr));
    params.emplace_back(CreateI32ParamC(AUDIO_DEVICE_APPEARANCE, audioHeadsetData.audioDeviceAppear));
    params.emplace_back(CreateStrParamC(AUDIO_DEVICE_NAME, audioHeadsetData.audioDeviceName));
    params.emplace_back(CreateUi16ParamC(FREQ_BAND, audioHeadsetData.freqBand));
    params.emplace_back(CreateUi16ParamC(AUDIO_SCENE_CODE, audioHeadsetData.audioSceneCode));
    params.emplace_back(CreateUi16ParamC(USER_SCENE_CODE, audioHeadsetData.userSceneCode));
    params.emplace_back(CreateUi16ParamC(ERROR_CODE, audioHeadsetData.errorCode));
    params.emplace_back(CreateI32ParamC(AUDIO_SOURCE_RSSI, audioHeadsetData.audioSourceRssi));
    params.emplace_back(CreateI32ParamC(OTHER_EAR_RSSI, audioHeadsetData.otherEarRssi));
    params.emplace_back(CreateUi16ParamC(WRONG_PACKAGE_RATE, audioHeadsetData.wrongPackRate));
    params.emplace_back(CreateUi16ParamC(MISORDER_CNT_RXSEQ, audioHeadsetData.misorderCntRxseq));
    params.emplace_back(CreateUi16ParamC(NOSYNC_RATE, audioHeadsetData.nosyncRate));
    params.emplace_back(CreateUi16ParamC(LNA_SWITCH, audioHeadsetData.lnaSwitch));
    params.emplace_back(CreateUi16ParamC(DOUBLE_TUNED_STATE, audioHeadsetData.doubleTunedState));
    params.emplace_back(CreateUi16ParamC(CODEC_TYPE, audioHeadsetData.codecType));
    params.emplace_back(CreateUi16ParamC(BITRATE_INFO, audioHeadsetData.bitrateInfo));
    params.emplace_back(CreateUi16ParamC(DEVICE_NUM_IN_LINK, audioHeadsetData.deviceNumInLink));
    params.emplace_back(CreateUi16ParamC(STRONG_CHANNEL_NUM, audioHeadsetData.strongChannelNum));
    params.emplace_back(CreateUi16ParamC(MID_CHANNEL_NUM, audioHeadsetData.midChannelNum));
    params.emplace_back(CreateUi16ParamC(WEAK_CHANNEL_NUM, audioHeadsetData.weakChannelNum));
    params.emplace_back(CreateI32ParamC(NOISE_AVG_VALUE, audioHeadsetData.noiseAvgValue));
    DftManagerReport(DFT_AUDIO_HEADSET_EXCEP, params.data(), params.size());
}

void DftCacheStreamTime(const std::string &addr, int32_t timeType)
{
    std::string time = GetMillTime(true);
    std::vector <DftParamC> params;
    params.emplace_back(CreateStrParamC(STREAM_DEVICE_ADDR, addr));
    params.emplace_back(CreateStrParamC(timeType, time));
    DftManagerCache(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
}

void DftCacheStreamConfig(const std::string &addr, uint16_t reconfig, const std::string &configParam)
{
    std::string time = GetMillTime(true);
    std::vector <DftParamC> params;
    params.emplace_back(CreateStrParamC(STREAM_DEVICE_ADDR, addr));
    params.emplace_back(CreateUi16ParamC(STREAM_JUDGE_RECONFIG, reconfig));
    if (!configParam.empty()) {
        params.emplace_back(CreateStrParamC(STREAM_CONFIG_PARAM, configParam));
    }
    params.emplace_back(CreateStrParamC(STREAM_CONFIGURING_TIME, time));
    DftManagerCache(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
}

void DftCacheStreamOpen(const std::string &addr)
{
    DftCacheStreamTime(addr, STREAM_OPENING_TIME);
}

void DftCacheStreamStart(const std::string &addr)
{
    DftCacheStreamTime(addr, STREAM_STARTING_TIME);
}

void DftCacheStreamStop(const std::string &addr)
{
    DftCacheStreamTime(addr, STREAM_STOPPING_TIME);
}

void DftCacheStreamRelease(const std::string &addr)
{
    DftCacheStreamTime(addr, STREAM_RELEASING_TIME);
}

void DftCacheStreamInfo(DftAudioStreamInfo &info)
{
    int32_t appearance = -1;
    std::string name;
    RawAddress device(info.addr);
    std::vector <DftParamC> params;
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, name, appearance);
    params.emplace_back(CreateStrParamC(STREAM_DEVICE_ADDR, info.addr));
    params.emplace_back(CreateUi8ParamC(STREAM_RUNNING_TYPE, info.runningType));
    params.emplace_back(CreateUi8ParamC(STREAM_POINT_TYPE, info.pointType));
    params.emplace_back(CreateStrParamC(STREAM_DEVICE_NAME, name));
    params.emplace_back(CreateI32ParamC(STREAM_DEVICE_APPEARANCE, appearance));
    if (!info.startBuff.empty()) {
        params.emplace_back(CreateStrParamC(STREAM_START_BUFF_TYPE, info.startBuff));
    }
    if (!info.reportaddr.empty()) {
        params.emplace_back(CreateStrParamC(STREAM_REPORT_ADDR, info.reportaddr));
    }
    DftManagerCache(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
}

void DftCacheStreamASC(DftAudioStreamInfo info)
{
    switch (info.state) {
        case STREAM_ASC_CONFIGURING:
        case STREAM_ASC_RECONFIGURING:
        case STREAM_ASC_STOPPING: {
            DftCacheStreamInfo(info);
            if (info.state == STREAM_ASC_STOPPING) {
                DftCacheStreamStop(info.addr);
            } else {
                DftCacheStreamConfig(info.addr, info.state != STREAM_ASC_CONFIGURING, info.configparam);
            }
            break;
        }
        case STREAM_ASC_RELEASING:
            DftCacheStreamRelease(info.addr);
            break;
        case STREAM_ASC_OPENING:
            DftCacheStreamOpen(info.addr);
            break;
        case STREAM_ASC_STARTING:
            DftCacheStreamStart(info.addr);
            break;
        default:
            break;
    }
    return;
}

void DftReportStreamSuccess(const std::string &addr, bool cbkdiscon)
{
    std::vector <DftParamC> params;
    std::string time = GetMillTime(true);
    {
        std::lock_guard<std::mutex> lock(g_countMutex);
        g_streamCountSuccess++;
        params.emplace_back(CreateStrParamC(STREAM_DEVICE_ADDR, addr));
        if (cbkdiscon) {
            params.emplace_back(CreateUi16ParamC(STREAM_OPERATION_TYPE, g_streamCountSuccess));
            g_streamCountSuccess = 0;
        }
    }
    if (cbkdiscon) {
        params.emplace_back(CreateUi8ParamC(STREAM_OPERATOR_RESULT, STREAM_RESULT_SUCCESS));
        params.emplace_back(CreateStrParamC(STREAM_RESULT_TIME, time));
        DftManagerReport(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
    } else {
        DftManagerEraseCache(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
    }
    RawAddress device(addr);
    RawAddress reportAddr;
    DftDeviceManager::GetInstance().GetReportAddr(device, reportAddr);
}

void DftReportStreamFail(const std::string &addr, int32_t state, int32_t reason)
{
    std::string time = GetMillTime(true);
    std::vector <DftParamC> params;
    params.emplace_back(CreateStrParamC(STREAM_DEVICE_ADDR, addr));
    params.emplace_back(CreateI32ParamC(STREAM_ASC_STATE, state));
    params.emplace_back(CreateUi8ParamC(STREAM_OPERATOR_RESULT, STREAM_RESULT_FAILED));
    params.emplace_back(CreateI32ParamC(STREAM_OPERATOR_ERROR_CODE, reason));
    params.emplace_back(CreateStrParamC(STREAM_RESULT_TIME, time));
    DftManagerReport(DFT_AUDIO_STREAM_EXCEP, params.data(), params.size());
}

void DftReportStreamASC(const std::string &addr, int32_t state, uint8_t result, int32_t reason)
{
    if (result != STREAM_RESULT_SUCCESS) {
        DftReportStreamFail(addr, state, reason);
    } else {
        DftReportStreamSuccess(addr, state == STREAM_ASC_DISCONNECTED);
    }
}

void DftCacheCdsmInfo(const std::string &device, const std::string &reportAddr, const std::string &otherAddr)
{
    if (device.empty() || reportAddr.empty() || otherAddr.empty()) {
        HILOGE("cdsm dft param invalid.");
        return;
    }

    std::string memberAddrList = GetEncryptAddr(device) + "," + GetEncryptAddr(otherAddr);

    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(DEVICE_ADDR, device));
    params.emplace_back(CreateStrParamC(REPORT_ADDR, reportAddr));
    params.emplace_back(CreateUi16ParamC(NUMBER_OF_MEMBER, CDSM_AUDIO_DEVICE_NUM));
    params.emplace_back(CreateStrParamC(ADDR_OF_MEMBER, memberAddrList));
    DftManagerCache(DFT_PAIR_EXCEP, params.data(), params.size());
}

static uint32_t GetChoppyCnt(const QOSM_DftAudioStats *stats)
{
    uint32_t cnt = 0;
    cnt += stats->pcmWriteGtFtCnt;
    cnt += stats->encoderMemAllocFailedCnt;
    cnt += stats->encoderFailedCnt;
    cnt += stats->leftTxFlushCnt;
    cnt += stats->rightTxFlushCnt;
    return cnt;
}

static void PushStatsCodecParamParams(std::vector<DftParamC> &params, const QOSM_DftAudioStats *stats)
{
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_DURATION, stats->duration));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_L2HC_VERSION, stats->l2hcVersion));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_CODEC_TYPE, stats->codecType));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_SAMPLE_RATE, stats->sampleRate));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_BIT_DEPTH, stats->bitDepth));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_CHANNEL_MODE, stats->channelMode));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_DEVICE_NUM_IN_COOPERATION_SET, stats->deviceNumInCooperationSet));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_SDU_INTERVAL, stats->sduInterval));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_FLUSH_TIMEOUT, stats->flushTimeout));
    params.emplace_back(CreateUi8ParamC(NL_DFT_STATS_BURST_NUM, stats->burstNum));
    params.emplace_back(CreateUi16ParamC(NL_DFT_STATS_MAX_DOWN_BITRATE, stats->maxDownBitrate));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_MAX_DOWN_BITRATE_DURATION, stats->maxDownBitrateDuration));
}

static void PushStatsEncoderParams(std::vector<DftParamC> &params, const QOSM_DftAudioStats *stats)
{
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_PCM_WRITE_CNT, stats->pcmWriteCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_PCM_WRITE_MAX_INTERVAL, stats->pcmWriteMaxInterval));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_PCM_WRITE_AVG_INTERVAL, stats->pcmWriteAvgInterval));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_PCM_WRITE_GT_FT_CNT, stats->pcmWriteGtFtCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_ENCODER_MEM_ALLOC_FAILED_CNT, stats->encoderMemAllocFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_ENCODER_FAILED_CNT, stats->encoderFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_PKT_CNT, stats->leftTxPktCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_NO_LINK_DROP_CNT, stats->leftTxNoLinkDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_FLUSH_CNT, stats->leftTxFlushCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_UART_FAILED_CNT, stats->leftTxUartFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_FLOW_CTRL_CNT, stats->leftTxFlowCtrlCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_TX_FLOW_CTRL_MAX_PKT_CNT, stats->leftTxFlowCtrlMaxPktCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_PKT_CNT, stats->rightTxPktCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_NO_LINK_DROP_CNT, stats->rightTxNoLinkDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_FLUSH_CNT, stats->rightTxFlushCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_UART_FAILED_CNT, stats->rightTxUartFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_FLOW_CTRL_CNT, stats->rightTxFlowCtrlCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_TX_FLOW_CTRL_MAX_PKT_CNT, stats->rightTxFlowCtrlMaxPktCnt));
}

static void PushStatsDecoderParams(std::vector<DftParamC> &params, const QOSM_DftAudioStats *stats)
{
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_INTR_EVENT_ENQUEUE_FAILED_CNT,
        stats->intrEventEnqueueFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_INTR_DATA_ENQUEUE_FAILED_CNT, stats->intrDataEnqueueFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_DATA_INVALID_FORMAT_CNT, stats->dataInvalidFormatCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_BAD_PKT_CNT, stats->badPktCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_INVALID_CONN_HANDLE_CNT, stats->invalidConnHandleCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_DATA_MEM_ALLOC_FAILED_CNT,
        stats->leftDataMemAllocFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_OLD_DATA_DROP_CNT, stats->leftOldDataDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_DECODER_FAILED_CNT, stats->leftDecoderFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_PCM_SIZE_NOT_MATCH_CNT, stats->leftPcmSizeNotMatchCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_DECODED_PCM_DROP_CNT, stats->leftDecodedPcmDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_READ_PCM_LOSS_CNT, stats->leftReadPcmLossCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_LEFT_READ_PCM_MISORDER_CNT, stats->leftReadPcmMisorderCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_DATA_MEM_ALLOC_FAILED_CNT,
        stats->rightDataMemAllocFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_OLD_DATA_DROP_CNT, stats->rightOldDataDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_DECODER_FAILED_CNT, stats->rightDecoderFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_PCM_SIZE_NOT_MATCH_CNT, stats->rightPcmSizeNotMatchCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_DECODED_PCM_DROP_CNT, stats->rightDecodedPcmDropCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_READ_PCM_LOSS_CNT, stats->rightReadPcmLossCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_RIGHT_READ_PCM_MISORDER_CNT, stats->rightReadPcmMisorderCnt));
}

static void PushStatsBitrateParams(std::vector<DftParamC> &params, const QOSM_DftAudioStats *stats,
    const std::string &bitrateFailedReason)
{
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_BITRATE_UPGRADE_CNT, stats->bitrateUpgradeCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_BITRATE_DOWN_CNT, stats->bitrateDownCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_BITRATE_DOWN_FAILED_CNT, stats->bitrateDownFailedCnt));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_BITRATE_DOWN_SET_LABEL_FAILED_CNT,
        stats->bitrateDownSetLabelFailedCnt));
    params.emplace_back(CreateStrParamC(NL_DFT_STATS_BITRATE_DOWN_FAILED_REASON, bitrateFailedReason));
}

static void PushStatsBandParams(std::vector<DftParamC> &params, const QOSM_DftAudioStats *stats,
    const std::string &band2GInfo, const std::string &band5GInfo)
{
    params.emplace_back(CreateStrParamC(NL_DFT_STATS_CHANNEL_CONT_TIME_24G, band2GInfo));
    params.emplace_back(CreateStrParamC(NL_DFT_STATS_CHANNEL_CONT_TIME_58G, band5GInfo));
    params.emplace_back(CreateUi32ParamC(NL_DFT_STATS_CHANNEL_FREQ_BAND_CHANGE_TIMES, stats->bandChangeCnt));
}

extern "C" void DftReportAudioCodecExcep(const QOSM_DftAudioCodecExcep *excep)
{
    if (excep == NULL) {
        return;
    }
    std::vector<DftParamC> params;
    std::string excepTime = excep->time;
    params.emplace_back(CreateStrParamC(NL_DFT_CODEC_EXCEP_TIME, excepTime));
    params.emplace_back(CreateUi8ParamC(NL_DFT_CODEC_EXCEP_TYPE, excep->codecType));
    params.emplace_back(CreateUi8ParamC(NL_DFT_CODEC_EXCEP_ALGO, excep->codecAlgo));
    params.emplace_back(CreateI32ParamC(NL_DFT_CODEC_EXCEP_ERROR_CODE, excep->errorCode));
    DftManagerReport(DFT_QOSM_CODEC_EXCEP, params.data(), params.size());
}

extern "C" void DftReportAudioChoppyExcep(const QOSM_DftAudioChoppyExcep *excep)
{
    if (excep == NULL) {
        return;
    }

    std::vector<DftParamC> params;
    std::string choppyTime = excep->time;
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_TIME, choppyTime));
    params.emplace_back(CreateUi8ParamC(NL_DFT_CHOPPY_EXCEP_CNT, excep->choppyCnt));

    RawAddress device = RawAddress::ConvertToString(excep->deviceAddr, sizeof(excep->deviceAddr));
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateI32ParamC(NL_DFT_CHOPPY_EXCEP_DEVICE_TYPE, appearance));
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_DEVICE_NAME, peerDeviceName));

    RawAddress reportAddr;
    DftDeviceManager::GetInstance().GetReportAddr(device, reportAddr);
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_REPORT_ADDR, reportAddr.GetAddress()));

    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_SCENE_CODE, excep->sceneCode));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_SUBSCENE_CODE, excep->subsceneCode));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_BITRATE_INFO, excep->bitrate));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_FREQ_BAND, excep->freqBand));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_POWER_INFO, excep->powerInfo));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_MAC_INFO, excep->macInfo));
    params.emplace_back(CreateI32ParamC(NL_DFT_CHOPPY_EXCEP_PEER_RSSI, excep->peerRssi));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_TXFLUSH_NUM, excep->txflushNum));
    params.emplace_back(CreateUi32ParamC(NL_DFT_CHOPPY_EXCEP_ACK_RATE, excep->ackRate));

    std::string chan2gStatus = excep->channelStatus24g;
    std::string chan5gStatus = excep->channelStatus5g;
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_CHANNEL_STATUS_24G, chan2gStatus));
    params.emplace_back(CreateI32ParamC(NL_DFT_CHOPPY_EXCEP_NOISE_AVG_VALUE_24G, excep->noiseAvgValue24g));
    params.emplace_back(CreateStrParamC(NL_DFT_CHOPPY_EXCEP_CHANNEL_STATUS_5G, chan5gStatus));
    params.emplace_back(CreateI32ParamC(NL_DFT_CHOPPY_EXCEP_NOISE_AVG_VALUE_5G, excep->noiseAvgValue5g));

    DftManagerReport(DFT_QOSM_CHOPPY_EXCEP, params.data(), params.size());
}

extern "C" void DftReportAudioStats(const QOSM_DftAudioStats *stats)
{
    if (stats == NULL) {
        return;
    }

    uint32_t choppyCnt = GetChoppyCnt(stats);
    if (choppyCnt > 0) {
        QOSM_DftAudioChoppyExcep choppyExcep = {};
        if (strcpy_s(choppyExcep.time, sizeof(choppyExcep.time), stats->startTime) == EOK) {
            choppyExcep.choppyCnt = choppyCnt > UINT8_MAX ? UINT8_MAX : static_cast<uint8_t>(choppyCnt);
            DftReportAudioChoppyExcep(&choppyExcep);
        }
    }

    std::vector<DftParamC> params;
    std::string startTime = stats->startTime;
    params.emplace_back(CreateStrParamC(NL_DFT_STATS_START_TIME, startTime));
    PushStatsCodecParamParams(params, stats);
    PushStatsEncoderParams(params, stats);
    PushStatsDecoderParams(params, stats);

    // must using std::string to push string
    std::string bitrateFailedReason = stats->bitrateDownFailedReason;
    PushStatsBitrateParams(params, stats, bitrateFailedReason);

    std::string band2GInfo = stats->band2GInfo;
    std::string band5GInfo = stats->band5GInfo;

    bool isAirplaneOn = false;
    {
        std::lock_guard<std::mutex> lock(g_modeMutex);
        isAirplaneOn = g_airplaneModeFlag;
    }
    if (!isAirplaneOn) {
        size_t lastComma2G = band2GInfo.rfind(',');
        if (lastComma2G != std::string::npos) {
            band2GInfo = band2GInfo.substr(0, lastComma2G + 1) + "0}";
        }
        size_t lastComma5G = band5GInfo.rfind(',');
        if (lastComma5G != std::string::npos) {
            band5GInfo = band5GInfo.substr(0, lastComma5G + 1) + "0}";
        }
    }

    HILOGI("band2GInfo = %{public}s, band5GInfo = %{public}s", band2GInfo.c_str(), band5GInfo.c_str());

    PushStatsBandParams(params, stats, band2GInfo, band5GInfo);
    DftManagerReport(DFT_QOSM_STATS, params.data(), params.size());
}

void DftSetAirplaneMode(bool isOn)
{
    std::lock_guard<std::mutex> lock(g_modeMutex);
    g_airplaneModeFlag = isOn;
}

}  // namespace Nearlink
}  // namespace OHOS
