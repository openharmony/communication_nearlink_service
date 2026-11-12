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

#include <gtest/gtest.h>
#include <pthread.h>
#include <stdlib.h>

#include "sdf_buff.h"
#include "sdf_dlist.h"
#include "sdf_map.h"
#include "sdf_mem.h"
#include "sdf_slist.h"
#include "sdf_vector.h"

extern SDF_MemHooks_S g_stMemfunc;

namespace {
int g_dtorCount = 0;

void CountingDtor(void *ptr)
{
    g_dtorCount++;
    SDF_MemFree(ptr);
}

void FreeSListEntry(SDF_SListEntry_S *entry)
{
    SDF_MemFree(entry);
}

void FreeDListEntry(SDF_DListEntry_S *entry)
{
    SDF_MemFree(entry);
}

int IntCmp(const void *lhs, const void *rhs)
{
    return *(static_cast<const int *>(lhs)) - *(static_cast<const int *>(rhs));
}

int IntSortDesc(const void *lhs, const void *rhs)
{
    // qsort 传入的是元素地址，容器元素本身是指针，需要二级取值
    int l = **static_cast<int *const *>(lhs);
    int r = **static_cast<int *const *>(rhs);
    return r - l;
}

struct TestNode {
    SDF_DListEntry_S entry;
    int value;
};

struct SListNode {
    SDF_SListEntry_S entry;
    int value;
};

bool MatchValue(void *ptr, void *args)
{
    return ptr != nullptr && args != nullptr && *(static_cast<int *>(ptr)) == *(static_cast<int *>(args));
}
} // namespace

TEST(SdfVectorTest, CreateWithZeroCapacityFails)
{
    // 容量为 0 的创建请求被拒绝，返回空指针
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    EXPECT_EQ(SDF_CreateVectorByCapacity(0, traits), nullptr);
}

TEST(SdfVectorTest, EmplaceBackRejectsInvalidArgs)
{
    // 向量或元素为空指针时入队失败，不会崩溃
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    int value = 1;
    EXPECT_FALSE(SDF_VectorEmplaceBack(nullptr, &value));
    EXPECT_FALSE(SDF_VectorEmplaceBack(vector, nullptr));
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, EmplaceBackGrowsBeyondCapacity)
{
    // 元素数超过初始容量时自动扩容，已存元素不丢失
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVectorByCapacity(2, traits);
    ASSERT_NE(vector, nullptr);
    int values[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, &values[i]));
    }
    EXPECT_EQ(vector->size, static_cast<size_t>(5));
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(SDF_VectorElementAt(vector, i), &values[i]);
    }
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, CleanVectorInvokesDtorForEachElement)
{
    // 清空向量时逐个调用析构钩子并归零长度，钩子参数为空则只清长度
    g_dtorCount = 0;
    SDF_Traits traits = {.dtor = CountingDtor, .cmptor = nullptr};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    for (int i = 0; i < 3; i++) {
        int *item = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
        ASSERT_NE(item, nullptr);
        *item = i;
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, item));
    }
    SDF_CleanVector(vector);
    EXPECT_EQ(g_dtorCount, 3);
    EXPECT_EQ(vector->size, static_cast<size_t>(0));

    SDF_CleanVector(nullptr);
    SDF_DestroyVector(nullptr);
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, RemoveAndRemoveLastGuardInvalidInput)
{
    // 向量为空或下标越界时删除操作静默返回，不崩溃
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    SDF_VectorRemove(nullptr, 0);
    SDF_VectorRemove(vector, 0); // 空向量，下标越界
    SDF_VectorRemoveLast(nullptr);
    SDF_VectorRemoveLast(vector); // 空向量
    EXPECT_EQ(vector->size, static_cast<size_t>(0));
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, RemoveMiddleMovesTailAndCallsDtor)
{
    // 删除中间元素时析构钩子只作用于被删元素，后续元素前移补位
    g_dtorCount = 0;
    SDF_Traits traits = {.dtor = CountingDtor, .cmptor = nullptr};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    int *items[3] = {nullptr, nullptr, nullptr};
    for (int i = 0; i < 3; i++) {
        items[i] = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
        ASSERT_NE(items[i], nullptr);
        *items[i] = (i + 1) * 10;
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, items[i]));
    }
    SDF_VectorRemove(vector, 1);
    EXPECT_EQ(g_dtorCount, 1);
    EXPECT_EQ(vector->size, static_cast<size_t>(2));
    EXPECT_EQ(SDF_VectorElementAt(vector, 0), items[0]);
    EXPECT_EQ(SDF_VectorElementAt(vector, 1), items[2]);

    SDF_VectorRemoveLast(vector);
    EXPECT_EQ(vector->size, static_cast<size_t>(1));
    SDF_DestroyVector(vector); // 析构钩子释放剩余堆元素
}

