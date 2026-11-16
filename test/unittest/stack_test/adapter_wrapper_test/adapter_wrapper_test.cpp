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

#include <cstdint>
#include <gtest/gtest.h>
#include "bnl_ext_func_wrapper.h"
#include "bnl_reg_ext_func.h"
#include "collab_ext_func_wrapper.h"
#include "collab_reg_ext_func.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"
#include "devd_ext_func_wrapper.h"
#include "devd_reg_ext_func.h"
#include "dli_errno.h"
#include "dli_ext_func_wrapper.h"
#include "dli_reg_ext_func.h"
#include "hadm_ext_func_wrapper.h"
#include "hadm_reg_ext_func.h"
#include "nlstk_public_define_ext.h"
#include "parameter_wrapper.h"
#include "qosm_ext_func_wrapper.h"
#include "qosm_reg_ext_func.h"

namespace {
constexpr uint32_t RET_U32 = 0x5A5A1234;
constexpr int RET_INT = 42;
constexpr uint16_t RET_U16 = 0xBEEF;

// dli 桩状态：函数表整体可用性、扩展返回值、调用计数与最近一次入参
DLI_ExtFuncList g_dliList;
bool g_dliListNull = false;
uint32_t g_dliRetU32 = RET_U32;
bool g_dliRetBool = true;
int g_dliCalls = 0;
struct DliLast {
    DLI_FreqBandExtParam *freqBandParam;
    DLI_ACBEnableSubrateParam *acbEnableParam;
    DLI_LocalPrivateFeatures *localFeatures;
    DLI_ACBSubrateParam *acbSubrateParam;
    DLI_SetConnFramePowerLevelParam *connFramePowerParam;
    DLI_ICGAutorateParam *icgAutorateParam;
    bool icgAutorateMcast;
    DLI_ICGCbkParam *icgAutorateCbk;
    DLI_ICGLabelParam *icgLabelParam;
    bool icgLabelMcast;
    DLI_ICGCbkParam *icgLabelCbk;
    DLI_ICBConnectionParam *icbParam;
    bool icbMcast;
    DLI_ICGCbkParam *icbCbk;
    DLI_ICGUpdatedParam *icgUpdateParam;
    bool icgUpdateMcast;
    DLI_ICGCbkParam *icgUpdateCbk;
    DLI_ImgAcbInfoParam *imgAcbParam;
    DLI_ICGCbkParam *imgAcbCbk;
} g_dliLast = {};

// collab 桩状态
COLLAB_ExtFuncList g_collabList;
bool g_collabListNull = false;
uint32_t g_collabRetU32 = RET_U32;
int g_collabCalls = 0;
struct CollabLast {
    const COLLAB_StmDevdScanCbk_S *stmCbk;
    const COLLAB_DevdCollabStmFunc_S *stmFunc;
    const COLLAB_StmDevdScanInitAfterReg_Cbk_S *stmAfterCbk;
    const COLLAB_CollabCmCbk_S *cmCbk;
    const COLLAB_CmCollabFunc_S *cmFunc;
    const COLLAB_TransFuncExt *transFunc;
    uint8_t apLinkNum;
    uint8_t apOccupiedBufferNum;
    COLLAB_CollabAdvCbk_S *advCbk;
} g_collabLast = {};

// bnl 桩状态
BNL_ExtFuncList g_bnlList;
bool g_bnlListNull = false;
int g_bnlCalls = 0;
struct BnlLast {
    const NLSTK_BnlLogicLinkState_S *state;
    const NLSTK_BnlSendMsg_S *msg;
    const NLSTK_BnlProxyFunc_S *func;
} g_bnlLast = {};

// common 桩状态
COMMON_ExtFuncList g_commonList;
bool g_commonListNull = false;
bool g_commonRetBool = true;
int g_commonCalls = 0;
struct CommonLast {
    const SLE_Addr_S *addr;
    uint16_t *customInterval;
} g_commonLast = {};

// devd 桩状态
Devd_ExtFuncList g_devdList;
bool g_devdListNull = false;
uint32_t g_devdRetU32 = RET_U32;
bool g_devdRetBool = true;
NLSTK_Errcode_E g_devdRetErr = NLSTK_ERRCODE_PARAM_ERR;
int g_devdCalls = 0;
struct DevdLast {
    const NLSTK_DevdCollabScanFilterFunc_S *filterFunc;
    NLSTK_DevdScanFilterSwitchAction_E action;
    NLSTK_DevdScanFilter_S *filter;
    SDF_Vector_S *scanners;
    NLSTK_DevdScanner_S *scanner;
    NLSTK_DevdScanSetting_S *setting;
    SDF_Vector_S *filters;
    NLSTK_DevdScanManager_S *manager;
} g_devdLast = {};

// hadm 桩状态
HADM_ExtFuncList g_hadmList;
bool g_hadmListNull = false;
bool g_hadmRetBool = true;
int g_hadmCalls = 0;
struct HadmLast {
    uint16_t companyid;
    uint16_t subversion;
    uint16_t readLcid;
    uint16_t multiToneLcid;
    uint8_t *pmInitSignal2Tone;
    uint8_t *pmReflSignal2Tone;
    uint16_t *occurrenceGroupPeriod;
    uint8_t toneControl;
    uint16_t clearLcid;
} g_hadmLast = {};

// qosm 桩状态
QOSM_ExtFuncList g_qosmList;
bool g_qosmListNull = false;
int g_qosmRetInt = RET_INT;
bool g_qosmRetBool = true;
int g_qosmCalls = 0;

// 系统参数桩：音频能力开关与读取计数
int32_t g_propInt32Ret = 0;
int g_propCalls = 0;

// dli 扩展桩
bool MockDliIsSupportAccessFilterListExt(void)
{
    g_dliCalls++;
    return g_dliRetBool;
}

uint32_t MockDliEnableFreqBandExt(DLI_FreqBandExtParam *param)
{
    g_dliCalls++;
    g_dliLast.freqBandParam = param;
    return g_dliRetU32;
}

uint32_t MockDliAcbEnableSubrate(DLI_ACBEnableSubrateParam *param)
{
    g_dliCalls++;
    g_dliLast.acbEnableParam = param;
    return g_dliRetU32;
}

uint32_t MockDliSetLocalPrivateFeatures(DLI_LocalPrivateFeatures *features)
{
    g_dliCalls++;
    g_dliLast.localFeatures = features;
    return g_dliRetU32;
}

uint32_t MockDliAcbSetSubrate(DLI_ACBSubrateParam *param)
{
    g_dliCalls++;
    g_dliLast.acbSubrateParam = param;
    return g_dliRetU32;
}

uint32_t MockDliSetConnFramePowerLevel(DLI_SetConnFramePowerLevelParam *param)
{
    g_dliCalls++;
    g_dliLast.connFramePowerParam = param;
    return g_dliRetU32;
}

uint32_t MockDliGetLocalPrivateFeatures(void)
{
    g_dliCalls++;
    return g_dliRetU32;
}

uint32_t MockDliSetICGAutorateParamExt(DLI_ICGAutorateParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    g_dliCalls++;
    g_dliLast.icgAutorateParam = param;
    g_dliLast.icgAutorateMcast = mcast;
    g_dliLast.icgAutorateCbk = cbkParam;
    return g_dliRetU32;
}

uint32_t MockDliSetICGLabel(DLI_ICGLabelParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    g_dliCalls++;
    g_dliLast.icgLabelParam = param;
    g_dliLast.icgLabelMcast = mcast;
    g_dliLast.icgLabelCbk = cbkParam;
    return g_dliRetU32;
}

uint32_t MockDliCreateICBExt(DLI_ICBConnectionParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    g_dliCalls++;
    g_dliLast.icbParam = param;
    g_dliLast.icbMcast = mcast;
    g_dliLast.icbCbk = cbkParam;
    return g_dliRetU32;
}

uint32_t MockDliUpdateICGParam(DLI_ICGUpdatedParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    g_dliCalls++;
    g_dliLast.icgUpdateParam = param;
    g_dliLast.icgUpdateMcast = mcast;
    g_dliLast.icgUpdateCbk = cbkParam;
    return g_dliRetU32;
}

uint32_t MockDliSetImgAcbInfo(DLI_ImgAcbInfoParam *param, DLI_ICGCbkParam *cbkParam)
{
    g_dliCalls++;
    g_dliLast.imgAcbParam = param;
    g_dliLast.imgAcbCbk = cbkParam;
    return g_dliRetU32;
}

// collab 扩展桩
uint32_t MockCollabStmDevdScanInit(const COLLAB_StmDevdScanCbk_S *cbk, const COLLAB_DevdCollabStmFunc_S *func)
{
    g_collabCalls++;
    g_collabLast.stmCbk = cbk;
    g_collabLast.stmFunc = func;
    return g_collabRetU32;
}

uint32_t MockCollabStmDevdScanInitAfterReg(const COLLAB_StmDevdScanInitAfterReg_Cbk_S *initAfterCbk)
{
    g_collabCalls++;
    g_collabLast.stmAfterCbk = initAfterCbk;
    return g_collabRetU32;
}

void MockCollabStmDevdScanDeInit(void)
{
    g_collabCalls++;
}

uint32_t MockCollabCmInit(const COLLAB_CollabCmCbk_S *cbk, const COLLAB_CmCollabFunc_S *func)
{
    g_collabCalls++;
    g_collabLast.cmCbk = cbk;
    g_collabLast.cmFunc = func;
    return g_collabRetU32;
}

void MockCollabCmDeInit(void)
{
    g_collabCalls++;
}

uint32_t MockCollabTransFuncRegister(const COLLAB_TransFuncExt *func)
{
    g_collabCalls++;
    g_collabLast.transFunc = func;
    return g_collabRetU32;
}

uint32_t MockCollabPreAssignTransBuffer(uint8_t apLinkNum)
{
    g_collabCalls++;
    g_collabLast.apLinkNum = apLinkNum;
    return g_collabRetU32;
}

uint32_t MockCollabContinueAssignTransBuffer(uint8_t apOccupiedBufferNum)
{
    g_collabCalls++;
    g_collabLast.apOccupiedBufferNum = apOccupiedBufferNum;
    return g_collabRetU32;
}

uint32_t MockCollabAdvInit(COLLAB_CollabAdvCbk_S *cbk)
{
    g_collabCalls++;
    g_collabLast.advCbk = cbk;
    return g_collabRetU32;
}

// bnl 扩展桩
void MockBnlLinkStateChange(const NLSTK_BnlLogicLinkState_S *state)
{
    g_bnlCalls++;
    g_bnlLast.state = state;
}

void MockBnlRecvMsg(const NLSTK_BnlSendMsg_S *msg)
{
    g_bnlCalls++;
    g_bnlLast.msg = msg;
}

void MockBnlInit(const NLSTK_BnlProxyFunc_S *func)
{
    g_bnlCalls++;
    g_bnlLast.func = func;
}

void MockBnlDeInit(void)
{
    g_bnlCalls++;
}

// common 扩展桩
bool MockCommonIsSupportConnHighSpeed(void)
{
    g_commonCalls++;
    return g_commonRetBool;
}

bool MockCommonIsSupportQueryQosInfo(void)
{
    g_commonCalls++;
    return g_commonRetBool;
}

bool MockCommonIsSupportSetMaxInterval(void)
{
    g_commonCalls++;
    return g_commonRetBool;
}

bool MockCommonIsSupportConnFramePowerLevel(void)
{
    g_commonCalls++;
    return g_commonRetBool;
}

bool MockCommonIsSupportScanFilter(void)
{
    g_commonCalls++;
    return g_commonRetBool;
}

bool MockCommonIsSupportCustomInterval(const SLE_Addr_S *addr, uint16_t *customInterval)
{
    g_commonCalls++;
    g_commonLast.addr = addr;
    g_commonLast.customInterval = customInterval;
    if (customInterval != nullptr) {
        *customInterval = RET_U16;
    }
    return g_commonRetBool;
}

// devd 扩展桩
uint32_t MockDevdScanFilterInit(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc)
{
    g_devdCalls++;
    g_devdLast.filterFunc = filterFunc;
    return g_devdRetU32;
}

void MockDevdScanFilterDeInit(void)
{
    g_devdCalls++;
}

NLSTK_Errcode_E MockDevdSetScanfilterSwitch(NLSTK_DevdScanFilterSwitchAction_E action)
{
    g_devdCalls++;
    g_devdLast.action = action;
    return g_devdRetErr;
}

bool MockDevdIsSupportScanFilter(void)
{
    g_devdCalls++;
    return g_devdRetBool;
}

void MockDevdSetScanfilter(NLSTK_DevdScanFilter_S *filter)
{
    g_devdCalls++;
    g_devdLast.filter = filter;
}

void MockDevdAddChipFilters(SDF_Vector_S *scanners, NLSTK_DevdScanner_S *scanner, NLSTK_DevdScanSetting_S *setting,
    SDF_Vector_S *filters)
{
    g_devdCalls++;
    g_devdLast.scanners = scanners;
    g_devdLast.scanner = scanner;
    g_devdLast.setting = setting;
    g_devdLast.filters = filters;
}

void MockDevdDeleteChipFilters(NLSTK_DevdScanManager_S *manager, NLSTK_DevdScanner_S *scanner)
{
    g_devdCalls++;
    g_devdLast.manager = manager;
    g_devdLast.scanner = scanner;
}

// hadm 扩展桩
bool MockHadmReadRemoteExtFeatures(uint16_t companyid, uint16_t subversion, uint16_t lcid)
{
    g_hadmCalls++;
    g_hadmLast.companyid = companyid;
    g_hadmLast.subversion = subversion;
    g_hadmLast.readLcid = lcid;
    return g_hadmRetBool;
}

void MockHadmCheckAndUpdateMultiToneConfig(uint16_t lcid, uint8_t *pmInitSignal2Tone, uint8_t *pmReflSignal2Tone,
    uint16_t *occurrenceGroupPeriod, uint8_t toneControl)
{
    g_hadmCalls++;
    g_hadmLast.multiToneLcid = lcid;
    g_hadmLast.pmInitSignal2Tone = pmInitSignal2Tone;
    g_hadmLast.pmReflSignal2Tone = pmReflSignal2Tone;
    g_hadmLast.occurrenceGroupPeriod = occurrenceGroupPeriod;
    g_hadmLast.toneControl = toneControl;
}

void MockHadmClearRemoteCsCaps(uint16_t lcid)
{
    g_hadmCalls++;
    g_hadmLast.clearLcid = lcid;
}

// qosm 扩展桩
void MockQosmAntennaDfxSendQueryCmd(void)
{
    g_qosmCalls++;
}

int MockQosmAntennaDfxGetAntennaPolicy(void)
{
    g_qosmCalls++;
    return g_qosmRetInt;
}

bool MockQosmIsLastEnableFreqBandByRecommend(void)
{
    g_qosmCalls++;
    return g_qosmRetBool;
}

bool MockQosmIsNeedRecoverFreqBandAbility(void)
{
    g_qosmCalls++;
    return g_qosmRetBool;
}
} // namespace

