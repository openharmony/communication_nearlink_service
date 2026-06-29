/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <stdatomic.h>
#include "securec.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sdf_sem.h"
#include "sdf_vector.h"
#include "sdf_string.h"
#include "dli.h"
#include "dli_errno.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_cmd.h"
#include "dli_reg_ext_func.h"
#include "nlstk_log.h"
#include "nlstk_cfgdb_api.h"
#include "nbc_api.h"
#include "nlstk_cfgdb.h"
#include "cp_worker.h"
#include "cp_errno_base.h"
#include "common_ext_func_wrapper.h"
#include "dli_ext_func_wrapper.h"
#include "dli_reg_ext_func.h"
#include "collab_ext_func_wrapper.h"

#define NEARLINK_FEATURE_INDEX 8
#define NEARLINK_FEATURE_COMPETENCE 0x4

#define DEFAULT_MAX_ADV_DATA_LEN 251
#define DEFAULT_MAX_ADV_NODE_NUM 1

#ifdef WATCH_STANDARD
#define DEFAULT_SEM_WAIT_TIME 2000
#else
#define DEFAULT_SEM_WAIT_TIME 1000
#endif

#define DLI_CONN_FRAME_TYPE_4 1             // 连接帧4类型，对齐SLE_CONN_FRAME_TYPE_4
#define DLI_CONN_TX_POWER_LEVEL_7_VALUE 21  // 连接7档功率值，对齐SLE_CONN_TX_POWER_LEVEL_7_VALUE

#define BITS_8 8

typedef struct {
    atomic_bool flag;
} __attribute__((packed)) CfgdbReadCommConfigFlag_S;

typedef struct {
    uint8_t version;
    uint16_t companyIdentifier;
    uint16_t subversion;
} __attribute__((packed)) CfgdbReadLocalVerEvt_S;

typedef struct {
    uint8_t feats[NLSTK_CFGDB_LOCAL_FEATURE_LEN]; /*!< 特性表格式，按协议bit定义 */
} __attribute__((packed)) CfgdbReadLocalFeatsEvt_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t ability[NLSTK_MANUFACTURER_ABILITY_LEN];
} CfgdbManufacturerAbility_S;

static SDF_Sem g_readCmdSem;
static NLSTK_CfgdbConfigListenerFunc g_configListener[NLSTK_CFGDB_MODULE_MAX][NLSTK_CFGDB_CONFIG_MAX] = {0};
static SLE_Addr_S g_publicAddress = {.addr = {0}, .type = PUBLIC_ADDRESS};
static uint8_t g_localVersion = 0;
static NLSTK_CfgdbLocalFeatures_S g_localFeatures = {0};
static uint16_t g_maxAdvDatalen = DEFAULT_MAX_ADV_DATA_LEN;
static uint8_t g_maxAdvNodeNum = DEFAULT_MAX_ADV_NODE_NUM;
static CfgdbLocalCsCaps_S g_localCsCaps = {0};
static CfgdbAlgoCaps_S g_algoCaps = {0};
static SDF_Vector_S *g_devManuAbl = NULL;
static COLLAB_CollabAdvCbk_S g_advCollabCbk = {};

static void GetPublicAddressCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void ReadLocalVersionCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void ReadLocalFeaturesCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void ReadMaxAdvDataLenCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);
static void ReadMaxAdvNodesNumCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);
static void ReadSupportedCryptoAlgoCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void ReadLocalCsCapsCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);

static void RssiChangeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void PowerLevelChangeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void CatchChipLogCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
static void ChipResetCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);

