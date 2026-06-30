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
#include "devd_local.h"
#include "sdf_mem.h"

static DevdLocalDevice_S g_LocalDevice;
static uint8_t g_preAdvHandle = DEVD_INVALID_ADV_HANDLE;

DevdLocalDevice_S *DevdGetLocalDevice(void)
{
    return &g_LocalDevice;
}

uint8_t DevdGetPreAdvHandle(void)
{
    return g_preAdvHandle;
}

void DevdSetPreAdvHandle(uint8_t newHandle)
{
    g_preAdvHandle = newHandle;
}

void DevdLocalDeviceInit(void)
{
    SDF_DListHeadInit(&g_LocalDevice.advList);
    g_LocalDevice.scanCbk = NULL;
    g_LocalDevice.reportCbk = NULL;
    g_LocalDevice.scanMgr.status = DEVD_SLE_STATUS_IDLE;
    g_LocalDevice.scanMgr.scanParams = NULL;
    g_LocalDevice.scanMgr.tmpScanParams = NULL;
    g_preAdvHandle = DEVD_INVALID_ADV_HANDLE;
}

void DevdLocalDeviceDeInit(void)
{
    DevdAdvNode_S *node = NULL;
    DevdAdvNode_S *tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &g_LocalDevice.advList, entry, DevdFreeAdvNode);
    if (g_LocalDevice.scanMgr.scanParams != NULL) {
        SDF_MemFree(g_LocalDevice.scanMgr.scanParams);
        g_LocalDevice.scanMgr.scanParams = NULL;
    }
    if (g_LocalDevice.scanMgr.tmpScanParams != NULL) {
        SDF_MemFree(g_LocalDevice.scanMgr.tmpScanParams);
        g_LocalDevice.scanMgr.tmpScanParams = NULL;
    }
    g_preAdvHandle = DEVD_INVALID_ADV_HANDLE;
}