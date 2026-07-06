# Nearlink Service

## Introduction

NearLink is a new-generation short-range wireless connection technology with features such as low power consumption, high reliability, and low latency. It is widely used in smart home, smart mobility, smart terminals, and other fields. For more information, see the [NearLink Alliance Official Website](https://www.isla.org.cn/).

The Nearlink service component is a system service in the OpenHarmony system that provides Nearlink wireless communication capabilities. It provides interfaces for devices to access and use Nearlink basic functions. The main features include:

- **Device Discovery**: Provides scanning and advertising capabilities, supporting fast device discovery and pairing.
- **Connection Management**: Provides full lifecycle management for device connection establishment, maintenance, and disconnection.
- **Service-oriented Data Interaction**: A service discovery framework based on the SSAP (SparkLink Service Access Protocol) property protocol, supporting server/client mode for property read/write and notifications between devices.



### System Architecture

The Nearlink service adopts a layered architecture design, divided from top to bottom into the application layer, framework layer, system service layer, and driver layer. The application layer accesses through ArkTS API or C++ Inner API; the framework layer provides unified interface encapsulation; the system service layer implements core business logic; and the driver layer is responsible for interacting with hardware chips. Each layer is decoupled through standardized interfaces to ensure system maintainability and scalability.

![Nearlink Architecture Diagram](figure/nearlink-architecture.png)

#### Module Function Description

The overall architecture is divided into the application layer, framework layer, system service layer, and driver layer.

- **Application Layer**
  * **Northbound Applications**: settings, systemUI, ecosystem applications, etc., which call Nearlink capabilities through ArkTS API.
  * **Native Services**: Other system services call Nearlink capabilities through Inner API (C++).

- **Framework Layer**
  * **ArkTS APIs**: Provides ArkTS interfaces for local device management, remote device discovery, connection, and management through [Connectivity Kit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md).
  * **Inner API**: Provides C++ interfaces such as NearlinkHost API, SleAdvertiser API, SsapServer API, etc., offering core capabilities such as Nearlink device management, advertising control, and service discovery for other system services (SA).

- **System Service Layer**
  * **Basic Capabilities**: Provides atomic capability encapsulation including device information management and state maintenance, human-machine interaction and collaborative operations, battery information monitoring and management, cooperative device discovery and networking, audio stream management and transmission, etc., providing unified and reusable basic service interfaces for upper-layer applications.
  * **Control Plane**: Provides complete control plane functions including device discovery and pairing, security authentication and key management, connection establishment and maintenance, service discovery and registration, QoS policy control, etc.
  * **Data Plane**: Provides complete data plane functions including basic frame transmission, basic mode low-latency communication, stream mode high-speed data interaction, enhanced mode reliable transmission, reliable mode data guarantee, etc., meeting data transmission requirements for low latency, high throughput, and strong reliability in various scenarios.

- **Driver Layer**
  * **DLI (Data Link Interface) Service**: Provides device logic layer interface encapsulation, implementing efficient communication and command interaction between upper-layer services and underlying chips.
  * **Serial Data Communication**: Implements full-duplex data transmission with Nearlink chips based on UART serial port, ensuring reliability and real-time performance of command delivery and data reporting.
  * **FD Event Listening Management**: Implements efficient monitoring and distribution of chip asynchronous events, data arrival, and exception states through file descriptor event-driven model.
  * **Nearlink Address Management**: Responsible for generation, allocation, maintenance, and query of device Nearlink addresses, ensuring uniqueness and correctness of addressing and identity identification between devices.

## Directory Structure

```
/foundation/communication/nearlink
├── interfaces                               # External API files provided by the module
│    └── inner_api                           # Internal interfaces
├── frameworks                               # Framework layer code directory
│    ├── js                                  # JS framework code
│    ├── native                              # Native framework code
├── sa_profile                               # Nearlink service definition directory
├── services                                 # Nearlink service code directory
│    ├── server                              # Server-side IPC message dispatch
│    ├── service                             # Basic services
│    ├── stack                               # Protocol stack
│    ├── hardware                            # Driver service adaptation
│    ├── common                              # Common utility module
├── test                                     # Testing
│    ├── unittest                            # TDD tests
│    └── fuzztest                            # Fuzz tests
└── LICENSE                                  # License declaration file
└── bundle.json                              # Component description file
```

## Constraints

- **Hardware Dependency Constraint**: The device hardware must integrate a wireless communication chip that supports the [NearLink protocol](https://www.isla.org.cn/trial?page=1&sort=), and the chip needs to support NearLink E1.0 or above protocol specifications, otherwise Nearlink-related capabilities cannot be used.
- **System Adaptation Constraint**: Currently, this component only supports OpenHarmony Standard System and needs to be used with a complete system service framework.
- **Product Configuration Constraint**: The `nearlink_service` component must be included in the product component declaration, otherwise this component will not participate in compilation and system image packaging. The product configuration must set `const.nearlink.enable = 1`, otherwise Nearlink capabilities will not be enabled.
- **Specification Capability Constraint**: The current open-source version only supports SLE low-power access mode; other modes are not yet supported. The number of simultaneous connections, ranging accuracy, and other capabilities vary depending on chip specifications and business concurrency, with differences in implementation across different vendor chips.

## Build

### Compilation

Use the following commands to compile for different target platforms:

- **Full Build**

    Build command after modifying build.gn file
    ```
    $ ./build.sh --product-name {product_name} --ccache
    ```
    Build command without modifying build.gn file
    ```
    $ ./build.sh --product-name {product_name} --ccache --fast-rebuild
    ```

- **Individual Component Build**

    ```
    $ ./build.sh --product-name {product_name} --ccache --build-target nearlink_service
    ```

> **Note:** {product_name} is the name of the currently supported platform.

### Configurable Features

This component supports feature trimming through product configuration files on demand. The default values for all feature switches are as follows.

| Feature Name | Default Value | Description | Applicable System |
|--------------|---------------|-------------|-------------------|
| nearlink_service_kia_enable | false | KIA (key information assets) disables Nearlink data transmission in specific scenarios | Standard System |
| nearlink_service_edm_enable | false | EDM (Enterprise Device Management) disables Nearlink functions in enterprise management scenarios | Standard System |
| nearlink_service_rss_background_task | false | Background task support, resource scheduling service integration | Standard System |
| nearlink_service_bas_enable | false | BAS (Battery Management Service) battery management service | Standard System |
| nearlink_service_power_manager_enhance | false | Power management enhancement, optimizing power consumption strategy | Standard System |
| nearlink_service_host_dynamic_running | false | Dynamic loading/unloading of driver processes | Standard System |
| nearlink_service_host_avoid_sleep | false | Avoid sleep, keeping service active | Standard System |
| nearlink_service_no_pairing_dialog | false | Skip pairing dialog, authenticate through other methods | Standard System |
| nearlink_service_pluggable_supported | false | Pluggable support, dynamic loading/unloading of modules | Standard System/Small System |

Products can override configurations in `vendor/{vendor}/{product}/config.json`. Refer to [Component-based Compilation Best Practices](https://gitcode.com/openharmony/build/blob/master/docs/%E9%83%A8%E4%BB%B6%E5%8C%96%E7%BC%96%E8%AF%91%E6%9C%80%E4%BD%B3%E5%AE%9E%E8%B7%B5.md).

Configuration Example (e.g., to enable EDM capability):

> ```json
> {
>   "component": {
>     "name": "nearlink_service",
>     "subsystem": "communication",
>     "features": [
>       "nearlink_service_edm_enable=true"
>     ]
>   }
> }
> ```

## Usage Guide

This chapter introduces the usage of Nearlink core modules, covering functions such as Nearlink switch, Nearlink advertising, and Nearlink scanning, providing standard call examples for both Native side (C++) and application side (ArkTS).

### Native-side Usage Guide

The following are the standard usage patterns for Nearlink core modules on the Native side. The Native side provides Nearlink capability access for other system services through C++ Inner API. The interfaces are defined in header files under the `interfaces/inner_api/include/` directory, covering core functions such as Nearlink switch control, device advertising, and scanning.

### Nearlink Switch

Provides Nearlink switch start/stop control, status query, and status change monitoring capabilities, adopting a global singleton pattern. The core interfaces are defined in the `nearlink_host.h` header file.

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `NearlinkHost::GetInstance()` | Get the global singleton instance of Nearlink host management class |
| `EnableNl(AutoConnPolicy policy = AUTO_CONN_GENERAL)` | Enable Nearlink function; the auto-reconnection strategy can be configured through the `policy` parameter, defaulting to normal auto-reconnection mode |
| `DisableNl()` | Disable Nearlink function |
| `IsSleEnabled()` | Query whether Nearlink SLE low-power mode is enabled, returns boolean value |
| `GetSleFullState()` | Get the complete Nearlink running state, returns `SleStateID` enumeration value |
| `IsNearlinkSupport()` | Detect whether device hardware supports Nearlink protocol |
| `RegisterObserver(const std::shared_ptr<NearlinkHostObserver> &observer)` | Register Nearlink status change observer to monitor status change events |
| `DeregisterObserver(const std::shared_ptr<NearlinkHostObserver> &observer)` | Deregister the registered status change observer |

Auto-reconnection strategy enumeration:
- `AUTO_CONN_GENERAL`: Normal auto-reconnection (default value)
- `AUTO_CONN_EXCEPT_AUDIO_DEVICES`: Do not auto-reconnect audio devices
- `AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES`: Do not auto-reconnect devices actively disconnected by user

**Call Example**
```cpp
#include "nearlink_host.h"
#include <memory>

// Custom status observer, inherits base class and overrides callback methods
class DemoHostObserver : public Nearlink::NearlinkHostObserver {
public:
    void OnStateChanged(int transport, int status) override
    {
        // Custom status change handling logic
        // transport corresponds to transport type, status corresponds to switch state enumeration
    }
};

void NearlinkSwitchDemo()
{
    // Get global singleton
    auto &host = Nearlink::NearlinkHost::GetInstance();

    // Pre-check: detect hardware support
    if (!host.IsNearlinkSupport()) {
        // Device does not support Nearlink, exit with exception
        return;
    }

    // Register status change observer
    auto observer = std::make_shared<DemoHostObserver>();
    host.RegisterObserver(observer);

    // Enable Nearlink, using default auto-reconnection strategy
    NlErrCode ret = host.EnableNl();
    if (ret != NL_NO_ERROR) {
        // Enable failed, handle exception based on error code
        return;
    }

    // Query current switch state
    bool isEnabled = host.IsSleEnabled();

    // Disable Nearlink
    ret = host.DisableNl();
    if (ret != NL_NO_ERROR) {
        // Disable failed handling
    }

    // Deregister observer, release monitoring resources
    host.DeregisterObserver(observer);
}
```
#### Nearlink Advertising

Provides Nearlink advertising start and stop capabilities, supporting custom advertising parameters, advertising data, and scan response data. The core interfaces are defined in the `nearlink_sle_advertiser.h` header file.

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `SleAdvertiser::CreateSleAdvertiser()` | Create advertising instance, factory method |
| `StartAdvertising(settings, advData, scanResponse, duration, callback)` | Start advertising; `settings` is advertising parameters, `advData` is advertising data, `scanResponse` is scan response data, `duration` is advertising duration (unit: 10ms, 0 means continuous advertising), `callback` is result callback |
| `StopAdvertising(callback)` | Stop advertising (matched by callback object) |
| `StopAdvertising(advHandle)` | Stop advertising (by advertising handle) |
| `EnableAdvertising(advHandle)` | Enable specified advertising set |
| `DisableAdvertising(advHandle)` | Disable specified advertising set |
| `SetAdvertisingData(advData, scanResponse, callback)` | Dynamically update advertising data and scan response data |
| `GetAdvHandle(callback, advHandle)` | Get advertising handle |

Advertising parameter class `SleAdvertiserSettings`:

| Interface | Function Description |
|-----------|----------------------|
| `SetConnectable(bool)` | Set whether connectable |
| `SetInterval(uint32_t)` | Set advertising interval |
| `SetTxPower(uint8_t)` | Set transmit power |
| `SetPrimaryPhy(int)` / `SetSecondaryPhy(int)` | Set primary/secondary PHY type |
| `SetLegacyMode(bool)` | Set whether in compatibility mode |

Advertising data class `SleAdvertiserData`:

| Interface | Function Description |
|-----------|----------------------|
| `AddServiceData(uuid, data)` | Add service data |
| `AddManufacturerData(id, data)` | Add manufacturer custom data |
| `AddServiceUuid(uuid)` | Add service UUID |
| `SetIncludeDeviceName(bool)` | Set whether to include device name in advertising |

Callback interface `SleAdvertiseCallback` (needs to be inherited and overridden):

| Interface | Function Description |
|-----------|----------------------|
| `OnStartResultEvent(result, advHandle)` | Start advertising result callback (must be overridden) |
| `OnStopResultEvent(result, advHandle)` | Stop advertising result callback |
| `OnSetAdvDataEvent(result)` | Update advertising data result callback (must be overridden) |

**Call Example**

```cpp
#include "nearlink_sle_advertiser.h"
#include "nearlink_uuid.h"
#include <memory>

using namespace OHOS::Nearlink;

// Custom advertising callback, inherits base class and overrides callback methods
class DemoAdvertiseCallback : public SleAdvertiseCallback {
public:
    void OnStartResultEvent(int result, int advHandle) override
    {
        if (result == 0) {
            // Advertising started successfully, advHandle is the advertising handle
        } else {
            // Advertising start failed, handle exception based on error code
        }
    }

    void OnStopResultEvent(int result, int advHandle) override
    {
        // Advertising stopped
    }

    void OnSetAdvDataEvent(int result) override
    {
        // Advertising data update result
    }
};

void NearlinkAdvertiseDemo()
{
    // Step 1: Create advertising instance
    auto advertiser = SleAdvertiser::CreateSleAdvertiser();

    // Step 2: Configure advertising parameters
    SleAdvertiserSettings settings;
    settings.SetConnectable(true);       // Set as connectable advertising
    settings.SetInterval(0x1388);        // Set advertising interval (5000 * 0.625ms)
    settings.SetTxPower(SLE_ADV_TX_POWER_MEDIUM);  // Set transmit power

    // Step 3: Construct advertising data
    SleAdvertiserData advData;
    UUID serviceUuid;
    serviceUuid.FromString("37bea880-fc70-11ea-b720-000000001234");
    advData.AddServiceUuid(serviceUuid);
    advData.AddServiceData(serviceUuid, "hello");
    advData.AddManufacturerData(0x027d, "vendor_data");
    advData.SetIncludeDeviceName(true);

    // Step 4: Construct scan response data
    SleAdvertiserData scanResponse;
    scanResponse.SetIncludeDeviceName(true);

    // Step 5: Create callback and start advertising
    auto callback = std::make_shared<DemoAdvertiseCallback>();
    uint16_t duration = 0;  // 0 means continuous advertising
    NlErrCode ret = advertiser->StartAdvertising(settings, advData, scanResponse, duration, callback);
    if (ret != NL_NO_ERROR) {
        // Start failed, handle exception based on error code
        return;
    }

    // Step 6: Stop advertising
    ret = advertiser->StopAdvertising(callback);
    if (ret != NL_NO_ERROR) {
        // Stop failed handling
    }
}
```

**Notes**

- `SleAdvertiser` is created through the factory method `CreateSleAdvertiser()`, and its lifecycle is managed by the caller.
- The `duration` parameter unit is 10ms; setting to 0 means continuous advertising until actively stopped.
- Complete error code definitions can be found in the `nearlink_errorcode.h` header file.

#### Nearlink Scanning

Provides Nearlink device scanning and discovery capabilities, supporting full scan and scan with filter conditions. The core interfaces are defined in the `nearlink_sle_scanner.h` header file.

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `SleCentralManager::CreateSleCentralManager(callback)` | Create scan manager instance, factory method |
| `StartScan()` | Start scan with default parameters |
| `StartFullScan(settings)` | Start full scan, scanning all visible devices |
| `StartScanWithFilter(settings, filters)` | Start scan with filter conditions, only reporting matched devices |
| `StopScan()` | Stop scanning |

Scan parameter class `SleScanSettings`:

| Interface | Function Description |
|-----------|----------------------|
| `SetScanMode(int)` | Set scan mode (`SCAN_MODE` enumeration) |
| `SetDuration(int)` | Set scan duration (10-60s) |
| `SetReportDelay(long)` | Set batch report delay time |
| `SetFrameType(uint8_t)` | Set frame type |

Filter class `SleScanFilter`:

| Interface | Function Description |
|-----------|----------------------|
| `SetDeviceId(string)` | Filter by device address |
| `SetName(string)` | Filter by device name |
| `SetServiceUuid(UUID)` | Filter by service UUID |
| `SetManufacturerId(uint16_t)` | Filter by manufacturer ID |
| `SetRssiThreshold(int8_t)` | Filter by RSSI threshold |

Result class `SleScanResult`:

| Interface | Function Description |
|-----------|----------------------|
| `GetPeripheralDevice()` | Get remote device information |
| `GetRssi()` | Get signal strength |
| `GetServiceUuids()` | Get service UUID list |
| `GetManufacturerData()` | Get manufacturer data |
| `GetServiceData()` | Get service data |
| `IsConnectable()` | Determine whether device is connectable |

Callback interface `SleCentralManagerCallback` (needs to be inherited and overridden):

| Interface | Function Description |
|-----------|----------------------|
| `OnScanCallback(result)` | Single scan result callback (must be overridden) |
| `OnStartOrStopScanEvent(resultCode, isStartScan)` | Start/stop scan result callback |

**Call Example**

```cpp
#include "nearlink_sle_scanner.h"
#include "nearlink_uuid.h"
#include <memory>

using namespace OHOS::Nearlink;

// Custom scan callback, inherits base class and overrides callback methods
class DemoScanCallback : public SleCentralManagerCallback {
public:
    void OnScanCallback(const SleScanResult &result) override
    {
        // Get scanned device information
        auto device = result.GetPeripheralDevice();
        int8_t rssi = result.GetRssi();
        bool connectable = result.IsConnectable();
        
        // Handle scan results
    }

    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override
    {
        if (isStartScan) {
            if (resultCode == 0) {
                // Scan started successfully
            } else {
                // Scan start failed
            }
        } else {
            // Scan stopped
        }
    }
};

void NearlinkScanDemo()
{
    // Step 1: Create scan callback
    auto callback = std::make_shared<DemoScanCallback>();

    // Step 2: Create scan manager
    auto scanner = SleCentralManager::CreateSleCentralManager(callback);

    // Step 3: Configure scan parameters
    SleScanSettings settings;
    settings.SetScanMode(SCAN_MODE_LOW_LATENCY);  // Low latency scan mode
    settings.SetDuration(30);                      // Scan duration 30 seconds

    // Step 4 (Optional): Configure filter conditions
    std::vector<SleScanFilter> filters;
    SleScanFilter filter;
    UUID serviceUuid;
    serviceUuid.FromString("37bea880-fc70-11ea-b720-000000001234");
    filter.SetServiceUuid(serviceUuid);            // Only scan devices containing specified service UUID
    filter.SetRssiThreshold(-70);                  // Only scan devices with signal strength greater than -70dBm
    filters.push_back(filter);

    // Step 5: Start scanning (Method 1: Full scan)
    NlErrCode ret = scanner->StartFullScan(settings);
    
    // Or start scanning (Method 2: Scan with filter conditions)
    // NlErrCode ret = scanner->StartScanWithFilter(settings, filters);
    
    if (ret != NL_NO_ERROR) {
        // Start scan failed, handle exception based on error code
        return;
    }

    // Step 6: Stop scanning
    ret = scanner->StopScan();
    if (ret != NL_NO_ERROR) {
        // Stop scan failed handling
    }
}
```

**Notes**

- `SleCentralManager` is created through the factory method `CreateSleCentralManager()`, and its lifecycle is managed by the caller.
- `SCAN_MODE` enumeration includes: `SCAN_MODE_LOW_POWER` (low power), `SCAN_MODE_BALANCED` (balanced mode), `SCAN_MODE_LOW_LATENCY` (low latency), etc.
- Filter conditions support multiple `SleScanFilter`, with OR relationship between multiple filter conditions.
- Complete error code definitions can be found in the `nearlink_errorcode.h` header file.

### Application-side Usage Guide

The following are the standard usage patterns for Nearlink core modules on the application side (ArkTS). Through the ArkTS interfaces provided by [@kit.ConnectivityKit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md), application developers can quickly integrate Nearlink functions to implement core capabilities such as device discovery, connection management, and data transmission.

> **Important Note:** Before using Nearlink ArkTS capabilities, you must first query whether the current device supports Nearlink through the `manager.isNearLinkSupported()` interface. If not supported, subsequent Nearlink-related interfaces should not be called.

#### Nearlink Capability Query

Provides device Nearlink capability support query, used to confirm whether the current device has Nearlink hardware and system support before calling any Nearlink interfaces. Provided through the `manager` module in [@kit.ConnectivityKit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md).

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `manager.isNearLinkSupported()` | Query whether the current device supports Nearlink capability, returns boolean value. `true` means supported, `false` means not supported |

**Call Example**

```typescript
import { manager } from '@kit.ConnectivityKit';

// Query whether current device supports Nearlink
let isSupported: boolean = manager.isNearLinkSupported();
if (isSupported) {
  // Device supports Nearlink, can continue calling Nearlink-related interfaces
  console.info('Current device supports Nearlink');
} else {
  // Device does not support Nearlink, should not call any Nearlink-related interfaces
  console.info('Current device does not support Nearlink');
}
```

**Notes**

- This interface is a synchronous call and does not require permission application.
- The underlying implementation determines whether the device has Nearlink capability through the system property `const.nearlink.enable`.
- In application business code, this interface should be called first for pre-validation, otherwise calling other interfaces on devices that do not support Nearlink may result in error codes or other exceptions.

#### Nearlink Switch

Provides Nearlink switch start/stop control, status query, and status change monitoring capabilities, provided through the `manager` module in [@kit.ConnectivityKit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md).

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `manager.getState()` | Get current Nearlink switch state, returns `manager.NearlinkState` enumeration value |
| `manager.enableNearlink()` | Enable Nearlink switch |
| `manager.disableNearlink()` | Disable Nearlink switch |
| `manager.on('stateChange', callback)` | Subscribe to Nearlink switch state change events |
| `manager.off('stateChange', callback)` | Unsubscribe from Nearlink switch state change events |

Status enumeration `manager.NearlinkState`:
| Enumeration Value | Function Description |
|-------------------|----------------------|
| `STATE_ON` | Nearlink switch is on |
| `STATE_OFF` | Nearlink switch is off |
| `STATE_TURNING_ON` | Nearlink switch is turning on |
| `STATE_TURNING_OFF` | Nearlink switch is turning off |

**Call Example**

```typescript
import { manager } from '@kit.ConnectivityKit';
import { BusinessError } from '@kit.BasicServicesKit';

// Step 1: Query current Nearlink switch state
try {
  let state: manager.NearlinkState = manager.getState();
  console.info('Current Nearlink state: ' + JSON.stringify(state));
} catch (err) {
  console.error('Query failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 2: Subscribe to Nearlink switch state changes
let onStateChange: (data: manager.NearlinkState) => void = (data: manager.NearlinkState) => {
  console.info('Nearlink state changed: ' + JSON.stringify(data));
  // Implement business logic based on state changes
};
try {
  manager.on('stateChange', onStateChange);
} catch (err) {
  console.error('Subscription failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 3: Enable Nearlink switch
try {
  manager.enableNearlink();
} catch (err) {
  console.error('Enable failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 4: Disable Nearlink switch
try {
  manager.disableNearlink();
} catch (err) {
  console.error('Disable failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 5: Unsubscribe from Nearlink switch state changes
try {
  manager.off('stateChange', onStateChange);
} catch (err) {
  console.error('Unsubscribe failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}
```

#### Nearlink Advertising

Provides Nearlink advertising start and stop capabilities, supporting custom advertising parameters and advertising data, provided through the `advertising` module in [@kit.ConnectivityKit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md).

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `advertising.startAdvertising(params)` | Start advertising; `params` is advertising parameter configuration, returns advertising index `advertisingId` |
| `advertising.stopAdvertising(advertisingId)` | Stop advertising; `advertisingId` is the index returned when starting advertising |
| `advertising.on('advertisingStateChange', callback)` | Subscribe to advertising state change events, triggering callback when advertising state changes |
| `advertising.off('advertisingStateChange', callback)` | Unsubscribe from advertising state change events |

Advertising parameters `advertising.AdvertisingSettings`:
| Field | Function Description |
|-------|----------------------|
| `interval` | Advertising interval |
| `power` | Transmit power, value from `advertising.TxPowerMode` enumeration |

Transmit power `advertising.TxPowerMode`:
| Enumeration Value | Function Description |
|-------------------|----------------------|
| `ADV_TX_POWER_ULTRA_LOW` | Ultra low power |
| `ADV_TX_POWER_LOW` | Low power |
| `ADV_TX_POWER_MEDIUM` | Medium power |
| `ADV_TX_POWER_HIGH` | High power |

Advertising data `advertising.AdvertisingData`:
| Field | Function Description |
|-------|----------------------|
| `serviceUuids` | Service UUID list |
| `manufacturerData` | Manufacturer custom data list |
| `serviceData` | Service data list |

**Call Example**

```typescript
import { advertising } from '@kit.ConnectivityKit';
import { BusinessError } from '@kit.BasicServicesKit';

// Step 1: Subscribe to advertising state change events
let onAdvStateChange: (data: advertising.AdvertisingStateChangeInfo) => void =
  (data: advertising.AdvertisingStateChangeInfo) => {
    console.info('advertisingId: ' + data.advertisingId);
    console.info('advertisingState: ' + data.state);
    // Implement business logic based on advertising state
  };
try {
  advertising.on('advertisingStateChange', onAdvStateChange);
} catch (err) {
  console.error('Subscription failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 2: Construct advertising parameters
let setting: advertising.AdvertisingSettings = {
  interval: 5000,
  power: advertising.TxPowerMode.ADV_TX_POWER_LOW
};

// Step 3: Construct advertising data
let manufactureValueBuffer = new Uint8Array(4);
manufactureValueBuffer[0] = 1;
manufactureValueBuffer[1] = 2;
manufactureValueBuffer[2] = 3;
manufactureValueBuffer[3] = 4;
let manufactureDataUnit: advertising.ManufacturerData = {
  manufacturerId: 4567,
  manufacturerData: manufactureValueBuffer.buffer
};
let serviceValueBuffer = new Uint8Array(4);
serviceValueBuffer[0] = 4;
serviceValueBuffer[1] = 6;
serviceValueBuffer[2] = 7;
serviceValueBuffer[3] = 8;
let serviceDataUnit: advertising.ServiceData = {
  serviceUuid: '37bea880-fc70-11ea-b720-000000001234',
  serviceData: serviceValueBuffer.buffer
};
let advData: advertising.AdvertisingData = {
  serviceUuids: ['37bea880-fc70-11ea-b720-000000001234'],
  manufacturerData: [manufactureDataUnit],
  serviceData: [serviceDataUnit]
};
let advertisingParams: advertising.AdvertisingParams = {
  advertisingSettings: setting,
  advertisingData: advData
};

// Step 4: Start advertising
let advId = -1;
try {
  advertising.startAdvertising(advertisingParams).then((advertisingId: number) => {
    advId = advertisingId;
    console.info('Start advertising succeeded, advertisingId: ' + advId);
  }).catch((err: BusinessError) => {
    console.error('Start advertising failed, errCode: ' + err.code + ', errMessage: ' + err.message);
  });
} catch (err) {
  console.error('Start advertising exception, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 5: Stop advertising
try {
  advertising.stopAdvertising(advId).then(() => {
    console.info('Stop advertising succeeded');
  }).catch((err: BusinessError) => {
    console.error('Stop advertising failed, errCode: ' + err.code + ', errMessage: ' + err.message);
  });
} catch (err) {
  console.error('Stop advertising exception, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 6: Unsubscribe from advertising state change events
try {
  advertising.off('advertisingStateChange', onAdvStateChange);
} catch (err) {
  console.error('Unsubscribe failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}
```

#### Nearlink Scanning

Provides Nearlink device scanning and discovery capabilities, supporting full scan and scan with filter conditions, provided through the `scan` module in [@kit.ConnectivityKit](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/connectivity/Readme-CN.md).

**Interface Description**

| Interface | Function Description |
|-----------|----------------------|
| `scan.startScan(filters, options)` | Start scanning; `filters` is scan filter condition list (empty for full scan), `options` is scan parameter configuration |
| `scan.stopScan()` | Stop scanning |
| `scan.on('deviceFound', callback)` | Subscribe to scan result events, triggering callback when peripheral device is scanned |
| `scan.off('deviceFound', callback)` | Unsubscribe from scan result events |

Scan mode `scan.ScanMode`:
| Enumeration Value | Function Description |
|-------------------|----------------------|
| `SCAN_MODE_LOW_POWER` | Low power mode |
| `SCAN_MODE_BALANCED` | Balanced mode |
| `SCAN_MODE_LOW_LATENCY` | Low latency mode |

Scan result `scan.ScanResults`:
| Field | Function Description |
|-------|----------------------|
| `address` | Remote device address |
| `deviceName` | Remote device name |
| `rssi` | Signal strength |

**Call Example**

```typescript
import { scan } from '@kit.ConnectivityKit';
import { BusinessError } from '@kit.BasicServicesKit';

// Step 1: Define scan result callback
let onDeviceFound: (data: Array<scan.ScanResults>) => void = (data: Array<scan.ScanResults>) => {
  console.info('Device scanned, addr: ' + data[0].address + ', name: ' + data[0].deviceName);
  // Implement business logic based on scan results
};

// Step 2: Subscribe to scan results
try {
  scan.on('deviceFound', onDeviceFound);
} catch (err) {
  console.error('Subscription failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 3: Configure scan parameters
let scanOptions: scan.ScanOptions = {
  scanMode: scan.ScanMode.SCAN_MODE_LOW_POWER
};

// Step 4: Start full scan
try {
  scan.startScan([], scanOptions).then(() => {
    console.info('Start scan succeeded');
  }).catch((err: BusinessError) => {
    console.error('Start scan failed, errCode: ' + err.code + ', errMessage: ' + err.message);
  });
} catch (err) {
  console.error('Start scan exception, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 5: Stop scanning
try {
  scan.stopScan().then(() => {
    console.info('Stop scan succeeded');
  }).catch((err: BusinessError) => {
    console.error('Stop scan failed, errCode: ' + err.code + ', errMessage: ' + err.message);
  });
} catch (err) {
  console.error('Stop scan exception, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}

// Step 6: Unsubscribe from scan results
try {
  scan.off('deviceFound', onDeviceFound);
} catch (err) {
  console.error('Unsubscribe failed, errCode: ' + (err as BusinessError).code + ', errMessage: ' + (err as BusinessError).message);
}
```

## Repositories Involved

[Nearlink Service](https://gitcode.com/openharmony-sig/communication_nearlink)

[Nearlink Driver Service](https://gitcode.com/openharmony/drivers_peripheral)