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
 * @file         sdf_dlist.h
 * @brief        通用双向链表
*/

#ifndef SDF_DLIST_H
#define SDF_DLIST_H

#include "sdf_typedef.h"
#include "sdf_slist.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief     初始化链表头
 * @param[IN] head 链表头对象指针
 */
#define SDF_DListHeadInit(head)       \
    do {                              \
        (head)->list.next = &((head)->list);     \
        (head)->list.prev = &((head)->list);     \
        (head)->size = 0;             \
    } while (0)

/**
 * @brief     初始化链表节点头
 * @param[IN] entry 链表节点头对象指针
 */
#define SDF_DListEntryInit(entry)     \
    do {                              \
        (entry)->next = (entry);      \
        (entry)->prev = (entry);      \
    } while (0)

/**
 * @brief     判断当前链表是否为空
 * @param[IN] head 链表头对象指针
 * @return    bool
 */
#define SDF_DListIsEmpty(head) ((head)->size == 0)

/**
 * @brief     获取链表头部节点元素
 * @param[IN] head 链表头对象指针
 */
#define SDF_DListFirst(head)      (head)->list.next

/**
 * @brief     获取链表尾部节点元素
 * @param[IN] head 链表头对象指针
 */
#define SDF_DListLast(head)       (head)->list.prev


/**
 * @brief     获取链表当前节点的下一个节点
 * @param[IN] node 当前节点
 */
#define SDF_DListNext(node) (node)->next

/**
 * @brief     获取链表当前节点的前一个节点
 * @param[IN] node 当前节点
 */
#define SDF_DListPrev(node) (node)->prev

/**
 * @brief     获取双向链表有效节点数量
 * @param[IN] head 链表头指针
 * @return 元素个数
 */
uint32_t SDF_DListCount(SDF_DListHead_S *head);

/**
 * @brief     使用源链表中的所有元素移动到目标链表
 * @param[IN] src 源链表头指针, 可以为空, 但用户保证已初始化
 * @param[OUT] dst 目标链表的链表头指针
 * @remark 用户保证目标链表头为空
 */
void SDF_DListMove(SDF_DListHead_S *dst, SDF_DListHead_S *src);

/**
 * @brief     插入链表节点
 * @param[IN] head 链表头对象指针
 * @param[IN] node 待插入链表节点指针
 * @param[IN] prv 插入位置的前一个元素
 * @param[IN] nxt 插入位置的后一个元素
 */
void SDF_DListAdd(SDF_DListHead_S *head, SDF_DListEntry_S *node, SDF_DListEntry_S *prev, SDF_DListEntry_S *next);

/**
 * @brief     使用头插法插入链表节点元素
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_DListElmHeadInsert(head, elm, field) \
    SDF_DListAdd(head, &((elm)->field), &(head)->list, SDF_DListFirst(head))

/**
 * @brief     使用尾插法插入链表节点
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_DListElmTailInsert(head, elm, field) \
    SDF_DListAdd((head), &((elm)->field), SDF_DListLast(head), &(head)->list)

/**
 * @brief     链表成员使用 foreach 方式遍历
 * @param[IN] elm 链表节点元素指针
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_DListElmForeach(elm, head, field)                                      \
    for ((elm) = SDF_CONTAINER_OF((head)->list.next, SDF_TYPE_OF(*(elm)), field);      \
         &((elm)->field) != &((head)->list);                                                 \
         (elm) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field))
/**
 * @brief     链表成员使用 foreach 方式遍历
 * @param[IN] elm 链表节点元素指针
 * @param[IN] tmp 遍历链表时所使用到的临时元素节点指针
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_DListElmSafeForeach(elm, tmp, head, field)                             \
    for ((elm) = SDF_CONTAINER_OF((head)->list.next, SDF_TYPE_OF(*(elm)), field),      \
         (tmp) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field); \
         &((elm)->field) != &((head)->list); (elm) = (tmp),                                  \
         (tmp) = SDF_CONTAINER_OF((tmp)->field.next, SDF_TYPE_OF(*(tmp)), field))

/**
 * @brief     移除链表节点
 * @param[IN] head 链表头对象指针
 * @param[IN] node 待移除链表节点指针
 */
SDF_DListEntry_S *SDF_DListDel(SDF_DListHead_S *head, SDF_DListEntry_S *node);

