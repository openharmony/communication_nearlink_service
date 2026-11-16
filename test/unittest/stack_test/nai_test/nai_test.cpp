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

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>

#include <gtest/gtest.h>

#include "nai_dft.h"
#include "nlstk_dft_api.h"
#include "nlstk_init_api.h"
#include "securec.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sle_crypto.h"

extern SDF_MemHooks_S g_stMemfunc;

extern "C" {
extern void *g_pluginSoHandle;
void NAI_DftCache(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId,
    NLSTK_DftParamValueType_E paramType, void *param);
void NAI_DftReport(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t res);
}

namespace {
void *const FOREIGN_PLUGIN_HANDLE = reinterpret_cast<void *>(0x1234);

bool HasNonZeroByte(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (data[i] != 0) {
            return true;
        }
    }
    return false;
}

void FillBytes(uint8_t *data, size_t len, uint8_t value)
{
    for (size_t i = 0; i < len; i++) {
        data[i] = value;
    }
}
} // namespace

TEST(NaiStackInitTest, DeinitAndDisableBeforeInitAreNoops)
{
    // 未初始化时反初始化、未使能时去使能均为空操作
    NLSTK_DeinitStack();
    NLSTK_DisableStack();
    EXPECT_FALSE(NLSTK_IsStackInited());
    EXPECT_FALSE(NLSTK_IsStackEnabled());
}

namespace {
// 反初始化会 dlclose 初始化链加载的固件栈库，卸载级联会让进程退出路径崩溃；
// 测试进程对这两个库各持一份永不释放的引用，只阻止最终卸载
#ifdef __aarch64__
constexpr const char *STACK_LIBS_PINNED[] = {
    "/system/lib64/libnearlink_stack_ext.z.so",
    "/system/lib64/libnearlink_stack_base.z.so",
};
#else
constexpr const char *STACK_LIBS_PINNED[] = {
    "/system/lib/libnearlink_stack_ext.z.so",
    "/system/lib/libnearlink_stack_base.z.so",
};
#endif

void PinStackLibrariesForProcessLifetime()
{
    for (const char *lib : STACK_LIBS_PINNED) {
        void *handle = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
        if (handle == nullptr) {
            fprintf(stderr, "nai_test: cannot pin %s: %s\n", lib, dlerror());
        }
    }
}
} // namespace

class NaiStackLifecycleTest : public testing::Test {
protected:
    void TearDown() override
    {
        if (NLSTK_IsStackInited()) {
            g_pluginSoHandle = nullptr;
            NLSTK_DeinitStack();
        }
        g_pluginSoHandle = nullptr;
    }
    static void SetUpTestCase()
    {
        PinStackLibrariesForProcessLifetime();
    }
    static void TearDownTestCase() {}
};

TEST_F(NaiStackLifecycleTest, InitSkipsPluginLoadWhenHandleHeld)
{
    // 已持有插件句柄时初始化跳过插件加载，状态机正常置位；清空句柄后反初始化安全
    g_pluginSoHandle = FOREIGN_PLUGIN_HANDLE;
    EXPECT_EQ(NLSTK_InitStack(), NLSTK_ERRCODE_SUCCESS);
    EXPECT_TRUE(NLSTK_IsStackInited());
    g_pluginSoHandle = nullptr; // 跳过插件级联卸载
    NLSTK_DeinitStack();
    EXPECT_FALSE(NLSTK_IsStackInited());
}

TEST_F(NaiStackLifecycleTest, RealPluginLifecycleRoundTrip)
{
    // 无外部句柄时初始化走真实插件加载并完成全链初始化，反初始化闭环
    g_pluginSoHandle = nullptr;
    EXPECT_EQ(NLSTK_InitStack(), NLSTK_ERRCODE_SUCCESS);
    EXPECT_TRUE(NLSTK_IsStackInited());
    NLSTK_DeinitStack();
    EXPECT_FALSE(NLSTK_IsStackInited());
}

TEST(NaiCryptoTest, AlgoGuardsRejectUnsupported)
{
    // 非法算法标识与非法长度被入口直接拒绝
    NLSTK_SmKeyPair_S keyPair = {};
    keyPair.algo = KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1;
    EXPECT_FALSE(Crypto_PubPriKeyPairGenerate(&keyPair));
    keyPair.algo = static_cast<uint8_t>(KEY_NEGOTIATION_ALGORITHM_ABILITY_RESERVED + 1);
    EXPECT_FALSE(Crypto_PubPriKeyPairGenerate(&keyPair));

    keyPair.algo = KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    uint8_t secKey[32] = {0};
    EXPECT_FALSE(Crypto_SecKeyGenerate(&keyPair, secKey, sizeof(secKey) - 1));
    keyPair.algo = KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1;
    EXPECT_FALSE(Crypto_SecKeyGenerate(&keyPair, secKey, sizeof(secKey)));

    uint8_t mac[16] = {0};
    NLSTK_SmDerivedMac_S input = {};
    input.algo = KEY_DERIVATION_ALGORITHM_ABILITY_HA1;
    input.key[0] = 1;
    input.buff = mac;
    input.buffSize = sizeof(mac);
    EXPECT_FALSE(Crypto_DerivedKeyGenerate(&input, mac, sizeof(mac)));
    input.algo = static_cast<uint8_t>(KEY_DERIVATION_ALGORITHM_ABILITY_RESERVED + 1);
    EXPECT_FALSE(Crypto_DerivedKeyGenerate(&input, mac, sizeof(mac)));
    input.algo = KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    EXPECT_FALSE(Crypto_DerivedKeyGenerate(&input, mac, sizeof(mac) - 1));
}

