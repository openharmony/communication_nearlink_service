# 星闪服务

## 简介

星闪（Nearlink）服务组件是 OpenHarmony 系统中提供短距离无线通信技术的服务，为设备提供接入与使用星闪无线通信协议的相关接口，包括SSAP服务发现等多种服务。

* 星闪：一种高性能短距无线通信协议

### 系统框架

![星闪框架原理图](figure/nearlink-architecture.png)

#### 模块功能说明

整体架构划分为应用框架层、星闪系统服务层和星闪驱动层。

- **应用框架层**
  * 有JS接口供hap应用调用，以及inner接口供其他SA调用。

- **系统服务层**
  * **IPC层（ipc）**：跨进程通信。
  * **Server层（services/server）**：负责消息分发。
  * **Service层（services/service）**：星闪服务层。
  * **Stack层（services/stack）**：协议栈底层实现。
  * **Hardware层（services/hardware）**：DLI适配器，与芯片交互。

## 目录

```
/foundation/communication/nearlink
├── interfaces                               # 模块对外提供接口文件
├── frameworks                               # 框架层代码目录
├── sa_profile                               # 星闪服务定义目录
├── services                                 # 星闪服务代码目录
│    ├── ipc                                 # IPC
│    ├── server                              # 消息分发层
│    ├── service                             # 星闪服务层
│    ├── stack                               # 协议栈层
│    ├── hardware                            # DLI适配器
├── test                                     # 测试
│    ├── unittest                            # 单元测试
│    └── fuzztest                            # FUZZ测试
└── LICENSE                                  # 版权声明文件
└── bundle.json                              # 部件描述文件
```

## 约束

    * **芯片要求**：必须支持星闪（NearLink）功能
    * **产品配置项**：const.nearlink.enable=1
    * **JS接口使用**：hap调用星闪JS接口前需保证manager.isNearLinkSupported()为true,以及manager.getState()获取的星闪开关状态为STATE_ON
    * **inner接口使用**：其他SA调用星闪inner接口前需保证星闪部件可被编译依赖且bool state = Nearlink::NearlinkHost::GetInstance().IsSleEnabled();中state的值为true

## 编译构建

**JS侧依赖**

``` TypeScript
//如需要使用JS接口需导入相应功能模块。
import { manager, scan, advertising, ssap, nearlinkConstant } from '@kit.ConnectivityKit';
```

**Native侧编译依赖**

``` GN
external_deps = [
  "nearlink_service:nearlink_framework",
]
```

**星闪模块编译**

根据不同的目标平台，使用以下命令进行编译：

- 全量编译

    修改build.gn文件后编译命令
    ```
    $ ./build.sh --product-name {product_name} --ccache
    ```
    未修改build.gn文件编译命令
    ```
    $ ./build.sh --product-name {product_name} --ccache --fast-rebuild
    ```

- 单独编译

    ```
    $ ./build.sh --product-name {product_name} --ccache --build-target nearlink_service
    ```

> **说明：**
> {product_name} 为当前支持的平台名称。

## 接口使用说明

为了更清晰地展示各模块的功能和使用方法，以下将对开关星闪、扫描、广播和SSAP等模块从JS侧和Native侧描述使用方法。

### 星闪开启与关闭

#### JS侧使用说明

  ``` TypeScript
  // 步骤一：导入相关模块。
  import { manager } from '@kit.ConnectivityKit';
  import { BusinessError } from '@kit.BasicServicesKit';

  // 步骤二：发起星闪状态查询。
  try {
    let state : manager.NearlinkState = manager.getState();
    console.info('state = ' + JSON.stringify(state));
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤三：通过注册的方式订阅星闪开关状态变化。
  let onReceiveEvent:(data: manager.NearlinkState) => void = (data: manager.NearlinkState) => {
    console.info('nearlink state = ' + JSON.stringify(data));
    // 根据星闪开关状态信息实现自身业务逻辑。
  };
  try {
    manager.on('stateChange', onReceiveEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤四：取消订阅星闪开关状态变化，其中onReceiveEvent是步骤三中定义的回调函数。
  try {
    manager.off('stateChange', onReceiveEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  ```

#### Native侧使用说明

1. inner接口错误码枚举

