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
#include "securec.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_ssap_app_server.h"
#include "ssaps_server_app.h"
#include "nlstk_icce_server.h"
#include "icce_type.h"
#include "icce_utils.h"
#include "icce_init.h"
#include "icce_server.h"

static int32_t g_icceAppId = ICCE_INVALID_APPID;
static NLSTK_IcceServiceInfo_S g_icceInfo = {0};

void IcceServerEnable(void)
{
    g_icceAppId = ICCE_INVALID_APPID;
    (void)memset_s(&g_icceInfo, sizeof(NLSTK_IcceServiceInfo_S), 0, sizeof(NLSTK_IcceServiceInfo_S));
    SsapRegServerAppParam_S param = {.appId = ICCE_INVALID_APPID};
    SsapServerRegApp(&param);
    NLSTK_CHECK_RETURN_VOID(param.appId != ICCE_INVALID_APPID, "[ICCE] reg server app failed");
    g_icceAppId = param.appId;
    NLSTK_LOG_INFO("[ICCE] reg server app success, appId=%d", g_icceAppId);
}

static uint32_t IccePropertySetDescriptor(NLSTK_SsapServicePropertyParam_S *property)
{
    // 目前只需要客户端属性配置描述符，后续可拓展；
    property->descriptors = (NLSTK_SsapServiceDescriptorParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServiceDescriptorParam_S));
    NLSTK_CHECK_RETURN(property->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[ICCE] property descriptors malloc fail");
    property->descriptorNum = 1;
    // perimission暂置为0，无需授权、认证、加密
    property->descriptors[0].type = DESC_TYPE_CLIENT_CONFIG;
    property->descriptors[0].operation.operationValue =
        SSAP_OPERATE_INDICATION_READ + SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->descriptors[0].val.len = ICCE_DESC_CLIENT_CONFIG_LEN;
    property->descriptors[0].val.data = (uint8_t *)SDF_MemZalloc(ICCE_DESC_CLIENT_CONFIG_LEN);
    NLSTK_CHECK_RETURN(property->descriptors[0].val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[ICCE] property descriptor value malloc fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static void IcceAddServiceStatement(NLSTK_ServiceParam_S *service)
{
    NLSTK_SsapServiceStatementParam_S *statement = &service->serviceStatement;
    NLSTK_SsapUuid_S uuid = IcceConvertUuidToStru(ICCE_SERVICE_UUID);
    memcpy_s(&statement->uuid, sizeof(NLSTK_SsapUuid_S), &uuid, sizeof(NLSTK_SsapUuid_S));
    statement->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
}

static uint32_t IcceAddPropertyInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_IcceServiceInfo_S *basicInfo,
    uint16_t uuid, uint8_t right)
{
    NLSTK_SsapUuid_S uuidStru = IcceConvertUuidToStru(uuid);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    property->permission.permissionValue = 0x00;
    property->operation.operationValue = right;
    property->val.len = sizeof(basicInfo->iccePort);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ICCE] property value malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->iccePort, property->val.len);
    NLSTK_LOG_INFO("[ICCE] port is %s", SDF_GET_UINT8_STR(property->val.data, property->val.len));
    return IccePropertySetDescriptor(property);
}

// 组装服务属性
static uint32_t IcceAddServiceProperty(NLSTK_ServiceParam_S *service, NLSTK_IcceServiceInfo_S *basicInfo)
{
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServicePropertyParam_S));
    NLSTK_CHECK_RETURN(service->property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ICCE] serviceProperty malloc fail");
    service->servicePropertyNum = 0x01;
    uint32_t ret = IcceAddPropertyInfo(service->property, basicInfo, ICCE_PORT_UUID, basicInfo->iccePortRight);

    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[ICCE] add device appearance fail");
    return NLSTK_ERRCODE_SUCCESS; // 所有属性添加成功
}

void IcceAddService(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceAddService");
    NLSTK_CHECK_RETURN_VOID(g_icceAppId != ICCE_INVALID_APPID, "[ICCE] appId is invalid");
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN_VOID(service != NULL, "[ICCE] service malloc fail");
    IcceAddServiceStatement(service);
    uint32_t ret = IcceAddServiceProperty(service, &g_icceInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] add service property fail");
        NLSTK_SsapFreeServiceParam(service);
        return;
    }
    ret = NLSTK_SsapServerAddService(g_icceAppId, service);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] add service fail");
    }
    NLSTK_SsapFreeServiceParam(service);
}

void IcceRemoveService(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceRemoveService");
    uint32_t ret = NLSTK_SsapServerClearServices(g_icceAppId);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ICCE] clear service fail");
    }
}

void IcceSaveInfo(void *arg)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceSaveInfo");
    NLSTK_IcceServiceInfo_S *info = (NLSTK_IcceServiceInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[ICCE] save info is null");
    g_icceInfo.iccePort = info->iccePort;
    g_icceInfo.iccePortRight = info->iccePortRight;
}