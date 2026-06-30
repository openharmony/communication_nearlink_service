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
#include "dli_connect_event.h"
#include "dli_log.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_event.h"

#define DLI_ICB_SYNC_DELAY_SIZE 3
#define DLI_ICB_TRANS_LATENCY_SIZE 3
 
typedef struct {
    uint8_t status;
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t imgSyncDelay[DLI_ICB_SYNC_DELAY_SIZE];
    uint8_t imbSyncDelay[DLI_ICB_SYNC_DELAY_SIZE];
    uint8_t transLatencyG2T[DLI_ICB_TRANS_LATENCY_SIZE];
    uint8_t transLatencyT2G[DLI_ICB_TRANS_LATENCY_SIZE];
    uint8_t phyG2T;
    uint8_t phyT2G;
    uint8_t mcsG2T;
    uint8_t mcsT2G;
    uint8_t pilotG2T;
    uint8_t pilotT2G;
    uint8_t nse;                      /*!< 0x01-0x1F, 在IMG中每一个IMB每个间隔内的子事件个数 */
    uint8_t bnG2T;
    uint8_t bnT2G;
    uint8_t ftG2T;                    /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                    /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint16_t maxPduG2T;               /*!< 0x0000-0x07FF, G到T方向上PDU Payload的最大字节数，以字节为单位 */
    uint16_t maxPduT2G;               /*!< 0x0000-0x07FF, T到G方向上PDU Payload的最大字节数，以字节为单位 */
    uint16_t imbInterval;             /*!< 连续两个IMB Anchor Point的间隔时间，取值范围[0x0014,0x3E80]，时间=N * 0.25ms，时间范围[5ms,4s] */
} __attribute__((packed)) DLI_ICBEstablishedEvtStd;

#define DLI_MAX_LABEL_CNT 16

void DLI_DataLengthChangeCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("data length change cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_DataLenChangeEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_DataLenChangeEvt));
    DLI_RunRegCbk(
        DLI_CBK_DATA_LEN_CHANGE, context, arg, (uint32_t)(sizeof(DLI_DataLenChangeEvt)), evtOpcode, DLI_SUCCESS);
}

void DLI_SetPhyCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("set phy cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_SetPhyEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_SetPhyEvt));
    DLI_SetPhyEvt *param = (DLI_SetPhyEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_SET_PHY, context, arg, (uint32_t)(sizeof(DLI_SetPhyEvt)), evtOpcode, param->status);
}

void DLI_AcbLowLatencyEnableCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("acb low latency reply cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_AcbLowLatencyEnableEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_AcbLowLatencyEnableEvt));
    DLI_AcbLowLatencyEnableEvt *param = (DLI_AcbLowLatencyEnableEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_ACB_LOW_LATENCY_EN,
        context,
        arg,
        (uint32_t)(sizeof(DLI_AcbLowLatencyEnableEvt)),
        evtOpcode,
        param->status);
}

void DLI_RemoteConnParamReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    // 收到此事件需要回复0x1808命令，调用方发送
    DLI_LOGI("remote connection parameter request cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_RemoteConnParamReqEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_RemoteConnParamReqEvt));
    DLI_RunRegCbk(DLI_CBK_REMOTE_CONNECT_PARAM_REQ,
        context,
        arg,
        (uint32_t)(sizeof(DLI_RemoteConnParamReqEvt)),
        evtOpcode,
        DLI_SUCCESS);
}

void DLI_ConnectionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("connection cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_ConnectionCompleteEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_ConnectionCompleteEvt));
    DLI_ConnectionCompleteEvt *data = (DLI_ConnectionCompleteEvt *)arg;
    DLI_ConnectionCompleteEvt param = {0};
    (void)memcpy_s(&param, sizeof(DLI_ConnectionCompleteEvt), data, sizeof(DLI_ConnectionCompleteEvt));
    DLI_RunRegCbk(
        DLI_CBK_CONNECT, context, &param, (uint32_t)(sizeof(DLI_ConnectionCompleteEvt)), evtOpcode, param.status);
}

void DLI_DisconnectionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("disconnection cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_DisconnectEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_DisconnectEvt));
    DLI_DisconnectEvt *param = (DLI_DisconnectEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_DISCONNECT, context, arg, (uint32_t)(sizeof(DLI_DisconnectEvt)), evtOpcode, param->status);
}

void DLI_ReadRemoteFeaturesCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("read remote features cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ReadRemoteFeatsEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ReadRemoteFeatsEvt));
    DLI_ReadRemoteFeatsEvt *param = (DLI_ReadRemoteFeatsEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_READ_REMOTE_FEATURE,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ReadRemoteFeatsEvt)),
        evtOpcode,
        param->status);
}

void DLI_SetAcbEvtParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("set acb evt param cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_SetAcbEvtParamEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_SetAcbEvtParamEvt));
    DLI_SetAcbEvtParamEvt *param = (DLI_SetAcbEvtParamEvt *)arg;
    DLI_RunRegCbk(
        DLI_CBK_SET_ACB_EVT_PARAM, context, arg, (uint32_t)(sizeof(DLI_SetAcbEvtParamEvt)), evtOpcode, param->status);
}

