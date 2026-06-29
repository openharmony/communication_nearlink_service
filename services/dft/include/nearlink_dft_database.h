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

#ifndef NEARLINK_DFT_DATABASE_H
#define NEARLINK_DFT_DATABASE_H

#include "hisysevent_record_c.h"
#include "nearlink_dft_database_ue.h"
#include "nearlink_dft_database_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADDR_LEN 6

// Stores event parameter information.
typedef struct DftParamDB {
    char name[MAX_LENGTH_OF_PARAM_NAME];
    HiSysEventParamType type;
} DftParamDB;

// Stores event information.
typedef struct DftEventDB {
    char name[MAX_LENGTH_OF_EVENT_NAME];
    int keyButt;
    int paramButt;
    DftParamDB *params;
} DftEventDB;

typedef struct DftExcepTypeDB {
    char name[MAX_LENGTH_OF_EVENT_NAME];
    HiSysEventEventType eventType;
} DftExcepTypeDB;

typedef enum DftParamType : int {
    DFT_PARAM_TYPE_INVALID = 0,
    // add param type below this
    DFT_BOOL,
    DFT_UINT8,
    DFT_INT8,
    DFT_UINT16,
    DFT_INT16,
    DFT_UINT32,
    DFT_INT32,
    DFT_STRING,
    DFT_SUB_REF,
    // add param type above this
    DFT_TYPE_BUTT,
} DftParamType;

/*******************************************************************************
 *  All events enum, including UE, Excep and statistic, are defined here.
 ******************************************************************************/

// NEARLINK DFT EVENT ENUM
typedef enum DftEventEnum : int {
    DFT_EVENT_INVALID = 0,
    // add exception and ue event below this
    DFT_SWITCH_EXCEP,
    DFT_DISCONN_EXCEP,
    DFT_ACCURATESEARCH,
    DFT_PAIR_EXCEP,
    DFT_UNPAIR_EXCEP,
    DFT_MULTIDEVICE_CONN,
    DFT_STACK_SSAP_EVENT_EXCEP,
    DFT_PAIRING_START_UE,
    DFT_PAIRING_CANCEL_UE,
    DFT_PAIRING_CFM_UE,
    DFT_DIS_EXPOSED_EXCEP,
    DFT_DIS_EXPOSED_UE,
    DFT_ICCE_PROFILE_EXCEP,
    DFT_ICCE_PROFILE_UE,
    DFT_ICCE_EXPOSED_EXCEP,
    DFT_MEASURE_STATE_EVENT_UE,
    DFT_STACK_SM_G_NEGO_EXCEP,
    DFT_STACK_SM_T_NEGO_EXCEP,
    DFT_STACK_SM_G_AUTH_EXCEP,
    DFT_STACK_SM_T_AUTH_EXCEP,
    DFT_STACK_SM_ENCP_EXCEP,
    DFT_DATATRANSFER_STATIS,
    DFT_DATATRANSFER_EXCEP,
    DFT_AUDIO_HEADSET_EXCEP,
    DFT_AUDIO_SINK_DEVICE_UE,
    DFT_AUDIO_SOURCE_DEVICE_UE,
    DFT_AUDIO_MUTE_STREAM_UE,
    DFT_AUDIO_STREAM_EXCEP,
    DFT_AUDIO_PROFILE_EXCEP,
    DFT_QOSM_CODEC_EXCEP,
    DFT_QOSM_CHOPPY_EXCEP,
    DFT_QOSM_STATS,
    // add exception and ue event above this
    DFT_EXCEP_BUTT,
    // add sub event below this
    DFT_FLOW_STATE,
    DFT_PEER_INFO,
    DFT_CONN_INFO,
    // add sub event param above this
    DFT_EVENT_BUTT,
} DftEventEnum;

/*******************************************************************************
 *  Exception Event param index define below
 ******************************************************************************/

