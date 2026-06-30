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
#include "securec.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "hid_type.h"
#include "hid_utils.h"
#include "hid_def.h"
#include "hid_common.h"
#include "hid_ssap.h"
#include "hid_stm.h"

typedef void (*HidStateMachineDispatch)(HidDevice_S *dev, HidStmParam_S msg);

static void HidUninitStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidInitStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidCreateLinkStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidGetServiceStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidReadPropertyStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidSetNotificationStateDispatch(HidDevice_S *dev, HidStmParam_S msg);
static void HidConnectedStateDispatch(HidDevice_S *dev, HidStmParam_S msg);

void HidStateMachineCall(HidDevice_S *dev, HidStmParam_S msg)
{
    static HidStateMachineDispatch hidStateMachine[HID_STATE_BUFF] = {
        [HID_STATE_UNINIT] = HidUninitStateDispatch,
        [HID_STATE_INIT] = HidInitStateDispatch,
        [HID_STATE_CREATE_LINK] = HidCreateLinkStateDispatch,
        [HID_STATE_GET_SERVICE] = HidGetServiceStateDispatch,
        [HID_STATE_READ_PROPERTY] = HidReadPropertyStateDispatch,
        [HID_STATE_SET_NOTIFICATION] = HidSetNotificationStateDispatch,
        [HID_STATE_CONNECTED] = HidConnectedStateDispatch,
    };
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "dev is null");
    if (msg.what != HID_ON_NOTIFY_PROPERTY) {
        NLSTK_LOG_INFO("[HID] enter HidStateMachineCall, addr = %s, state = %u, msg = %u",
            GET_ENC_ADDR(&dev->addr), dev->state, msg.what);
    }
    hidStateMachine[dev->state](dev, msg);
}

static void HidUninitStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，此时还未注册AppId，进入注册AppId流程
        case HID_ON_USER_CONNECT: {
            HID_DEV_STATE_CHANGE(dev, HID_STATE_INIT);
            HidStateChangeCbk(&dev->addr, HID_CONNECTING, HID_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
            HidStateMachineCall(dev, newMsg);
            break;
        }
        default:
            break;
    }
}

static void HidInitStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行注册AppId流程
        case HID_ON_USER_CONNECTING: {
            NLSTK_SsapAppClientCb_S cbk = HidGetSsapCbk();
            NLSTK_ConnParam_S connParam = { 0 };
            NLSTK_Errcode_E ret = NLSTK_SsapClientRegAppAsyn(&dev->addr, &connParam, &cbk);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
                HidRemoveDevice(&dev->addr);
            }
            break;
        }
        // 收到注册AppId成功事件，说明上一次用户调用了连接，继续执行连接流程
        case HID_ON_REGISTER_APP: {
            int32_t appId = *(int32_t *)msg.extData;
            if (appId < 0) {
                NLSTK_LOG_ERROR("[HID] invalid appId");
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
                HidRemoveDevice(&dev->addr);
                return;
            }
            dev->appId = appId;
            HID_DEV_STATE_CHANGE(dev, HID_STATE_CREATE_LINK);
            HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
            HidStateMachineCall(dev, newMsg);
            break;
        }
        default:
            break;
    }
}

static void HidCreateLinkStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行建链流程
        case HID_ON_USER_CONNECTING: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(dev->appId);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
                NLSTK_SsapClientDeregAppAsync(dev->appId);
                HidRemoveDevice(&dev->addr);
            }
            break;
        }
        // 收到底层链路状态变化回调，若为已连接，继续执行profile连接流程：获取Hid服务
        case HID_ON_STATE_CHANGED: {
            uint8_t state = *(uint8_t *)msg.extData;
            if (state == SSAP_CONNECT_STATE_CONNECTED) {
                HID_DEV_STATE_CHANGE(dev, HID_STATE_GET_SERVICE);
                HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
                HidStateMachineCall(dev, newMsg);
            } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_SUCCESS);
                NLSTK_SsapClientDeregAppAsync(dev->appId);
                HidRemoveDevice(&dev->addr);
            }
            break;
        }
        // 收到用户断开连接事件
        case HID_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void HidOnStateChangedAfterCreateLinkState(HidDevice_S *dev, HidStmParam_S msg, HidConnectState_E preState)
{
    uint8_t state = *(uint8_t *)msg.extData;
    if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, preState, NLSTK_ERRCODE_SUCCESS);
        NLSTK_SsapClientDeregAppAsync(dev->appId);
        HidRemoveDevice(&dev->addr);
    }
}