TEST(SdfVectorTest, FindFirstByStartIndexSkipsEarlierMatches)
{
    // 从指定起点查找时忽略起点之前的匹配项；非法参数直接返回未找到
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    int values[3] = {7, 7, 8};
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, &values[i]));
    }
    size_t index = 999;
    int target = 7;
    EXPECT_FALSE(SDF_VectorFindFirstByStartIndex(nullptr, MatchValue, &target, 0, &index));
    EXPECT_FALSE(SDF_VectorFindFirstByStartIndex(vector, nullptr, &target, 0, &index));
    EXPECT_FALSE(SDF_VectorFindFirstByStartIndex(vector, MatchValue, &target, vector->size, &index));
    EXPECT_FALSE(SDF_VectorFindFirstByStartIndex(vector, MatchValue, &target, 0, nullptr));
    EXPECT_TRUE(SDF_VectorFindFirstByStartIndex(vector, MatchValue, &target, 1, &index));
    EXPECT_EQ(index, static_cast<size_t>(1));

    EXPECT_TRUE(SDF_VectorFindFirst(vector, MatchValue, &target, &index));
    EXPECT_EQ(index, static_cast<size_t>(0));
    int missing = 99;
    EXPECT_FALSE(SDF_VectorFindFirst(vector, MatchValue, &missing, &index));
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, ElementAtAndPopGuardBounds)
{
    // 越界访问返回空指针；弹出中间元素返回该元素且长度收缩
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    int values[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, &values[i]));
    }
    EXPECT_EQ(SDF_VectorElementAt(nullptr, 0), nullptr);
    EXPECT_EQ(SDF_VectorElementAt(vector, 3), nullptr);
    EXPECT_EQ(SDF_VectorPopElement(nullptr, 0), nullptr);
    EXPECT_EQ(SDF_VectorPopElement(vector, 3), nullptr);

    EXPECT_EQ(SDF_VectorPopElement(vector, 1), &values[1]);
    EXPECT_EQ(vector->size, static_cast<size_t>(2));
    EXPECT_EQ(SDF_VectorElementAt(vector, 1), &values[2]);
    SDF_DestroyVector(vector);
}

TEST(SdfVectorTest, SortOrdersElementsByComparator)
{
    // 空向量或缺失比较函数时排序不生效也不崩溃；正常路径按比较函数重排
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    SDF_VectorSort(nullptr, IntSortDesc);
    SDF_VectorSort(vector, nullptr);
    SDF_VectorSort(vector, IntSortDesc); // 空向量
    int values[4] = {3, 1, 4, 2};
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(SDF_VectorEmplaceBack(vector, &values[i]));
    }
    SDF_VectorSort(vector, IntSortDesc);
    for (int i = 0; i < 4; i++) {
        int *item = static_cast<int *>(SDF_VectorElementAt(vector, i));
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(*item, 4 - i);
    }
    SDF_DestroyVector(vector);
}