static const DLI_CbkLineStru g_cbkTable[] = {
    {DLI_CBK_GET_PUBLIC_ADDRESS, GetPublicAddressCbk},
    {DLI_CBK_READ_LOCAL_VERSION, ReadLocalVersionCbk},
    {DLI_CBK_READ_LOCAL_FEATURE, ReadLocalFeaturesCbk},
    {DLI_CBK_READ_ADV_DATA_LEN, ReadMaxAdvDataLenCbk},
    {DLI_CBK_READ_ADV_SETS_NUM, ReadMaxAdvNodesNumCbk},
    {DLI_CBK_READ_SUPPORT_CRYPTO_ALGO, ReadSupportedCryptoAlgoCbk},
    {DLI_CBK_READ_LOCAL_MEASURE_CAPS, ReadLocalCsCapsCbk},
    {DLI_CBK_RSSI_CHANGE, RssiChangeCbk},
    {DLI_CBK_POWER_LEVEL_CHANGE, PowerLevelChangeCbk},
    {DLI_CBK_CATCH_CHIP_LOG, CatchChipLogCbk},
    {DLI_CBK_CHIP_RESET_NOTIFY, ChipResetCbk}
};

NLSTK_Errcode_E NLSTK_CfgdbRegisterConfigListener(NLSTK_CfgdbConfigListenerFunc func, NLSTK_CfgdbModule_E module,
    NLSTK_CfgdbConfig_E config)
{
    NLSTK_CHECK_RETURN(func != NULL && module < NLSTK_CFGDB_MODULE_MAX && config < NLSTK_CFGDB_CONFIG_MAX,
        NLSTK_ERRCODE_PARAM_ERR, "NLSTK_CfgdbRegisterConfigListener param invalid");
    g_configListener[module][config] = func;
    return NLSTK_ERRCODE_SUCCESS;
}

static void NotifyConfigUpdate(void *arg, NLSTK_CfgdbConfig_E config)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL && config < NLSTK_CFGDB_CONFIG_MAX, "NotifyConfigUpdate param invalid");
    for (uint8_t i = 0; i < NLSTK_CFGDB_MODULE_MAX; i++) {
        if (g_configListener[i][config] != NULL) {
            g_configListener[i][config](arg);
        }
    }
}

static void GetPublicAddressCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("GetPublicAddressCbk enter");
    NLSTK_CHECK_RETURN_VOID(
        cmdRes != NULL && cmdRes->eventParameter != NULL && cmdRes->size == SLE_ADDR_LEN, "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "GetPublicAddressCbk fail, status = 0x%04X", status);

    g_publicAddress.type = PUBLIC_ADDRESS;
    (void)memcpy_s(g_publicAddress.addr, SLE_ADDR_LEN, cmdRes->eventParameter, SLE_ADDR_LEN);
    NLSTK_LOG_INFO("GetPublicAddressCbk addr:%s", GET_ENC_ADDR(&g_publicAddress));
    NotifyConfigUpdate(&g_publicAddress, NLSTK_CFGDB_CONFIG_PUBLIC_ADDRESS);
}

static void ReadLocalVersionCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("ReadLocalVersionCbk enter");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL &&
        cmdRes->size == sizeof(CfgdbReadLocalVerEvt_S), "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "ReadLocalVersionCbk fail, status = 0x%04X", status);
    CfgdbReadLocalVerEvt_S *param = (CfgdbReadLocalVerEvt_S *)cmdRes->eventParameter;
    g_localVersion = param->version;
    NLSTK_LOG_INFO("read local version %d", g_localVersion);
    NotifyConfigUpdate(&g_localVersion, NLSTK_CFGDB_CONFIG_LOCAL_VERSION);
}

uint32_t ReadLocalMeasureCaps(void)
{
    if (DLI_IsSupportNewDisMeasure()) {
        return DLI_ReadLocalMeasureCaps();
    } else {
        if (DLI_GetExtFuncList()->readLocalMeasureCapsExt != NULL) {
            return DLI_GetExtFuncList()->readLocalMeasureCapsExt();
        }
    }
    return NLSTK_ERR;
}