``` cpp
enum NlErrCode {
    // Common error codes
    NL_ERR_PERMISSION_FAILED = 201,
    NL_ERR_SYSTEM_PERMISSION_FAILED = 202,
    NL_ERR_PROHIBITED_BY_EDM = 203,
    NL_ERR_INVALID_PARAM = 401,
    NL_ERR_API_NOT_SUPPORT = 801,
    NL_ERR_DYNAMIC_POWER_NOT_SUPPORT = 901,
    NL_ERR_DYNAMIC_POWER_SET_FAILED = 1001,

    // Customized error codes
    NL_NO_ERROR = 0,

    NL_ERR_BASE_SYSCAP = 1009700000,
    NL_ERR_SERVICE_DISCONNECTED     = NL_ERR_BASE_SYSCAP + 1,
    NL_ERR_UNBONDED_DEVICE          = NL_ERR_BASE_SYSCAP + 2,
    NL_ERR_SLE_OFF                  = NL_ERR_BASE_SYSCAP + 3,
    NL_ERR_PROFILE_DISABLED         = NL_ERR_BASE_SYSCAP + 4,
    NL_ERR_DEVICE_DISCONNECTED      = NL_ERR_BASE_SYSCAP + 5,
    NL_ERR_MAX_CONNECTION           = NL_ERR_BASE_SYSCAP + 6,
    NL_ERR_TIMEOUT                  = NL_ERR_BASE_SYSCAP + 7,
    NL_ERR_UNAVAILABLE_PROXY        = NL_ERR_BASE_SYSCAP + 8,
    NL_ERR_INVALID_STATE            = NL_ERR_BASE_SYSCAP + 9,
    NL_ERR_PROFILE_PROHIBITED_BY_EDM    = NL_ERR_BASE_SYSCAP + 10,
    NL_ERR_PROFILE_CONNECTION_NOT_ESTABLISHED = NL_ERR_BASE_SYSCAP + 11,
    NL_ERR_DATATRANSFER_DUPLICATE_REGISTER    = NL_ERR_BASE_SYSCAP + 20,
    NL_ERR_DATATRANSFER_LIMITED     = NL_ERR_BASE_SYSCAP + 21,
    NL_ERR_DATATRANSFER_NO_REGISTER = NL_ERR_BASE_SYSCAP + 22,
    NL_ERR_DATATRANSFER_WRITE_DATA_CONGESTED = NL_ERR_BASE_SYSCAP + 23,
    NL_RANGING_RESULT_ERR           = NL_ERR_BASE_SYSCAP + 24,
    NL_ERR_CONNECTION_NOT_ESTABLISHED = NL_ERR_BASE_SYSCAP + 30,
    NL_ERR_INVALID_INTERGER         = NL_ERR_BASE_SYSCAP + 40,
    NL_ERR_INVALID_ADDRESS          = NL_ERR_BASE_SYSCAP + 41,
    NL_ERR_EMPTY_ARRAY              = NL_ERR_BASE_SYSCAP + 42,
    NL_ERR_INVALID_UUID             = NL_ERR_BASE_SYSCAP + 43,
    NL_ERR_STANDARD_UUID_NOT_ALLOWED = NL_ERR_BASE_SYSCAP + 44,
    NL_ERR_INVALID_PASSCODE         = NL_ERR_BASE_SYSCAP + 45,
    NL_ERR_STRING_LENGTH_LIMITED    = NL_ERR_BASE_SYSCAP + 46,
    NL_ERR_CDSM_NOT_SUPPORT         = NL_ERR_BASE_SYSCAP + 50,
    NL_ERR_PEER_NOT_SUPPORT_BATTERY_SERVICE = NL_ERR_BASE_SYSCAP + 51,
    NL_ERR_IMPL_ERROR               = NL_ERR_BASE_SYSCAP + 98,
    NL_ERR_INTERNAL_ERROR           = NL_ERR_BASE_SYSCAP + 99,
    NL_ERR_IPC_TRANS_FAILED         = NL_ERR_BASE_SYSCAP + 100,
    NL_ERR_SOCKET_TRANS_FAILED      = NL_ERR_BASE_SYSCAP + 101,
    NL_ERR_FEATURE_NOT_SUPPORT      = NL_ERR_BASE_SYSCAP + 102,

    // Inner error codes
    NL_ERR_INVALID_ADV_ID           = -40,
    NL_ERR_INVALID_SWITCH_OPERATION = -100,
};
``` 
2. inner接口调用

