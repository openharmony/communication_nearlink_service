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
 * Description: 用于实现C函数的动态换桩功能
 */
#include <errno.h>
#include <limits.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "stub_replace.h"
#include "securec.h"

enum ServType {
    ZERO_NUM,
    ONE_NUM,
    TWO_NUM,
    THREE_NUM,
    FOUR_NUM,
};

/*
 * 用于记录系统内存页大小
 */
static long g_pageSize = -1;

/*
 * 得到指定函数代码所在内存页的首地址
 * fn - 函数地址(函数指针)
 */
static inline void* FuncPageGet(uintptr_t fn)
{
    return (void*)(fn & (~(g_pageSize - 1)));
}

/*
 * 此文件不使用系统的memcpy函数, 这样就就可以用STUB_Replace动态替代系统的mempcy
 */
static int32_t StubCopy(void* dest, void* src, uint32_t size)
{
    if ((src == NULL) || (dest == NULL)) {
        return ERANGE;
    }

    uint8_t* localDst = (uint8_t*)(dest);
    uint8_t* localSrc = (uint8_t*)(src);
    for (uint32_t i = 0; i < size; i++) {
        localDst[i] = localSrc[i];
    }
    return 0;
}

/*
 * 此文件不使用系统的memset函数, 这样就就可以用STUB_Replace动态替代系统的memset
 */
static void StubSet(void* dest, int val, uint32_t size)
{
    if (dest == NULL) {
        return;
    }

    uint8_t* localDst = (uint8_t*)(dest);
    for (uint32_t i = 0; i < size; i++) {
        localDst[i] = val;
    }
}

#if defined(__arm__) || defined(__thumb__)
/* 函数指针的最低位标明thumb函数，实际地址需要清除最低位 */
inline static void *RealAddr(void *ptr)
{
    return (void*)(((uintptr_t)ptr) & (~(uintptr_t)1));
}

static int ReplaceT32(void* srcFn, const void* stubFn)
{
    uint16_t instr1 = 0xF000;
    uint16_t instr2;
    uint32_t addrDiff = RealAddr(stubFn) - (RealAddr(srcFn) + 4) - 4; // 跳转指令与srcFn差4字节，PC减去4字节为当前地址
    uint32_t imm;
    if (abs((int32_t)addrDiff) >= 0x100000) { // 最大支持跳转范围
        return -1;
    }
    if (((uintptr_t)stubFn) & 0x01) {
        // Thumb 指令集下的 BL 对应机器码为 [1 1 1 1 0 S imm10][1 1 J1 1 J2 imm11]
        // 地址偏移量计算: I1: NOT(J1 EOR S); I2: NOT(J2 EOR S);
        // imm32: SignExtend(S:I1:I2:imm10:imm11:'0', 32)
        instr2 = 0xF800; // 机器码对应位 J1  J2 取 1
        if (stubFn < srcFn) {
            instr1 = 0xF400; // 机器码对应位 S 取 1
        }
        imm = addrDiff >> 1; // 地址右移1位
        imm &= (1 << 21) - 1; // 取低21位
        instr1 |= (imm >> 11) & 0x3FF; // 右移11位，取imm10
        instr2 |= (imm & 0x7FF);
    } else {
        // Thumb 指令集下的 BLX 对应机器码为 [1 1 1 1 0 S imm10H][1 1 J1 0 J2 imm10L H]
        // 地址偏移量计算: I1 = NOT(J1 EOR S); I2 = NOT(J2 EOR S)
        // imm32 = SignExtend(S:I1:I2:imm10H:imm10L:'00', 32)
        instr2 = 0xE800; // 机器码对应位 J1  J2 取 1
        if (stubFn < srcFn) {
            instr1 = 0xF400; // 机器码对应位 S 取 1
        }
        imm = addrDiff >> 2; // 右移2位
        imm &= (1 << 20) - 1; // 取低20位
        instr1 |= (imm >> 10) & 0x3FF; // 取10位
        instr2 |= (imm & 0x3FF) << 1; // 取低10位
    }
    uint8_t* text        = (uint8_t*)RealAddr(srcFn);
    ((uint16_t*)text)[0] = 0xb580;
    ((uint16_t*)text)[1] = 0xaf00;
    ((uint16_t*)text)[2] = instr1; // BL/BLX 偏移2
    ((uint16_t*)text)[3] = instr2; // 偏移3
    ((uint16_t*)text)[4] = 0xaf00; // 偏移4
    ((uint16_t*)text)[5] = 0xbd80; // 偏移5
    return 0;
}

static int ReplaceA32(void* srcFn, const void* stubFn)
{
    uint32_t inst;
    uint32_t addrDiff = RealAddr(stubFn) - (srcFn + 4) - 8;
    uint32_t imm24;
    if (abs((int32_t)addrDiff) >= 0x1000000) { // 最大支持跳转范围
        return -1;
    }
    if (((uintptr_t)stubFn) & 0x01) {
        // a32 指令集下的 BLX 对应机器码为 [1 1 1 1 1 0 1 H imm24]
        // imm32 = SignExtend(imm24:H:'0', 32)
        uint32_t h = (addrDiff & 0b10) >> 1; // 地址差值的bit[1]
        imm24      = (addrDiff >> 2); // 右移2位
        imm24 &= (1 << 24) - 1; // 取低24位
        inst = 0xfa000000 | imm24 | (h << 24); // h 位于 bit[24]
    } else {
        // a32 指令集下的 BL 对应机器码为 [(!= 1111) 1 0 1 1 imm24]
        // imm32 = SignExtend(imm24:'00', 32)
        imm24 = (addrDiff >> 2); // 右移2位
        imm24 &= (1 << 24) - 1; // 取低24位
        inst = 0xeb000000 | imm24;
    }
    ((uint32_t*)srcFn)[0] = 0xe92d4000;
    ((uint32_t*)srcFn)[1] = inst; // BL/BLX
    ((uint32_t*)srcFn)[2] = 0xe8bd8000; // 偏移2
    return 0;
}
#endif