void DLI_ConnectionUpdateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("connection update cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_ConnectionUpdateCmpEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_ConnectionUpdateCmpEvt));
    DLI_ConnectionUpdateCmpEvt *param = (DLI_ConnectionUpdateCmpEvt *)arg;
    DLI_RunRegCbk(
        DLI_CBK_CONNECT_UPDATE, context, arg, (uint32_t)(sizeof(DLI_ConnectionUpdateCmpEvt)), evtOpcode, param->status);
}

void DLI_ReadRemoteVersionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("read remote version cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ReadRemoteVersionEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ReadRemoteVersionEvt));
    DLI_ReadRemoteVersionEvt *param = (DLI_ReadRemoteVersionEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_READ_REMOTE_VERSION,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ReadRemoteVersionEvt)),
        evtOpcode,
        param->status);
}

void DLI_FreqBandSwitchCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("freq band switch cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(len >= sizeof(DLI_FreqBandSwitchEvt), "check len=%u, minDataLen=%zu",
        len, sizeof(DLI_FreqBandSwitchEvt));
    DLI_FreqBandSwitchEvt *param = (DLI_FreqBandSwitchEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_SWITCH_FREQ_BAND,
        context,
        arg,
        sizeof(DLI_FreqBandSwitchEvt),
        evtOpcode,
        param->status);
}

void DLI_IOBConnectReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("param connect request cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBConnectReqEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBConnectReqEvt));
    DLI_RunRegCbk(DLI_CBK_IOB_CONNECT_REQ,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ICBConnectReqEvt)),
        evtOpcode,
        DLI_SUCCESS);
}

void DLI_IOBEstablishedCbkStd(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_ICBEstablishedEvt param = {};
    DLI_LOGI("create icb cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(len >= sizeof(DLI_ICBEstablishedEvtStd), "check len=%u, minDataLen=%zu", len,
        sizeof(DLI_ICBEstablishedEvtStd));
 
    DLI_ICBEstablishedEvtStd *stdEvt = (DLI_ICBEstablishedEvtStd *)arg;
    param.status = stdEvt->status;
    param.labelId = 0; // lable id is ignored in std event
    if (memcpy_s(&param.lcid, sizeof(DLI_ICBEstablishedEvt) - offsetof(DLI_ICBEstablishedEvt, lcid),
        &stdEvt->lcid, sizeof(DLI_ICBEstablishedEvtStd) - offsetof(DLI_ICBEstablishedEvtStd, lcid)) != EOK) {
        DLI_LOGE("copy param from std event failed");
        return;
    }
 
    DLI_RunRegCbk(DLI_CBK_IOB_ESTABLISHED,
        context,
        &param,
        (uint32_t)(sizeof(DLI_ICBEstablishedEvt)),
        evtOpcode,
        param.status);
}

void DLI_IOBEstablishedCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("create icb cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBEstablishedEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBEstablishedEvt));
    DLI_ICBEstablishedEvt *param = (DLI_ICBEstablishedEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_IOB_ESTABLISHED,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ICBEstablishedEvt)),
        evtOpcode,
        param->status);
}

void DLI_IOBReportParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("report iob param cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_IOBQualityReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_IOBQualityReportEvt));
    DLI_RunRegCbk(DLI_CBK_IOB_REPORT_PARAM,
        context,
        arg,
        (uint32_t)(sizeof(DLI_IOBQualityReportEvt)),
        evtOpcode,
        DLI_SUCCESS);
}

void DLI_IOGLabelReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("set iog label cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICGLabelReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICGLabelReportEvt));
    DLI_ICGLabelReportEvt *param = (DLI_ICGLabelReportEvt *)arg;
    DLI_CHECK_RETURN(param->labelCnt != 0 && param->labelCnt < DLI_MAX_LABEL_CNT, "label cnt is illegal");
    uint32_t totalSize = (uint32_t)sizeof(DLI_ICGLabelReportEvt) + param->labelCnt * sizeof(DLI_ICGLabel);
    DLI_CHECK_RETURN(len == totalSize, "check len=%u, totalSize=%u", len, totalSize);
    DLI_RunRegCbk(DLI_CBK_IOG_LABEL_REPORT,
        context,
        arg,
        totalSize,
        evtOpcode,
        param->status);
}

void DLI_IOGUpdateParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("update iog param cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBParamUpdateEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBParamUpdateEvt));
    DLI_ICBParamUpdateEvt *param = (DLI_ICBParamUpdateEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_IOG_PARAM_UPDATE,
        context,
        arg,
        (uint32_t)sizeof(DLI_ICBParamUpdateEvt),
        evtOpcode,
        param->status);
}

void DLI_IMBConnectReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("param connect request cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBConnectReqEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBConnectReqEvt));
    DLI_RunRegCbk(DLI_CBK_IMB_CONNECT_REQ,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ICBConnectReqEvt)),
        evtOpcode,
        DLI_SUCCESS);
}