extern "C" {
// 被测包装层的依赖入口：各模块函数表获取器，返回内容可按用例控制
DLI_ExtFuncList *DLI_GetExtFuncList(void)
{
    return g_dliListNull ? nullptr : &g_dliList;
}

COLLAB_ExtFuncList *COLLAB_GetExtFuncList(void)
{
    return g_collabListNull ? nullptr : &g_collabList;
}

BNL_ExtFuncList *BNL_GetExtFuncList(void)
{
    return g_bnlListNull ? nullptr : &g_bnlList;
}

COMMON_ExtFuncList *COMMON_GetExtFuncList(void)
{
    return g_commonListNull ? nullptr : &g_commonList;
}

Devd_ExtFuncList *Devd_GetExtFuncList(void)
{
    return g_devdListNull ? nullptr : &g_devdList;
}

HADM_ExtFuncList *HADM_GetExtFuncList(void)
{
    return g_hadmListNull ? nullptr : &g_hadmList;
}

QOSM_ExtFuncList *QOSM_GetExtFuncList(void)
{
    return g_qosmListNull ? nullptr : &g_qosmList;
}

// 被测包装层的依赖入口：系统参数读取，避免测试触碰真实系统参数
int32_t PropertyGetInt32(const char *key, int32_t defaultValue)
{
    (void)key;
    (void)defaultValue;
    g_propCalls++;
    return g_propInt32Ret;
}
}