TEST(SdfSListTest, AddRejectsNullArgsAndInsertsAfterPos)
{
    // 节点或位置为空时不插入；正常插入时节点挂在位置节点之后
    SDF_SListHead_S head;
    SDF_SListHeadInit(&head);
    SListNode node = {{nullptr}, 1};
    SDF_SListAdd(&node.entry, nullptr);
    SDF_SListAdd(nullptr, &head);
    EXPECT_TRUE(SDF_SListIsEmpty(&head));

    SDF_SListAdd(&node.entry, &head);
    EXPECT_FALSE(SDF_SListIsEmpty(&head));
    EXPECT_EQ(head.next, &node.entry);
}

TEST(SdfSListTest, DelGuardsAndRemovesNextNode)
{
    // 位置为空或已到尾部时删除返回空；正常删除摘除位置节点的下一个
    SDF_SListHead_S head;
    SDF_SListHeadInit(&head);
    EXPECT_EQ(SDF_SListDel(nullptr), nullptr);
    // 空表直删没有空表保护，返回链头自身；空表保护只在 HeadDel 里
    EXPECT_EQ(SDF_SListDel(&head), &head);

    SListNode first = {{nullptr}, 1};
    SListNode second = {{nullptr}, 2};
    SDF_SListAdd(&second.entry, &head);
    SDF_SListAdd(&first.entry, &head);
    EXPECT_EQ(SDF_SListDel(&head), &first.entry);
    EXPECT_EQ(head.next, &second.entry);
}

TEST(SdfSListTest, HeadDelOnEmptyListReturnsNull)
{
    // 空表头删返回空指针；非空表头删返回原首节点并推进链头
    SDF_SListHead_S head;
    SDF_SListHeadInit(&head);
    EXPECT_EQ(SDF_SListHeadDel(&head), nullptr);

    SListNode node = {{nullptr}, 5};
    SDF_SListAdd(&node.entry, &head);
    EXPECT_EQ(SDF_SListHeadDel(&head), &node.entry);
    EXPECT_TRUE(SDF_SListIsEmpty(&head));
}

TEST(SdfSListTest, AllFreeReleasesEveryNode)
{
    // 表头或钩子为空时不动作；正常路径逐个释放所有节点
    SDF_SListHead_S head;
    SDF_SListHeadInit(&head);
    SDF_SListAllFree(nullptr, FreeSListEntry);
    SDF_SListAllFree(&head, nullptr);

    for (int i = 0; i < 3; i++) {
        SListNode *node = static_cast<SListNode *>(SDF_MemZalloc(sizeof(SListNode)));
        ASSERT_NE(node, nullptr);
        node->value = i;
        SDF_SListAdd(&node->entry, &head);
    }
    SDF_SListAllFree(&head, FreeSListEntry);
    // 释放后链头仍指向已释放内存，须由调用方重新初始化才能复用
    SDF_SListHeadInit(&head);
    EXPECT_TRUE(SDF_SListIsEmpty(&head));
}

TEST(SdfDListTest, CountAndMoveGuardNullArgs)
{
    // 表头为空时计数返回 0；搬移任一侧为空时不动作
    EXPECT_EQ(SDF_DListCount(nullptr), 0U);

    SDF_DListHead_S dst;
    SDF_DListHead_S src;
    SDF_DListHeadInit(&dst);
    SDF_DListHeadInit(&src);
    SDF_DListMove(nullptr, &src);
    SDF_DListMove(&dst, nullptr);
    EXPECT_EQ(SDF_DListCount(&dst), 0U);
}

TEST(SdfDListTest, MoveSplicesAllNodesToTarget)
{
    // 搬移把源表全部节点接入目标表，源表归零
    SDF_DListHead_S dst;
    SDF_DListHead_S src;
    SDF_DListHeadInit(&dst);
    SDF_DListHeadInit(&src);
    TestNode node = {{nullptr, nullptr}, 1};
    SDF_DListElmTailInsert(&src, &node, entry);
    SDF_DListMove(&dst, &src);
    EXPECT_EQ(SDF_DListCount(&dst), 1U);
    EXPECT_EQ(SDF_DListCount(&src), 0U);
    EXPECT_EQ(SDF_DListFirst(&dst), &node.entry);
}