/*
    Parameter Index, Corresponding to g_templateOneExcepParam.
    If you have the same param structure as g_templateOneExcepParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum ExcepParamsTemplateOneEnum : int {
    EXCEP_TEMPLATE_ONE_INVALID = 0,
    // add key param below this
    // add key param above this
    EXCEP_TEMPLATE_ONE_KEY_BUTT,
    // add normal param below this
    EXCEP_TEMPLATE_ONE_ERROR_CODE,
    EXCEP_TEMPLATE_ONE_SUB_ERROR_CODE,
    EXCEP_TEMPLATE_ONE_TIME,
    // add normal param above this
    EXCEP_TEMPLATE_ONE_PARAM_BUTT,
} ExcepParamsTemplateOneEnum;

/*
    Parameter Index, Corresponding to g_templateTwoExcepParam.
    If you have the same param structure as g_templateTwoExcepParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum ExcepParamsTemplateTwoEnum : int {
    EXCEP_TEMPLATE_TWO_INVALID = 0,
    // add key param below this
    // add key param above this
    EXCEP_TEMPLATE_TWO_KEY_BUTT,
    // add normal param below this
    EXCEP_TEMPLATE_TWO_ERROR_CODE,
    EXCEP_TEMPLATE_TWO_SUB_ERROR_CODE,
    EXCEP_TEMPLATE_TWO_CALLING_NAME,
    EXCEP_TEMPLATE_TWO_TIME,
    // add normal param above this
    EXCEP_TEMPLATE_TWO_PARAM_BUTT,
} ExcepParamsTemplateTwoEnum;

typedef enum ExcepParamsTemplateThreeEnum : int {
    EXCEP_TEMPLATE_THREE_INVALID = 0,
    // add key param below this
    // add key param above this
    EXCEP_TEMPLATE_THREE_KEY_BUTT,
    // add normal param below this
    EXCEP_TEMPLATE_THREE_PEER_INFO,
    EXCEP_TEMPLATE_THREE_ERROR_CODE,
    EXCEP_TEMPLATE_THREE_SUB_ERROR_CODE,
    EXCEP_TEMPLATE_THREE_TIME,
    // add normal param above this
    EXCEP_TEMPLATE_THREE_PARAM_BUTT,
} ExcepParamsTemplateThreeEnum;

typedef enum ExcepParamsTemplateFourEnum : int {
    EXCEP_TEMPLATE_FOUR_INVALID = 0,
    // add key param below this
    EXCEP_TEMPLATE_FOUR_DEVICE_ADDR,
    // add key param above this
    EXCEP_TEMPLATE_FOUR_KEY_BUTT,
    // add normal param below this
    EXCEP_TEMPLATE_FOUR_DEVICE_NAME,
    EXCEP_TEMPLATE_FOUR_DEVICE_APPEARANCE,
    EXCEP_TEMPLATE_FOUR_ERROR_CODE,
    EXCEP_TEMPLATE_FOUR_SUB_ERROR_CODE,
    EXCEP_TEMPLATE_FOUR_TIME,
    // add normal param above this
    EXCEP_TEMPLATE_FOUR_PARAM_BUTT,
} ExcepParamsTemplateFourEnum;

typedef enum ExcepParamsTemplateFiveEnum : int {
    EXCEP_TEMPLATE_FIVE_INVALID = 0,
    // add key param below this
    EXCEP_TEMPLATE_FIVE_DEVICE_ADDR,
    // add key param above this
    EXCEP_TEMPLATE_FIVE_KEY_BUTT,
    // add normal param below this
    EXCEP_TEMPLATE_FIVE_DEVICE_NAME,
    EXCEP_TEMPLATE_FIVE_DEVICE_APPEARANCE,
    EXCEP_TEMPLATE_FIVE_ERROR_CODE,
    EXCEP_TEMPLATE_FIVE_SUB_ERROR_CODE,
    EXCEP_TEMPLATE_FIVE_CALLING_NAME,
    EXCEP_TEMPLATE_FIVE_TIME,
    // add normal param above this
    EXCEP_TEMPLATE_FIVE_PARAM_BUTT,
} ExcepParamsTemplateFiveEnum;

// NEARLINK_SWITCH_EXCEPTION param enum
typedef enum DftSwitchExcepParamEnum : int {
    SWITCH_INVALID = 0,
    // add key param below this
    // add key param above this
    SWITCH_KEY_BUTT,
    // add normal param below this
    SWITCH_TYPE_INFO,
    SWITCH_OPER_RES,
    SWITCH_ERR_CODE,
    SWITCH_INFO_MSG,
    SWITCH_USED_APP,
    SWITCH_START_TIME,
    SWITCH_END_TIME,
    SUB_STATE_FLOW,
    // add normal param above this
    SWITCH_PARAM_BUTT,
} DftSwitchExcepParamEnum;

// NEARLINK_DISCONN_EXCEPTION param enum
typedef enum DftDisconnExcepParamEnum : int {
    DISCONN_INVALID = 0,
    // add key param below this
    DISCONN_PEER_ADDR,
    DISCONN_PEER_TYPE,
    // add key param above this
    DISCONN_KEY_BUTT,
    // add normal param below this
    DISCONN_SCENE,
    DISCONN_ERRORCODE,
    DISCONN_INFO_MSG,
    DISCONN_TIME,
    DISCONN_DEVICE_RSSI,
    SUB_PEER_INFO,
    SUB_CONN_INFO,
    DISCONN_PARAM_BUTT,
    // add normal param above this
} DftDisconnExcepParamEnum;

// NEARLINK NEARLINK_PAIR_INFO param enum
typedef enum DftPairExcepParamEnum : int {
    PAIR_INVALID = 0,
    // add key param below this
    SLE_DEVICE_ADDR,
    // add key param above this
    PAIR_KEY_BUTT,
    // add normal param below this
    SLE_DEVICE_TYPE,
    SLE_DEVICE_NAME,
    SLE_APP_NAME,
    SLE_PAIR_PROCESS_TYPE,
    SLE_CONN_PATH,
    SLE_ACB_START_TIME,
    SLE_ACB_FINISH_TIME,
    SLE_PAIR_START_TIME,
    SLE_PAIR_FINISH_TIME,
    SLE_SSAP_START_TIME,
    SLE_SSAP_FINISH_TIME,
    SLE_DIS_START_TIME,
    SLE_DIS_FINISH_TIME,
    SLE_HID_START_TIME,
    SLE_HID_FINISH_TIME,
    SLE_ICCE_START_TIME,
    SLE_ICCE_FINISH_TIME,
    SLE_PROFILE_START_TIME,
    SLE_PROFILE_FINISH_TIME,
    SLE_CONN_ERROR_CODE,
    SLE_CONN_DEV_COUNT,
    SLE_PAIR_TYPE,
    SLE_CONN_TYPE,
    SLE_CONN_RESULT,
    SLE_PAIR_RESULT,
    SLE_CONN_ERROR_REASON,
    SLE_DISCON_RSSI,
    SLE_NOISE_VALUE,
    REPORT_ADDR,
    NUMBER_OF_MEMBER,
    ADDR_OF_MEMBER,
    // add normal param above this
    PAIR_PARAM_BUTT,
} DftPairExcepParamEnum;

// NEARLINK NEARLINK_UNPAIR_INFO param enum
typedef enum DftUnPairExcepParamEnum : int {
    UNPAIR_INVALID = 0,
    // add key param below this
    UNPAIR_DEVICE_ADDR,
    // add key param above this
    UNPAIR_KEY_BUTT,
    // add normal param below this
    UNPAIR_DEVICE_TYPE,
    UNPAIR_APP_NAME,
    UNPAIR_CONSTATE,
    UNPAIR_SCENE,
    UNPAIR_DELETEKEY_RESULT,
    UNPAIR_DELETEKEY_TIME,
    UNPAIR_RESULT,
    UNPAIR_ERRORCODE,
    UNPAIR_START_TIME,
    UNPAIR_END_TIME,
    // add normal param above this
    UNPAIR_PARAM_BUTT,
} DftUnPairExcepParamEnum;

// NEARLINK_ACCURATESEARCH param enum
typedef enum DftAccurateSearchExcepParamEnum : int {
    ACCURATESEARCH_INVALID = 0,
    // add key param below this
    PEER_ADDR,
    // add key param above this
    ACCURATESEARCH_KEY_BUTT,
    // add normal param below this
    PATH,
    CONN_RESULT,
    CONN_FAIL_ERROR_CODE,
    MEASURE_RESULT,
    MEASURE_FAIL_ERROR_CODE,
    MEASURE_START_TIME,
    MEASURE_END_TIME,
    DISCONN_ERROR_CODE,
    RSSI_VALUE,
    // add normal param above this
    ACCURATESEARCH_PARAM_BUTT,
} DftAccurateSearchExcepParamEnum;

// NEARLINK_MULDEVICE_CONN param enum
typedef enum DftMultideviceConnParamEnum : int {
    MULTIDEVICECONN_INVALID = 0,
    // add key param below this
    DEVICE_ADDR,
    // add key param above this
    MULTIDEVICECONN_KEY_BUTT,
    // add normal param below this
    CONN_MODULE_NAME,
    DEVICE_CONN_STATE,
    DEVICE_SERVICE_STATE,
    DEVICE_CONN_LAST_TIME,
    DEVICE_DISCONN_LAST_TIME,
    DEVICE_UPDATE_TIME,
    // add normal param above this
    MULTIDEVICECONN_PARAM_BUTT,
} DftMultideviceConnParamEnum;

// NEARLINK_ICCE_CAR_PROFILE_EXCEP param enum
typedef enum DftIcceProfileExcepParamEnum : int {
    ICCE_PROFILE_EXCEP_INVALID = 0,
    // add key param below this
    ICCE_PROFILE_EXCEP_DEVICE_ADDR,
    // add key param above this
    ICCE_PROFILE_EXCEP_KEY_BUTT,
    // add normal param below this
    ICCE_PROFILE_EXCEP_DEVICE_NAME,
    ICCE_PROFILE_EXCEP_DEVICE_APPEARANCE,
    ICCE_PROFILE_EXCEP_ERROR_CODE,
    ICCE_PROFILE_EXCEP_SUB_ERROR_CODE,
    // add normal param above this
    ICCE_PROFILE_EXCEP_PARAM_BUTT,
} DftIcceProfileExcepParamEnum;

// NEARLINK_AUDIO_PROFILE_EXCEPTION param enum
typedef enum DftAudioProfileExcepParamEnum : int {
    AUDIO_PROFILE_EXCEP_INVALID = 0,
    // add key param below this
    AUDIO_PROFILE_EXCEP_DEVICE_ADDR,
    // add key param above this
    AUDIO_PROFILE_EXCEP_KEY_BUTT,
    // add normal param below this
    AUDIO_PROFILE_EXCEP_DEVICE_NAME,
    AUDIO_PROFILE_EXCEP_DEVICE_APPEARANCE,
    AUDIO_PROFILE_EXCEP_ERROR_CODE,
    AUDIO_PROFILE_EXCEP_SUB_ERROR_CODE,
    AUDIO_PROFILE_EXCEP_EXTRA_PARAM,
    // add normal param above this
    AUDIO_PROFILE_EXCEP_PARAM_BUTT,
} DftAudioProfileExcepParamEnum;

typedef enum DftStackSmGNegoExcepParamEnum : int {
    SM_G_NEGO_INVALID = 0,
    // add key param below this
    SM_DEVICE_ADDR,
    // add key param above this
    SM_G_NEGO_KEY_BUTT,
    // add normal param below this
    SM_G_NEGO_PEER_INFO,
    SM_G_NEGO_START_PAIR_TIME,
    SM_G_NEGO_GEN_KEY_TIME,
    SM_G_NEGO_RECV_PAIR_START_TIME,
    SM_G_NEGO_SEND_PAIR_REQ_TIME,
    SM_G_NEGO_RECV_PAIR_RESP_TIME,
    SM_G_NEGO_SEND_PAIR_CFM_TIME,
    SM_G_NEGO_RECV_PAIR_INIT_TIME,
    SM_G_NEGO_ERR_CODE,
    SM_G_NEGO_RES,
    // add normal param above this
    SM_G_NEGO_PARAM_BUTT,
} DftStackSmGNegoExcepParamEnum;

typedef enum DftStackSmTNegoExcepParamEnum : int {
    SM_T_NEGO_INVALID = 0,
    // add key param below this
    SM_T_NEGO_DEVICE_ADDR,
    // add key param above this
    SM_T_NEGO_KEY_BUTT,
    // add normal param below this
    SM_T_NEGO_PEER_INFO,
    SM_T_NEGO_START_PAIR_TIME,
    SM_T_NEGO_GEN_KEY_TIME,
    SM_T_NEGO_SEND_PAIR_START_TIME,
    SM_T_NEGO_RECV_PAIR_REQ_TIME,
    SM_T_NEGO_SEND_PAIR_RESP_TIME,
    SM_T_NEGO_RECV_PAIR_CFM_TIME,
    SM_T_NEGO_SEND_PAIR_INIT_TIME,
    SM_T_NEGO_ERR_CODE,
    SM_T_NEGO_RES,
    // add normal param above this
    SM_T_NEGO_PARAM_BUTT,
} DftStackSmTNegoExcepParamEnum;

typedef enum DftStackSmGAuthExcepParamEnum : int {
    SM_G_AUTH_INVALID = 0,
    // add key param below this
    SM_G_AUTH_DEVICE_ADDR,
    // add key param above this
    SM_G_AUTH_KEY_BUTT,
    // add normal param below this
    SM_G_AUTH_PEER_INFO,
    SM_G_AUTH_METHOD,
    SM_G_AUTH_GEN_PASSCODE_TIME,
    SM_G_AUTH_RECV_CFM_TIME,
    SM_G_AUTH_SEND_RA_TIME,
    SM_G_AUTH_RECV_RB_TIME,
    SM_G_AUTH_USER_CFM_TIME,
    SM_G_AUTH_GEN_KEY_TIME,
    SM_G_AUTH_GEN_CFMKEY_TIME,
    SM_G_AUTH_SEND_CFMKEY_TIME,
    SM_G_AUTH_RECV_CFMKEY_TIME,
    SM_G_AUTH_ERR_CODE,
    SM_G_AUTH_RES,
    // add normal param above this
    SM_G_AUTH_PARAM_BUTT,
} DftStackSmGAuthExcepParamEnum;

typedef enum DftStackSmTAuthExcepParamEnum : int {
    SM_T_AUTH_INVALID = 0,
    // add key param below this
    SM_T_AUTH_DEVICE_ADDR,
    // add key param above this
    SM_T_AUTH_KEY_BUTT,
    // add normal param below this
    SM_T_AUTH_PEER_INFO,
    SM_T_AUTH_METHOD,
    SM_T_AUTH_GEN_CFM_TIME,
    SM_T_AUTH_RECV_CFM_TIME,
    SM_T_AUTH_RECV_RA_TIME,
    SM_T_AUTH_SEND_CFM_TIME,
    SM_T_AUTH_SEND_RB_TIME,
    SM_T_AUTH_USER_CFM_TIME,
    SM_T_AUTH_GEN_KEY_TIME,
    SM_T_AUTH_GEN_CFMKEY_TIME,
    SM_T_AUTH_SEND_CFMKEY_TIME,
    SM_T_AUTH_RECV_CFMKEY_TIME,
    SM_T_AUTH_ERR_CODE,
    SM_T_AUTH_RES,
    // add normal param above this
    SM_T_AUTH_PARAM_BUTT,
} DftStackSmTAuthExcepParamEnum;

typedef enum DftStackSmEncpExcepParamEnum : int {
    SM_ENCP_INVALID = 0,
    // add key param below this
    SM_ENCP_DEVICE_ADDR,
    // add key param above this
    SM_ENCP_KEY_BUTT,
    // add normal param below this
    SM_ENCP_PEER_INFO,
    SM_ENCP_ENABLE_TIME,
    SM_ENCP_REQ_PARAM_TIME,
    SM_ENCP_ERR_CODE,
    SM_ENCP_RES,
    // add normal param above this
    SM_ENCP_PARAM_BUTT,
} DftStackSmEncpExcepParamEnum;

// NEARLINK_DTFR_CONN param enum
typedef enum DftDatatransferConnParamEnum : int {
    DATATRANSFERCONN_INVALID = 0,
    // add key param below this
    DTFR_DEVICE_ADDR,
    // add key param above this
    DATATRANSFERCONN_KEY_BUTT,
    // add normal param below this
    TIMESTAMP,
    DEVICE_NAME,
    DEVICE_APPEARANCE,
    CALLING_NAME,
    SCENE_CODE,
    SUB_SCENE_CODE,
    // add normal param above this
    DATATRANSFERCONN_PARAM_BUTT,
} DftDatatransferConnParamEnum;

// NEARLINK_DTFR_EXCEP param enum
typedef enum DftDatatransferExcepParamEnum : int {
    DATATRANSFEREXCEP_INVALID = 0,
    // add key param below this
    DTFREXCEP_DEVICE_ADDR,
    // add key param above this
    DATATRANSFEREXCEP_KEY_BUTT,
    // add normal param below this
    DTFREXCEP_TIMESTAMP,
    DTFREXCEP_DEVICE_NAME,
    DTFREXCEP_DEVICE_APPEARANCE,
    DTFREXCEP_CALLING_NAME,
    DTFREXCEP_TYPE,
    DTFREXCEP_ERROR_CODE,
    DTFREXCEP_SUB_ERR_CODE,
    // add normal param above this
    DATATRANSFEREXCEP_PARAM_BUTT,
} DftDatatransferExcepParamEnum;

typedef enum DftAudioHeadsetExcepParamEnum : int {
    AUDIO_HEADSET_EXCEP_INVALID = 0,
    // add key param below this
    AUDIO_DEVICE_ADDR,
    // add key param above this
    AUDIO_HEADSET_EXCEP_KEY_BUTT,
    // add normal param below this
    AUDIO_DEVICE_APPEARANCE,
    AUDIO_DEVICE_NAME,
    FREQ_BAND,
    AUDIO_SCENE_CODE,
    USER_SCENE_CODE,
    ERROR_CODE,
    AUDIO_SOURCE_RSSI,
    OTHER_EAR_RSSI,
    WRONG_PACKAGE_RATE,
    MISORDER_CNT_RXSEQ,
    NOSYNC_RATE,
    LNA_SWITCH,
    DOUBLE_TUNED_STATE,
    CODEC_TYPE,
    BITRATE_INFO,
    DEVICE_NUM_IN_LINK,
    STRONG_CHANNEL_NUM,
    MID_CHANNEL_NUM,
    WEAK_CHANNEL_NUM,
    NOISE_AVG_VALUE,
    // add normal param above this
    AUDIO_HEADSET_EXCEP_PARAM_BUTT,
} DftAudioHeadsetExcepParamEnum;

// NEARLINK_AUDIO_STREAM_EXCEP
typedef enum DftAudioStreamExcepParamEnum : int {
    STREAM_INVALID = 0,
    // add key param below this
    STREAM_DEVICE_ADDR,
    // add key param above this
    STREAM_KEY_BUTT,
    // add normal param below this
    STREAM_DEVICE_NAME,
    STREAM_DEVICE_APPEARANCE,
    STREAM_REPORT_ADDR,
    STREAM_OPERATION_TYPE,
    STREAM_RUNNING_TYPE,
    STREAM_ASC_STATE,
    STREAM_START_BUFF_TYPE,
    STREAM_JUDGE_RECONFIG,
    STREAM_CONFIG_PARAM,
    STREAM_CONFIGURING_TIME,
    STREAM_OPENING_TIME,
    STREAM_STARTING_TIME,
    STREAM_STOPPING_TIME,
    STREAM_RELEASING_TIME,
    STREAM_RESULT_TIME,
    STREAM_POINT_TYPE,
    STREAM_OPERATOR_RESULT,
    STREAM_OPERATOR_ERROR_CODE,
    // add normal param above this
    STREAM_PARAM_BUTT,
} DftAudioStreamExcepParamEnum;

 /*******************************************************************************
 *  Sub Event param index define below
 ******************************************************************************/