/**
 * @brief     移除链表节点元素
 * @param[IN] head 链表头对象指针
 * @param[IN] elm 待移除链表节点元素指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 */
#define SDF_DListElmDel(head, elm, field)    SDF_DListDel((head), &((elm)->field))

/**
 * @brief     移除链表节点并转换成节点元素
 * @param[IN] head 链表头对象指针
 * @param[IN] node 待移除链表节点指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 * @remark 用户需要判断 elm 是否无效, 即其由 head 节点转换而来, 此为非法
 */
#define SDF_DListElmNodeDel(head, node, elm, field)  \
    (elm) = SDF_CONTAINER_OF(SDF_DListDel((head), (node)), SDF_TYPE_OF(*(elm)), field)

/**
 * @brief     移除链表头部节点
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 * @remark 用户需要判断 elm 是否无效, 即其由 head 节点转换而来, 此为非法
 */
#define SDF_DListElmHeadDel(head, elm, field)  SDF_DListElmNodeDel((head), SDF_DListFirst(head), elm, field)

/**
 * @brief     移除链表尾部部节点
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 * @remark 用户需要判断 elm 是否无效, 即其由 head 节点转换而来, 此为非法
 */
#define SDF_DListElmTailDel(head, elm, field)  SDF_DListElmNodeDel((head), SDF_DListLast(head), elm, field)

/**
 * @brief     判空后移除链表节点并转换成节点元素
 * @param[IN] head 链表头对象指针
 * @param[IN] node 待移除链表节点指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 */
#define SDF_DListElmSafeNodeDel(head, node, elm, field)    \
    (elm) = SDF_DListIsEmpty(head) ? NULL : SDF_CONTAINER_OF(SDF_DListDel((head), (node)), SDF_TYPE_OF(*(elm)), field)

/**
 * @brief     判空后移除链表头部节点
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 */
#define SDF_DListElmSafeHeadDel(head, elm, field)  SDF_DListElmSafeNodeDel((head), SDF_DListFirst(head), elm, field)

/**
 * @brief     判空后移除链表尾部部节点
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[OUT] elm 返回的被移除的节点元素指针
 */
#define SDF_DListElmSafeTailDel(head, elm, field)  SDF_DListElmSafeNodeDel((head), SDF_DListLast(head), elm, field)

void SDF_DListPosConcat(SDF_DListEntry_S *pos, SDF_DListHead_S *src);

#define SDF_DListHeadConcat(dst, src) SDF_DListPosConcat(dst, src)

#define SDF_DListTailConcat(dst, src) SDF_DListPosConcat(SDF_DListLast(dst), src)

/**
 * @brief     链表节点元素释放回调原型
 * @param[IN] entry 链表节点元素的节点成员对象
 */
typedef void (*SDF_DListNodeFreeHook)(SDF_DListEntry_S *entry);

/**
 * @brief     使用通用释放元素回调释放链表中所有有效元素
 * @param[IN] head 链表头对象指针
 * @param[IN] freeHook 通用释放元素回调, 回调内部将 entry 转换成具体数据成员
 * @remark 链表头不释放
 */
void SDF_DListDestroy(SDF_DListHead_S *head, SDF_DListNodeFreeHook freeHook);

/**
 * @brief     使用原生元素销毁回调释放链表中所有有效元素
 * @param[IN] elm 能被原生回调 freeHook 直接释放节点元素指针变量
 * @param[IN] tmp 遍历链表时所使用到的临时元素节点变量
 * @param[IN] head 链表头对象指针
 * @param[IN] field 链表节点元素对象的节点头成员名
 * @param[IN] freeHook 原生元素销毁回调
 */
#define SDF_DListElmAllFree(elm, tmp, head, field, freeHook)                            \
    do {                                                                                \
        (elm) = SDF_CONTAINER_OF((head)->list.next, SDF_TYPE_OF(*(elm)), field);        \
        while (&((elm)->field) != &((head)->list)) {                                   \
            (tmp) = SDF_CONTAINER_OF((elm)->field.next, SDF_TYPE_OF(*(elm)), field);    \
            freeHook((elm));                                                            \
            (elm) = (tmp);                                                              \
        }                                                                               \
        SDF_DListHeadInit(head);                                                        \
    } while (0)
#ifdef __cplusplus
}
#endif

#endif // SDF_DLIST_H