TEST(SdfDListTest, AddGuardsNullArgs)
{
    // 头、节点、前驱或后继任一为空时不插入
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);
    TestNode node = {{nullptr, nullptr}, 1};
    SDF_DListAdd(nullptr, &node.entry, &head.list, &head.list);
    SDF_DListAdd(&head, nullptr, &head.list, &head.list);
    SDF_DListAdd(&head, &node.entry, nullptr, &head.list);
    SDF_DListAdd(&head, &node.entry, &head.list, nullptr);
    EXPECT_EQ(SDF_DListCount(&head), 0U);
}

TEST(SdfDListTest, HeadAndTailInsertKeepOrder)
{
    // 头插与尾插维护双向链表顺序和计数
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);
    TestNode first = {{nullptr, nullptr}, 1};
    TestNode second = {{nullptr, nullptr}, 2};
    SDF_DListElmTailInsert(&head, &first, entry);
    SDF_DListElmTailInsert(&head, &second, entry);
    EXPECT_EQ(SDF_DListCount(&head), 2U);
    EXPECT_EQ(SDF_DListFirst(&head), &first.entry);
    EXPECT_EQ(SDF_DListLast(&head), &second.entry);

    TestNode third = {{nullptr, nullptr}, 3};
    SDF_DListElmHeadInsert(&head, &third, entry);
    EXPECT_EQ(SDF_DListFirst(&head), &third.entry);
    EXPECT_EQ(SDF_DListCount(&head), 3U);
}

TEST(SdfDListTest, DelGuardsAndResetsEntry)
{
    // 头或节点为空时删除返回空；正常删除后节点自指、计数减一
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);
    EXPECT_EQ(SDF_DListDel(nullptr, nullptr), nullptr);

    TestNode node = {{nullptr, nullptr}, 1};
    SDF_DListElmTailInsert(&head, &node, entry);
    EXPECT_EQ(SDF_DListDel(&head, &node.entry), &node.entry);
    EXPECT_EQ(SDF_DListCount(&head), 0U);
    EXPECT_EQ(node.entry.next, &node.entry);
    EXPECT_EQ(node.entry.prev, &node.entry);
}

TEST(SdfDListTest, PosConcatGuardsAndSplicesList)
{
    // 位置或源表为空、源表为空表时不拼接；正常拼接搬走全部节点
    SDF_DListHead_S dst;
    SDF_DListHead_S src;
    SDF_DListHeadInit(&dst);
    SDF_DListHeadInit(&src);
    TestNode node = {{nullptr, nullptr}, 1};
    SDF_DListElmTailInsert(&dst, &node, entry);
    SDF_DListPosConcat(nullptr, &src);
    SDF_DListPosConcat(&dst.list, nullptr);
    SDF_DListPosConcat(&dst.list, &src); // 源表为空
    EXPECT_EQ(SDF_DListCount(&dst), 1U);

    TestNode moved = {{nullptr, nullptr}, 2};
    SDF_DListElmTailInsert(&src, &moved, entry);
    SDF_DListPosConcat(&dst.list, &src);
    // 拼接只搬移节点链接，两侧计数不随之更新，计数语义按现行行为另行登记
    EXPECT_EQ(SDF_DListLast(&dst), &moved.entry);
    EXPECT_NE(SDF_DListFirst(&dst), &dst.list);
}