``` cpp
  #include "nearlink_host.h"

  NlErrCode errCode = Nearlink::NearlinkHost::GetInstance().EnableNl(); // 打开星闪开关,成功打开返回NL_NO_ERROR。

  NlErrCode errCode = Nearlink::NearlinkHost::GetInstance().DisableNl(); // 关闭星闪开关,成功关闭后返回NL_NO_ERROR。

  bool state = Nearlink::NearlinkHost::GetInstance().IsSleEnabled(); // 检查星闪开关状态，true为星闪打开，false为星闪关闭。

```

### 广播

#### JS侧使用说明

  ``` TypeScript
  // 步骤一：导入相关模块。
  import { advertising } from '@kit.ConnectivityKit';
  import { BusinessError } from '@kit.BasicServicesKit';

  // 步骤二：订阅星闪广播状态变化事件。
  let onReceiveEvent:(data: advertising.AdvertisingStateChangeInfo) => void =
    (data:advertising.AdvertisingStateChangeInfo) => {
    console.info('advertisingId:' + data.advertisingId); // 广播索引号。
    console.info('advertisingState:' + data.state);
    // 根据星闪广播参数信息实现自身业务逻辑。
  };
  try {
    advertising.on('advertisingStateChange', onReceiveEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤三：构造用户需要的广播参数及数据。
  let manufactureValueBuffer = new Uint8Array(4);
  manufactureValueBuffer[0] = 1;
  manufactureValueBuffer[1] = 2;
  manufactureValueBuffer[2] = 3;
  manufactureValueBuffer[3] = 4;
  let serviceValueBuffer = new Uint8Array(4);
  serviceValueBuffer[0] = 4;
  serviceValueBuffer[1] = 6;
  serviceValueBuffer[2] = 7;
  serviceValueBuffer[3] = 8;
  console.info('manufactureValueBuffer = ' + JSON.stringify(manufactureValueBuffer));
  console.info('serviceValueBuffer = ' + JSON.stringify(serviceValueBuffer));
  let setting: advertising.AdvertisingSettings = {
    interval:5000,
    power:advertising.TxPowerMode.ADV_TX_POWER_LOW
  };
  let manufactureDataUnit: advertising.ManufacturerData = {
    manufacturerId:4567,
    manufacturerData:manufactureValueBuffer.buffer
  };
  let serviceDataUnit: advertising.ServiceData = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000001234',
    serviceData:serviceValueBuffer.buffer
  };
  let advData: advertising.AdvertisingData = {
    serviceUuids:['37bea880-fc70-11ea-b720-000000001234'],
    manufacturerData:[manufactureDataUnit],
    serviceData:[serviceDataUnit]
  };
  let advertisingParams: advertising.AdvertisingParams = {
    advertisingSettings: setting,
    advertisingData: advData
  };

  // 步骤四：开启星闪广播，返回advertisingId表示当前广播索引。
  let advId = -1;
  try {
    advertising.startAdvertising(advertisingParams).then((advertisingId:number) => {
      advId = advertisingId;
      console.info('advertising id:' + JSON.stringify(advId));
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  //步骤五：停止星闪广播，其中advId是步骤四开启广播后返回的advertisingId。
  try {
    advertising.stopAdvertising(advId).then(() => {
        console.info('stop advertising success');
      }).catch ((err: BusinessError) => {
        console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
      });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 取消订阅星闪广播状态变化事件。
  try {
    advertising.off('advertisingStateChange', onReceiveEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  ```

#### Native侧使用说明

1. 广播相关数据结构

``` cpp
enum class SlePhyType : uint8_t {
    PHY_LE_1M = 1,
    PHY_LE_2M = 2,
    PHY_LE_CODED = 3,
    PHY_LE_ALL_SUPPORTED = 255
};
```
2. inner接口调用

