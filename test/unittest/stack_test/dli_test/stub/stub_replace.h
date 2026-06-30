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
 * Description: 用于实现C函数的动态换桩功能
 *
 * 用法说明：
 * 1、在LLT用例中需动态更换桩函数时, 需要包含本头文件, 及链接本文件中对应函数的实现文件
 * 2、程序初始化时调用一次STUB_Init(), 或者在调用换桩函数STUB_Replace之前调用一次，进程生命期内调用过一次即可
 * 3、比如源码中有个函数为：void FuncXXX(int a, int b), 测试工程写了一个同样原型的桩函数void stub_FuncXXX(int a, int b)
 * 4、在某个用例中想将调用FuncXXX中的地方全部替换为调用stub_FuncXXX，那么在该用例入口处构造一个stubInfo对象mm，
 * 并调用STUB_Replace(&mm, FuncXXX, stub_FuncXXX)即可实现
 * 5、该用例执行完成后，想恢复原来的函数调用，则再调用一次STUB_Reset(&mm)即可
 * 6、Linux与MinGW的实现方式不一样，linux系统直接包含头文件、链接库即可；MinGW LLT需增加一个编译宏：_WINDOWS_MINGW_
 *
 */

#ifndef STUB_REPALCE_H
#define STUB_REPALCE_H

#include <stdint.h>

#if defined(__x86_64__)
/* 借用函数入口的前14个字节构造跳转指令, 短跳转5个字节即可, 长跳转需14个字节 */
#define CODESIZE 14U
#elif defined(__aarch64__) || defined(_M_ARM64)
/* arm64 构造跳转指令需要16字节 */
#define CODESIZE 16U
#elif defined(__arm__)
/* arm32 构造跳转指令需要12字节 */
#define CODESIZE 12U
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void*         fn;
    unsigned char codeBuf[CODESIZE];
} FuncStubInfo;

/*
 * 动态换桩功能初始化, 调用一次即可
 * return - 0:成功，非0:错误码
 */
int STUB_Init(void);

/*
 * 将指定函数替换为指定的桩函数
 * stubInfo - 该次桩替换的记录信息, 用于STUB_Reset恢复时使用
 * srcFn - 源码中函数
 * stubFn - 需要替代插入运行的桩函数
 * return - 0:成功，非0:错误码
 */
int STUB_Replace(FuncStubInfo* stubInfo, void* srcFn, const void* stubFn);

/*
 * 恢复源函数，去掉其插桩处理
 * stubInfo - 插桩时记录的信息
 * return - 0:成功，非0:错误码
 */
int STUB_Reset(FuncStubInfo* stubInfo);

#ifdef __cplusplus
}
#endif

#endif