TEST(SdfDListTest, DestroyFreesAllNodes)
{
    // 头或钩子为空时不动作；正常路径释放全部节点并清空计数
    SDF_DListHead_S head;
    SDF_DListHeadInit(&head);
    SDF_DListDestroy(nullptr, FreeDListEntry);
    SDF_DListDestroy(&head, nullptr);

    for (int i = 0; i < 3; i++) {
        TestNode *node = static_cast<TestNode *>(SDF_MemZalloc(sizeof(TestNode)));
        ASSERT_NE(node, nullptr);
        node->value = i;
        SDF_DListElmTailInsert(&head, node, entry);
    }
    SDF_DListDestroy(&head, FreeDListEntry);
    EXPECT_EQ(SDF_DListCount(&head), 0U);
}

TEST(SdfMapTest, CtorRequiresCompleteTraits)
{
    // 键或值特征函数缺失时创建被拒绝；析构对空指针安全
    SDF_Traits full = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Traits keyNoDtor = {.dtor = nullptr, .cmptor = IntCmp};
    SDF_Traits keyNoCmp = {.dtor = SDF_MemFree, .cmptor = nullptr};
    SDF_Traits valNoDtor = {.dtor = nullptr, .cmptor = nullptr};
    EXPECT_EQ(SDF_MapCtor(keyNoDtor, full), nullptr);
    EXPECT_EQ(SDF_MapCtor(keyNoCmp, full), nullptr);
    EXPECT_EQ(SDF_MapCtor(full, valNoDtor), nullptr);
    SDF_MapDtor(nullptr);

    SDF_Map *map = SDF_MapCtor(full, full);
    ASSERT_NE(map, nullptr);
    SDF_MapDtor(map);
}

TEST(SdfMapTest, MoveInsertRejectsInvalidAndDuplicate)
{
    // 映射、键或值为空时插入失败；重复键插入失败且不覆盖原值
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Map *map = SDF_MapCtor(traits, traits);
    ASSERT_NE(map, nullptr);
    int *key = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    int *val = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    ASSERT_NE(key, nullptr);
    ASSERT_NE(val, nullptr);
    *key = 1;
    *val = 100;
    EXPECT_FALSE(SDF_MapMoveInsert(nullptr, key, val));
    EXPECT_FALSE(SDF_MapMoveInsert(map, nullptr, val));
    EXPECT_FALSE(SDF_MapMoveInsert(map, key, nullptr));
    EXPECT_TRUE(SDF_MapMoveInsert(map, key, val));

    int *dupKey = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    int *dupVal = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    ASSERT_NE(dupKey, nullptr);
    ASSERT_NE(dupVal, nullptr);
    *dupKey = 1;
    *dupVal = 200;
    EXPECT_FALSE(SDF_MapMoveInsert(map, dupKey, dupVal));
    SDF_MapIter *found = SDF_MapFind(map, dupKey);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*(static_cast<int *>(found->val)), 100);
    SDF_MemFree(dupKey);
    SDF_MemFree(dupVal);
    SDF_MapDtor(map);
}

TEST(SdfMapTest, FindGuardsAndMatchesByKey)
{
    // 映射或键为空时查找返回空；键相等语义由比较函数决定
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Map *map = SDF_MapCtor(traits, traits);
    ASSERT_NE(map, nullptr);
    int key = 1;
    EXPECT_EQ(SDF_MapFind(nullptr, &key), nullptr);
    EXPECT_EQ(SDF_MapFind(map, nullptr), nullptr);
    EXPECT_EQ(SDF_MapFind(map, &key), nullptr);

    int *heapKey = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    int *heapVal = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    ASSERT_NE(heapKey, nullptr);
    ASSERT_NE(heapVal, nullptr);
    *heapKey = 1;
    *heapVal = 100;
    EXPECT_TRUE(SDF_MapMoveInsert(map, heapKey, heapVal));
    // 栈上等值键也能命中（比较函数按值比较）
    EXPECT_NE(SDF_MapFind(map, &key), nullptr);
    int missing = 2;
    EXPECT_EQ(SDF_MapFind(map, &missing), nullptr);
    SDF_MapDtor(map);
}