class UT_ADAPTER_WRAPPER_TEST : public testing::Test {
protected:
    void SetUp() override
    {
        g_dliList = {};
        g_dliListNull = false;
        g_dliRetU32 = RET_U32;
        g_dliRetBool = true;
        g_dliCalls = 0;
        g_dliLast = {};

        g_collabList = {};
        g_collabListNull = false;
        g_collabRetU32 = RET_U32;
        g_collabCalls = 0;
        g_collabLast = {};

        g_bnlList = {};
        g_bnlListNull = false;
        g_bnlCalls = 0;
        g_bnlLast = {};

        g_commonList = {};
        g_commonListNull = false;
        g_commonRetBool = true;
        g_commonCalls = 0;
        g_commonLast = {};

        g_devdList = {};
        g_devdListNull = false;
        g_devdRetU32 = RET_U32;
        g_devdRetBool = true;
        g_devdRetErr = NLSTK_ERRCODE_PARAM_ERR;
        g_devdCalls = 0;
        g_devdLast = {};

        g_hadmList = {};
        g_hadmListNull = false;
        g_hadmRetBool = true;
        g_hadmCalls = 0;
        g_hadmLast = {};

        g_qosmList = {};
        g_qosmListNull = false;
        g_qosmRetInt = RET_INT;
        g_qosmRetBool = true;
        g_qosmCalls = 0;

        g_propInt32Ret = 0;
        g_propCalls = 0;
    }

    void TearDown() override {}
};