static uint32_t HidBuildProperty(HidDevice_S *dev, NLSTK_SsapServ_S *service, uint16_t index)
{
    uint16_t prtyUuid = HidConvertUuidTo16Bits(service->properties[index].uuid);
    if (prtyUuid == HID_REPORT_MAP_UUID || prtyUuid == HID_REPORT_MAP_UUID_PEN) {
        dev->service.descHandle = service->properties[index].handle;
    } else if (prtyUuid == HID_WORK_STATE_UUID || prtyUuid == HID_WORK_STATE_UUID_PEN) {
        dev->service.workStateHandle = service->properties[index].handle;
    } else if (prtyUuid == HID_REPORT_INDEX_UUID || prtyUuid == HID_REPORT_INDEX_UUID_PEN) {
        uint16_t *handle = (uint16_t *)SDF_MemZalloc(sizeof(uint16_t));
        if (handle == NULL) {
            NLSTK_LOG_ERROR("[HID] handle malloc fail");
            SDF_CleanVector(dev->service.indexHandle);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        *handle = service->properties[index].handle;
        if (!SDF_VectorEmplaceBack(dev->service.indexHandle, handle)) {
            NLSTK_LOG_ERROR("[HID] report emplace back fail");
            SDF_CleanVector(dev->service.indexHandle);
            SDF_MemFree(handle);
            return NLSTK_ERRCODE_FAIL;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t HidBuildService(HidDevice_S *dev, NLSTK_SsapServ_S *service, uint16_t serviceNum)
{
    NLSTK_LOG_INFO("[HID] find service num = %u", serviceNum);
    if (serviceNum > HID_MAX_SERVICE_NUM) {
        NLSTK_LOG_ERROR("[HID] serviceNum is invalid");
        return NLSTK_ERRCODE_FAIL;
    }

    for (uint16_t i = 0; i < serviceNum; i++) {
        uint16_t srvcUuid = HidConvertUuidTo16Bits(service[i].uuid);
        if (srvcUuid != HID_SERVICE_UUID && srvcUuid != HID_SERVICE_UUID_PEN) {
            continue;
        }
        if (service[i].propertyNum > HID_MAX_SERVICE_NUM) {
            NLSTK_LOG_ERROR("[HID] propertyNum is invalid");
            return NLSTK_ERRCODE_FAIL;
        }
        dev->service.handle = service[i].handle;
        dev->service.endHandle = service[i].endHandle;
        for (uint16_t j = 0; j < service[i].propertyNum; j++) {
            NLSTK_CHECK_RETURN(HidBuildProperty(dev, &service[i], j) == NLSTK_ERRCODE_SUCCESS,
                NLSTK_ERRCODE_FAIL, "[HID] build property fail");
        }
        return NLSTK_ERRCODE_SUCCESS;
    }
    return NLSTK_ERRCODE_FAIL;
}

static void HidHandleExceptionAfterLinkConnected(HidDevice_S *dev)
{
    if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HID] disconnect fail");
    }
    NLSTK_SsapClientDeregAppAsync(dev->appId);
    HidRemoveDevice(&dev->addr);
}

static void HidOnGetServiceInGetServiceState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidGetServiceMsg_S *getServiceMsg = (HidGetServiceMsg_S *)msg.extData;
    if (getServiceMsg == NULL || getServiceMsg->service == NULL || getServiceMsg->serviceNum == 0) {
        NLSTK_LOG_ERROR("[HID] get service msg is null");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    if (HidBuildService(dev, getServiceMsg->service, getServiceMsg->serviceNum) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HID] hid service not found or create handle index fail");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_MALLOC_FAIL);
        HidHandleExceptionAfterLinkConnected(dev);
    } else {
        HID_DEV_STATE_CHANGE(dev, HID_STATE_READ_PROPERTY);
        HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
        HidStateMachineCall(dev, newMsg);
    }
    if (getServiceMsg->func != NULL) {
        getServiceMsg->func(getServiceMsg->service, getServiceMsg->serviceNum);
    }
}

static void HidGetServiceStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取服务流程
        case HID_ON_USER_CONNECTING: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientGetServicesAsyn(dev->appId);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] get service fail");
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
                HidHandleExceptionAfterLinkConnected(dev);
            }
            break;
        }
        // 收到获取服务回调，解析服务属性句柄，进入读取属性流程
        case HID_ON_GET_SERVICE: {
            HidOnGetServiceInGetServiceState(dev, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case HID_ON_STATE_CHANGED: {
            HidOnStateChangedAfterCreateLinkState(dev, msg, HID_CONNECTING);
            break;
        }
        // 收到用户断开连接事件
        case HID_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void HidOnUserConnectingInReadPropertyState(HidDevice_S *dev, HidStmParam_S msg)
{
    NLSTK_Errcode_E ret = NLSTK_SsapClientReadProperty(dev->appId, dev->service.descHandle);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HID] read desc fail");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    for (size_t i = 0; i < dev->service.indexHandle->size; i++) {
        uint16_t *handle = SDF_VectorElementAt(dev->service.indexHandle, i);
        ret = NLSTK_SsapClientReadProperty(dev->appId, *handle);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[HID] read report info fail");
            HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
            HidHandleExceptionAfterLinkConnected(dev);
            return;
        }
        dev->lastReadHandle = *handle;
    }
}

