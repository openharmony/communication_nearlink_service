# Nearlink Service

## Introduction

The Nearlink service component provides short-range wireless communication services in the OpenHarmony system, offering interfaces for devices to access and use the Nearlink wireless communication protocol, including SSAP service discovery and various other services.

* Nearlink: A high-performance short-range wireless communication protocol.

### System Architecture

![Nearlink Architecture Diagram](figure/nearlink-architecture.png)

#### Module Description

The overall architecture is divided into the application framework layer, Nearlink system service layer, and Nearlink driver layer.

- **Application Framework Layer**
  * Provides JS interfaces for hap applications to call, and inner interfaces for other SAs to call.

- **System Service Layer**
  * **IPC Layer (ipc)**: Cross-process communication.
  * **Server Layer (services/server)**: Responsible for message dispatch.
  * **Service Layer (services/service)**: Nearlink service layer.
  * **Stack Layer (services/stack)**: Protocol stack underlying implementation.
  * **Hardware Layer (services/hardware)**: DLI adapter, communicates with the chip.

## Directory Structure

```
/foundation/communication/nearlink
├── interfaces                               # External API files provided by the module
├── frameworks                               # Framework layer code directory
├── sa_profile                               # Nearlink service definition directory
├── services                                 # Nearlink service code directory
│    ├── ipc                                 # IPC
│    ├── server                              # Message dispatch layer
│    ├── service                             # Nearlink service layer
│    ├── stack                               # Protocol stack layer
│    ├── hardware                            # DLI adapter
├── test                                     # Test
│    ├── unittest                            # Unit test
│    └── fuzztest                            # Fuzz test
└── LICENSE                                  # License declaration file
└── bundle.json                              # Component description file
```

### Constraints

    * **Chip Requirement**: Must support Nearlink functionality
    * **Product Configuration Item**: const.nearlink.enable=1
    * **JS Interface Usage**: Before calling Nearlink JS interfaces, the hap application must ensure that manager.isNearLinkSupported() returns true, and the Nearlink switch state obtained via manager.getState() is STATE_ON
    * **Inner Interface Usage**: Before other SAs call Nearlink inner interfaces, ensure that the Nearlink component can be compiled as a dependency and that the value of bool state = Nearlink::NearlinkHost::GetInstance().IsSleEnabled(); is true

## Build

**JS-side Dependency**

``` TypeScript
// Import the corresponding functional module if you need to use JS interfaces.
import { manager, scan, advertising, ssap, nearlinkConstant } from '@kit.ConnectivityKit';
```

**Native-side Build Dependency**

``` GN
external_deps = [
  "nearlink_service:nearlink_framework",
]
```

**Nearlink Module Build**

Use the following commands to build for different target platforms:

- Full build

    Build command after modifying build.gn file
    ```
    $ ./build.sh --product-name {product_name} --ccache
    ```
    Build command without modifying build.gn file
    ```
    $ ./build.sh --product-name {product_name} --ccache --fast-rebuild
    ```

- Individual build

    ```
    $ ./build.sh --product-name {product_name} --ccache --build-target nearlink_service
    ```

> **Note:**
> {product_name} is the name of the currently supported platform.

## Usage

Developers can use the following methods to determine whether the system supports the Nearlink service:

1. Developers call `canIUse(SystemCapability.Communication.Nearlink.Core)` to check whether the OpenHarmony device supports this SysCap.
2. If the above SysCap is supported, developers can call the `isSleEnable` API to check whether the Nearlink service is currently supported.

### API Description

The Nearlink service component provides SLE-related APIs and various profile service APIs. The main APIs and their functions are as follows:

**Table 1** API Description

