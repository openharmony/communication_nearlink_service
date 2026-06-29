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

#include "sle_logic_link_mgr.h"
#include "sdf_mem.h"
#include "securec.h"
#include "cm_log.h"
#include "cm_def.h"

static SDF_DListHead_S g_deviceLinkListHead = {
    .list = {&(g_deviceLinkListHead).list, &(g_deviceLinkListHead).list}, .size = 0};

void SleLogicLinkInit(void)
{
    // 容错性处理: 系统初始化时，此时链表大小应为0，否则存在没有执行销毁系统的情况
    uint32_t size = (uint32_t)SDF_DListCount(&g_deviceLinkListHead);
    if (size != 0) {
        CM_LOGE("logic link list size:%u, not empty, please check, first deinit", size);
        SleLogicLinkDeInit();
    }
    SDF_DListHeadInit(&g_deviceLinkListHead);
}

static void SleLogicLinkFreeNode(SDF_DListEntry_S *node)
{
    SleLogicLink_S *link = (SleLogicLink_S *)node;
    if (link == NULL) {
        return;
    }
    SDF_MemFree(link);
    link = NULL;
}

void SleLogicLinkDeInit(void)
{
    SDF_DListDestroy(&g_deviceLinkListHead, SleLogicLinkFreeNode);
    CM_LOGI("deinit success");
}

SleLogicLink_S *SleLogicLinkAdd(SLE_Addr_S *addr)
{
    SleLogicLink_S *node = (SleLogicLink_S *)SDF_MemZalloc(sizeof(SleLogicLink_S));
    if (node == NULL) {
        return NULL;
    }
    node->lcid = CM_INVALID_LCID;
    node->protocolVersion = CM_INVALID_VERSION;
    node->exchangeVersion = CM_INVALID_VERSION;
    (void)memcpy_s(&node->rmtAddr, sizeof(node->rmtAddr), addr, sizeof(SLE_Addr_S));
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(&g_deviceLinkListHead, node, entry);
    CM_LOGI("add a node addr:%s, logic link size:%u", GET_ENC_ADDR(addr), SDF_DListCount(&g_deviceLinkListHead));
    return node;
}

void SleLogicLinkRemove(SleLogicLink_S *node)
{
    CM_LOGI("remove a node addr:%s, lcid:0x%04x", GET_ENC_ADDR(&node->rmtAddr), node->lcid);
    SDF_DListElmDel(&g_deviceLinkListHead, node, entry);
    CM_LOGD("after remove the node, logic link size:%u", SDF_DListCount(&g_deviceLinkListHead));
    SleLogicLinkFreeNode((SDF_DListEntry_S *)node);
}

SleLogicLink_S *SleLogicLinkGetByAddr(const SLE_Addr_S *addr)
{
    CM_LOGD("logic link size:%u, addr:%s", SDF_DListCount(&g_deviceLinkListHead), GET_ENC_ADDR(addr));
    if (SDF_DListIsEmpty(&g_deviceLinkListHead)) {
        CM_LOGD("device link list is empty");
        return NULL;
    }

    SleLogicLink_S *node = NULL;
    SDF_DListElmForeach(node, &(g_deviceLinkListHead), entry) {
        if (memcmp(&node->rmtAddr, addr, sizeof(SLE_Addr_S)) == 0) {
            return node;
        }
    }
    return NULL;
}

SleLogicLink_S *SleLogicLinkGetByLcid(uint16_t lcid)
{
    CM_LOGD("logic link size:%u, lcid:0x%04x", SDF_DListCount(&g_deviceLinkListHead), lcid);
    if (SDF_DListIsEmpty(&g_deviceLinkListHead)) {
        CM_LOGD("device link list is empty");
        return NULL;
    }
    SleLogicLink_S *node = NULL;
    SDF_DListElmForeach(node, &(g_deviceLinkListHead), entry) {
        if (node->lcid == lcid) {
            return node;
        }
    }
    return NULL;
}

SleLogicLink_S *SleLogicLinkGetByStatus(uint16_t status)
{
    CM_LOGI("logic link size:%u, status:0x%04x", SDF_DListCount(&g_deviceLinkListHead), status);
    if (SDF_DListIsEmpty(&g_deviceLinkListHead)) {
        CM_LOGD("device link list is empty");
        return NULL;
    }
    SleLogicLink_S *node = NULL;
    SDF_DListElmForeach(node, &(g_deviceLinkListHead), entry) {
        if (node->status == status) {
            return node;
        }
    }
    return NULL;
}

uint32_t SleLogicLinkGetConnectedSize(void)
{
    if (SDF_DListIsEmpty(&g_deviceLinkListHead)) {
        return 0;
    }
    uint32_t size = 0;
    SleLogicLink_S *node = NULL;
    SDF_DListElmForeach(node, &(g_deviceLinkListHead), entry) {
        if (node->status == CM_LINK_STATE_CONNECTED) {
            size++;
        }
    }
    return size;
}