static void HidOnReadReportIndexInReadPropertyState(HidDevice_S *dev, HidReadPropertyMsg_S * readMsg)
{
    if (readMsg->property->value.len < HID_REPORT_INDEX_INFO_LEN) {
        NLSTK_LOG_ERROR("[HID] report index info len error");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    HidReport_S *newReport = (HidReport_S *)SDF_MemZalloc(sizeof(HidReport_S));
    if (newReport == NULL) {
        NLSTK_LOG_ERROR("[HID] new report malloc fail");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_MALLOC_FAIL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    uint8_t *data = readMsg->property->value.data;
    HID_PARSE_TO_UINT8(newReport->reportId, data);
    HID_PARSE_TO_UINT8(newReport->reportType, data);
    HID_PARSE_TO_UINT16(newReport->reportHandle, data);
    HID_PARSE_TO_UINT16(newReport->reportSrcPort, data);
    HID_PARSE_TO_UINT16(newReport->reportDestPort, data);
    NLSTK_LOG_INFO("[HID] reportId = %u, reportType = %u, reportHandle = %u", newReport->reportId,
        newReport->reportType, newReport->reportHandle);
    if (!SDF_VectorEmplaceBack(dev->report, newReport)) {
        NLSTK_LOG_ERROR("[HID] report emplace back fail");
        SDF_MemFree(newReport);
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_FAIL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    if (readMsg->property->handle == dev->lastReadHandle) {
        for (size_t i = 0; i < dev->report->size; i++) {
            HidReport_S *report = SDF_VectorElementAt(dev->report, i);
            NLSTK_Errcode_E ret = NLSTK_SsapClientReadProperty(dev->appId, report->reportHandle);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] report emplace back fail");
                HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
                HidHandleExceptionAfterLinkConnected(dev);
                return;
            }
            dev->lastReadHandle = report->reportHandle;
        }
    }
}

static void HidOnReadReportInfoInReadPropertyState(HidDevice_S *dev, HidReadPropertyMsg_S * readMsg)
{
    size_t index = 0;
    if (!SDF_VectorFindFirst(dev->report, HidCompHandle, &readMsg->property->handle, &index)) {
        NLSTK_LOG_ERROR("[HID] report handle not found");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_NO_RECORD);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    HidReport_S *report = SDF_VectorElementAt(dev->report, index);
    report->reportInfoValue.len = readMsg->property->value.len;
    if (report->reportInfoValue.len > HID_MAX_DATA_LEN) {
        NLSTK_LOG_ERROR("[HID] reportInfoValue len is invalid");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    report->reportInfoValue.data = (uint8_t *)SDF_MemZalloc(report->reportInfoValue.len);
    if (report->reportInfoValue.data == NULL) {
        NLSTK_LOG_ERROR("[HID] report value malloc fail");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_MALLOC_FAIL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    (void)memcpy_s(report->reportInfoValue.data, report->reportInfoValue.len,
        readMsg->property->value.data, readMsg->property->value.len);
    if (readMsg->property->handle == dev->lastReadHandle) {
        HID_DEV_STATE_CHANGE(dev, HID_STATE_SET_NOTIFICATION);
        HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
        HidStateMachineCall(dev, newMsg);
    }
}

static void HidOnReadPropertyInReadPropertyState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidReadPropertyMsg_S *readMsg = (HidReadPropertyMsg_S *)msg.extData;
    if (readMsg == NULL || readMsg->ret != NLSTK_ERRCODE_SUCCESS ||
        readMsg->property == NULL || readMsg->property->value.data == NULL || readMsg->property->value.len == 0) {
        NLSTK_LOG_ERROR("[HID] read property fail");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    uint16_t uuid16Bits = HidConvertUuidTo16Bits(readMsg->property->uuid);
    if (readMsg->property->handle == dev->service.descHandle) {
        dev->desc.type = readMsg->property->value.data[0];
        dev->desc.descLen = readMsg->property->value.len - 1 > 0 ? readMsg->property->value.len - 1 : 0;
        if (dev->desc.descLen == 0) {
            return;
        }
        if (dev->desc.descLen > HID_MAX_DATA_LEN) {
            NLSTK_LOG_ERROR("[HID] descLen is invalid");
            return;
        }
        dev->desc.desc = (uint8_t *)SDF_MemZalloc(dev->desc.descLen);
        if (dev->desc.desc == NULL) {
            NLSTK_LOG_ERROR("[HID] desc data malloc fail");
            HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_MALLOC_FAIL);
            HidHandleExceptionAfterLinkConnected(dev);
            return;
        }
        (void)memcpy_s(dev->desc.desc, dev->desc.descLen, readMsg->property->value.data + 1, dev->desc.descLen);
    } else if (uuid16Bits == HID_REPORT_INDEX_UUID || uuid16Bits == HID_REPORT_INDEX_UUID_PEN) {
        HidOnReadReportIndexInReadPropertyState(dev, readMsg);
    } else {
        HidOnReadReportInfoInReadPropertyState(dev, readMsg);
    }
}

static void HidReadPropertyStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取读取属性流程
        case HID_ON_USER_CONNECTING: {
            HidOnUserConnectingInReadPropertyState(dev, msg);
            break;
        }
        // 收到读取属性回调，存储属性值，若读取属性完成，进入设置属性通知流程
        case HID_ON_READ_PROPERTY: {
            HidOnReadPropertyInReadPropertyState(dev, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case HID_ON_STATE_CHANGED: {
            HidOnStateChangedAfterCreateLinkState(dev, msg, HID_CONNECTING);
            break;
        }
        // 收到用户断开连接事件
        case HID_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void HidOnUserConnectingInSetNotificationState(HidDevice_S *dev, HidStmParam_S msg)
{
    for (size_t i = 0; i < dev->report->size; i++) {
        HidReport_S *report = SDF_VectorElementAt(dev->report, i);
        if (report->reportType != HID_INPUT_REPORT_TYPE) {
            continue;
        }
        NLSTK_Errcode_E ret = NLSTK_SsapClientSetPropertyNtf(dev->appId, report->reportHandle, true);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[HID] set property ntf fail");
            HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, ret);
            HidHandleExceptionAfterLinkConnected(dev);
            return;
        }
        dev->lastSetNtfHandle = report->reportHandle;
    }
}

static void HidOnSetNtfInSetNotificationState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidSetPropertyNtfMsg_S *setNtfMsg = (HidSetPropertyNtfMsg_S *)msg.extData;
    if (setNtfMsg == NULL || setNtfMsg->ret != NLSTK_ERRCODE_SUCCESS || setNtfMsg->enable == false) {
        NLSTK_LOG_ERROR("[HID] set property ntf res error");
        HidStateChangeCbk(&dev->addr, HID_DISCONNECTED, HID_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        HidHandleExceptionAfterLinkConnected(dev);
        return;
    }
    if (setNtfMsg->handle == dev->lastSetNtfHandle) {
        HID_DEV_STATE_CHANGE(dev, HID_STATE_CONNECTED);
        HidStmParam_S newMsg = { .what = HID_ON_USER_CONNECTING, .extData = NULL };
        HidStateMachineCall(dev, newMsg);
    }
}

static void HidSetNotificationStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行订阅通知流程
        case HID_ON_USER_CONNECTING: {
            HidOnUserConnectingInSetNotificationState(dev, msg);
            break;
        }
        // 收到订阅通知回调，若订阅全部完成且无异常，将状态切换为已连接
        case HID_ON_SET_NOTIFICATION: {
            HidOnSetNtfInSetNotificationState(dev, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case HID_ON_STATE_CHANGED: {
            HidOnStateChangedAfterCreateLinkState(dev, msg, HID_CONNECTING);
            break;
        }
        // 收到用户断开连接事件
        case HID_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

void HidFreeHidInformation(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    HidInformation_S *info = (HidInformation_S *)ptr;
    // 仅析构申请的部分，内部数据为浅拷贝，不析构内存
    if (info->reportInfo != NULL) {
        SDF_MemFree(info->reportInfo);
    }
    SDF_MemFree(info);
}

static void HidOnUserGetInfoInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidGetInfoParam_S *param = (HidGetInfoParam_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    param->freeFunc = HidFreeHidInformation;
    param->info = (HidInformation_S *)SDF_MemZalloc(sizeof(HidInformation_S));
    NLSTK_CHECK_RETURN_VOID(param->info != NULL, "[HID] info malloc fail");
    HidInformation_S *info = param->info;
    // desc的数据部分是浅拷贝，后续不释放该内存
    info->desc = dev->desc;
    info->reportInfo = (HidReportInfo_S *)SDF_MemZalloc(sizeof(HidReportInfo_S) * dev->report->size);
    NLSTK_CHECK_RETURN_VOID(info->reportInfo != NULL, "[HID] report info malloc fail");
    info->reportNum = dev->report->size;
    for (size_t i = 0; i < dev->report->size; i++) {
        HidReport_S *report = SDF_VectorElementAt(dev->report, i);
        info->reportInfo[i].reportIdAndType.reportId = report->reportId;
        info->reportInfo[i].reportIdAndType.reportType = report->reportType;
        // 这里的data是浅拷贝，后续不释放data的内存
        info->reportInfo[i].reportInfoValue = report->reportInfoValue;
    }
}

static void HidOnUserReadInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidReadParam_S *readParam = (HidReadParam_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(readParam != NULL, "[HID] read param is null");
    switch (readParam->type) {
        case HID_TYPE_AND_FORMAT_DESC: {
            if (NLSTK_SsapClientReadProperty(dev->appId, dev->service.descHandle) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] read desc fail");
            }
            break;
        }
        case HID_WORK_STATUS_INDICATION: {
            if (NLSTK_SsapClientReadProperty(dev->appId, dev->service.workStateHandle) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] read work status fail");
            }
            break;
        }
        // 下沉之后reportIndex没有办法直接读，实际上上层也没有读该属性的场景和意义
        case HID_REPORT_INDEX_INFO:
            break;
        case HID_INPUT_REPORT_INFO:
        case HID_OUTPUT_REPORT_INFO:
        case HID_FEATURE_REPORT_INFO: {
            size_t index = 0;
            HidReportIdAndType_S reportIdAndType = {
                .reportId = readParam->reportId,
                .reportType = readParam->reportType
            };
            NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(dev->report, HidCompReportIdAndType,
                &reportIdAndType, &index), "[HID] report id and type not found");
            HidReport_S *report = SDF_VectorElementAt(dev->report, index);
            if (NLSTK_SsapClientReadProperty(dev->appId, report->reportHandle) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] read report info fail");
            }
            break;
        }
        default:
            break;
    }
}