| API Name                     | Description                                                         |
| ---------------------------- | ------------------------------------------------------------------- |
| **isNearLinkSupported**      | Check whether the local device supports the Nearlink service |
| **getState**                 | Get the Nearlink switch state |
| **startAdvertising**         | Start Nearlink advertising |
| **stopAdvertising**          | Stop Nearlink advertising |
| **startScan**                | Start Nearlink scanning |
| **stopScan**                 | Stop Nearlink scanning |
| **createServer**             | Create an SSAP server instance |
| **addService**               | Add a service on the server side |
| **sendResponse**             | Respond to client read/write requests |
| **notifyPropertyChanged**    | Notify the client of property value updates |
| **createClient**             | Create an SSAP client instance |
| **connect**                  | Initiate a connection to the server |
| **getServices**              | Get the list of services supported by the server |
| **addService**               | Add a service on the server side |
| **readProperty**             | Read a server property |
| **writeProperty**            | Write a server property |
| **setPropertyNotification**  | Enable/disable notifications for property changes |
| **createPort**               | Register a port service |
| **writeData**                | Send data to a remote device by address and UUID |


### Development Steps

When implementing a Nearlink application, developers need to create a new EntryAbility in a DevEco Studio project. The specific steps are as follows:
The following demonstrates a complete workflow for implementing a peripheral lifecycle management application using the JS API.

1. **Implement Nearlink**: The base class for Nearlink application capabilities, providing device discovery and offline notification functions, which need to be inherited and implemented by the application.
2. **Register Device**: Register a partner device. The lifecycle of the registered device can be sensed by PartnerAgentExtensionAbility.
3. **Query Device Registration Status**: Determine whether the current application has already registered the device.
4. **Unregister Device**: After calling this API to unregister, the application's PartnerAgentExtensionAbility process will no longer receive discovery and offline status notifications for this device.

### File Directory Structure

1. In the ets directory corresponding to the project Module, right-click and select "New > Directory" to create a new directory named entryability.

2. In the entryability directory, right-click and select "New > File" to create two files: EntryAbility.ets and PartnerAgentAbility.ets. The directory structure is as follows:

   ``` TypeScript
   ├──entry/src/main/ets                           // Code area
      ├──entryability
      │  └──EntryAbility.ets                       // Application entry class
      └──pages
         ├──Index.ets                              // Page navigation
         ├──MainPage.ets                           // Application home page
         └──ScanConfigPage.ets                     // Scan configuration page
         └──SsapPage.ets                           // SSAP page
         └──SsapClientPage.ets                     // SSAP client page
         └──SsapServerPage.ets                     // SSAP server page
   └──entry/src/main/resources                     // Application resource directory
   ```



#### Complete Example

1. EntryAbility.ets file.

    EntryAbility loads the page drawn by ets/pages/Index.dts.

  ``` TypeScript
  import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
  import { hilog } from '@kit.PerformanceAnalysisKit';
  import { window } from '@kit.ArkUI';

  export default class EntryAbility extends UIAbility {
    onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
    }

    onDestroy(): void {
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    }

    onWindowStageCreate(windowStage: window.WindowStage): void {
      // Main window is created, set main page for this ability
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

      windowStage.loadContent('pages/Index', (err) => {
        if (err.code) {
          hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
          return;
        }
        hilog.info(0x0000, 'testTag', 'Succeeded in loading the content.');
      });
    }

    onWindowStageDestroy(): void {
      // Main window is destroyed, release UI related resources
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
    }

    onForeground(): void {
      // Ability has brought to foreground
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
    }

    onBackground(): void {
      // Ability has back to background
      hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
    }
  }
  ```