static void ReadLocalFeaturesCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("ReadLocalFeaturesCbk enter");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL &&
        cmdRes->size == NLSTK_CFGDB_LOCAL_FEATURE_LEN, "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "ReadLocalFeaturesCbk fail, status = 0x%04X", status);

    CfgdbReadLocalFeatsEvt_S *param = (CfgdbReadLocalFeatsEvt_S *)cmdRes->eventParameter;
    g_localFeatures.status = status;
    (void)memcpy_s(g_localFeatures.feats, NLSTK_CFGDB_LOCAL_FEATURE_LEN, param->feats, NLSTK_CFGDB_LOCAL_FEATURE_LEN);
    NotifyConfigUpdate(&g_localFeatures, NLSTK_CFGDB_CONFIG_LOCAL_FEATURE);

    // DLI_LocalFeatures_S 与 NLSTK_CfgdbLocalFeatures_S 结构体定义保持一致，包括是否字节对齐
    DLI_SetLocalFeatures((DLI_LocalFeatures_S *)&g_localFeatures);

    // 读取CS特性，需要在local feature读完后读取
    if ((g_localFeatures.feats[NEARLINK_FEATURE_INDEX] & NEARLINK_FEATURE_COMPETENCE) == NEARLINK_FEATURE_COMPETENCE) {
        if (DLI_IsSupportNewDisMeasure()) {
            NLSTK_LOG_INFO("[HADM] Start to read local measure caps.");
            DLI_ReadLocalMeasureCaps();
        } else {
            if (DLI_GetExtFuncList()->readLocalMeasureCapsExt != NULL) {
                NLSTK_LOG_INFO("[HADM] Start to read local measure caps ext.");
                DLI_GetExtFuncList()->readLocalMeasureCapsExt();
            }
        }
    } else {
        // 如果不支持CS，POST信号量通知主线程读取完成
        SDF_SemPost(g_readCmdSem);
    }
}

static void ReadMaxAdvDataLenCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL &&
        cmdRes->size == sizeof(uint16_t), "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "ReadMaxAdvDataLenCbk fail, status = 0x%04X", status);

    uint16_t *value = (uint16_t *)cmdRes->eventParameter;
    NLSTK_LOG_INFO("[DEVD]max adv data len = 0x%x", *value);
    g_maxAdvDatalen = *value;
}

static void ReadMaxAdvNodesNumCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL &&
        cmdRes->size == sizeof(uint8_t), "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "ReadMaxAdvNodesNumCbk fail, status = 0x%04X", status);

    uint8_t *value = (uint8_t *)cmdRes->eventParameter;
    NLSTK_LOG_INFO("[DEVD]max adv node num = 0x%x", *value);
    g_maxAdvNodeNum = *value;
    if (g_advCollabCbk.notifySysMaxAdvNodesNum != NULL) {
        g_advCollabCbk.notifySysMaxAdvNodesNum(g_maxAdvNodeNum);
    }
}

static void ReadSupportedCryptoAlgoCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("ReadSupportedCryptoAlgoCbk enter");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "Read supported crypto algorithm callback failed.");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL &&
        cmdRes->size == sizeof(CfgdbAlgoCaps_S), "Read crypto algo callback error.");

    CfgdbAlgoCaps_S *param = (CfgdbAlgoCaps_S *)cmdRes->eventParameter;
    (void)memcpy_s(&g_algoCaps, sizeof(CfgdbAlgoCaps_S), param, sizeof(CfgdbAlgoCaps_S));
}

static void ReadLocalCsCapsCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("ReadLocalCsCapsCbk enter");
    if (status != DLI_SUCCESS) {
        // 如果不支持CS，POST信号量通知主线程读取完成
        NLSTK_LOG_ERROR("ReadLocalCsCapsCbk fail, status = 0x%04X", status);
        SDF_SemPost(g_readCmdSem);
        return;
    }
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL, "Read local cs caps callback error.");
    DLI_ReadLocalCsCapsEvt *param = (DLI_ReadLocalCsCapsEvt *)cmdRes->eventParameter;
    g_localCsCaps.status = (uint8_t)status;
    (void)memcpy_s(&g_localCsCaps.caps, SLE_MEASURE_LEN, &param->caps, SLE_MEASURE_LEN);
    SDF_SemPost(g_readCmdSem);
}

static void RssiChangeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("RssiChangeCbk enter");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL && cmdRes->size > 0, "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "RssiChangeCbk fail, status = 0x%04X", status);

    NLSTK_CfgdbChanInfo_S chanInfo;
    chanInfo.data = cmdRes->eventParameter;
    chanInfo.len = cmdRes->size;
    NotifyConfigUpdate(&chanInfo, NLSTK_CFGDB_CONFIG_RSSI);
}