static void HidOnUserWriteInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidWriteParam_S *writeParam = (HidWriteParam_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(writeParam != NULL && writeParam->value != NULL, "[HID] write param is null");
    switch (writeParam->type) {
        case HID_WORK_STATUS_INDICATION: {
            if (NLSTK_SsapClientWriteProperty(dev->appId, dev->service.workStateHandle, writeParam->value, true)
                != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] write work status fail");
            }
            break;
        }
        case HID_OUTPUT_REPORT_INFO:
        case HID_FEATURE_REPORT_INFO: {
            size_t index = 0;
            HidReportIdAndType_S reportIdAndType = {
                .reportId = writeParam->reportId,
                .reportType = writeParam->reportType
            };
            NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(dev->report, HidCompReportIdAndType,
                &reportIdAndType, &index), "[HID] report id and type not found");
            HidReport_S *report = SDF_VectorElementAt(dev->report, index);
            if (NLSTK_SsapClientWriteProperty(dev->appId, report->reportHandle, writeParam->value, true)
                != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] write report info fail");
            }
            break;
        }
        default:
            break;
    }
}

static bool HidCheckPropertyParam(HidReadPropertyMsg_S *readMsg)
{
    if (readMsg->ret != NLSTK_ERRCODE_SUCCESS || readMsg->property->value.data == NULL) {
        NLSTK_LOG_ERROR("[HID] read property fail");
        return false;
    }
    if (readMsg->property->value.len > HID_MAX_DATA_LEN) {
        NLSTK_LOG_ERROR("[HID] property value len invalid");
        return false;
    }
    return true;
}

