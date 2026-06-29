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
 */

/****************************************************************************
 *
 * dli标准命令发送接口
 *
 ***************************************************************************/

#ifndef DLI_CMD_H
#define DLI_CMD_H

#include <stdint.h>
#include <stdbool.h>
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  读取媒体接入层唯一标识
 * @param  [in]  < param > 读取公共地址参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_GetPublicAddress(DLI_PublicAddrParam *param);

/**
 * @brief  设置媒体接入层唯一标识
 * @param  [in]  < addr > 媒体接入层唯一标识
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetPublicAddress(uint8_t *addr);

/**
 * @brief  可用信道指示
 * @param  [in]  < channelMap > 跳频地图
*  @param  [in]  < len > 长度
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len);

/**
 * @brief  读取本端特性
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadLocalFeatures(void);

/**
 * @brief  保存本端特性数据，系统初始化阶段由NBC模块读取本端特性在回调后进行设置
 */
void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features);

/**
 * @brief  读取控制器缓存
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadBufferSize(void);

/**
 * @brief  读取本端版本信息
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadLocalVersion(void);

/**
 * @brief  stack启动时发送的公共命令，用于读取公共地址等，保存在DLI中，提供对外接口
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadCommConfigValue(void);

/* ---------------------------------------------devd模块命令-------------------------------------------------------*/

/**
 * @brief  设置查询参数
 * @param  [in]  < scanParam > 查询参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetScanParam(DLI_ScanParam *scanParam);

/**
 * @brief  开启查询
 * @param  [in]  < scanEnable > 查询使能参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EnableScan(DLI_ScanEnable *scanEnable);

/**
 * @brief  读取最大广播数据长度
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadMaximumAdvDataLen(void);

/**
 * @brief  读取广播集合大小
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadAdvSetsNum(void);

/**
 * @brief  配置广播参数
 * @param  [in]  < advParam > 广播参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetAdvParam(DLI_AdvParam *advParam);

/**
 * @brief 设置广播数据内容
 * @param [in] advData 广播数据参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetAdvData(DLI_AdvData *advData, uint16_t dataOff);

/**
 * @brief 设置扫描响应数据内容
 * @param [in] scanRspData 扫描响应数据参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetScanRspData(DLI_ScanRspData *scanRspData, uint16_t dataOff);

/**
 * @brief  开启广播
 * @param  [in]  < advHandle > 广播句柄
 * @param  [in]  < advEnable > 广播使能参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable);

/**
 * @brief  删除广播集合
 * @param  [in]  < advHandle > 广播句柄
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_RemoveAdvSet(uint8_t advHandle);

/* ---------------------------------------------cm模块命令-------------------------------------------------------*/
/**
 * @brief  读取白名单列表大小
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadAcceptFilterListSize(void);

/**
 * @brief  清除白名单列表
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ClearAcceptFilterList(void);

/**
 * @brief  增加白名单列表设备
 * @param  [in]  < addr > 要增加的白名单列表地址
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_AddDeviceToAcceptFilterList(SLE_Addr_S *addr);

/**
 * @brief  删除白名单列表设备
 * @param  [in]  < addr > 设备信息
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr);

/**
 * @brief  创建连接
 * @param  [in]  < version > 原语版本号，当前标准版本设置为0
 * @param  [in]  < localIndex > 本地索引
 * @param  [in]  < param > 创建连接参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param);

/**
 * @brief  取消建立连接
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_CancelCreateConnection(void);

/**
 * @brief  断开连接
 * @param  [in]  < version > 原语版本号，当前标准版本设置为0
 * @param  [in]  < localIndex > 本地索引
 * @param  [in]  < param > 断开连接参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param);

/**
 * @brief  连接参数更新
 * @param  [in]  < version > 原语版本号，当前标准版本设置为0
 * @param  [in]  < localIndex > 本地索引
 * @param  [in]  < param > 更新连接参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param);

/**
 * @brief  设置物理层参数
 * @param  [in]  < param > 物理层参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetPhy(DLI_SetPhyParam *param);

/**
 * @brief  设置数据长度
 * @param  [in]  < param > 数据长度
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param);

/**
 * @brief  读取RSSI
 * @param  [in]  < param > 读取参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadRemoteRssi(DLI_ConnHandleStru *param);

/**
 * @brief  读取对端特性
 * @param  [in]  < param > 连接句柄
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param);

/**
 * @brief  读取对端版本信息
 * @param  [in]  < param > 连接句柄
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param);

/**
 * @brief  设置对端连接参数更新请求的响应
 * @param  [in]  < param > 设置参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param);

/**
 * @brief  设置编码调制参数
 * @param  [in]  < param > 编码调制参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetMcs(DLI_SetMcsParam *param);
/* ---------------------------------------------sm模块命令-------------------------------------------------------*/