TEST(NaiCryptoTest, EcdhKeyPairAndSecKeySucceed)
{
    // ECDH 密钥对生成成功且密钥材料非全零，自协商派生共享密钥成功
    NLSTK_SmKeyPair_S keyPair = {};
    keyPair.algo = KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    ASSERT_TRUE(Crypto_PubPriKeyPairGenerate(&keyPair));
    EXPECT_TRUE(HasNonZeroByte(keyPair.priKey, sizeof(keyPair.priKey)));
    EXPECT_TRUE(HasNonZeroByte(keyPair.localPubKey, sizeof(keyPair.localPubKey)));

    ASSERT_EQ(memcpy_s(keyPair.remotePubKey, sizeof(keyPair.remotePubKey), keyPair.localPubKey,
        sizeof(keyPair.localPubKey)), EOK);
    uint8_t secKey[32] = {0};
    EXPECT_TRUE(Crypto_SecKeyGenerate(&keyPair, secKey, sizeof(secKey)));
    EXPECT_TRUE(HasNonZeroByte(secKey, sizeof(secKey)));
}

TEST(NaiCryptoTest, SecKeyRejectsInvalidKeyMaterial)
{
    // 非法私钥或不在曲线上的对端公钥派生失败
    NLSTK_SmKeyPair_S keyPair = {};
    keyPair.algo = KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    ASSERT_TRUE(Crypto_PubPriKeyPairGenerate(&keyPair));
    ASSERT_EQ(memcpy_s(keyPair.remotePubKey, sizeof(keyPair.remotePubKey), keyPair.localPubKey,
        sizeof(keyPair.localPubKey)), EOK);
    uint8_t secKey[32] = {0};

    FillBytes(keyPair.priKey, sizeof(keyPair.priKey), 0xFF); // 超出曲线阶
    EXPECT_FALSE(Crypto_SecKeyGenerate(&keyPair, secKey, sizeof(secKey)));

    ASSERT_TRUE(Crypto_PubPriKeyPairGenerate(&keyPair));
    ASSERT_EQ(memcpy_s(keyPair.remotePubKey, sizeof(keyPair.remotePubKey), keyPair.localPubKey,
        sizeof(keyPair.localPubKey)), EOK);
    FillBytes(keyPair.remotePubKey, sizeof(keyPair.remotePubKey), 0xFF); // 非曲线点
    EXPECT_FALSE(Crypto_SecKeyGenerate(&keyPair, secKey, sizeof(secKey)));
}

TEST(NaiCryptoTest, DerivedKeyCmacIsDeterministic)
{
    // 同输入的派生密钥结果确定且非全零
    uint8_t input[16] = {0};
    for (size_t i = 0; i < sizeof(input); i++) {
        input[i] = static_cast<uint8_t>(i + 1);
    }
    NLSTK_SmDerivedMac_S param = {};
    param.algo = KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    FillBytes(param.key, sizeof(param.key), 0x5A);
    param.buff = input;
    param.buffSize = sizeof(input);

    uint8_t first[16] = {0};
    uint8_t second[16] = {0};
    ASSERT_TRUE(Crypto_DerivedKeyGenerate(&param, first, sizeof(first)));
    ASSERT_TRUE(Crypto_DerivedKeyGenerate(&param, second, sizeof(second)));
    EXPECT_EQ(memcmp(first, second, sizeof(first)), 0);
    EXPECT_TRUE(HasNonZeroByte(first, sizeof(first)));
}

TEST(NaiCryptoTest, Sha256MatchesKnownVector)
{
    // SHA256 结果与公开测试向量一致，非法输入被拒绝
    const char *abc = "abc";
    NLSTK_VariableData_S value = {};
    value.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(abc));
    value.len = 3;
    NLSTK_SmSha256Hash hash = {};
    ASSERT_TRUE(Crypto_Sha256(&value, &hash));
    const uint8_t expected[32] = {0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE,
        0x5D, 0xAE, 0x22, 0x22, 0x3B, 0x00, 0x36, 0x1A, 0x39, 0x61, 0x77, 0xA9, 0xCB, 0x41, 0x0F, 0xF6,
        0x1F, 0x20, 0x01, 0x5A};
    EXPECT_EQ(memcmp(hash.sha256Hash, expected, sizeof(expected)), 0);

    EXPECT_FALSE(Crypto_Sha256(nullptr, &hash));
    NLSTK_VariableData_S bad = {};
    bad.data = hash.sha256Hash;
    bad.len = 0;
    EXPECT_FALSE(Crypto_Sha256(&bad, &hash));
    bad.len = 1;
    EXPECT_FALSE(Crypto_Sha256(&bad, nullptr));
    bad.data = nullptr;
    EXPECT_FALSE(Crypto_Sha256(&bad, &hash));
}