static void HidParseReportIndexInfo(HidReportIndexInfo_S *value, uint8_t *data)
{
    HID_PARSE_TO_UINT8(value->reportId, data);
    HID_PARSE_TO_UINT8(value->reportType, data);
    HID_PARSE_TO_UINT16(value->reportHandle, data);
    HID_PARSE_TO_UINT16(value->reportSrcPort, data);
    HID_PARSE_TO_UINT16(value->reportDestPort, data);
}

static void HidOnReadPropertyInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidReadPropertyMsg_S *readMsg = (HidReadPropertyMsg_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(readMsg != NULL && readMsg->property != NULL, "[HID] read property msg is null");
    uint16_t uuid16Bits = HidConvertUuidTo16Bits(readMsg->property->uuid);
    HidPropertyType_E type = HidGetPropertyTypeByUuid(uuid16Bits);
    NLSTK_CHECK_RETURN_VOID(type != HID_PROPERTY_TYPE_BUFF, "[HID] read property type error");
    if (!HidCheckPropertyParam(readMsg)) {
        NLSTK_LOG_ERROR("[HID] read property fail");
        HidReadCbk(&dev->addr, type, NULL, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (type == HID_INPUT_REPORT_INFO || type == HID_OUTPUT_REPORT_INFO || type == HID_FEATURE_REPORT_INFO) {
        size_t index = 0;
        NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(dev->report, HidCompHandle, &readMsg->property->handle, &index),
            "[HID] report handle not found");
        HidReport_S *report = SDF_VectorElementAt(dev->report, index);
        SDF_MemFree(report->reportInfoValue.data);
        NLSTK_CHECK_RETURN_VOID(readMsg->property->value.len != 0 && readMsg->property->value.data != NULL,
            "[HID] readMsg property value error");
        report->reportInfoValue.len = readMsg->property->value.len;
        report->reportInfoValue.data = (uint8_t *)SDF_MemZalloc(report->reportInfoValue.len);
        NLSTK_CHECK_RETURN_VOID(report->reportInfoValue.data != NULL, "[HID] report value malloc fail");
        (void)memcpy_s(report->reportInfoValue.data, report->reportInfoValue.len,
            readMsg->property->value.data, readMsg->property->value.len);
        HidReportInfo_S value = {
            .reportIdAndType = { .reportId = report->reportId, .reportType = report->reportType },
            .reportInfoValue = report->reportInfoValue
        };
        HidReadCbk(&dev->addr, type, &value, NLSTK_ERRCODE_SUCCESS);
    } else if (type == HID_TYPE_AND_FORMAT_DESC) {
        HidTypeAndFormatDesc_S value = {0};
        value.type = readMsg->property->value.data[0];
        value.descLen = readMsg->property->value.len - 1;
        (void)memcpy_s(value.desc, value.descLen, readMsg->property->value.data + 1, value.descLen);
        HidReadCbk(&dev->addr, type, &value, NLSTK_ERRCODE_SUCCESS);
    } else if (type == HID_WORK_STATUS_INDICATION) {
        uint8_t value = readMsg->property->value.data[0];
        HidReadCbk(&dev->addr, type, &value, NLSTK_ERRCODE_SUCCESS);
    } else if (type == HID_REPORT_INDEX_INFO) {
        NLSTK_CHECK_RETURN_VOID(readMsg->property->value.len >= HID_REPORT_INDEX_INFO_LEN,
            "[HID] report index info len error");
        HidReportIndexInfo_S value = {0};
        uint8_t *data = readMsg->property->value.data;
        HidParseReportIndexInfo(&value, data);
        HidReadCbk(&dev->addr, type, &value, NLSTK_ERRCODE_SUCCESS);
    }
}

static void HidOnWritePropertyInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    HidWritePropertyMsg_S *writeMsg = (HidWritePropertyMsg_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(writeMsg != NULL && writeMsg->property != NULL, "[HID] write property msg is null");
    uint16_t uuid16Bits = HidConvertUuidTo16Bits(writeMsg->property->uuid);
    HidPropertyType_E type = HidGetPropertyTypeByUuid(uuid16Bits);
    NLSTK_CHECK_RETURN_VOID(type != HID_PROPERTY_TYPE_BUFF, "[HID] write property type error");
    if (writeMsg->ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HID] write property fail");
    }
    HidWriteCbk(&dev->addr, type, writeMsg->ret);
}