TEST(SdfMapTest, EraseRemovesHeadMiddleAndRejectsUnknown)
{
    // 空参数或未知键删除失败；头、中、尾位置条目均可删除
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Map *map = SDF_MapCtor(traits, traits);
    ASSERT_NE(map, nullptr);
    int probe = 0;
    EXPECT_FALSE(SDF_MapErase(nullptr, &probe));
    EXPECT_FALSE(SDF_MapErase(map, nullptr));
    EXPECT_FALSE(SDF_MapErase(map, &probe));

    for (int i = 0; i < 3; i++) {
        int *key = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
        int *val = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
        ASSERT_NE(key, nullptr);
        ASSERT_NE(val, nullptr);
        *key = i;
        *val = i * 10;
        EXPECT_TRUE(SDF_MapMoveInsert(map, key, val));
    }
    int headKey = 0; // 后插在前：条目顺序为 2 -> 1 -> 0
    int midKey = 1;
    int firstKey = 2;
    EXPECT_TRUE(SDF_MapErase(map, &midKey)); // 删中间条目（非迭代起点）
    EXPECT_TRUE(SDF_MapErase(map, &firstKey)); // 删头插条目
    EXPECT_TRUE(SDF_MapErase(map, &headKey)); // 删最后条目
    EXPECT_EQ(SDF_MapFind(map, &headKey), nullptr);
    EXPECT_EQ(map->entry, nullptr);
    SDF_MapDtor(map);
}

TEST(SdfBuffTest, NewWithReserveKeepsHeadroomAndExtra)
{
    // 预留缓冲按头部预留加数据长度分配；额外预留叠加在头room与总长上
    SDF_Buff_S *plain = SDF_BuffNew(10);
    ASSERT_NE(plain, nullptr);
    EXPECT_EQ(plain->buffLen, 10U);
    SDF_BuffFree(plain);

    SDF_Buff_S *reserved = SDF_BuffNewWithReserve(10);
    ASSERT_NE(reserved, nullptr);
    EXPECT_EQ(SDF_BuffHeadRoom(reserved), static_cast<uint64_t>(SDF_BUFF_MAX_HEADROOM_SIZE));
    EXPECT_EQ(reserved->buffLen, 10U + SDF_BUFF_MAX_RESERVED_SIZE);
    SDF_BuffFree(reserved);

    SDF_Buff_S *extra = SDF_BuffNewWithExtraReserve(10, 4);
    ASSERT_NE(extra, nullptr);
    EXPECT_EQ(SDF_BuffHeadRoom(extra), static_cast<uint64_t>(SDF_BUFF_MAX_HEADROOM_SIZE) + 4);
    EXPECT_EQ(extra->buffLen, 10U + SDF_BUFF_MAX_RESERVED_SIZE + 4);
    SDF_BuffFree(extra);
}

TEST(SdfBuffTest, AppendAndPrependRespectRoom)
{
    // 追加/前插超过可用空间返回空；成功时数据区按预期扩展
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(4);
    ASSERT_NE(buff, nullptr);
    SDF_BuffFree(nullptr);
    EXPECT_EQ(SDF_BuffAppend(nullptr, 1), nullptr);
    EXPECT_EQ(SDF_BuffAppend(buff, 7), nullptr); // 可用尾部空间为预留尾room加数据长度共 6

    uint8_t *tail = SDF_BuffAppend(buff, 2);
    ASSERT_NE(tail, nullptr);
    EXPECT_EQ(SDF_DataLenGet(buff), 2U);

    EXPECT_EQ(SDF_BuffPrepend(nullptr, 1), nullptr);
    EXPECT_EQ(SDF_BuffPrepend(buff, SDF_BUFF_MAX_HEADROOM_SIZE + 1), nullptr);
    uint8_t *front = SDF_BuffPrepend(buff, 2);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(SDF_BuffHeadRoom(buff), static_cast<uint64_t>(SDF_BUFF_MAX_HEADROOM_SIZE) - 2);
    EXPECT_EQ(SDF_DataLenGet(buff), 4U);
    SDF_BuffFree(buff);
}

