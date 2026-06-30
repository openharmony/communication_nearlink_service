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

#include "sdf_dlist.h"

#include "stddef.h"

uint32_t SDF_DListCount(SDF_DListHead_S *head)
{
    if (head == NULL) {
        return 0;
    }
    return head->size;
}

void SDF_DListMove(SDF_DListHead_S *dst, SDF_DListHead_S *src)
{
    if (dst == NULL || src == NULL) {
        return;
    }
    dst->list.next = src->list.next;
    dst->list.prev  = src->list.prev;
    src->list.next->prev = &(dst->list);
    src->list.prev->next = &(dst->list);
    dst->size += src->size;
    SDF_DListHeadInit(src);
}

void SDF_DListAdd(SDF_DListHead_S *head, SDF_DListEntry_S *node, SDF_DListEntry_S *prev, SDF_DListEntry_S *next)
{
    if (head == NULL || node == NULL || prev == NULL || next == NULL) {
        return;
    }
    node->next = next;
    node->prev = prev;
    next->prev = node;
    prev->next = node;
    head->size++;
}

SDF_DListEntry_S *SDF_DListDel(SDF_DListHead_S *head, SDF_DListEntry_S *node)
{
    if (head == NULL || node == NULL) {
        return NULL;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    SDF_DListEntryInit(node);
    head->size--;
    return node;
}

static inline void SDF_DListConcat(SDF_DListEntry_S *first, SDF_DListEntry_S *last,
    SDF_DListEntry_S *prev, SDF_DListEntry_S *next)
{
    first->prev = prev;
    prev->next = first;
    last->next = next;
    next->prev = last;
}

void SDF_DListPosConcat(SDF_DListEntry_S *pos, SDF_DListHead_S *src)
{
    if (pos == NULL || src == NULL) {
        return;
    }
    if (SDF_DListIsEmpty(src)) {
        return;
    }
    SDF_DListConcat(pos, pos->next, src->list.prev, src->list.next);
    SDF_DListHeadInit(src);
    src->size++;
}

void SDF_DListDestroy(SDF_DListHead_S *head, SDF_DListNodeFreeHook freeHook)
{
    if (head == NULL || freeHook == NULL) {
        return;
    }
    SDF_DListEntry_S *entry = head->list.next;
    SDF_DListEntry_S *tmp = NULL;
    while (!SDF_DListIsEmpty(head)) {
        tmp = entry->next;
        if (freeHook != NULL) {
            freeHook(entry);
        }
        entry = tmp;
        head->size--;
    }
}