static void HidOnNotifyPropertyInConnectedState(HidDevice_S *dev, HidStmParam_S msg)
{
    NLSTK_SsapClientReadPropertyInfo_S *ntfMsg = (NLSTK_SsapClientReadPropertyInfo_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(ntfMsg != NULL && ntfMsg->value.data != NULL,
        "[HID] notify property msg is null");
    // 当前只会订阅输入报告信息
    uint16_t uuid16Bits = HidConvertUuidTo16Bits(ntfMsg->uuid);
    NLSTK_CHECK_RETURN_VOID(uuid16Bits == HID_INPUT_REPORT_UUID || uuid16Bits == HID_INPUT_REPORT_UUID_PEN,
        "[HID] notify property type not input report");
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(dev->report, HidCompHandle, &ntfMsg->handle, &index),
        "[HID] report handle not found");
    HidReport_S *report = SDF_VectorElementAt(dev->report, index);
    HidReportInfo_S value = {
        .reportIdAndType = { .reportId = report->reportId, .reportType = report->reportType },
        .reportInfoValue = ntfMsg->value
    };
    HidNotifyCbk(&dev->addr, HID_INPUT_REPORT_INFO, &value);
}

static void HidConnectedStateDispatch(HidDevice_S *dev, HidStmParam_S msg)
{
    switch (msg.what) {
        // 收到属性通知回调，向上层回调通知结果
        case HID_ON_NOTIFY_PROPERTY: {
            HidOnNotifyPropertyInConnectedState(dev, msg);
            break;
        }
        // 收到用户发起连接事件，此时连接流程已全部完成，向上层回调状态为已连接
        case HID_ON_USER_CONNECTING: {
            HidStateChangeCbk(&dev->addr, HID_CONNECTED, HID_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        // 收到用户断开连接事件
        case HID_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(dev->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        // 收到用户发起获取设备HID信息事件
        case HID_ON_USER_GET_INFO: {
            HidOnUserGetInfoInConnectedState(dev, msg);
            break;
        }
        // 收到用户发起读取属性事件
        case HID_ON_USER_READ: {
            HidOnUserReadInConnectedState(dev, msg);
            break;
        }
        // 收到用户发起写入属性事件
        case HID_ON_USER_WRITE: {
            HidOnUserWriteInConnectedState(dev, msg);
            break;
        }
        // 收到读取属性回调，向上层回调读取结果
        case HID_ON_READ_PROPERTY: {
            HidOnReadPropertyInConnectedState(dev, msg);
            break;
        }
        // 收到写入属性回调，向上层回调写入结果
        case HID_ON_WRITE_PROPERTY: {
            HidOnWritePropertyInConnectedState(dev, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case HID_ON_STATE_CHANGED: {
            HidOnStateChangedAfterCreateLinkState(dev, msg, HID_CONNECTED);
            break;
        }
        default:
            break;
    }
}