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

#include "sdf_slist.h"

#include "stddef.h"

void SDF_SListAdd(SDF_SListEntry_S *node, SDF_SListEntry_S *pos)
{
    if (node == NULL || pos == NULL) {
        return;
    }
    node->next = pos->next;
    pos->next = node;
}

SDF_SListEntry_S *SDF_SListDel(SDF_SListEntry_S *pos)
{
    if (pos == NULL || pos->next == NULL) {
        return NULL;
    }
    SDF_SListEntry_S *ret;
    ret = pos->next;
    pos->next = pos->next->next;
    return ret;
}

SDF_SListEntry_S *SDF_SListHeadDel(SDF_SListHead_S *head)
{
    if (SDF_SListIsEmpty(head)) {
        return NULL;
    }
    return SDF_SListDel(head);
}

void SDF_SListAllFree(SDF_SListHead_S *head, SDF_SListFreeHook freeHook)
{
    if (head == NULL ||freeHook == NULL) {
        return;
    }
    SDF_SListEntry_S *entry = head->next;
    SDF_SListEntry_S *tmp = NULL;
    while (entry != head) {
        tmp = entry->next;
        freeHook(entry);
        entry = tmp;
    }
}