static void PowerLevelChangeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_DEBUG("PowerLevelChangeCbk enter");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL && cmdRes->size > 0, "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "PowerLevelChangeCbk fail, status = 0x%04X", status);

    NLSTK_CfgdbChanInfo_S chanInfo;
    chanInfo.data = cmdRes->eventParameter;
    chanInfo.len = cmdRes->size;
    NotifyConfigUpdate(&chanInfo, NLSTK_CFGDB_CONFIG_POWER_LEVEL);
}

static void CatchChipLogCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    NLSTK_LOG_INFO("catch chip log statu: %hu", status);
}

static void ChipResetCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    NLSTK_LOG_INFO("ChipResetCbk enter");
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL, "arg is invalid");
    NLSTK_CHECK_RETURN_VOID(status == DLI_SUCCESS, "ChipResetCbk fail, status = 0x%04X", status);

    NLSTK_CHECK_RETURN_VOID(cmdRes->size == sizeof(uint8_t), "cmdRes size check failed");
    uint8_t arg = *(uint8_t *)cmdRes->eventParameter;
    NotifyConfigUpdate(&arg, NLSTK_CFGDB_CONFIG_CHIP_RESET);
}

uint32_t CfgdbInit(void)
{
    uint32_t ret = DLI_CmdCbkReg(NBC, NULL, 0, g_cbkTable, sizeof(g_cbkTable) / sizeof(DLI_CbkLineStru));
    NLSTK_CHECK_RETURN(ret == DLI_SUCCESS, NLSTK_ERR, "CfgdbInit register dli callbacks failure with ret = %d.", ret);
    SDF_Traits traits = {.dtor = SDF_MemFree};
    g_devManuAbl = SDF_CreateVector(traits);
    if (g_devManuAbl == NULL) {
        DLI_CmdCbkUnReg(NBC, NULL, 0, g_cbkTable, sizeof(g_cbkTable) / sizeof(DLI_CbkLineStru));
        NLSTK_LOG_ERROR("CfgdbInit create vector failed.");
        return NLSTK_ERR;
    }
    g_readCmdSem = (SDF_Sem)SDF_MemZalloc(SDF_SEM_SIZE);
    if (g_readCmdSem == NULL) {
        DLI_CmdCbkUnReg(NBC, NULL, 0, g_cbkTable, sizeof(g_cbkTable) / sizeof(DLI_CbkLineStru));
        SDF_DestroyVector(g_devManuAbl);
        g_devManuAbl = NULL;
        NLSTK_LOG_ERROR("CfgdbInit malloc sem failed.");
        return NLSTK_ERR;
    }
    ret = COLLAB_AdvInit(&g_advCollabCbk);
    if (ret != 0) {
        NLSTK_LOG_ERROR("CfgdbInit collab adv init failed, ret=%u", ret);
    }
    return NLSTK_OK;
}

void CfgdbDeinit(void)
{
    // current no collab adv deinit func
    DLI_CmdCbkUnReg(NBC, NULL, 0, g_cbkTable, sizeof(g_cbkTable) / sizeof(DLI_CbkLineStru));
    (void)memset_s(&g_configListener, sizeof(g_configListener), 0, sizeof(g_configListener));
    (void)memset_s(&g_publicAddress, sizeof(g_publicAddress), 0, sizeof(g_publicAddress));
    (void)memset_s(&g_localFeatures, sizeof(g_localFeatures), 0, sizeof(g_localFeatures));
    g_localVersion = 0;
    g_maxAdvDatalen = DEFAULT_MAX_ADV_DATA_LEN;
    g_maxAdvNodeNum = DEFAULT_MAX_ADV_NODE_NUM;
    (void)memset_s(&g_localCsCaps, sizeof(g_localCsCaps), 0, sizeof(g_localCsCaps));
    (void)memset_s(&g_algoCaps, sizeof(g_algoCaps), 0, sizeof(g_algoCaps));
    SDF_DestroyVector(g_devManuAbl);
    g_devManuAbl = NULL;
    if (g_readCmdSem != NULL) {
        SDF_MemFree(g_readCmdSem);
        g_readCmdSem = NULL;
    }
}