``` cpp
  // 头文件引用。
  #include "nearlink_sle_advertiser.h"
  #include "nearlink_uuid.h"

  constexpr const char *SERVICE_UUID = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxxx";

  // 广播相关参数声明。
  SleAdvertiserSettings settings;
  settings.SetConnectable(true);
  settings.SetPrimaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_xxx)); // 这里填入SlePhyType的枚举值。
  SleAdvertiserData advData;
  std::string serviceData = "xxx"; // 这里填入广播需要携带的信息。
  UUID uuid;
  uuid.FromString(SERVICE_UUID);
  advData.AddServiceData(uuid, serviceData);
  SleAdvertiserData scanResponse;
  scanResponse.SetIncludeDeviceName(true);
  scanResponse.AddManufacturerData(0x027d, SERVICE_UUID);
  int duration = 0;
  auto callback = std::make_shared<AdvertiserCallback>();

  // 开启星闪广播,成功开启后返回NL_NO_ERROR。
  std::shared_ptr<SleAdvertiser> sleAdvertiser = SleAdvertiser::CreateSleAdvertiser();
  NlErrCode errCode = sleAdvertiser->StartAdvertising(settings, advData, scanResponse, duration, callback);

  // 关闭星闪广播,成功关闭后返回NL_NO_ERROR,这里的callback需要与开启广播中的callback保持一致。
  NlErrCode errCode = sleAdvertiser->StopAdvertising(callback);
```

### 扫描

#### JS侧使用说明
  ``` TypeScript
  // 步骤一：导入相关模块。
  import { scan } from '@kit.ConnectivityKit';
  import { BusinessError } from '@kit.BasicServicesKit';

  // 步骤二：定义扫描结果回调。
  let onReceiveEvent:(data: Array<scan.ScanResults>) => void = (data: Array<scan.ScanResults>) => {
    console.info('scan result addr:' + data[0].address + 'name:' + data[0].deviceName);
    // 根据星闪扫描回调信息实现自身业务逻辑。
  };

  // 步骤三：订阅扫描结果。
  try {
    scan.onDeviceFound(onReceiveEvent);
    // 订阅星闪扫描结果。返回的扫描结果中携带的地址为远端设备随机地址。
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤四：配置扫描参数,扫描过滤器配置期望的设备名称、地址等信息,具体详见说明。
  let scanOptions: scan.ScanOptions = {
    scanMode: scan.ScanMode.SCAN_MODE_LOW_POWER
  };

  // 步骤五：开启星闪扫描,此处开启全量扫描。
  try {
    scan.startScan([], scanOptions).then(() => {
      console.info('start scan success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤六：停止星闪扫描。
  try {
    scan.stopScan().then(() => {
      console.info('stop scan success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  //步骤7：取消订阅扫描结果，其中onReceiveEvent是在步骤3中注册的回调函数。
  try {
    scan.off('deviceFound', onReceiveEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  ```

> **步骤四说明：**
> 1. 扫描过滤器如需开启全量扫描，过滤条件可为空
> 2. 过滤器可以配置多组，组之间的条件是或的关系
> 3. 一组过滤器内的条件是与的关系,address和deviceName同时满足才会上报。


#### Native侧使用说明

1. 扫描状态SCAN_MODE枚举
``` cpp
typedef enum {
    SCAN_MODE_LOW_POWER = 0x00,
    SCAN_MODE_BALANCED = 0x01,
    SCAN_MODE_LOW_LATENCY = 0x02,
    SCAN_MODE_OP_P2_60_3000 = 0x03,
    SCAN_MODE_OP_P10_60_600 = 0x04,
    SCAN_MODE_OP_P25_60_240 = 0x05,
    SCAN_MODE_OP_P100_1000_1000 = 0x06,
    SCAN_MODE_OP_P50_100_200 = 0x07,
    SCAN_MODE_OP_P50_240_480 = 0x08,
    SCAN_MODE_OP_P50_480_960 = 0x09,
    SCAN_MODE_OP_P100_240_240 = 0x0A,
    SCAN_MODE_FULL_SCAN = 0x0B,
    SCAN_MODE_INVALID
} SCAN_MODE;
```

2. inner接口调用

``` cpp
  // 头文件引用。
  #include "nearlink_sle_scanner.h"

  // 注册广播相关回调。
  SleScanCallback = std::make_shared<xxxSleScanCallback>(); // xxxSleScanCallback为自定义的回调类

  //扫描参数设置。
  Nearlink::SleScanSettings SleScanSettings {};
  SleScanSettings.SetScanMode(Nearlink::SCAN_MODE_xxx); //这里填入的为SCAN_MODE中的枚举值。

  // 开启星闪扫描，成功开启后返回NL_NO_ERROR。
  std::shared_ptr<Nearlink::SleCentralManager> SleCentralManager = Nearlink::SleCentralManager::CreateSleCentralManager(SleScanCallback);
  NlErrCode errCode = SleCentralManager->StartFullScan(SleScanSettings);

  // 关闭星闪扫描,成功关闭后返回NL_NO_ERROR。
  NlErrCode errCode = SleCentralManager->StopScan();
```

