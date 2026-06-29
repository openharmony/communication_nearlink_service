/**
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
 * @file         sdf_slist.h
 * @brief        通用单向链表
*/

#ifndef SDF_SLIST_H
#define SDF_SLIST_H

#include "sdf_typedef.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief     初始化链表头
 * @param[IN] head 链表头对象指针
 */
#define SDF_SListHeadInit(head) ((head)->next = (head))

/**
 * @brief     初始化链表节点头
 * @param[IN] entry 链表节点头对象指针
 */
#define SDF_SListEntryInit(entry) ((entry)->next = (entry))

/**
 * @brief     判断当前链表是否为空
 * @param[IN] head 链表头对象指针
 * @return    bool
 */
#define SDF_SListIsEmpty(head) ((head)->next == (head))

/**
 * @brief     插入链表节点
 * @param[IN] node 待插入链表节点指针
 * @param[IN] pos 插入位置的前一个元素
 */
void SDF_SListAdd(SDF_SListEntry_S *node, SDF_SListEntry_S *pos);

/**
 * @brief     使用头插法插入链表节点
 * @param[IN] head 链表头对象指针
 * @param[IN] node 链表节点对象指针
 */
#define SDF_SListHeadInsert(head, node) SDF_SListAdd((node), (head))

/**
 * @brief     使用头插法插入链表节点元素
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_SListElmHeadInsert(head, elm, field) SDF_SListAdd(&((elm)->field), (head))

/**
 * @brief     链表成员使用 foreach 方式遍历
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_SListElmForeach(elm, head, field)                                      \
    for ((elm) = SDF_CONTAINER_OF((head)->next, SDF_TYPE_OF(*(elm)), field);      \
         &((elm)->field) != (head);                                                 \
         (elm) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field))

/**
 * @brief     链表成员使用 foreach 方式遍历
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_SListElmSafeForeach(elm, tmp, head, field)                             \
    for ((elm) = SDF_CONTAINER_OF((head)->next, SDF_TYPE_OF(*(elm)), field),      \
         (tmp) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field);   \
         &((elm)->field) != (head); (elm) = (tmp),                                  \
         (tmp) = SDF_CONTAINER_OF((tmp)->field.next, SDF_TYPE_OF(*(tmp)), field))

/**
 * @brief     移除当前节点的下一个链表节点
 * @param[IN] node 待移除链表节点的前一个链表节点指针
 */
SDF_SListEntry_S *SDF_SListDel(SDF_SListEntry_S *pos);

#define SDF_SListElmDel(posElm, field) \
    SDF_CONTAINER_OF(SDF_SListDel((&(posElm)->field)), SDF_TYPE_OF(*(elm)), field)

SDF_SListEntry_S *SDF_SListHeadDel(SDF_SListHead_S *head);

/**
 * @brief     移除链表头部节点
 * @param[IN] head 链表头对象指针
 * @param[OUT] node 返回的被移除的节点元素指针
 * @remark 用户需要判断 node 是否为 head
 */
#define SDF_SListElmHeadDel(head, elm, field)  \
    (elm) = SDF_CONTAINER_OF(SDF_SListDel(head), SDF_TYPE_OF(*(elm)), field)

#define SDF_SListElmSafeHeadDel(head, elm, field)                          \
    (elm) = SDF_SListIsEmpty(head) ? NULL :                                \
        SDF_CONTAINER_OF(SDF_SListDel(head), SDF_TYPE_OF(*(elm)), field)

typedef void (*SDF_SListFreeHook)(SDF_SListEntry_S *entry);

/**
 * @brief     使用通用释放元素回调释放链表中所有有效元素
 * @param[IN] head 链表头对象指针
 * @param[IN] freeHook 通用释放元素回调, 回调内部将 entry 转换成具体数据成员
 */
void SDF_SListAllFree(SDF_SListHead_S *head, SDF_SListFreeHook freeHook);

/**
 * @brief     使用原生元素销毁回调释放链表中所有有效元素
 * @param[IN] elm 能被原生回调 freeHook 直接释放节点元素指针变量
 * @param[IN] tmp 遍历链表时所使用到的临时元素节点变量
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[IN] freeHook 原生元素销毁回调
 */
#define SDF_SListElmAllFree(elm, tmp, head, field, freeHook)                           \
    do {                                                                                \
        (elm) = SDF_CONTAINER_OF((head)->next, SDF_TYPE_OF(*(elm)), field);           \
        while (&((elm)->field) != (head)) {                                               \
            (tmp) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field);  \
            freeHook((elm));                                                            \
            (elm) = (tmp);                                                              \
        }                                                                               \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // SDF_SLIST_H