static void CfgdbReadCommConfigValueInner(void *param)
{
    CfgdbReadCommConfigFlag_S *configFlag = (CfgdbReadCommConfigFlag_S *)param;
    if (DLI_ReadLocalVersion() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadLocalVersion failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    DLI_PublicAddrParam cmd = {.mediaAccessLayerIDType = 0};
    if (DLI_GetPublicAddress(&cmd) != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI GetPublicAddress failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    if (DLI_ReadMaximumAdvDataLen() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadMaximumAdvDataLen failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    if (DLI_ReadAdvSetsNum() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadAdvSetsNum failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    if (DLI_ReadSupportCryptoAlgo() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadSupportCryptoAlgo failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    // 命令在DLI全部顺序读取，读取本地特性需要放在最后，在读取本地特性回调中读取CS特性
    if (DLI_ReadLocalFeatures() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadLocalFeatures failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    if (DLI_ReadLocalPrivateFeatures() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI ReadLocalPrivateFeatures failed.");
        atomic_store(&configFlag->flag, false);
        return;
    }
    bool isSupport = COMMON_IsSupportConnFramePowerLevel();
    if (isSupport) {
        DLI_SetConnFramePowerLevelParam powerLevelParam = {0};
        powerLevelParam.frameType = DLI_CONN_FRAME_TYPE_4;
        powerLevelParam.txPower = DLI_CONN_TX_POWER_LEVEL_7_VALUE;
        if (DLI_SetConnFramePowerLevel(&powerLevelParam) != DLI_SUCCESS) {
            NLSTK_LOG_ERROR("CfgdbReadCommConfigValue DLI SetConnFramePoweLevel failed.");
            atomic_store(&configFlag->flag, false);
            return;
        }
    }
}

uint32_t CfgdbReadCommConfigValue(void)
{
    if (g_readCmdSem == NULL || SDF_SemInit(g_readCmdSem, 0, 0) != NLSTK_OK) {
        return NLSTK_ERR;
    }
    CfgdbReadCommConfigFlag_S configFlag = {0};
    atomic_init(&configFlag.flag, true);
    uint32_t ret = CP_PostTaskBlocked(CfgdbReadCommConfigValueInner, (void *)&configFlag, NULL, DEFAULT_SEM_WAIT_TIME);
    if (ret != NLSTK_OK || !configFlag.flag) {
        NLSTK_LOG_ERROR("CP_PostTaskBlocked failed, ret=%u", ret);
        return ret;
    }
    if (SDF_SemTimeWait(g_readCmdSem, DEFAULT_SEM_WAIT_TIME) != 0) {
        NLSTK_LOG_ERROR("CfgdbReadCommConfigValue wait sem failed.");
        return NLSTK_ERR;
    }
    return NLSTK_OK;
}

uint32_t NLSTK_CfgdbGetPublicAddress(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "NLSTK_CfgdbGetPublicAddress param invalid");
    (void)memcpy_s(addr, sizeof(SLE_Addr_S), &g_publicAddress, sizeof(SLE_Addr_S));
    return NLSTK_OK;
}

uint8_t CfgdbGetLocalVersion(void)
{
    return g_localVersion;
}

uint16_t CfgdbGetMaxAdvDataLen(void)
{
    return g_maxAdvDatalen;
}

static uint8_t CfgdbAssignCollabMaxAdvNodesNum(void)
{
    if (g_advCollabCbk.assignCollabMaxAdvNodesNum == NULL) {
        return 0;
    }
    return g_advCollabCbk.assignCollabMaxAdvNodesNum();
}

uint8_t CfgdbGetMaxAdvNodesNum(void)
{
    uint8_t collabMaxAdvNodesNum = CfgdbAssignCollabMaxAdvNodesNum();
    if (g_maxAdvNodeNum < collabMaxAdvNodesNum) {
        // 异常情况处理
        NLSTK_LOG_WARN("g_maxAdvNodeNum less than collab max nodes num:%hhu", collabMaxAdvNodesNum);
        return g_maxAdvNodeNum;
    }
    return (g_maxAdvNodeNum > DEFAULT_MAX_ADV_NODE_NUM) ? (g_maxAdvNodeNum - collabMaxAdvNodesNum) :
        g_maxAdvNodeNum;
}

uint32_t CfgdbReadAlgoCaps(CfgdbAlgoCaps_S *caps)
{
    NLSTK_CHECK_RETURN(caps != NULL, NLSTK_ERR, "CfgdbReadAlgoCaps param invalid");
    (void)memcpy_s(caps, sizeof(CfgdbAlgoCaps_S), &g_algoCaps, sizeof(CfgdbAlgoCaps_S));
    return NLSTK_OK;
}

uint32_t CfgdbReadLocalCsCaps(CfgdbLocalCsCaps_S *caps)
{
    NLSTK_CHECK_RETURN(caps != NULL, NLSTK_ERRCODE_POINTER_NULL, "CfgdbReadLocalCsCaps param invalid");
    (void)memcpy_s(caps, sizeof(CfgdbLocalCsCaps_S), &g_localCsCaps, sizeof(CfgdbLocalCsCaps_S));
    return NLSTK_ERRCODE_SUCCESS;
}

SLE_Addr_S *NBC_GetPublicAddress(void)
{
    return &g_publicAddress;
}

uint32_t NLSTK_CfgdbGetChipLog(void)
{
    NLSTK_LOG_INFO("NLSTK_CfgdbGetChipLog");
    return 0;
}

static bool CfgdbCompAddr(void *ptr, void *arg)
{
    CfgdbManufacturerAbility_S *manuAbl = (CfgdbManufacturerAbility_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    return (memcmp(&manuAbl->addr, addr, sizeof(SLE_Addr_S)) == 0);
}

static void CfgdbSetManufacturerAbilityInner(void *arg)
{
    CfgdbManufacturerAbility_S *param = (CfgdbManufacturerAbility_S *)arg;
    size_t index = 0;
    CfgdbManufacturerAbility_S *manuAbl = NULL;
    if (!SDF_VectorFindFirst(g_devManuAbl, CfgdbCompAddr, &param->addr, &index)) {
        manuAbl = (CfgdbManufacturerAbility_S *)SDF_MemZalloc(sizeof(CfgdbManufacturerAbility_S));
        NLSTK_CHECK_RETURN_VOID(manuAbl != NULL, "malloc failed");
        if (!SDF_VectorEmplaceBack(g_devManuAbl, manuAbl)) {
            SDF_MemFree(manuAbl);
            NLSTK_LOG_ERROR("Set manufacturer ability emplace back failed");
            return;
        }
    } else {
        manuAbl = SDF_VectorElementAt(g_devManuAbl, index);
    }
    (void)memcpy_s(manuAbl, sizeof(CfgdbManufacturerAbility_S), param, sizeof(CfgdbManufacturerAbility_S));
    NLSTK_LOG_INFO("Set manufacturer ability, addr: %s, ability: %s", GET_ENC_ADDR(&manuAbl->addr),
        SDF_GET_UINT8_STR(manuAbl->ability, NLSTK_MANUFACTURER_ABILITY_LEN));
}

uint32_t NLSTK_CfgdbSetManufacturerAbility(SLE_Addr_S *addr, NLSTK_ManufacturerAbility_S *ability)
{
    NLSTK_CHECK_RETURN(addr != NULL && ability != NULL, NLSTK_ERRCODE_POINTER_NULL,
        "NLSTK_CfgdbSetManufacturerAbility param invalid");
    CfgdbManufacturerAbility_S *manuAbl =
        (CfgdbManufacturerAbility_S *)SDF_MemZalloc(sizeof(CfgdbManufacturerAbility_S));
    NLSTK_CHECK_RETURN(manuAbl != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "malloc failed");
    (void)memcpy_s(&manuAbl->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(manuAbl->ability, NLSTK_MANUFACTURER_ABILITY_LEN, ability->ability, NLSTK_MANUFACTURER_ABILITY_LEN);
    uint32_t ret = CP_PostTaskBlocked(CfgdbSetManufacturerAbilityInner, manuAbl, SDF_MemFree, SEM_ALWAYS_WAIT);
    if (ret != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

bool CfgdbGetManufacturerSupport(SLE_Addr_S *addr, CfgdbManufacturerAbility_E ability)
{
    NLSTK_CHECK_RETURN(addr != NULL, false, "addr is null");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_devManuAbl, CfgdbCompAddr, addr, &index)) {
        NLSTK_LOG_ERROR("CfgdbGetManufacturerSupport devManuAbl map find cur addr not exist");
        return false;
    }
    CfgdbManufacturerAbility_S *manuAbl = SDF_VectorElementAt(g_devManuAbl, index);
    return (manuAbl->ability[ability / BITS_8] & (1 << (ability % BITS_8))) != 0;
}

static void CfgdbRemoveManufacturerAbilityInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_devManuAbl, CfgdbCompAddr, addr, &index)) {
        return;
    }
    SDF_VectorRemove(g_devManuAbl, index);
}

uint32_t NLSTK_CfgdbRemoveManufacturerAbility(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
        "NLSTK_CfgdbRemoveManufacturerAbility param invalid");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(tempAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "malloc failed");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = CP_PostTaskBlocked(CfgdbRemoveManufacturerAbilityInner, tempAddr, SDF_MemFree, SEM_ALWAYS_WAIT);
    if (ret != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void CfgdbRegisterCbksInner(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "arg is null.");
    NLSTK_CfgdbCbk_S *cbk = (NLSTK_CfgdbCbk_S *)arg;
    if (cbk->rssiCbk != NULL) {
        NLSTK_CfgdbRegisterConfigListener(
            cbk->rssiCbk, NLSTK_CFGDB_MODULE_NEARLINK_SERVICE, NLSTK_CFGDB_CONFIG_RSSI);
    }
    if (cbk->powerLevelCbk != NULL) {
        NLSTK_CfgdbRegisterConfigListener(
            cbk->powerLevelCbk, NLSTK_CFGDB_MODULE_NEARLINK_SERVICE, NLSTK_CFGDB_CONFIG_POWER_LEVEL);
    }
    if (cbk->localFeatureCbk != NULL) {
        NLSTK_CfgdbRegisterConfigListener(
            cbk->localFeatureCbk, NLSTK_CFGDB_MODULE_NEARLINK_SERVICE, NLSTK_CFGDB_CONFIG_LOCAL_FEATURE);
    }
    if (cbk->chipResetNotifyCbk != NULL) {
        NLSTK_CfgdbRegisterConfigListener(
            cbk->chipResetNotifyCbk, NLSTK_CFGDB_MODULE_NEARLINK_SERVICE, NLSTK_CFGDB_CONFIG_CHIP_RESET);
    }
}

NLSTK_Errcode_E NLSTK_CfgdbRegisterCbks(NLSTK_CfgdbCbk_S *cbkIn)
{
    NLSTK_CHECK_RETURN(cbkIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "cfgdb cbkIn param is null");

    NLSTK_CfgdbCbk_S *cbk = (NLSTK_CfgdbCbk_S *)SDF_MemZalloc(sizeof(NLSTK_CfgdbCbk_S));
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_POINTER_NULL, "cfgdb cbk param malloc fail");
    cbk->rssiCbk = cbkIn->rssiCbk;
    cbk->powerLevelCbk = cbkIn->powerLevelCbk;
    cbk->localFeatureCbk = cbkIn->localFeatureCbk;
    cbk->chipResetNotifyCbk = cbkIn->chipResetNotifyCbk;
    uint32_t ret = CP_PostTask(CfgdbRegisterCbksInner, cbk, SDF_MemFree);

    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "CP_PostTask failed in NLSTK_CfgdbRegisterCbks");

    return NLSTK_ERRCODE_SUCCESS;
}