/**
 * @brief  读取本端加密算法
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadSupportCryptoAlgo(void);

/**
 * @brief  设置控制器控制信令数据
 * @param  [in]  < data > 控制器控制信令数据
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetControllerData(DLI_ControllerData *data);

/**
 * @brief  启动链路加密
 * @param  [in]  < param > 启动链路加密参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EnableEncryption(DLI_EnableEncryptParam *param);

/**
 * @brief  拒绝链路加密参数请求
 * @param  [in]  < param > 拒绝链路加密参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EncryptionParamReqNegativeReply(DLI_ConnHandleStru *param);

/**
 * @brief  回复链路加密参数请求
 * @param  [in]  < param > 回复链路加密参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EncryptionParamReqReply(DLI_EncryptReqReplyParam *param);

/**
 * @brief  使用给出的密钥对普通数据进行加密
 * @param  [in]  < param > 待加密数据
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_Encrypt(DLI_EncryptParam *param);

/**
 * @brief  启动组播同步链路加密
 * @param  [in]  < param > 组播加密信息
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param);
/* ---------------------------------------------hadm模块命令-------------------------------------------------------*/

/**
 * @brief  读取本地measure特性
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadLocalMeasureCaps(void);

/**
 * @brief  读取远端measure特性
 * @param  [in]  < param > 读取远端measure特性参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param);

/**
 * @brief  配置measure参数
 * @param  [in]  < param > 配置参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param);

/**
 * @brief  使能measure
 * @param  [in]  < param > 使能参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param);

/**
 * @brief  设置同步链路参数
 * @param  [in]  < param > 同步链路参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_SetICGParam(DLI_ICGParam *param, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  设置同步链路test参数
 * @param  [in]  < param > 同步链路test参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_SetICGTestParam(DLI_ICGTestParam *param, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  移除同步链路参数
 * @param  [in]  < opCode > {@DLI_CmdOpcode}
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_RemoveICGParam(DLI_CmdOpcode opCode, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  创建同步链路
 * @param  [in]  < param > 同步链路建链参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_CreateICB(DLI_ICBConnectionParam *param, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  断开同步链路
 * @param  [in]  < param > 同步链路断链参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_DisconnectICB(DLI_DisconnectParam *param, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  接受同步链路建链请求
 * @param  [in]  < param > 接受建链请求参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_AcceptICBReq(DLI_AcceptICBReqParam *param);

/**
 * @brief  拒绝同步链路建链请求
 * @param  [in]  < param > 拒绝建链请求参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_RejectICBReq(DLI_RejectICBReqParam *param);

/**
 * @brief  设置同步链路数据路径以及编解码参数
 * @param  [in]  < dataPath > 数据路径以及编解码参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_SetupICBDataPath(DLI_SetupICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  移除同步链路数据路径以及编解码参数
 * @param  [in]  < dataPath > 数据路径以及编解码参数
 * @param  [in]  < cbkParam > 回调参数
 * @return DLI_SUCCESS: 成功, OTHER: 失败
 */
uint32_t DLI_RemoveICBDataPath(DLI_RemoveICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam);

/**
 * @brief  读取ACB数据长度
 */
uint16_t DLI_GetAcbDataLen(void);

/**
 * @brief  配置命令
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetCmd(DLI_CmdParams *params);

/**
 * @brief  是否支持标准版测距命令
 * @return true: 支持, false: 不支持
 */
bool DLI_IsSupportNewDisMeasure(void);

/**
 * @brief  是否支持bypass自动回连特性
 * @return true: 支持, false: 不支持
 */
bool DLI_IsSupportConnBypassAdv(void);

uint8_t DLI_GetPhyCountByFrameType(uint8_t frameType);
#ifdef __cplusplus
}
#endif

#endif