TEST(SdfBuffTest, TrimPrefixAndSuffixGuardDataLen)
{
    // 截去量超过数据长度时失败；成功时前截推进偏移、后截收缩长度
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(4);
    ASSERT_NE(buff, nullptr);
    ASSERT_NE(SDF_BuffAppend(buff, 4), nullptr);

    EXPECT_EQ(SDF_BuffTrimPrefix(nullptr, 1), nullptr);
    EXPECT_EQ(SDF_BuffTrimPrefix(buff, 5), nullptr);
    EXPECT_NE(SDF_BuffTrimPrefix(buff, 1), nullptr);
    EXPECT_EQ(SDF_DataLenGet(buff), 3U);

    EXPECT_EQ(SDF_BuffTrimSuffix(nullptr, 1), -1);
    EXPECT_EQ(SDF_BuffTrimSuffix(buff, 4), -1);
    EXPECT_EQ(SDF_BuffTrimSuffix(buff, 1), 0);
    EXPECT_EQ(SDF_DataLenGet(buff), 2U);
    SDF_BuffFree(buff);
}

TEST(SdfBuffTest, CopyDuplicatesBufferContent)
{
    // 空缓冲拷贝返回空；拷贝体与原缓冲头部字段及数据一致
    EXPECT_EQ(SDF_BuffCopy(nullptr), nullptr);
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(4);
    ASSERT_NE(buff, nullptr);
    uint8_t *data = SDF_BuffAppend(buff, 4);
    ASSERT_NE(data, nullptr);
    data[0] = 0x11;
    data[3] = 0x44;

    SDF_Buff_S *copy = SDF_BuffCopy(buff);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->dataOff, buff->dataOff);
    EXPECT_EQ(copy->dataLen, buff->dataLen);
    EXPECT_EQ(copy->buffLen, buff->buffLen);
    EXPECT_EQ(SDF_DataOffset(copy)[0], 0x11);
    EXPECT_EQ(SDF_DataOffset(copy)[3], 0x44);
    SDF_BuffFree(copy);
    SDF_BuffFree(buff);
}

namespace {
pthread_t g_mainThread;
int g_allocSkip = -1;
int g_allocSeq = 0;

// 故障注入分配钩子：仅主线程从第 skip+1 次分配起失败，后台线程放行
void *MainThreadFailAlloc(size_t size)
{
    if (g_allocSkip >= 0 && pthread_equal(pthread_self(), g_mainThread) && ++g_allocSeq > g_allocSkip) {
        return nullptr;
    }
    return malloc(size);
}
} // namespace

class SdfMemFaultTest : public testing::Test {
protected:
    void SetUp() override
    {
        g_mainThread = pthread_self();
        m_saved = g_stMemfunc;
        SDF_MemHooks_S hook = {.mAlloc = MainThreadFailAlloc, .mFree = m_saved.mFree};
        ASSERT_EQ(SDF_MemHookReg(&hook), SDF_OK);
        g_allocSkip = -1;
        g_allocSeq = 0;
    }
    void TearDown() override
    {
        g_allocSkip = -1;
        g_stMemfunc = m_saved;
    }
    void FailAfterMainAllocs(int skip)
    {
        g_allocSeq = 0;
        g_allocSkip = skip;
    }
    SDF_MemHooks_S m_saved = {};
};

TEST_F(SdfMemFaultTest, VectorCreateBufAllocFails)
{
    // 第二级分配（元素缓冲）失败时向量创建返回空并回收描述结构
    FailAfterMainAllocs(1);
    SDF_Traits traits = {};
    EXPECT_EQ(SDF_CreateVector(traits), nullptr);
    g_allocSkip = -1;
}

