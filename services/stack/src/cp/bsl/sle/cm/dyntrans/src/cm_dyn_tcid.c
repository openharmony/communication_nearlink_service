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

#include "cm_dyn_tcid.h"
#include "sdf_mem.h"
#include "sdf_dlist.h"
#include "cm_log.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"

#define CM_DYN_UNICAST_TCID_SIZE_MAX ((CM_TCID_UC_END) - (CM_TCID_UC_BEGIN) + 1)

typedef struct {
    SDF_DListEntry_S entry;                                 /* 链表节点 */
    uint16_t lcid;
    bool unicastTcidUsed[CM_DYN_UNICAST_TCID_SIZE_MAX];     /* 单播TCID分配表, false未使用，true已使用 */
} CM_DynTcidPool_S;

static SDF_DListHead_S g_cmDynTransPoolListHead = { .list = { NULL, NULL }, .size = 0 };
static volatile bool g_cmDynTcidIsInited = false;

void CM_DynTcidInit(void)
{
    CM_CHECK_RETURN(!g_cmDynTcidIsInited, "dyn tc module has been inited");
    SDF_DListHeadInit(&g_cmDynTransPoolListHead);
    g_cmDynTcidIsInited = true;
    CM_LOGI("CM_DynTcidInit success");
}

void CM_DynTcidDeInit(void)
{
    CM_CHECK_RETURN(g_cmDynTcidIsInited, "dyn tc module has not been inited");
    CM_DynTcidPool_S *tmp = NULL;
    CM_DynTcidPool_S *node = NULL;
    SDF_DListElmSafeForeach(node, tmp, &g_cmDynTransPoolListHead, entry) {
        (void)CM_DynTcidDestroyPool(node->lcid);
    }
    g_cmDynTcidIsInited = false;
    CM_LOGI("CM_DynTcidDeInit");
}

static CM_DynTcidPool_S *CM_DynTransGetTcidPool(uint16_t lcid)
{
    CM_DynTcidPool_S *node = NULL;
    SDF_DListElmForeach(node, &g_cmDynTransPoolListHead, entry) {
        if (node->lcid == lcid) {
            return node;
        }
    }
    return NULL;
}

uint32_t CM_DynTcidActivatePool(uint16_t lcid)
{
    CM_DynTcidPool_S *node = CM_DynTransGetTcidPool(lcid);
    if (node != NULL) {
        // 已激活，不做任何操作
        return CM_SUCCESS;
    }
    node = (CM_DynTcidPool_S *)SDF_MemZalloc(sizeof(CM_DynTcidPool_S));
    CM_CHECK_RETURN_RET(node != NULL, CM_MEM_ERR, "malloc dyn tcid pool node failed");
    node->lcid = lcid;
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(&g_cmDynTransPoolListHead, node, entry);
    CM_LOGI("CM_DynTcidActivatePool lcid:0x%04x success", lcid);
    return CM_SUCCESS;
}

uint32_t CM_DynTcidDestroyPool(uint16_t lcid)
{
    CM_DynTcidPool_S *node = CM_DynTransGetTcidPool(lcid);
    if (node == NULL) {
        // 已销毁，不做任何操作
        return CM_SUCCESS;
    }
    SDF_DListElmDel(&g_cmDynTransPoolListHead, node, entry);
    SDF_MemFree(node);
    node = NULL;
    CM_LOGI("CM_DynTcidDestroyPool lcid:%04x success", lcid);
    return CM_SUCCESS;
}

static uint8_t CM_DynAllocateUnicastTcid(CM_DynTcidPool_S *node, uint8_t minValue, uint8_t maxValue)
{
    for (uint8_t i = 0; i < (sizeof(node->unicastTcidUsed)) && i <= (maxValue - minValue); i++) {
        if (!node->unicastTcidUsed[i]) {
            uint8_t tcid = i + minValue;
            node->unicastTcidUsed[i] = true;
            CM_LOGD("dyn get a valid tcid:0x%02x", tcid);
            return tcid;
        }
    }
    CM_LOGE("all tcid is allocated, minValue:0x%02x, maxValue:0x%02x", minValue, maxValue);
    return CM_TRANS_INVALID_TCID;
}

static uint32_t CM_DynReturnAllocatedUnicastTcid(CM_DynTcidPool_S *node, uint8_t tcid, uint8_t minValue,
    uint8_t maxValue)
{
    if (tcid < minValue || tcid > maxValue || ((tcid - minValue) >= sizeof(node->unicastTcidUsed))) {
        CM_LOGE("tcid is invalid, tcid:0x%02x, minValue:0x%02x, maxValue:0x%02x", tcid, minValue, maxValue);
        return CM_FAIL;
    }
    if (!node->unicastTcidUsed[tcid - minValue]) {
        CM_LOGE("tcid is not allocated, tcid:0x%02x, minValue:0x%02x", tcid, minValue);
        return CM_FAIL;
    }
    node->unicastTcidUsed[tcid - minValue] = false;
    CM_LOGI("dyn return a allocated lcid:0x%04x, tcid:0x%02x", node->lcid, tcid);
    return CM_SUCCESS;
}

uint8_t CM_DynTcidAllocate(uint16_t lcid, uint8_t transportMode)
{
    CM_DynTcidPool_S *node = CM_DynTransGetTcidPool(lcid);
    if (node == NULL) {
        CM_LOGE("get tcid pool failed, lcid:0x%04x", lcid);
        return CM_TRANS_INVALID_TCID;
    }
    uint8_t newTcid = CM_TRANS_INVALID_TCID;

    switch (transportMode) {
        case CM_ACCESS_TRANS_MODE_UNICAST:
            newTcid = CM_DynAllocateUnicastTcid(node, CM_TCID_UC_BEGIN, CM_TCID_UC_END);
            break;
        default:
            CM_LOGE("transportMode[%hhu] not support", transportMode);
            return newTcid;
    }
    return newTcid;
}

uint32_t CM_DynTcidRelease(uint16_t lcid, uint8_t tcid, uint8_t transportMode)
{
    CM_DynTcidPool_S *node = CM_DynTransGetTcidPool(lcid);
    if (node == NULL) {
        CM_LOGE("get tcid pool failed, lcid:0x%04x", lcid);
        return CM_FAIL;
    }
    uint32_t ret = CM_FAIL;
    switch (transportMode) {
        case CM_ACCESS_TRANS_MODE_UNICAST:
            ret = CM_DynReturnAllocatedUnicastTcid(node, tcid, CM_TCID_UC_BEGIN, CM_TCID_UC_END);
            break;
        default:
            CM_LOGE("transportMode[%hhu] not support", transportMode);
            return ret;
    }
    return ret;
}