### SSAP服务端

#### JS侧使用说明
  ``` TypeScript
  // 步骤一：导入相关模块。
  import { ssap } from '@kit.ConnectivityKit';
  import { BusinessError } from '@kit.BasicServicesKit';

  // 步骤二：创建ssap服务端实例。
  let server: ssap.Server;
  try {
    server = ssap.createServer();
    console.info('server: ' + JSON.stringify(server));
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤三：添加服务端支持的服务，其中server对象在步骤2创建，后续步骤中使用的server对象相同
  // 构造descriptor
  let descriptorsArray: Array<ssap.PropertyDescriptor> = [];
  let arrayBuffer = new ArrayBuffer(2);
  let descValue = new Uint8Array(arrayBuffer);
  descValue[0] = 11;
  descValue[1] = 22;
  let descriptor: ssap.PropertyDescriptor = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
    propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
    value: arrayBuffer,
    descriptorType:ssap.PropertyDescriptorType.PROPERTY,
    isWriteable:true
  };
  descriptorsArray[0] = descriptor;
  // 构造properties
  let propertiesArray: Array<ssap.Property> = [];
  let arrayBufferProperty = new ArrayBuffer(1);
  let properValue = new Uint8Array(arrayBufferProperty);
  properValue[0] = 1;
  let property1: ssap.Property = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
    propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
    value: arrayBufferProperty,
    descriptors:descriptorsArray,
    operation:3 // 属性可读且可写
  };
  let property2: ssap.Property = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
    propertyUuid: '37bea880-fc70-11ea-b720-000000003421',
    value: arrayBufferProperty,
    descriptors:descriptorsArray,
    operation:3 // 属性可读且可写
  };
  propertiesArray[0] = property1;
  propertiesArray[1] = property2;
  // 构造服务
  let service: ssap.Service = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
    properties:propertiesArray
  };
  try {
    server.addService(service);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤四：订阅连接状态变化事件。
  let onReceiveConnectionChangeEvent:(data: ssap.ConnectionChangeState) => void = (data: ssap.ConnectionChangeState) => {
    console.info('data:' + JSON.stringify(data));
  };
  try {
    server.on('connectionStateChange', onReceiveConnectionChangeEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤五：订阅客户端读属性请求事件。
  let onReceivePropertyReadEvent:(data: ssap.PropertyReadRequest) => void = (data: ssap.PropertyReadRequest) => {
    console.info('data:' + JSON.stringify(data));
  };
  try {
    server.on('propertyRead', onReceivePropertyReadEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤六：收到客户端读属性请求事件后，回复响应。读属性请求通过步骤五订阅。
  // 订阅客户端的读写请求，收到请求后通过该接口回复
  let arrayBuffer = new ArrayBuffer(2);
  let descValue = new Uint8Array(arrayBuffer);
  descValue[0] = 11;
  descValue[1] = 22;
  let resp: ssap.ServerResponse = {
    address: '00:11:22:33:AA:FF', // 请求方的客户端地址
    requestId: 1, // 请求方传入
    value: arrayBuffer // 回复的数据
  };
  try {
    // 地址是服务端缓存的已连接的客户端设备
    server.sendResponse(resp);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  //步骤七：通知客户端属性值更新。其中参数address是步骤四中获取的已连接客户端设备地址。
  // 构造properties
  let arrayBufferProperty = new ArrayBuffer(8);
  let properValue = new Uint8Array(arrayBufferProperty);
  properValue[0] = 123; // 本次更新后的值
  let property: ssap.Property = {
    serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
    propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
    value: arrayBufferProperty
  };
  try {
    let address = '00:11:22:33:AA:FF'; // 已连接的设备地址
    server.notifyPropertyChanged(address, property).then(() => {
      console.info('notifyPropertyChanged success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  ```

#### Native侧使用说明

1. SSAP服务端相关枚举值

