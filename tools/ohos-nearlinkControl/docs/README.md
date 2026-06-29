# ohos-nearlinkControl

## 概述

NearLink 控制工具，用于启用和禁用 NearLink（星闪）功能。

## 功能列表

- enable: 启用 NearLink 功能
- disable: 禁用 NearLink 功能

## 依赖

- 系统能力：NearLink (SLE)
- 权限：ohos.permission.ACCESS_NEARLINK, ohos.permission.MANAGE_NEARLINK

## 基本用法

```bash
ohos-nearlinkControl <command> [options]
```

## 命令列表

| 命令 | 说明 | 参数 | 权限 | 前置依赖 |
|------|------|------|------|----------|
| enable | 启用 NearLink 功能 | --autoConnPolicy (可选) | ohos.permission.ACCESS_NEARLINK, ohos.permission.MANAGE_NEARLINK | 无 |
| disable | 禁用 NearLink 功能 | 无 | ohos.permission.ACCESS_NEARLINK, ohos.permission.MANAGE_NEARLINK | 无 |

**前置依赖说明**：
- **无**：该命令可直接执行，无需前置条件

## 详细说明

### enable

启用 NearLink 功能，支持可选的自动连接策略。

**参数**：
- `--autoConnPolicy <number>`: 自动连接策略，可选值：
  - `0`: AUTO_CONN_GENERAL (默认策略，开启后正常自动回连)
  - `1`: AUTO_CONN_EXCEPT_AUDIO_DEVICES (不回连音频设备)
  - `2`: AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES (不回连用户主动断连的设备)

### disable

禁用 NearLink 功能。

**参数**：无

## 示例

```bash
# 启用 NearLink（使用默认策略）
ohos-nearlinkControl enable

# 启用 NearLink（不回连音频设备）
ohos-nearlinkControl enable --autoConnPolicy 1

# 禁用 NearLink
ohos-nearlinkControl disable

# 查看帮助信息
ohos-nearlinkControl --help

# 查看 enable 命令帮助
ohos-nearlinkControl enable --help

# 查看 disable 命令帮助
ohos-nearlinkControl disable --help
```

## 错误码

| 错误码 | 说明 |
|--------|------|
| NL_NO_ERROR (0) | 成功 |
| NL_ERR_PERMISSION_FAILED (201) | 权限不足 |
| NL_ERR_INVALID_PARAM (401) | 参数无效 |
| NL_ERR_API_NOT_SUPPORT (801) | API 不支持 |
| NL_ERR_SERVICE_DISCONNECTED (1009700001) | 服务断开 |
| NL_ERR_INTERNAL_ERROR (1009700099) | 内部错误 |