static void SetProtection(uintptr_t srcPoint, bool nextPage, int prot)
{
    if (mprotect(FuncPageGet(srcPoint), g_pageSize, prot) < 0) {
        perror("STUB_Replace: mprotect failed");
        exit(-1);
    }
    if (nextPage && mprotect(FuncPageGet(srcPoint + CODESIZE), g_pageSize, prot) < 0) {
        perror("STUB_Replace: mprotect failed");
        exit(-1);
    }
}

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_ARM64)
static void WriteJumpInstruction(void* srcFn, const void* stubFn)
{
    unsigned char tmpBuf[CODESIZE];
    int idx = 0;
    tmpBuf[idx++] = 0xFF;
    tmpBuf[idx++] = 0x25;
    memset_s(tmpBuf + idx, FOUR_NUM, 0, FOUR_NUM);
    idx += FOUR_NUM;
    for (int shift = 0; shift < (FOUR_NUM * FOUR_NUM * FOUR_NUM); shift += (TWO_NUM * FOUR_NUM)) {
        tmpBuf[idx++] = (((uintptr_t)stubFn) >> shift) & 0xFF;
    }
    (void)memcpy_s(srcFn, idx, tmpBuf, idx);
}
#endif

/*
 * 将指定函数替换为指定的桩函数
 * stubInfo - 该次桩替换的记录信息, 用于STUB_Reset恢复时使用
 * srcFn - 源码中函数
 * stubFn - 需要替代插入运行的桩函数
 * return - 0:成功，非0:错误码
 */
int STUB_Replace(FuncStubInfo* stubInfo, void* srcFn, const void* stubFn)
{
    #if defined(__arm__) || defined(__thumb__)
        stubInfo->fn = RealAddr(srcFn);
    #else
        stubInfo->fn = srcFn;
    #endif
        StubCopy(stubInfo->codeBuf, (char*)(stubInfo->fn), CODESIZE);
        uintptr_t srcPoint = (uintptr_t)srcFn;
        bool nextPage = (g_pageSize - (srcPoint % g_pageSize)) < CODESIZE;
        SetProtection(srcPoint, nextPage, PROT_READ | PROT_WRITE | PROT_EXEC);
    #if defined(__x86_64__) || defined(__aarch64__) || defined(_M_ARM64)
        WriteJumpInstruction(srcFn, stubFn);
    #elif defined(__arm__) || defined(__thumb__)
        if (((uintptr_t)srcFn) & 0x01) {
            if (ReplaceT32(srcFn, stubFn) != 0) return -1;
        } else {
            if (ReplaceA32(srcFn, stubFn) != 0) return -1;
        }
    #endif
        __builtin___clear_cache((char*)(stubInfo->fn), (char*)(stubInfo->fn) + CODESIZE);
        SetProtection(srcPoint, nextPage, PROT_READ | PROT_EXEC);
        return 0;
}

/*
 * 恢复源函数，去掉其插桩处理
 * stubInfo - 插桩时记录的信息
 * return - 0:成功，非0:错误码
 */
int STUB_Reset(FuncStubInfo* stubInfo)
{
    bool nextPage = false;

    if (stubInfo->fn == NULL) {
        return -1;
    }

    uintptr_t srcPoint = (uintptr_t)stubInfo->fn;
    if ((g_pageSize - (srcPoint % g_pageSize)) < CODESIZE) {
        nextPage = true;
    }

    /* 需要改源函数对应的指令内容, 先添加内存写权限 */
    if (mprotect(FuncPageGet((uintptr_t)stubInfo->fn), g_pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
        perror("STUB_Reset: error mprotect to w+r+x faild");
        return -1;
    }
    if (nextPage) {
        if (mprotect(FuncPageGet(srcPoint + CODESIZE), g_pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
            perror("STUB_Replace: set error mprotect to w+r+x faild");
            return -1;
        }
    }

    /* 恢复记录下来被改写的原函数中的mov/push/mov几条指令 */
    if (StubCopy(stubInfo->fn, stubInfo->codeBuf, CODESIZE) < 0) {
        return -1;
    }
    /* 把缓存的指令刷入 */
    __builtin___clear_cache((char*)stubInfo->fn, (char*)stubInfo->fn + CODESIZE);
    /* 已恢复则关掉内存修改权限 */
    if (mprotect(FuncPageGet((uintptr_t)stubInfo->fn), g_pageSize, PROT_READ | PROT_EXEC) < 0) {
        perror("STUB_Reset: error mprotect to r+x failed");
        return -1;
    }
    if (nextPage) {
        if (mprotect(FuncPageGet(srcPoint + CODESIZE), g_pageSize, PROT_READ | PROT_EXEC) < 0) {
            perror("STUB_Replace: set error mprotect to r+x failed");
            return -1;
        }
    }

    StubSet(stubInfo, 0, sizeof(FuncStubInfo));
    return 0;
}

/*
 * 动态换桩功能初始化, 获取内存页大小， 调用一次即可
 * return - 0:成功，非0:错误码
 */
int STUB_Init(void)
{
    if (g_pageSize != -1) {
        return 0;
    }
    g_pageSize = sysconf(_SC_PAGE_SIZE);
    if (g_pageSize < 0) {
        perror("STUB_Init: get system _SC_PAGE_SIZE configure failed");
        return -1;
    }
    return 0;
}