``` cpp
  enum class SsapServiceType : int {
      VENDOR_PROMARY = 0x08,   /**< vendor primary service */
      VENDOR_SECONDARY = 0x09, /**< vendor secondary service */
  };
```
``` cpp
enum OperationIndication {
        OPERATION_READ = 0x01, /**< readable */
        OPERATION_WRITE_NO_RESPONSE = 0x02,
        OPERATION_WRITE_WITH_RESPONSE = 0x04,
        OPERATION_NOTIFY = 0x08,
        OPERATION_INDICATION = 0x10,
        OPERATION_BROADCAST = 0x20,
        OPERATION_WRITE_CLIENT_CONFIG = 0x200,
        OPERATION_WRITE_SERVER_CONFIG = 0x400,
    };
```
``` cpp
enum PropertyDescriptorType {
        DESCRIPTOR_TYPE_PROPERTY = 0x01,
        DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG = 0x02,
        DESCRIPTOR_TYPE_SERVER_PROPERTY_CONFIG = 0x03,
        DESCRIPTOR_TYPE_PROPERTY_FORMAT = 0x04,
        DESCRIPTOR_TYPE_PROPERT_RFU = 0x05,
        DESCRIPTOR_TYPE_VENDOR = 0xFF,
    };
```

2. inner接口调用

``` cpp
  // 头文件引用
  #include "nearlink_ssap_descriptor.h"
  #include "nearlink_ssap_property.h"
  #include "nearlink_ssap_server.h"

  // SSAP server相关参数设置。
  std::shared_ptr<Nearlink::SsapService> SsapService = std::make_shared<Nearlink::SsapService>(
        uuid, Nearlink::SsapServiceType::VENDOR_xxx); // 根据自身业务要求填入SsapServiceType中的枚举值
  std::shared_ptr<Nearlink::SsapProperty> SsapProperty =
        std::make_shared<Nearlink::SsapProperty>(Nearlink::SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY,
            uuidProperty,
            Nearlink::SsapProperty::OperationIndication::OPERATION_xxx |
                Nearlink::SsapProperty::OperationIndication::OPERATION_xxx |
            0);// 根据自身业务要求填入OperationIndication中的枚举值
  std::shared_ptr<Nearlink::SsapDescriptor> SsapDescriptor = std::make_shared<Nearlink::SsapDescriptor>(
        Nearlink::SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_xxx, 0);// 根据自身业务要求填入PropertyDescriptorType中的枚举值
  SsapProperty->AddDescriptor(*SsapDescriptor);
  SsapService->AddProperty(*SsapProperty);

  // 注册SSAP状态变化相关回调。
  ssapServerCallback = std::make_shared<xxxServerCallback>(); // xxxServerCallback为自定义的回调类

  // 增加SSAP服务端，成功开启后返回NL_NO_ERROR
  std::shared_ptr<SsapServer> ssapServer = SsapServer::CreateSsapServer(ssapServerCallback);
  NlErrCode errCode = ssapServer->AddService(*SsapService);

  // 获取property的变化
  std::string remoteAddr = "00:11:22:33:AA:FF";
  Nearlink::NearlinkRemoteDevice device(remoteAddr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
  std::vector<Nearlink::SsapProperty> properties = service->GetProperty();
  Nearlink::SsapProperty property(properties.at(i));
  ssapServer->NotifyPropertyChanged(device, property, true);

  // 移除SSAP服务端,成功删除后返回NL_NO_ERROR
  NlErrCode errCode = ssapServer->RemoveSsapService(*SsapService);
```

### SSAP客户端

