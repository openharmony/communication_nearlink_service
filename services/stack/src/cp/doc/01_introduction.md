# 星闪协议栈CP(Control Panel 控制面)

## 模块目录结构

```
| -- cp: Control Panel，控制面
    | -- bal: Basic Application Layer，基础应用层
        | -- audioctl: Audio Control，音频控制
            | -- actm: Audio Configuration and Transmission Management，音频流配置与传输管理
            | -- cctl: Call Control，通话控制
            | -- cdsm: Cooperative Devices Set Management，合作集设备管理
            | -- mctl: Media Control，媒体控制
    | -- bsl: Basic Service Layer，基础应用层
        | -- sle: Sparklink Low Energy，星闪同步低功耗模式
            | -- cm: Connection Management，连接管理
            | -- devd: Device Disvocery，设备发现
            | -- hadm: High Accuracy Distance Measurement，高精度测量
            | -- mm: Measurement Management，测量管理
            | -- qosm: Quality of Service Management，QoS管理
            | -- serm: Service Management，服务管理
            | -- sm: Security Management，安全管理
    | -- nlstkfwk: Things to Link Framework，物联框架
```