TEST(NaiCryptoTest, RandNumGenerateFillsBuffer)
{
    // 随机数生成成功且两次结果不同
    uint8_t first[32] = {0};
    uint8_t second[32] = {0};
    ASSERT_TRUE(Crypto_RandNumGenerate(first, sizeof(first)));
    ASSERT_TRUE(Crypto_RandNumGenerate(second, sizeof(second)));
    EXPECT_NE(memcmp(first, second, sizeof(first)), 0);
}

TEST(NaiDftTest, ParamCreateAndFreeAllTypes)
{
    // 字符串与子事件引用参数创建后按类型正确释放
    DftParamC inner[2];
    inner[0] = CreateStrParamC(NAI_DEVICE_ADDR, DFT_ADDR_SIZE, "00:11:22:33:44:55(0)");
    inner[1] = CreateUi16ParamC(2, 7);
    DftSubEventRefC *ref = CreateSubEventRef(DFT_PEER_INFO, inner, 2);
    ASSERT_NE(ref, nullptr);
    DftParamC outer[1];
    outer[0] = CreateRefParamC(SM_ENCP_PEER_INFO, ref);
    DftFreeParamsWithSubParam(outer, 1); // 连同子参数一起释放
}

TEST(NaiDftTest, CacheRoutesByParamType)
{
    // 缓存入口按参数类型分流：整型、字符串各走其路，未知类型与空入参被忽略
    SLE_Addr_S addr = {};
    uint16_t value = 3;
    NAI_DftCache(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &value);

    NLSTK_DftParamValueString_S str = {};
    ASSERT_EQ(strncpy_s(str.str, sizeof(str.str), "ut", strlen("ut")), EOK);
    NAI_DftCache(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, NLSTK_DFT_PARAM_VALUE_TYPE_STRING, &str);

    NAI_DftCache(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1,
        static_cast<NLSTK_DftParamValueType_E>(0x99), nullptr);
    NAI_DftCache(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, nullptr);
    NAI_DftCache(nullptr, DFT_STACK_SM_G_NEGO_EXCEP, 1, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &value);
}

TEST(NaiDftTest, ReportRoutesByEventId)
{
    // 上报入口按事件标识分流：安全管理、硬件管理与未知事件各走其路
    SLE_Addr_S addr = {};
    NAI_DftReport(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, 0);
    NAI_DftReport(&addr, DFT_STACK_HADM_EXCEP, 1, 0);
    NAI_DftReport(&addr, 0x9999, 1, 0);
    NAI_DftReport(nullptr, DFT_STACK_HADM_EXCEP, 1, 0);
}

namespace {
int g_liveAllocs = 0;
int g_allocSeq = 0;
int g_allocSkip = -1;
pthread_t g_mainThread;

void *CountOrFailAlloc(size_t size)
{
    if (g_allocSkip >= 0 && pthread_equal(pthread_self(), g_mainThread) && ++g_allocSeq > g_allocSkip) {
        return nullptr;
    }
    void *ptr = malloc(size);
    if (ptr != nullptr) {
        g_liveAllocs++;
    }
    return ptr;
}

void CountFree(void *ptr)
{
    if (ptr != nullptr) {
        g_liveAllocs--;
    }
    free(ptr);
}
} // namespace

TEST(NaiDftTest, ReportSmRecoversFromAllocFailure)
{
    // 安全管理上报中途分配失败时走回收分支，内存占用回到基线
    g_mainThread = pthread_self();
    SDF_MemHooks_S saved = g_stMemfunc;
    SDF_MemHooks_S hook = {.mAlloc = CountOrFailAlloc, .mFree = CountFree};
    ASSERT_EQ(SDF_MemHookReg(&hook), SDF_OK);
    SLE_Addr_S addr = {};

    int baseline = g_liveAllocs;
    g_allocSeq = 0;
    g_allocSkip = 2; // 对端参数数组分配失败
    NAI_DftReport(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, 0);
    g_allocSkip = -1;
    EXPECT_EQ(g_liveAllocs, baseline);

    g_allocSeq = 0;
    g_allocSkip = 4; // 子事件引用分配失败
    NAI_DftReport(&addr, DFT_STACK_SM_G_NEGO_EXCEP, 1, 0);
    g_allocSkip = -1;
    EXPECT_EQ(g_liveAllocs, baseline);

    g_stMemfunc = saved;
}