#### JS侧使用说明
  ``` TypeScript
  // 步骤一：导入相关模块。
  import { ssap } from '@kit.ConnectivityKit';
  import { BusinessError } from '@kit.BasicServicesKit';

  // 步骤二：创建ssap客户端实例。
  let addr: string = '00:11:22:33:AA:FF'; // 扫描获取到的远端设备地址。
  let client: ssap.Client;
  try {
    client = ssap.createClient(addr);
    console.info('client: ' + JSON.stringify(client));
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤三：订阅连接状态变化事件,其中client对象在步骤二创建，后续步骤中使用的client对象相同。
  let onReceiveConnectionChangeEvent:(data: ssap.ConnectionChangeState) => void = (data: ssap.ConnectionChangeState) => {
    console.info('data:' + JSON.stringify(data));
    // 根据星闪连接状态变化回调信息实现自身业务逻辑。
  };
  try {
    client.on('connectionStateChange', onReceiveConnectionChangeEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤四：订阅属性变化事件。
  let onReceivePropertyChangeEvent:(data: ssap.Property) => void = (data: ssap.Property) => {
    console.info('data:' + JSON.stringify(data));
    // 根据星闪属性变化回调信息实现自身业务逻辑。
  };
  try {
    client.on('propertyChange', onReceivePropertyChangeEvent);
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤五：向服务端发起连接。连接成功后会收到步骤3中订阅的连接状态变化的回调，之后可以进行数据交互。
  try {
    client.connect().then(() => {
      console.info('connect success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤六：获取服务端支持的服务列表。
  try {
    scan.stopScan().then(() => {
      console.info('stop scan success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  //步骤七：读取指定服务的属性值，参数property中的serviceUuid以及propertyUuid通过步骤六获取。
  try {
    // 创建property,实际开发时需要通过getServices接口从服务端获取
    let arrayBufferC = new ArrayBuffer(1);
    let properV = new Uint8Array(arrayBufferC);
    properV[0] = 1;
    let property: ssap.Property = {
      serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
      propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
      value: arrayBufferC
    };
    client.readProperty(property).then((result: ssap.Property) => {
      console.info('readProperty successfully:' + JSON.stringify(result));
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  // 步骤八：写入指定服务的属性值，参数property中的serviceUuid以及propertyUuid通过步骤6获取。
  try {
    let arrayBufferC = new ArrayBuffer(1);
    // 期望写入的property值
    let properV = new Uint8Array(arrayBufferC);
    properV[0] = 1;
    let property: ssap.Property = {
      serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
      propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
      value: arrayBufferC
    };
    client.writeProperty(property, ssap.PropertyWriteType.WRITE_NO_RESPONSE).then(() => {
      console.info('writeProperty success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }

  // 步骤九：设置支持属性变化通知，参数property中的serviceUuid以及propertyUuid通过步骤六获取。
  // 之后如果服务端属性值发生变化，则客户端通过步骤四订阅的事件接收新数据。
  try {
    let arrayBufferC = new ArrayBuffer(1);
    let properV = new Uint8Array(arrayBufferC);
    properV[0] = 1;
    let property: ssap.Property = {
      serviceUuid:'37bea880-fc70-11ea-b720-000000004386',
      propertyUuid: '37bea880-fc70-11ea-b720-000000001234',
      value: arrayBufferC
    };
    client.setPropertyNotification(property, true).then(() => {
      console.info('setPropertyNotification success');
    }).catch ((err: BusinessError) => {
      console.error('errCode: ' + err.code + ', errMessage: ' + err.message);
    });
  } catch (err) {
    console.error('errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
  }
  ```

#### Native侧使用说明

1. inner接口调用

``` cpp
  // 头文件引用
  #include "nearlink_ssap_client.h"

  constexpr const char *SERVICE_UUID = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxxx";

  // SSAP client相关参数设置。
  std::string addr = "00:11:22:33:AA:FF";
  std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));

  // 注册SSAP状态变化相关回调。
  ssapCallback = std::make_shared<xxxClientCallback>(); // xxxClientCallback为自定义的回调类

  // SSAP客户端连接，成功连接后返回NL_NO_ERROR
  std::shared_ptr<Nearlink::SsapClient> ssapClient = SsapClient::CreateSsapClient(device);
  NlErrCode errCode = ssapClient->Connect(ssapCallback_);

  // SSAP客户端写入property变化,成功写入后返回NL_NO_ERROR
  std::shared_ptr<SsapService> service = ssapClient->GetService(UUID::FromString(SERVICE_UUID));
  std::vector<OHOS::Nearlink::SsapProperty> properties = service->GetProperty();
  OHOS::Nearlink::SsapProperty ssapProperty(properties.at(i));
  std::vector<uint8_t> &ctrlData; //需要写入的数据
  uint8_t *data = const_cast<uint8_t *>(ctrlData.data());
  ssapProperty.SetValue(data, ctrlData.size());
  NlErrCode errCode = ssapClient->WriteProperty(ssapProperty);

  // SSAP客户端断连，成功断连后返回NL_NO_ERROR
  NlErrCode errCode = ssapClient->Disconnect();
```

## 相关仓

[星闪服务](https://gitcode.com/openharmony-sig/communication_nearlink)