// sub event PEER_INFO param enum
typedef enum DftPeerInfoParamEnum : int {
    PEER_INFO_INVALID = 0,
    // add key param below this
    PEER_INFO_ADDR,
    PEER_INFO_TYPE,
    // add key param above this
    PEER_INFO_KEY_BUTT,
    // add normal param below this
    PEER_INFO_NAME,
    PEER_CONN_LAST_TIME,
    PEER_HID_STATE,
    PEER_HID_CONN_TIME,
    PEER_INFO_APPEARANCE,
    PEER_INFO_MANUFACTURER,
    // add normal param above this
    PEER_INFO_PARAM_BUTT,
} DftPeerInfoParamEnum;

// sub event CONN_INFO param enum
typedef enum DftConnInfoParamEnum : int {
    CONN_INFO_INVALID = 0,
    // add key param below this
    CONN_INFO_ADDR,
    CONN_INFO_TYPE,
    // add key param above this
    CONN_INFO_KEY_BUTT,
    // add normal param below this
    UP_CONN_REQ_TIME,
    LAST_CONN_REQ_TIME,
    CONN_COMP_TIME,
    UP_DISCONN_REQ_TIME,
    DISCONN_COMP_TIME,
    AUTH_COMP_TIME,
    ENCRY_COMP_TIME,
    HID_COMP_TIME,
    // add normal param above this
    CONN_INFO_PARAM_BUTT,
} DftConnInfoParamEnum;

// sub event STATE_FLOW param enum
typedef enum DftStateFlowParamEnum : int {
    STATE_FLOW_INFO_INVALID = 0,
    // add key param below this
    // add key param above this
    STATE_FLOW_KEY_BUTT,
    // add normal param below this
    STATE_FLOW,
    // add normal param above this
    STATE_FLOW_PARAM_BUTT,
} DftStateFlowParamEnum;

bool IsValidExcep(DftEventEnum eventId);
bool IsValidEvent(DftEventEnum eventId);
bool IsValidSubEvent(DftEventEnum eventId);
bool IsKeyParam(DftEventEnum eventId, int paramId);
bool IsValidParam(DftEventEnum eventId, int paramId, DftParamType type);
bool IsSingleValueType(DftParamType type);
bool IsJsonAbleType(DftParamType type);
HiSysEventParamType ConvertToHiSysType(DftParamType type);
const char *GetEventName(DftEventEnum eventId);
const char *GetParamName(DftEventEnum eventId, int paramId, DftParamType type);
const char *GetDomainName(DftEventEnum eventId);
HiSysEventEventType GetEventType(DftEventEnum eventId);

#ifdef __cplusplus
}
#endif

#endif // NEARLINK_DFT_DATABASE_H