2. Index.ets file
    Application navigation entry page. Index.ets builds page navigation through the Navigation component, configures navigation mapping for MainPage, ScanConfigPage, and SsapPage target pages, automatically jumps to the MainPage home page on page load, and requests the user to grant the ohos.permission.ACCESS_NEARLINK Nearlink access permission.

  ``` TypeScript
  import { hilog } from '@kit.PerformanceAnalysisKit';
  import { common, abilityAccessCtrl } from '@kit.AbilityKit';
  import { BusinessError } from '@kit.BasicServicesKit';
  import { MainPage } from './MainPage';
  import { ScanConfigPage } from './ScanConfigPage';
  import { SsapPage } from './SsapPage';

  @Entry
  @Component
  struct Index {
    logTag: string = 'Index';
    domainId: number = 0x0000;
    @Provide('pageInfos') pageInfos: NavPathStack = new NavPathStack();

    aboutToAppear() {
      hilog.info(this.domainId, this.logTag, `${this.logTag} aboutToAppear`);
    }

    @Builder
    PageMap(name: string, params?: Record<string, Object>) {
      if (name === 'ScanConfigPage') {
        ScanConfigPage({
        })
      } else if (name === 'SsapPage') {
        SsapPage({
        })
      } else {
        MainPage({
        })
      }
    }

    build() {
      Navigation(this.pageInfos) {
      }
      .onAppear(() => {
        this.pageInfos.pushPathByName('MainPage', null, true);
        let atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
        try {
          let context = this.getUIContext().getHostContext() as common.UIAbilityContext;
          // ohos.permission.ACCESS_NEARLINK is needed in this example.
          // This HAP will ask user for permission when it's opened in the first time.
          atManager.requestPermissionsFromUser(context, ['ohos.permission.ACCESS_NEARLINK']);
        } catch (err) {
          hilog.error(this.domainId, this.logTag,
            `errCode: ${(err as BusinessError).code}, errMessage: ${(err as BusinessError).message}`);
        }
      })
      .titleMode(NavigationTitleMode.Free)
      .hideTitleBar(true)
      .hideToolBar(true)
      .mode(NavigationMode.Stack)
      .navDestination(this.PageMap)
    }
  }
   ```

5. main_pages.json file.

   Corresponds to the page for drawing Nearlink service function buttons under the ets/pages/ path.

  ``` JSON
  {
    "src": [
      "pages/Index"
    ]
  }
  ```

6. Register EntryAbility in the module.json5 configuration file corresponding to the project Module. The type label needs to be set to "entry", and the srcEntry label indicates the code path corresponding to the current EntryAbility component.
  ``` JSON5
  "extensionAbilities": [
      {
        "name": "EntryBackupAbility",
        "srcEntry": "./ets/entrybackupability/EntryBackupAbility.ets",
        "type": "backup",
        "exported": false,
        "metadata": [
          {
            "name": "ohos.extension.backup",
            "resource": "$profile:backup_config"
          }
        ],
      }
    ],
    "requestPermissions": [
      {
        "name": "ohos.permission.ACCESS_NEARLINK",
        "reason": "$string:permission_reason_nearlink",
        "usedScene": {
          "abilities": [
            "EntryFormAbility"
          ],
          "when": "inuse",
        },
      },
    ]
  ```



### Constraints

* **System Requirements**
    * Component configuration: The `nearlink` component has been added to the build in productdefine_common/inherit/rich.json.
  ``` JSON
    {
      "subsystem": "communication",
      "components": [
        {
          "component": "..."
        },
        {
          "component": "nearlink",
          "features": [
            "nearlink_feature = true"
          ]
        },
        {
          "component": "..."
        }
      ]
    }
  ```
  If the OpenHarmony device does not need to support the Nearlink service, this configuration item can be removed.

    ``` JSON
    {
      "subsystem": "communication",
      "components": [
        {
          "component": "..."
        },
        {
          "component": "..."
        }
      ]
    }
  ```

    * The OpenHarmony development device must support Nearlink functionality, with the CCM configuration item const.nearlink.enable=1, and needs to use the Nearlink Kit interface @kit.NearLinkKit.

* **Permission Description**
    * Applications need to request Nearlink permissions (ohos.permission.ACCESS_NEARLINK).
    * Currently supported permissions include ohos.permission.ACCESS_NEARLINK, ohos.permission.GET_NEARLINK_LOCAL_MAC, ohos.permission.MANAGE_NEARLINK, and ohos.permission.GET_NEARLINK_PEER_MAC, where ohos.permission.GET_NEARLINK_PEER_MAC returns the real address rather than a random address.

## Repositories Involved

[Nearlink Service](https://gitcode.com/openharmony-sig/communication_nearlink)