TEST_F(SdfMemFaultTest, VectorGrowAllocFails)
{
    // 扩容分配失败时入队返回失败，已有元素不受影响
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVectorByCapacity(2, traits);
    ASSERT_NE(vector, nullptr);
    int values[3] = {1, 2, 3};
    ASSERT_TRUE(SDF_VectorEmplaceBack(vector, &values[0]));
    ASSERT_TRUE(SDF_VectorEmplaceBack(vector, &values[1]));

    FailAfterMainAllocs(0);
    EXPECT_FALSE(SDF_VectorEmplaceBack(vector, &values[2]));
    g_allocSkip = -1;
    EXPECT_EQ(vector->size, static_cast<size_t>(2));
    SDF_DestroyVector(vector);
}

TEST_F(SdfMemFaultTest, MapCtorAllocFails)
{
    // 描述结构分配失败时映射创建返回空
    FailAfterMainAllocs(0);
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    EXPECT_EQ(SDF_MapCtor(traits, traits), nullptr);
    g_allocSkip = -1;
}

TEST_F(SdfMemFaultTest, MapMoveInsertNodeAllocFails)
{
    // 条目节点分配失败时插入返回失败，既有条目不受影响
    SDF_Traits traits = {.dtor = SDF_MemFree, .cmptor = IntCmp};
    SDF_Map *map = SDF_MapCtor(traits, traits);
    ASSERT_NE(map, nullptr);
    int *key = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    int *val = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    ASSERT_NE(key, nullptr);
    ASSERT_NE(val, nullptr);
    *key = 1;
    *val = 100;
    ASSERT_TRUE(SDF_MapMoveInsert(map, key, val));

    int *key2 = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    int *val2 = static_cast<int *>(SDF_MemZalloc(sizeof(int)));
    ASSERT_NE(key2, nullptr);
    ASSERT_NE(val2, nullptr);
    *key2 = 2;
    *val2 = 200;
    FailAfterMainAllocs(0);
    EXPECT_FALSE(SDF_MapMoveInsert(map, key2, val2));
    g_allocSkip = -1;
    SDF_MemFree(key2);
    SDF_MemFree(val2);
    EXPECT_NE(SDF_MapFind(map, key), nullptr);
    SDF_MapDtor(map);
}

TEST_F(SdfMemFaultTest, BuffNewAllocFailsInAllVariants)
{
    // 三种缓冲构造在分配失败时都返回空
    FailAfterMainAllocs(0);
    EXPECT_EQ(SDF_BuffNew(8), nullptr);
    EXPECT_EQ(SDF_BuffNewWithReserve(8), nullptr);
    EXPECT_EQ(SDF_BuffNewWithExtraReserve(8, 4), nullptr);
    g_allocSkip = -1;
}

TEST_F(SdfMemFaultTest, BuffCopyAllocFails)
{
    // 拷贝分配失败时返回空，源缓冲完好
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(4);
    ASSERT_NE(buff, nullptr);
    ASSERT_NE(SDF_BuffAppend(buff, 4), nullptr);

    FailAfterMainAllocs(0);
    EXPECT_EQ(SDF_BuffCopy(buff), nullptr);
    g_allocSkip = -1;
    EXPECT_EQ(buff->dataLen, 4U);
    SDF_BuffFree(buff);
}

TEST(SdfVectorTest, CleanVectorWithoutDtorKeepsElements)
{
    // 无析构钩子的向量清空只归零长度，元素本身不动
    SDF_Traits traits = {};
    SDF_Vector_S *vector = SDF_CreateVector(traits);
    ASSERT_NE(vector, nullptr);
    int values[2] = {1, 2};
    ASSERT_TRUE(SDF_VectorEmplaceBack(vector, &values[0]));
    ASSERT_TRUE(SDF_VectorEmplaceBack(vector, &values[1]));

    SDF_CleanVector(vector);
    EXPECT_EQ(vector->size, static_cast<size_t>(0));
    SDF_DestroyVector(vector); // 无钩子不释放栈上元素
}