void DLI_IMBEstablishedCbkStd(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_ICBEstablishedEvt param = {};
    DLI_LOGI("create icb cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(len >= sizeof(DLI_ICBEstablishedEvtStd), "check len=%u, minDataLen=%zu", len,
        sizeof(DLI_ICBEstablishedEvtStd));
 
    DLI_ICBEstablishedEvtStd *stdEvt = (DLI_ICBEstablishedEvtStd *)arg;
    param.status = stdEvt->status;
    param.labelId = 0; // lable id is ignored in std event
    if (memcpy_s(&param.lcid, sizeof(DLI_ICBEstablishedEvt) - offsetof(DLI_ICBEstablishedEvt, lcid),
        &stdEvt->lcid, sizeof(DLI_ICBEstablishedEvtStd) - offsetof(DLI_ICBEstablishedEvtStd, lcid)) != EOK) {
        DLI_LOGE("copy param from std event failed");
        return;
    }
 
    DLI_RunRegCbk(DLI_CBK_IMB_ESTABLISHED,
        context,
        &param,
        (uint32_t)(sizeof(DLI_ICBEstablishedEvt)),
        evtOpcode,
        param.status);
}

void DLI_IMBEstablishedCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("create icb cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBEstablishedEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBEstablishedEvt));
    DLI_ICBEstablishedEvt *param = (DLI_ICBEstablishedEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_IMB_ESTABLISHED,
        context,
        arg,
        (uint32_t)(sizeof(DLI_ICBEstablishedEvt)),
        evtOpcode,
        param->status);
}

void DLI_IMBReportParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("report imb param cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_IMBQualityReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_IMBQualityReportEvt));
    DLI_IMBQualityReportEvt *param = (DLI_IMBQualityReportEvt *)arg;
    uint32_t totalSize = (uint32_t)sizeof(DLI_IMBQualityReportEvt) + param->txCount * sizeof(DLI_nodeChannelInfo);
    DLI_CHECK_RETURN(len == totalSize, "check len=%u, totalSize=%u", len, totalSize);

    DLI_RunRegCbk(DLI_CBK_IMB_REPORT_PARAM,
        context,
        arg,
        totalSize,
        evtOpcode,
        DLI_SUCCESS);
}

void DLI_IMGLabelReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("set img label cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICGLabelReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICGLabelReportEvt));
    DLI_ICGLabelReportEvt *param = (DLI_ICGLabelReportEvt *)arg;
    uint32_t totalSize = (uint32_t)sizeof(DLI_ICGLabelReportEvt) + param->labelCnt * sizeof(DLI_ICGLabel);
    DLI_CHECK_RETURN(len == totalSize, "check len=%u, totalSize=%u", len, totalSize);
    DLI_RunRegCbk(DLI_CBK_IMG_LABEL_REPORT,
        context,
        arg,
        totalSize,
        evtOpcode,
        param->status);
}

void DLI_IMGUpdateParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("update img param cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ICBParamUpdateEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ICBParamUpdateEvt));
    DLI_ICBParamUpdateEvt *param = (DLI_ICBParamUpdateEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_IMG_PARAM_UPDATE,
        context,
        arg,
        sizeof(DLI_ICBParamUpdateEvt),
        evtOpcode,
        param->status);
}

static void DLI_AcbSetSubrateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("acb set subrate cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_AcbSetSubrateEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_AcbSetSubrateEvt));
    DLI_AcbSetSubrateEvt *param = (DLI_AcbSetSubrateEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_ACB_SET_SUBRATE,
        context,
        arg,
        sizeof(DLI_AcbSetSubrateEvt),
        evtOpcode,
        param->status);
}

static void DLI_AcbReqSubrateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("acb req subrate cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_AcbReqSubrateEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_AcbReqSubrateEvt));
    DLI_AcbReqSubrateEvt *param = (DLI_AcbReqSubrateEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_ACB_REQ_SUBRATE,
        context,
        arg,
        sizeof(DLI_AcbReqSubrateEvt),
        evtOpcode,
        param->status);
}

void DLI_AcbSubrateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("acb subrate cbk in, evtOpcode: 0x%04X.", evtOpcode);
    uint8_t *p = (uint8_t *)arg;
    DLI_CHECK_RETURN(
        len >= sizeof(uint8_t), "check len=%u, minDataLen=%zu", len, sizeof(uint8_t));
    uint8_t subEventCode = *p++;
    len -= sizeof(uint8_t);
    if (subEventCode == DLI_SUBEVENT_ACB_SUBRATE_CHANGE) {
        DLI_AcbSetSubrateCbk(context, (void *)p, len, evtOpcode);
    } else if (subEventCode == DLI_SUBEVENT_ACB_SUBRATE_REQ) {
        DLI_AcbReqSubrateCbk(context, (void *)p, len, evtOpcode);
    } else {
        DLI_LOGW("unsupported sub event code: 0x%04x", subEventCode);
    }
}
