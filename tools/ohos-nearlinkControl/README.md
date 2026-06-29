# ohos-nearlinkControl

NearLink 控制工具 - 用于启用和禁用 NearLink（星闪）功能

## 构建

```bash
# 在 OpenHarmony 构建环境中
hb build -f -p //vendor/huawei/foundation/communication/nearlink/tools/ohos-nearlinkControl:ohos-nearlinkControl
```

## 使用方法

```bash
# 启用 NearLink
ohos-nearlinkControl enable

# 启用 NearLink（指定自动连接策略）
ohos-nearlinkControl enable --autoConnPolicy 1

# 禁用 NearLink
ohos-nearlinkControl disable

# 查看帮助
ohos-nearlinkControl --help
```

## 详细文档

- [docs/README.md](docs/README.md) - 完整的功能说明和使用示例
- [tests/TEST.md](tests/TEST.md) - 测试用例文档
- [config.json](config.json) - 工具描述文件