// ---------- dli 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_IsSupportAccessFilterListExt_Passthrough)
{
    // 已注册扩展时返回扩展的能力查询结果
    g_dliList.isSupportAccessFilterListExt = MockDliIsSupportAccessFilterListExt;
    g_dliRetBool = true;
    EXPECT_TRUE(DLI_IsSupportAccessFilterListExt());
    EXPECT_EQ(g_dliCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_IsSupportAccessFilterListExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_dliListNull = true;
    EXPECT_FALSE(DLI_IsSupportAccessFilterListExt());
    g_dliListNull = false;
    EXPECT_FALSE(DLI_IsSupportAccessFilterListExt());
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_EnableFreqBandExt_Passthrough)
{
    // 已注册扩展时参数指针与返回值原样透传
    DLI_FreqBandExtParam param = {};
    g_dliList.enableFreqBandExt = MockDliEnableFreqBandExt;
    EXPECT_EQ(DLI_EnableFreqBandExt(&param), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.freqBandParam, &param);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_EnableFreqBandExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_FreqBandExtParam param = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_EnableFreqBandExt(&param), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_EnableFreqBandExt(&param), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ACBEnableSubrate_Passthrough)
{
    // 已注册扩展时参数指针与返回值原样透传
    DLI_ACBEnableSubrateParam param = {};
    g_dliList.acbEnableSubrate = MockDliAcbEnableSubrate;
    EXPECT_EQ(DLI_ACBEnableSubrate(&param), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.acbEnableParam, &param);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ACBEnableSubrate_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ACBEnableSubrateParam param = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_ACBEnableSubrate(&param), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_ACBEnableSubrate(&param), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetLocalPrivateFeatures_Passthrough)
{
    // 已注册扩展时参数指针与返回值原样透传
    DLI_LocalPrivateFeatures features = {};
    g_dliList.setLocalPrivateFeatures = MockDliSetLocalPrivateFeatures;
    EXPECT_EQ(DLI_SetLocalPrivateFeatures(&features), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.localFeatures, &features);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetLocalPrivateFeatures_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_LocalPrivateFeatures features = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_SetLocalPrivateFeatures(&features), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_SetLocalPrivateFeatures(&features), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ACBSetSubrate_Passthrough)
{
    // 已注册扩展时参数指针与返回值原样透传
    DLI_ACBSubrateParam param = {};
    g_dliList.acbSetSubrate = MockDliAcbSetSubrate;
    EXPECT_EQ(DLI_ACBSetSubrate(&param), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.acbSubrateParam, &param);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ACBSetSubrate_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ACBSubrateParam param = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_ACBSetSubrate(&param), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_ACBSetSubrate(&param), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetConnFramePowerLevel_Passthrough)
{
    // 已注册扩展时参数指针与返回值原样透传
    DLI_SetConnFramePowerLevelParam param = {};
    g_dliList.setConnFramePowerLevel = MockDliSetConnFramePowerLevel;
    EXPECT_EQ(DLI_SetConnFramePowerLevel(&param), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.connFramePowerParam, &param);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetConnFramePowerLevel_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_SetConnFramePowerLevelParam param = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_SetConnFramePowerLevel(&param), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_SetConnFramePowerLevel(&param), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ReadLocalPrivateFeatures_AudioDisabled)
{
    // 音频能力未开启时不读取扩展，直接返回 0
    g_propInt32Ret = 0;
    g_dliList.getLocalPrivateFeatures = MockDliGetLocalPrivateFeatures;
    EXPECT_EQ(DLI_ReadLocalPrivateFeatures(), 0U);
    EXPECT_EQ(g_propCalls, 1);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ReadLocalPrivateFeatures_Passthrough)
{
    // 音频能力开启且扩展已注册时返回扩展结果
    g_propInt32Ret = 1;
    g_dliList.getLocalPrivateFeatures = MockDliGetLocalPrivateFeatures;
    EXPECT_EQ(DLI_ReadLocalPrivateFeatures(), RET_U32);
    EXPECT_EQ(g_propCalls, 1);
    EXPECT_EQ(g_dliCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ReadLocalPrivateFeatures_NullGuards)
{
    // 音频能力开启但函数表为空或函数未注册时都返回 0
    g_propInt32Ret = 1;
    g_dliListNull = true;
    EXPECT_EQ(DLI_ReadLocalPrivateFeatures(), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_ReadLocalPrivateFeatures(), 0U);
    EXPECT_EQ(g_propCalls, 2);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetICGAutorateParamExt_Passthrough)
{
    // 已注册扩展时三个参数与返回值原样透传
    DLI_ICGAutorateParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliList.setICGAutorateParamExt = MockDliSetICGAutorateParamExt;
    EXPECT_EQ(DLI_SetICGAutorateParamExt(&param, true, &cbkParam), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.icgAutorateParam, &param);
    EXPECT_TRUE(g_dliLast.icgAutorateMcast);
    EXPECT_EQ(g_dliLast.icgAutorateCbk, &cbkParam);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetICGAutorateParamExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ICGAutorateParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_SetICGAutorateParamExt(&param, false, &cbkParam), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_SetICGAutorateParamExt(&param, false, &cbkParam), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetICGLabelExt_Passthrough)
{
    // 已注册扩展时三个参数与返回值原样透传
    DLI_ICGLabelParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliList.setICGLabel = MockDliSetICGLabel;
    EXPECT_EQ(DLI_SetICGLabelExt(&param, false, &cbkParam), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.icgLabelParam, &param);
    EXPECT_FALSE(g_dliLast.icgLabelMcast);
    EXPECT_EQ(g_dliLast.icgLabelCbk, &cbkParam);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetICGLabelExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ICGLabelParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_SetICGLabelExt(&param, true, &cbkParam), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_SetICGLabelExt(&param, true, &cbkParam), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_CreateICBExt_Passthrough)
{
    // 已注册扩展时三个参数与返回值原样透传
    DLI_ICBConnectionParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliList.createICBExt = MockDliCreateICBExt;
    EXPECT_EQ(DLI_CreateICBExt(&param, true, &cbkParam), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.icbParam, &param);
    EXPECT_TRUE(g_dliLast.icbMcast);
    EXPECT_EQ(g_dliLast.icbCbk, &cbkParam);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_CreateICBExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ICBConnectionParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_CreateICBExt(&param, false, &cbkParam), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_CreateICBExt(&param, false, &cbkParam), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_UpdateICGParamExt_Passthrough)
{
    // 已注册扩展时三个参数与返回值原样透传
    DLI_ICGUpdatedParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliList.updateICGParam = MockDliUpdateICGParam;
    EXPECT_EQ(DLI_UpdateICGParamExt(&param, false, &cbkParam), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.icgUpdateParam, &param);
    EXPECT_FALSE(g_dliLast.icgUpdateMcast);
    EXPECT_EQ(g_dliLast.icgUpdateCbk, &cbkParam);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_UpdateICGParamExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    DLI_ICGUpdatedParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_UpdateICGParamExt(&param, true, &cbkParam), 0U);
    g_dliListNull = false;
    EXPECT_EQ(DLI_UpdateICGParamExt(&param, true, &cbkParam), 0U);
    EXPECT_EQ(g_dliCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetImgAcbInfoExt_Passthrough)
{
    // 已注册扩展时两个参数与返回值原样透传
    DLI_ImgAcbInfoParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliList.setImgAcbInfo = MockDliSetImgAcbInfo;
    EXPECT_EQ(DLI_SetImgAcbInfoExt(&param, &cbkParam), RET_U32);
    EXPECT_EQ(g_dliCalls, 1);
    EXPECT_EQ(g_dliLast.imgAcbParam, &param);
    EXPECT_EQ(g_dliLast.imgAcbCbk, &cbkParam);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_SetImgAcbInfoExt_NullGuards)
{
    // 函数表为空或该函数未注册时都返回未知命令错误码
    DLI_ImgAcbInfoParam param = {};
    DLI_ICGCbkParam cbkParam = {};
    g_dliListNull = true;
    EXPECT_EQ(DLI_SetImgAcbInfoExt(&param, &cbkParam), static_cast<uint32_t>(DLI_UNKNOWN_COMMAND));
    g_dliListNull = false;
    EXPECT_EQ(DLI_SetImgAcbInfoExt(&param, &cbkParam), static_cast<uint32_t>(DLI_UNKNOWN_COMMAND));
    EXPECT_EQ(g_dliCalls, 0);
}

// ---------- collab 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanInit_Passthrough)
{
    // 已注册扩展时两个回调指针与返回值原样透传
    COLLAB_StmDevdScanCbk_S cbk = {};
    COLLAB_DevdCollabStmFunc_S func = {};
    g_collabList.stmDevdScanInit = MockCollabStmDevdScanInit;
    EXPECT_EQ(COLLAB_StmDevdScanInit(&cbk, &func), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.stmCbk, &cbk);
    EXPECT_EQ(g_collabLast.stmFunc, &func);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanInit_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    COLLAB_StmDevdScanCbk_S cbk = {};
    COLLAB_DevdCollabStmFunc_S func = {};
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_StmDevdScanInit(&cbk, &func), 0U);
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_StmDevdScanInit(&cbk, &func), 0U);
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanInitAfterReg_Passthrough)
{
    // 已注册扩展时回调指针与返回值原样透传
    COLLAB_StmDevdScanInitAfterReg_Cbk_S cbk = {};
    g_collabList.stmDevdScanInitAfterReg = MockCollabStmDevdScanInitAfterReg;
    EXPECT_EQ(COLLAB_StmDevdScanInitAfterReg(&cbk), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.stmAfterCbk, &cbk);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanInitAfterReg_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    COLLAB_StmDevdScanInitAfterReg_Cbk_S cbk = {};
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_StmDevdScanInitAfterReg(&cbk), 0U);
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_StmDevdScanInitAfterReg(&cbk), 0U);
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanDeInit_Passthrough)
{
    // 已注册扩展时转交去初始化
    g_collabList.stmDevdScanDeInit = MockCollabStmDevdScanDeInit;
    COLLAB_StmDevdScanDeInit();
    EXPECT_EQ(g_collabCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_StmDevdScanDeInit_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    g_collabListNull = true;
    COLLAB_StmDevdScanDeInit();
    g_collabListNull = false;
    COLLAB_StmDevdScanDeInit();
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_CmInit_Passthrough)
{
    // 已注册扩展时两个回调指针与返回值原样透传
    COLLAB_CollabCmCbk_S cbk = {};
    COLLAB_CmCollabFunc_S func = {};
    g_collabList.cmInit = MockCollabCmInit;
    EXPECT_EQ(COLLAB_CmInit(&cbk, &func), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.cmCbk, &cbk);
    EXPECT_EQ(g_collabLast.cmFunc, &func);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_CmInit_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    COLLAB_CollabCmCbk_S cbk = {};
    COLLAB_CmCollabFunc_S func = {};
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_CmInit(&cbk, &func), 0U);
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_CmInit(&cbk, &func), 0U);
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_CmDeInit_Passthrough)
{
    // 已注册扩展时转交去初始化
    g_collabList.cmDeInit = MockCollabCmDeInit;
    COLLAB_CmDeInit();
    EXPECT_EQ(g_collabCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_CmDeInit_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    g_collabListNull = true;
    COLLAB_CmDeInit();
    g_collabListNull = false;
    COLLAB_CmDeInit();
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_TransFuncRegister_Passthrough)
{
    // 已注册扩展时函数指针与返回值原样透传
    COLLAB_TransFuncExt func = {};
    g_collabList.transFuncRegister = MockCollabTransFuncRegister;
    EXPECT_EQ(COLLAB_TransFuncRegister(&func), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.transFunc, &func);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_TransFuncRegister_NullGuards)
{
    // 函数表为空或该函数未注册时都返回失败错误码，且不触碰扩展
    COLLAB_TransFuncExt func = {};
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_TransFuncRegister(&func), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_TransFuncRegister(&func), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_PreAssignTransBuffer_Passthrough)
{
    // 已注册扩展时链路数与返回值原样透传
    g_collabList.preAssignTransBuffer = MockCollabPreAssignTransBuffer;
    EXPECT_EQ(COLLAB_PreAssignTransBuffer(3), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.apLinkNum, 3);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_PreAssignTransBuffer_NullGuards)
{
    // 函数表为空或该函数未注册时都返回失败错误码，且不触碰扩展
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_PreAssignTransBuffer(3), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_PreAssignTransBuffer(3), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_ContinueAssignTransBuffer_Passthrough)
{
    // 已注册扩展时占用数与返回值原样透传
    g_collabList.continueAssignTransBuffer = MockCollabContinueAssignTransBuffer;
    EXPECT_EQ(COLLAB_ContinueAssignTransBuffer(5), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.apOccupiedBufferNum, 5);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_ContinueAssignTransBuffer_NullGuards)
{
    // 函数表为空或该函数未注册时都返回失败错误码，且不触碰扩展
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_ContinueAssignTransBuffer(5), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_ContinueAssignTransBuffer(5), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    EXPECT_EQ(g_collabCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_AdvInit_Passthrough)
{
    // 已注册扩展时回调指针与返回值原样透传
    COLLAB_CollabAdvCbk_S cbk = {};
    g_collabList.advInit = MockCollabAdvInit;
    EXPECT_EQ(COLLAB_AdvInit(&cbk), RET_U32);
    EXPECT_EQ(g_collabCalls, 1);
    EXPECT_EQ(g_collabLast.advCbk, &cbk);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COLLAB_AdvInit_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 0，且不触碰扩展
    COLLAB_CollabAdvCbk_S cbk = {};
    g_collabListNull = true;
    EXPECT_EQ(COLLAB_AdvInit(&cbk), 0U);
    g_collabListNull = false;
    EXPECT_EQ(COLLAB_AdvInit(&cbk), 0U);
    EXPECT_EQ(g_collabCalls, 0);
}

// ---------- bnl 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyLinkStateChange_Passthrough)
{
    // 已注册代理时状态指针原样转交
    NLSTK_BnlLogicLinkState_S state = {};
    g_bnlList.linkStateChange = MockBnlLinkStateChange;
    BNL_ProxyLinkStateChange(&state);
    EXPECT_EQ(g_bnlCalls, 1);
    EXPECT_EQ(g_bnlLast.state, &state);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyLinkStateChange_NullGuards)
{
    // 函数表为空或代理未注册时都安静返回
    NLSTK_BnlLogicLinkState_S state = {};
    g_bnlListNull = true;
    BNL_ProxyLinkStateChange(&state);
    g_bnlListNull = false;
    BNL_ProxyLinkStateChange(&state);
    EXPECT_EQ(g_bnlCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyRecvMsg_Passthrough)
{
    // 已注册代理时消息指针原样转交
    NLSTK_BnlSendMsg_S msg = {};
    g_bnlList.recvMsg = MockBnlRecvMsg;
    BNL_ProxyRecvMsg(&msg);
    EXPECT_EQ(g_bnlCalls, 1);
    EXPECT_EQ(g_bnlLast.msg, &msg);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyRecvMsg_NullGuards)
{
    // 函数表为空或代理未注册时都安静返回
    NLSTK_BnlSendMsg_S msg = {};
    g_bnlListNull = true;
    BNL_ProxyRecvMsg(&msg);
    g_bnlListNull = false;
    BNL_ProxyRecvMsg(&msg);
    EXPECT_EQ(g_bnlCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyInit_Passthrough)
{
    // 已注册代理时转交初始化并返回成功
    NLSTK_BnlProxyFunc_S func = {};
    g_bnlList.init = MockBnlInit;
    EXPECT_EQ(BNL_ProxyInit(&func), static_cast<uint32_t>(NLSTK_ERRCODE_SUCCESS));
    EXPECT_EQ(g_bnlCalls, 1);
    EXPECT_EQ(g_bnlLast.func, &func);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyInit_NullMemberReturnsSuccess)
{
    // 代理未注册视为不支持但按成功处理，返回成功
    NLSTK_BnlProxyFunc_S func = {};
    EXPECT_EQ(BNL_ProxyInit(&func), static_cast<uint32_t>(NLSTK_ERRCODE_SUCCESS));
    EXPECT_EQ(g_bnlCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyInit_NullFuncList)
{
    // 函数表取不到时返回失败错误码
    g_bnlListNull = true;
    NLSTK_BnlProxyFunc_S func = {};
    EXPECT_EQ(BNL_ProxyInit(&func), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    EXPECT_EQ(g_bnlCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyDeInit_Passthrough)
{
    // 已注册代理时转交去初始化
    g_bnlList.deInit = MockBnlDeInit;
    BNL_ProxyDeInit();
    EXPECT_EQ(g_bnlCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, BNL_ProxyDeInit_NullGuards)
{
    // 函数表为空或代理未注册时都安静返回
    g_bnlListNull = true;
    BNL_ProxyDeInit();
    g_bnlListNull = false;
    BNL_ProxyDeInit();
    EXPECT_EQ(g_bnlCalls, 0);
}

// ---------- common 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportConnHighSpeed_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_commonList.isSupportConnHighSpeed = MockCommonIsSupportConnHighSpeed;
    g_commonRetBool = true;
    EXPECT_TRUE(COMMON_IsSupportConnHighSpeed());
    EXPECT_EQ(g_commonCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportConnHighSpeed_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportConnHighSpeed());
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportConnHighSpeed());
    EXPECT_EQ(g_commonCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportQueryQosInfo_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_commonList.isSupportQueryQosInfo = MockCommonIsSupportQueryQosInfo;
    EXPECT_TRUE(COMMON_IsSupportQueryQosInfo());
    EXPECT_EQ(g_commonCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportQueryQosInfo_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportQueryQosInfo());
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportQueryQosInfo());
    EXPECT_EQ(g_commonCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportSetMaxInterval_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_commonList.isSupportSetMaxInterval = MockCommonIsSupportSetMaxInterval;
    EXPECT_TRUE(COMMON_IsSupportSetMaxInterval());
    EXPECT_EQ(g_commonCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportSetMaxInterval_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportSetMaxInterval());
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportSetMaxInterval());
    EXPECT_EQ(g_commonCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportConnFramePowerLevel_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_commonList.isSupportConnFramePowerLevel = MockCommonIsSupportConnFramePowerLevel;
    EXPECT_TRUE(COMMON_IsSupportConnFramePowerLevel());
    EXPECT_EQ(g_commonCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportConnFramePowerLevel_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportConnFramePowerLevel());
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportConnFramePowerLevel());
    EXPECT_EQ(g_commonCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportScanFilter_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_commonList.isSupportScanFilter = MockCommonIsSupportScanFilter;
    EXPECT_TRUE(COMMON_IsSupportScanFilter());
    EXPECT_EQ(g_commonCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportScanFilter_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportScanFilter());
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportScanFilter());
    EXPECT_EQ(g_commonCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportCustomInterval_Passthrough)
{
    // 已注册扩展时地址指针与出参都原样透传
    SLE_Addr_S addr = {};
    uint16_t interval = 0;
    g_commonList.isSupportCustomInterval = MockCommonIsSupportCustomInterval;
    EXPECT_TRUE(COMMON_IsSupportCustomInterval(&addr, &interval));
    EXPECT_EQ(g_commonCalls, 1);
    EXPECT_EQ(g_commonLast.addr, &addr);
    EXPECT_EQ(g_commonLast.customInterval, &interval);
    EXPECT_EQ(interval, RET_U16);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, COMMON_IsSupportCustomInterval_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持且不写出参
    SLE_Addr_S addr = {};
    uint16_t interval = 0;
    g_commonListNull = true;
    EXPECT_FALSE(COMMON_IsSupportCustomInterval(&addr, &interval));
    g_commonListNull = false;
    EXPECT_FALSE(COMMON_IsSupportCustomInterval(&addr, &interval));
    EXPECT_EQ(g_commonCalls, 0);
    EXPECT_EQ(interval, 0);
}

// ---------- devd 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdScanFilterInit_Passthrough)
{
    // 已注册扩展时过滤函数指针与返回值原样透传
    NLSTK_DevdCollabScanFilterFunc_S filterFunc = {};
    g_devdList.devdScanFilterInit = MockDevdScanFilterInit;
    EXPECT_EQ(DevdScanFilterInit(&filterFunc), RET_U32);
    EXPECT_EQ(g_devdCalls, 1);
    EXPECT_EQ(g_devdLast.filterFunc, &filterFunc);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdScanFilterInit_NullGuards)
{
    // 函数表为空或该函数未注册时都返回失败错误码，且不触碰扩展
    NLSTK_DevdCollabScanFilterFunc_S filterFunc = {};
    g_devdListNull = true;
    EXPECT_EQ(DevdScanFilterInit(&filterFunc), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    g_devdListNull = false;
    EXPECT_EQ(DevdScanFilterInit(&filterFunc), static_cast<uint32_t>(NLSTK_ERRCODE_FAIL));
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdScanFilterDeInit_Passthrough)
{
    // 已注册扩展时转交去初始化
    g_devdList.devdScanFilterDeInit = MockDevdScanFilterDeInit;
    DevdScanFilterDeInit();
    EXPECT_EQ(g_devdCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdScanFilterDeInit_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    g_devdListNull = true;
    DevdScanFilterDeInit();
    g_devdListNull = false;
    DevdScanFilterDeInit();
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdSetScanfilterSwitch_Passthrough)
{
    // 已注册扩展时开关动作与返回值原样透传
    g_devdList.devdSetScanfilterSwitch = MockDevdSetScanfilterSwitch;
    EXPECT_EQ(DevdSetScanfilterSwitch(DEVD_FILTER_ENABLE), g_devdRetErr);
    EXPECT_EQ(g_devdCalls, 1);
    EXPECT_EQ(g_devdLast.action, DEVD_FILTER_ENABLE);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdSetScanfilterSwitch_NullGuards)
{
    // 函数表为空或该函数未注册时都返回失败错误码，且不触碰扩展
    g_devdListNull = true;
    EXPECT_EQ(DevdSetScanfilterSwitch(DEVD_FILTER_ENABLE), NLSTK_ERRCODE_FAIL);
    g_devdListNull = false;
    EXPECT_EQ(DevdSetScanfilterSwitch(DEVD_FILTER_ENABLE), NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdIsSupportScanFilter_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_devdList.devdIsSupportScanFilter = MockDevdIsSupportScanFilter;
    g_devdRetBool = true;
    EXPECT_TRUE(DevdIsSupportScanFilter());
    EXPECT_EQ(g_devdCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdIsSupportScanFilter_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_devdListNull = true;
    EXPECT_FALSE(DevdIsSupportScanFilter());
    g_devdListNull = false;
    EXPECT_FALSE(DevdIsSupportScanFilter());
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdSetScanfilter_Passthrough)
{
    // 已注册扩展时过滤配置指针原样转交
    NLSTK_DevdScanFilter_S filter = {};
    g_devdList.devdSetScanfilter = MockDevdSetScanfilter;
    DevdSetScanfilter(&filter);
    EXPECT_EQ(g_devdCalls, 1);
    EXPECT_EQ(g_devdLast.filter, &filter);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdSetScanfilter_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    NLSTK_DevdScanFilter_S filter = {};
    g_devdListNull = true;
    DevdSetScanfilter(&filter);
    g_devdListNull = false;
    DevdSetScanfilter(&filter);
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdAddChipFilters_Passthrough)
{
    // 已注册扩展时四个参数指针全部原样转交
    SDF_Vector_S scanners = {};
    NLSTK_DevdScanner_S scanner = {};
    NLSTK_DevdScanSetting_S setting = {};
    SDF_Vector_S filters = {};
    g_devdList.devdAddChipFilters = MockDevdAddChipFilters;
    DevdAddChipFilters(&scanners, &scanner, &setting, &filters);
    EXPECT_EQ(g_devdCalls, 1);
    EXPECT_EQ(g_devdLast.scanners, &scanners);
    EXPECT_EQ(g_devdLast.scanner, &scanner);
    EXPECT_EQ(g_devdLast.setting, &setting);
    EXPECT_EQ(g_devdLast.filters, &filters);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdAddChipFilters_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    SDF_Vector_S scanners = {};
    NLSTK_DevdScanner_S scanner = {};
    NLSTK_DevdScanSetting_S setting = {};
    SDF_Vector_S filters = {};
    g_devdListNull = true;
    DevdAddChipFilters(&scanners, &scanner, &setting, &filters);
    g_devdListNull = false;
    DevdAddChipFilters(&scanners, &scanner, &setting, &filters);
    EXPECT_EQ(g_devdCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdDeleteChipFilters_Passthrough)
{
    // 已注册扩展时管理器与扫描器指针原样转交
    NLSTK_DevdScanManager_S manager = {};
    NLSTK_DevdScanner_S scanner = {};
    g_devdList.devdDeleteChipFilters = MockDevdDeleteChipFilters;
    DevdDeleteChipFilters(&manager, &scanner);
    EXPECT_EQ(g_devdCalls, 1);
    EXPECT_EQ(g_devdLast.manager, &manager);
    EXPECT_EQ(g_devdLast.scanner, &scanner);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DevdDeleteChipFilters_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    NLSTK_DevdScanManager_S manager = {};
    NLSTK_DevdScanner_S scanner = {};
    g_devdListNull = true;
    DevdDeleteChipFilters(&manager, &scanner);
    g_devdListNull = false;
    DevdDeleteChipFilters(&manager, &scanner);
    EXPECT_EQ(g_devdCalls, 0);
}

// ---------- hadm 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ReadRemoteExtFeatures_Passthrough)
{
    // 已注册扩展时三个厂商参数与返回值原样透传
    g_hadmList.readRemoteExtFeatures = MockHadmReadRemoteExtFeatures;
    EXPECT_TRUE(DLI_ReadRemoteExtFeatures(0x124B, 3, 7));
    EXPECT_EQ(g_hadmCalls, 1);
    EXPECT_EQ(g_hadmLast.companyid, 0x124B);
    EXPECT_EQ(g_hadmLast.subversion, 3);
    EXPECT_EQ(g_hadmLast.readLcid, 7);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, DLI_ReadRemoteExtFeatures_NullGuards)
{
    // 函数表为空或该函数未注册时都返回不支持，且不触碰扩展
    g_hadmListNull = true;
    EXPECT_FALSE(DLI_ReadRemoteExtFeatures(0x124B, 3, 7));
    g_hadmListNull = false;
    EXPECT_FALSE(DLI_ReadRemoteExtFeatures(0x124B, 3, 7));
    EXPECT_EQ(g_hadmCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, HADM_ExtCheckAndUpdateMultiToneConfig_Passthrough)
{
    // 已注册扩展时五个参数全部原样转交
    uint8_t initSignal = 1;
    uint8_t reflSignal = 2;
    uint16_t period = 100;
    g_hadmList.checkAndUpdateMultiToneConfig = MockHadmCheckAndUpdateMultiToneConfig;
    HADM_ExtCheckAndUpdateMultiToneConfig(9, &initSignal, &reflSignal, &period, 3);
    EXPECT_EQ(g_hadmCalls, 1);
    EXPECT_EQ(g_hadmLast.multiToneLcid, 9);
    EXPECT_EQ(g_hadmLast.pmInitSignal2Tone, &initSignal);
    EXPECT_EQ(g_hadmLast.pmReflSignal2Tone, &reflSignal);
    EXPECT_EQ(g_hadmLast.occurrenceGroupPeriod, &period);
    EXPECT_EQ(g_hadmLast.toneControl, 3);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, HADM_ExtCheckAndUpdateMultiToneConfig_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回且不修改任何参数
    uint8_t initSignal = 1;
    uint8_t reflSignal = 2;
    uint16_t period = 100;
    g_hadmListNull = true;
    HADM_ExtCheckAndUpdateMultiToneConfig(9, &initSignal, &reflSignal, &period, 3);
    g_hadmListNull = false;
    HADM_ExtCheckAndUpdateMultiToneConfig(9, &initSignal, &reflSignal, &period, 3);
    EXPECT_EQ(g_hadmCalls, 0);
    EXPECT_EQ(initSignal, 1);
    EXPECT_EQ(reflSignal, 2);
    EXPECT_EQ(period, 100);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, HADM_ExtClearRemoteCsCaps_Passthrough)
{
    // 已注册扩展时链路号原样转交
    g_hadmList.clearRemoteCsCaps = MockHadmClearRemoteCsCaps;
    HADM_ExtClearRemoteCsCaps(11);
    EXPECT_EQ(g_hadmCalls, 1);
    EXPECT_EQ(g_hadmLast.clearLcid, 11);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, HADM_ExtClearRemoteCsCaps_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    g_hadmListNull = true;
    HADM_ExtClearRemoteCsCaps(11);
    g_hadmListNull = false;
    HADM_ExtClearRemoteCsCaps(11);
    EXPECT_EQ(g_hadmCalls, 0);
}

// ---------- qosm 包装层 ----------

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_EXT_AntennaDfxSendQueryCmd_Passthrough)
{
    // 已注册扩展时转交查询命令
    g_qosmList.antennaDfxSendQueryCmd = MockQosmAntennaDfxSendQueryCmd;
    QOSM_EXT_AntennaDfxSendQueryCmd();
    EXPECT_EQ(g_qosmCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_EXT_AntennaDfxSendQueryCmd_NullGuards)
{
    // 函数表为空或该函数未注册时都安静返回
    g_qosmListNull = true;
    QOSM_EXT_AntennaDfxSendQueryCmd();
    g_qosmListNull = false;
    QOSM_EXT_AntennaDfxSendQueryCmd();
    EXPECT_EQ(g_qosmCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_EXT_AntennaDfxGetAntennaPolicy_Passthrough)
{
    // 已注册扩展时返回扩展策略值
    g_qosmList.antennaDfxGetAntennaPolicy = MockQosmAntennaDfxGetAntennaPolicy;
    EXPECT_EQ(QOSM_EXT_AntennaDfxGetAntennaPolicy(), RET_INT);
    EXPECT_EQ(g_qosmCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_EXT_AntennaDfxGetAntennaPolicy_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 -1，且不触碰扩展
    g_qosmListNull = true;
    EXPECT_EQ(QOSM_EXT_AntennaDfxGetAntennaPolicy(), -1);
    g_qosmListNull = false;
    EXPECT_EQ(QOSM_EXT_AntennaDfxGetAntennaPolicy(), -1);
    EXPECT_EQ(g_qosmCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_IsLastEnableFreqBandByRecommend_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_qosmList.isLastEnableFreqBandByRecommend = MockQosmIsLastEnableFreqBandByRecommend;
    g_qosmRetBool = true;
    EXPECT_TRUE(QOSM_IsLastEnableFreqBandByRecommend());
    EXPECT_EQ(g_qosmCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_IsLastEnableFreqBandByRecommend_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 false，且不触碰扩展
    g_qosmListNull = true;
    EXPECT_FALSE(QOSM_IsLastEnableFreqBandByRecommend());
    g_qosmListNull = false;
    EXPECT_FALSE(QOSM_IsLastEnableFreqBandByRecommend());
    EXPECT_EQ(g_qosmCalls, 0);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_IsNeedRecoverFreqBandAbility_Passthrough)
{
    // 已注册扩展时返回扩展查询结果
    g_qosmList.isNeedRecoverFreqBandAbility = MockQosmIsNeedRecoverFreqBandAbility;
    g_qosmRetBool = true;
    EXPECT_TRUE(QOSM_IsNeedRecoverFreqBandAbility());
    EXPECT_EQ(g_qosmCalls, 1);
}

TEST_F(UT_ADAPTER_WRAPPER_TEST, QOSM_IsNeedRecoverFreqBandAbility_NullGuards)
{
    // 函数表为空或该函数未注册时都返回 false，且不触碰扩展
    g_qosmListNull = true;
    EXPECT_FALSE(QOSM_IsNeedRecoverFreqBandAbility());
    g_qosmListNull = false;
    EXPECT_FALSE(QOSM_IsNeedRecoverFreqBandAbility());
    EXPECT_EQ(g_qosmCalls, 0);
}
