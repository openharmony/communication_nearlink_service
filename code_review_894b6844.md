## 检视结论

审查范围：`6c966ae0319896724f4bfbbeeeea8a280df2d2f5..894b6844be858bb343ec5dbe033beaf46985a7b6`，共 32 个修改文件（21 个生产文件、11 个测试/桩/构建文件）。本报告基于完整 diff、当前调用链、ProfileManager 生命周期、GN target、相关 mock 及关联未修改文件的静态审查。

### 1. 循环依赖是否真正消除
结论: 有问题

理由: 本提交确实删除了多条具体实现 include 边，例如 ASC 到 `TwsService.h`/`CcpService.h`、CCP 到 `ASCService.h`/`TwsService.h`、CDSM/电量/ICCE/PORT 到 `SleRemoteDeviceAdapter.h`。ASC、TWS、CCP 的新调用通过同一个 `SleInterfaceProfileManager` 取服务，局部 include 依赖有所下降。但是循环依赖没有在模块层真正消除，且当前单一 GN target 不能证明未来拆分后的链接解耦：

- `IRemoteDeviceQuery::GetInstance()` 的唯一生产定义仍位于 `SleRemoteDeviceAdapter.cpp:77-80`，并硬编码返回具体 adapter 单例。CDSM、DeviceBatteryManager、ICCE、PORT 虽不再看到具体头，仍保留对 adapter provider 符号的耦合；模块拆 target 后仍需显式链接 adapter。
- `DeviceBatteryManager.h:22` 依赖 TWS 私有 `TwsDefines.h`，而 `TwsHiBoxParser.cpp:30` 反向依赖 `DeviceBatteryManager.h`，common 与 TWS 的既有模块环未被本提交消除。
- `ASCService.cpp:31` 仍依赖 TWS 私有定义，`TwsHiBoxParser.cpp:27` 仍直接依赖 `ASCService.h`，ASC 与 TWS 的既有具体模块环未清理完。
- adapter 仍调用 ProfileTws/ProfileCdsm，而 CDSM、电量等经新静态工厂反向调用 adapter。当前它们位于同一个 target，不能据此认定存在 GN target 环，但模块级双向符号依赖仍然存在。
- 所有生产模块仍被放在单个 `nearlink_service_impl` target 中，构建图无法证明这些模块可独立编译和链接。
- 仓库没有可用的 CleanArch/循环依赖扫描脚本或本次扫描产物，无法独立复算“17 降到 11”，也无法证明未新增跨模块环。

因此，本提交降低了显式 include 数量，但部分修改只是把 concrete include 替换为静态 service locator/provider 耦合，不能认定循环依赖已在所有层面完成治理。上述部分反向边是父提交既有问题，本次问题是未完成治理，而非新引入这些边。

### 2. 行为等价性
结论: 有问题

理由: 同版本、全量重编译的树内主路径基本等价：原 `ASCService::GetService()`、`TwsService::GetService()`、`CcpService::GetService()` 本身就是对同一个 ProfileManager 的查询；修改后调用目标、NULL 返回、同步/异步边界、错误码和业务副作用未见变化。`IRemoteDeviceQuery::GetInstance()` 当前也恒定返回原 `SleRemoteDeviceAdapter` 单例，没有新增未注册状态。

但是不能整体判定为行为完全等价：

- `ProfileASC` 和 `ProfileTws` 被追加纯虚函数，接口 vtable 和派生实现契约发生变化；这些 Profile 指针还通过独立的 `nearlink_service_impl`/`nearlink_mcp_manager` DSO 边界传递。`CcpService` 的继承/vtable 契约以及 `SleRemoteDeviceAdapter` 的对象布局也发生变化，违反“不改变 ABI/API”的约束。若存在未锁步重编译的旧实现、插件或测试件，可能不兼容；仓库信息不足以确认是否支持这类混合版本。
- `BatteryInfo` 迁移时给 13 个成员增加了默认成员初始化器。当前仓内实例均使用 `{}`，未发现业务值变化；但 `BatteryInfo info;` 的默认初始化语义由不确定值变为全 0，这已经超出单纯移动类型。
- 新增测试多数只验证“不崩溃”，没有证明真实调用目标、参数、返回值、调用次数和副作用等价。

未发现锁步全量构建下明确改变业务结果、错误码、协议数据或消息时序的代码，但已经存在明确 ABI/类型语义变化，故不能按“纯依赖治理且完全行为等价”通过。

### 3. 生命周期与初始化顺序
结论: 需讨论

理由: 本提交没有新增 setter、函数指针注册、回调注册表或可清空全局指针。Profile 查询沿用既有 ProfileManager 生命周期，`IRemoteDeviceQuery` 返回函数局部静态单例，不存在 `SetXXX -> UseXXX -> SetXXX(NULL)` 注册窗口，也没有新增 init/deinit 顺序。

需讨论点是 `IRemoteDeviceQuery.h:27-29` 同时暴露 public virtual destructor 和裸 singleton 指针。原 `SleRemoteDeviceAdapter` 私有析构可阻止调用者删除单例；现在 `delete IRemoteDeviceQuery::GetInstance()` 在接口层可通过编译并对静态对象执行非法释放。应明确非 owning 契约，返回引用或将接口析构限制为 protected。ProfileManager 返回裸指针并在 `Stop()` 中删除对象的既有停服并发窗口未被本提交扩大，但仍需依靠“业务线程停止后再 Stop”的系统约束。

### 4. 线程安全与并发风险
结论: 通过

理由: 本提交没有新增可变 static/global 函数指针、回调指针或注册状态。新虚调用均为即时调用，没有跨异步任务保存接口指针；原有的 CCP、PORT、DeviceAdapter 线程切换和同步等待保持不变。`IRemoteDeviceQuery` 的函数局部静态初始化由 C++ 运行时保证线程安全。

残余风险是 ProfileManager 查询返回裸指针后与 `ProfileServiceManager::Stop()` 并发删除的既有竞态，以及既有 TWS/MCP 动态库卸载和回调并发问题；静态 diff 未显示本提交新增这些窗口。建议在停服/重载测试中继续验证。

### 5. 类型 / 结构体 / 头文件迁移正确性
结论: 有问题

理由: `BatteryInfo` 的字段名称、字段类型、字段顺序、偏移和当前 13 字节布局保持一致，未发现 IPC/RPC/序列化使用，仓内均使用 `{}` 初始化。但存在以下问题：

- 内部共享 DTO `BatteryInfo` 被放入 TWS 私有聚合头 `TwsDefines.h`，使 common/BAS 继续依赖 TWS 以及其 hibox/SDF/stack 头，类型所有权方向不符合共享契约应位于更低层或独立契约层的规则。common 到 `TwsDefines.h` 的 include 在父提交已存在，本提交的类型迁移使其成为实质依赖。
- 增加默认成员初始化器改变了普通默认初始化语义。
- `IRemoteDeviceQuery.h` 使用 `uint8_t` 却未直接包含 `<cstdint>`，公共头不自包含。
- `TwsHiBoxParser.h` 删除 `DeviceBatteryManager.h` 后，`BatteryInfo`、`TwsDeviceDatas` 和双录参数等类型失去原传递定义，仍未包含直接定义头；`TwsMessage` 的自包含缺口在父提交已存在。该头继续依赖 include 顺序。
- `ProfileASC`、`ProfileTws` 追加虚函数以及 concrete 类层次变化属于明确 C++ ABI 变化。

### 6. 构建系统与链接依赖
结论: 有问题

理由: 本提交没有修改生产 BUILD，只把 battery manager UT 加入聚合测试。新增头放在 `services/service/include`，GN 不强制列入 `sources`，当前大一统 `nearlink_service_impl` 中 provider 符号可解析。但存在以下构建/链接风险：

- 新公共头 `IRemoteDeviceQuery.h` 包含 utils 的 `raw_address.h`，而 `nearlink_service_impl` 对 utils 是 private `deps`，公共 config 只暴露 service/include；只依赖 service target 的新消费者可能缺少 utils include path。当前新增消费者都在同一 target，尚未形成已确认的构建失败。
- `utils/BUILD.gn:28-37` 既有配置又反向把 service/include 放入 utils 的 public include path，会掩盖真实依赖方向。
- `IRemoteDeviceQuery::GetInstance()` 位于 adapter 实现文件，PORT/ICCE/CDSM/common target 一旦拆分仍需要 adapter 链接依赖。
- 当前注释掉的 `libnearlink_service_impl.versionscript` 没有 `ProfileCcp` 和 `IRemoteDeviceQuery` 导出项；恢复 symbol hiding 或拆 target 时需要同步验证符号可见性，不能据此认定当前构建已存在 undefined symbol。
- 共用的 `vcp_test/mock_asc_service.cpp` 原本就依赖生产 DSO 补足部分 ASC 虚方法，本次新增三个方法后没有同步补桩，进一步扩大了 mock 对生产 DSO 的依赖和隔离缺口。

当前环境没有 GN、Ninja、C/C++ 编译器和生成的 out 目录，未执行全量/独立编译。`git diff --check 6c966ae 894b684` 已通过。

### 7. Mock / Stub / 测试桩正确性
结论: 有问题

理由: 本次修改的两个 `mock_tws_service.cpp` 已补齐 `GetReportAddr`，签名、引用、返回类型与生产声明一致；`mock_sle_remote_device_adapter.cpp` 也补了 `IRemoteDeviceQuery::GetInstance()`，签名一致。问题如下：

- `ProfileASC` 新增三个纯虚函数后，共用于 VCP/MCP 测试的 `test/.../vcp_test/mock_asc_service.cpp` 未补定义。该桩在父提交已存在其他虚方法由生产 DSO 解析的隔离问题，本次继续增加了三个生产符号依赖，不能视为完整测试桩。
- `IRemoteDeviceQuery` mock 仍固定返回 concrete singleton，不能配置 NULL、非零地址类型、vendor/audio true/false、调用次数或参数，无法验证接口转发。
- TWS mock 的 `GetReportAddr` 固定 identity，未覆盖合作集地址映射和失败路径。
- 没有新增 setter/重复注册/清空路径，因此相关测试项不适用；但接口 alternate implementation 和错误 provider 注册也未覆盖。

### 8. UT / IT / 回归测试质量
结论: 有问题

理由: 新增 UT 不能证明行为等价：

- ASC 四个新增用例没有结果或调用次数断言；fixture 已启动 ProfileManager，也没有建立 TWS/CCP 确实为 NULL 的前置条件。三个 TwsNull 用例还没有建立 `IsVendorDevice() == true`，可能根本不会查询 TWS。
- battery 的 `PublishBatteryLevel_TwsNull_001` 未注册该地址为音频设备；默认 fixture 下 `SleRemoteDeviceManager::IsAudioDevice()` 对未知地址返回 false，因此用例在目标分支前返回，未到达 TWS NULL 分支。
- CCP/CDSM/PORT/TWS 新用例主要断言对象非空、固定返回值或仅 sleep，没有观察接口方法是否被调用及副作用。
- ICCE 地址字节断言有效，但地址类型固定期望 0，没有可配置 fake，无法证明 `GetPeerDeviceAddrType()` 的非零结果被透传。
- ASC 新用例写入 `QosM` 全局状态但 fixture 不清理；多个 fixture 启动 manager 后不 Stop，固定 sleep 也可能造成顺序依赖和 flaky。
- `BatteryInfo` 大小/偏移测试能检查布局，但“迁出 pragma pack”和“新增默认值兼容”的描述不准确：旧类型不在 pack 区域，旧代码中的 `{}` 本来就会零初始化。

---

## 逐文件检视

### 文件: services/service/include/IRemoteDeviceQuery.h
修改类型: 接口抽取 / 公共头文件

结论: 有问题

理由: 抽取了 adapter 的五个能力，方法签名与原 concrete 方法一致，消费方不再包含 `SleRemoteDeviceAdapter.h`。但头文件未包含 `<cstdint>` 就使用 `uint8_t`，并通过静态 `GetInstance()` 把接口和 provider 定位绑定在一起。

风险: 公共头依赖 include 顺序；接口只有编译期解耦，没有解除 provider 符号耦合；public destructor 允许误删静态 singleton。

建议: 补 `<cstdint>`；将契约放入独立低层 target；由 composition root 注入 provider，或提供明确的非 owning 引用访问；避免 public 可删除的 singleton 裸指针。

### 文件: services/service/include/SleInterfaceProfileASC.h
修改类型: 公共接口扩展

结论: 有问题

理由: 把 TWS 所需的三个本端能力查询提升到 `ProfileASC`，调用方向合理，但向既有纯虚接口尾部追加三个槽位，改变接口 ABI 和所有实现类的抽象契约。

风险: 若存在未锁步重编译的旧实现/插件/测试件，新旧接口契约可能不兼容；共用 ASC mock 未补新方法。仓库信息不足以确认是否允许混合版本。

建议: 不修改既有 ABI 接口，新增窄接口（例如本端能力查询接口）并由 manager 单独暴露；若确认只允许锁步全量升级，仍需 ABI 审核、版本策略及全部实现清单。

### 文件: services/service/include/SleInterfaceProfileCcp.h
修改类型: 接口抽取

结论: 需讨论

理由: 只抽取 ASC 实际需要的三个 CCP 方法，接口粒度和依赖方向合理，基类已有 virtual destructor。新增接口本身没有 NULL/注册状态。

风险: `CcpService` 改变继承层次和 vtable；version script 未包含 `ProfileCcp`；manager 仍按字符串返回基类并由调用方 `static_cast`，依赖名称与实际类型严格匹配。

建议: 补导出/ABI 策略和 manager 类型约束；增加通过 `ProfileCcp` 的真实分派测试。

### 文件: services/service/include/SleInterfaceProfileTws.h
修改类型: 公共接口扩展

结论: 有问题

理由: 将现有 `TwsService::GetReportAddr` 提升为接口方法，可解除 DeviceBatteryManager 对 concrete TWS 的调用，但向既有 `ProfileTws` 追加纯虚槽位，改变公共 C++ ABI。

风险: 旧实现、跨 DSO 使用者、预编译 mock 或非锁步版本可能不能满足新 vtable 契约；是否支持该部署方式需确认。

建议: 新建窄的 report-address resolver 接口，避免扩展既有 ABI；执行 cross-DSO CFI 和 ABI diff。

### 文件: services/service/src/adapter/SleRemoteDeviceAdapter.cpp
修改类型: 接口实现 / 静态工厂桥接

结论: 需讨论

理由: `IRemoteDeviceQuery::GetInstance()` 返回原 singleton，五个方法实现、线程切换、等待和副作用均未改变，因此当前调用行为等价。

风险: 工厂定义在 concrete adapter 实现中，所有接口消费者仍依赖 adapter provider 符号；当前同属一个 target，拆分后才会成为显式 target/link 依赖。

建议: 将 provider 注册放到 composition root，接口消费者只依赖契约 target；为 provider 增加可替换 fake，而不是在测试可执行文件中覆盖全局符号。

### 文件: services/service/src/adapter/SleRemoteDeviceAdapter.h
修改类型: 接口实现 / 类层次调整

结论: 有问题

理由: 五个原方法正确标记 `override`，签名一致。但原非多态 concrete 类新增多态基类和 vptr，对象布局及 CFI 类型关系发生变化。

风险: 违反“不改变 ABI”的要求；原 private destructor 的所有权保护被 public base destructor 绕过。

建议: 避免让既有 concrete 类直接改变 ABI，可使用独立 adapter facade 实现窄接口并转发到 singleton；至少补 ABI 基线和所有消费者重编译证明。

### 文件: services/service/src/asc/ASCService.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: TWS/CCP 获取方式与旧 `GetService()` 最终查询同一 manager，原 NULL 返回和业务分支保持：TWS 缺失时左耳/主角色默认 true、角色设置退出、静音返回内部错误；CCP 缺失时跳过通知/恢复。

风险: 仍直接包含 TWS 私有 `TwsDefines.h` 获取角色/媒体枚举，ASC 到 TWS 模块依赖未真正抽成契约；大量裸 `static_cast` 依赖 manager 注册类型正确。

建议: 把角色、nature、media 等契约枚举放入 `SleInterfaceProfileTws.h` 或独立契约头；补真实 NULL 和非 NULL 分派/副作用测试。

### 文件: services/service/src/asc/ASCService.h
修改类型: include 调整 / override 标注

结论: 需讨论

理由: 删除 concrete `TwsService.h` 是正确方向，三个原有方法仅增加 `override`，函数签名和实现未变。

风险: 该改动使 `ASCService` 满足新 vtable，但所有替代实现和 mock 也必须同步；当前共用 mock 未补齐。

建议: 若保留接口扩展，枚举并补齐所有实现/桩；优先改为新增窄接口以避免 ABI 变化。

### 文件: services/service/src/ccp/CcpService.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: ASC/TWS 查询改为 ProfileManager，原 NULL 条件、active device、音频通知和挂断时间戳副作用保持一致。

风险: `SleAudioFrameworkAdapter.h` 仍传递包含 concrete `ASCService.h`，所以 CCP 编译单元仍间接依赖 ASC concrete；新增 `TwsDefines.h` 在本文件无直接符号使用，扩散了 TWS 私有依赖。

建议: 清理未使用 `TwsDefines.h`；让 `SleAudioFrameworkAdapter.h` 自身移除 `ASCService.h`，仅在实现文件依赖所需接口。

### 文件: services/service/src/ccp/CcpService.h
修改类型: 接口实现 / 类层次调整

结论: 有问题

理由: 三个方法正确 override `ProfileCcp`，函数签名和异步执行实现未变。但继承从 `SleInterfaceProfile` 改为 `ProfileCcp`，vtable 契约发生变化。

风险: `CcpService` ABI 变化；外部按旧类布局或 vtable 使用时不兼容。

建议: 确认 CcpService 不作为稳定二进制接口暴露并执行 ABI 检查；否则使用独立 facade/adapter，而不是改变既有 concrete 类层次。

### 文件: services/service/src/ccp/CcpSystemInterface.cpp
修改类型: include 调整 / 接口依赖

结论: 通过

理由: `ASCService::GetService()` 改为同一 manager 的 `ProfileASC` 查询，NULL 时仍返回 false，连接列表语义不变；删除 concrete include 符合分层。

风险: manager 返回裸指针及错误名称注册导致 `static_cast` UB 的风险为既有框架约束。

建议: 增加 ProfileASC 为 NULL、空列表、非空列表三路测试。

### 文件: services/service/src/cdsm/CdsmService.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: 两个调用最终仍进入原 adapter singleton，同步等待、vendor 判断和 controller 下发行为未变。

风险: CDSM 仍依赖 adapter provider 符号，而 adapter 反向依赖 CDSM/ProfileCdsm；当前同属一个 GN target，不构成已确认的 target/link 环。`SleInterfaceProfileTws.h` 仍为未使用 include。

建议: 使用可注入的低层 device-query contract provider；移除未使用 include；增加 vendor true/false 及 controller 调用次数断言。

### 文件: services/service/src/common/DeviceBatteryManager.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: 音频设备判断仍调用原 adapter；TWS 获取仍来自同一 ProfileManager；锁顺序、地址映射、电量缓存、事件上报和 NULL 退出语义保持。

风险: common 仍依赖 adapter provider 符号；当前同属一个 GN target，不构成已确认的 target/link 环。另有 ProfileTws ABI 扩展及新增测试未进入目标路径的问题。

建议: 为音频设备判断和 report 地址 resolver 提供可注入 fake；断言 report 地址、电量缓存和 common event 副作用。

### 文件: services/service/src/common/DeviceBatteryManager.h
修改类型: 类型迁移

结论: 有问题

理由: 删除本地 `BatteryInfo` 定义后继续通过 `TwsDefines.h` 取得类型，字段布局未变，但 common 内部共享头依赖 TWS 私有聚合头。

风险: common/BAS 无法独立于 TWS 裁剪；与 `TwsHiBoxParser.cpp -> DeviceBatteryManager.h` 共同保留模块环；引入 hibox/SDF/stack 传递依赖。

建议: 将 `BatteryInfo` 放到独立、低层、自包含的电量契约头，并让 TWS、BAS、DeviceBatteryManager 单向依赖该契约。

### 文件: services/service/src/icce/icce_utils.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: 地址转换和地址类型查询最终调用原实现，返回结构和默认值不变。

风险: ICCE 仍依赖 adapter provider 符号；当前同属一个 GN target，不构成已确认的 target/link 环。公共接口头的 `<cstdint>` 缺失目前被其他 include 顺序掩盖；UT 只验证 type=0。

建议: 用可配置 fake 验证非零 type 透传，并独立编译该 TU/公共头。

### 文件: services/service/src/port/PortClientStackAdapter.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: 栈地址转换仍调用同一个 `GetPeerDeviceAddrType` 实现，字节序和返回语义未变。

风险: PORT client 仍依赖 adapter provider 符号；当前同属一个 GN target，不构成已确认的 target/link 环。没有非零地址类型测试。

建议: 增加可注入 query fake，验证 type 和地址字节完整透传。

### 文件: services/service/src/port/PortService.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: `Connect()` 异步任务中的首个副作用仍为原 adapter 的 `SetConnDirectActive`，后续连接状态和固定返回 `PORT_SUCCESS` 未变。

风险: 静态接口工厂保留 provider 符号耦合，拆 target 后才需显式链接；新增 UT 只断言异步任务投递前的固定返回值，不能证明副作用发生。

建议: 通过 spy/fake 等待任务完成并校验调用次数、设备地址及后续栈调用，而不是固定 sleep。

### 文件: services/service/src/tws/TwsDefines.h
修改类型: 类型迁移 / include 调整

结论: 有问题

理由: `BatteryInfo` 字段顺序和大小保持，但其默认成员初始化语义发生变化；内部共享电量 DTO 被放进包含 hibox/SDF/stack 类型的 TWS 私有聚合头。新增 `raw_address.h` 在本文件没有直接 `RawAddress` 使用。

风险: 依赖扩散、裁剪困难、类型语义超范围变化；common 到 TWS 的层次倒置持续存在。

建议: 抽取独立 `BatteryInfo` 契约头；保持原始初始化语义或单独说明行为变更；移除无直接用途的 include。

### 文件: services/service/src/tws/TwsHiBoxParser.h
修改类型: include 调整

结论: 有问题

理由: 删除 `DeviceBatteryManager.h` 有助于移除反向头依赖，但没有补齐由该头原先传递提供的 `BatteryInfo`、`TwsDeviceDatas` 和相关双录参数定义；`TwsMessage` 的自包含缺口在父提交已存在。

风险: 单独 include 该头仍依赖前置定义，可能编译失败；当前由 `TwsService.h` 和测试 include 顺序掩盖。具体工具链结果需通过单头编译确认。

建议: 显式包含 `TwsMessage.h`、类型契约头及其他直接依赖；增加 header self-contained 编译测试。

### 文件: services/service/src/tws/TwsService.cpp
修改类型: include 调整 / 接口依赖

结论: 需讨论

理由: 所有 ASC 获取点都改为与旧 `ASCService::GetService()` 等价的 ProfileManager 查询；NULL 时原本也会提前返回，能力写入、音频动作和状态查询语义未变。

风险: `ProfileASC` ABI 被扩展；新增能力 UT 无断言；TWS 模块的 parser 实现仍直接依赖 ASC concrete。

建议: 使用独立能力查询接口；增加 ASC 返回 true/false、ASC NULL、能力索引和值写入的可观察测试。

### 文件: services/service/src/tws/TwsService.h
修改类型: include 调整 / 接口实现

结论: 需讨论

理由: 删除 `DeviceBatteryManager.h` 并将既有 `GetReportAddr` 标记 override，函数实现和签名未变。

风险: `ProfileTws` vtable 扩展；该头的 include 顺序目前掩盖 `TwsHiBoxParser.h` 不自包含问题。

建议: 修复 parser 头自包含；避免扩展既有 ProfileTws ABI，改用窄 resolver 接口。

### 文件: test/unittest/mock/mock_sle_remote_device_adapter.cpp
修改类型: 测试桩

结论: 需讨论

理由: 新增 `IRemoteDeviceQuery::GetInstance()` 签名与生产一致并返回同一 mock singleton，可解决使用该桩的链接目标。

风险: 该桩不可配置，五个新接口路径只能返回固定值/空操作；生产 DSO 与测试可执行文件同时定义工厂时依赖当前动态符号抢占行为。

建议: 改成直接实现 `IRemoteDeviceQuery` 的可配置 fake，记录参数和调用次数；明确每个 test target 的 provider 唯一性。

### 文件: test/unittest/services_test/service_test/BUILD.gn
修改类型: 构建配置 / UT 聚合

结论: 通过

理由: 正确把已有 `battery_manager_test:nl_battery_manager_unittest` 加入总 UT group，没有新增 production `public_deps` 或 include path。

风险: 聚合构建能发现该 target 的编译问题，但不能证明各生产模块独立链接；产品宏矩阵仍需实际构建。

建议: 在 CI 中同时构建聚合 target 和受影响模块/测试的独立 target。

### 文件: test/unittest/services_test/service_test/asc_test/mock_tws_service.cpp
修改类型: 测试桩

结论: 通过

理由: `GetReportAddr(const RawAddress &)` 的签名与生产接口完全一致，identity 返回适合基础 stub。

风险: 不支持合作集地址映射、失败和调用次数验证。

建议: 对涉及 report 地址语义的用例使用可配置返回值和 spy。

### 文件: test/unittest/services_test/service_test/asc_test/nearlink_asc_service_test.cpp
修改类型: UT

结论: 有问题

理由: 四个新增用例均未断言目标结果/调用次数；fixture 在 `SetUpTestCase` 已启动 ProfileManager，没有证明 TWS/CCP 为 NULL；三个 TWS 用例也未建立 vendor=true，可能不查询 TWS。`QosM` 写入后未清理。

风险: 新接口绕过、NULL 分支失效、参数错误或副作用缺失时测试仍通过；shuffle/repeat 可能受全局状态污染。

建议: 显式构造 NULL/non-NULL profile，断言默认返回、CCP start/stop 调用、TWS role/nature 调用及 controller 副作用；TearDown 清理 QosM 和 manager 状态。

### 文件: test/unittest/services_test/service_test/battery_manager_test/device_battery_manager_test.cpp
修改类型: UT / 类型布局验证

结论: 有问题

理由: size/offset 断言可验证当前布局，但 `PublishBatteryLevel_TwsNull_001` 未把设备设置为音频设备；默认设备管理器对该未知地址返回 false，因此不会到达 TWS NULL 分支，也未断言缓存或事件。

风险: 目标路径完全失效时测试仍通过；singleton 状态可能跨用例污染。默认值和 pragma pack 测试描述与 parent 实际语义不一致。

建议: 注入 audio=true query fake，分别测试 TWS NULL、identity、合作集映射；断言电量缓存、当前设备、上报值和事件次数，并清理 singleton 状态。

### 文件: test/unittest/services_test/service_test/ccp_test/ccp_service_test.cpp
修改类型: UT

结论: 有问题

理由: 一个用例只断言环境中的 false，另一个只断言原对象非空，没有观察 `ProfileASC` 的调用、参数或音频通知副作用。

风险: manager 查错 profile、虚调用未发生或参数丢失时仍通过。

建议: 注册可观察 ProfileASC fake，覆盖 NULL、active match/mismatch、NearlinkOut true/false，并断言 action/address。

### 文件: test/unittest/services_test/service_test/cdsm_test/nearlink_cdsm_test.cpp
修改类型: UT

结论: 有问题

理由: 仅覆盖未知设备返回 false和“不崩溃”，没有验证 business type 映射或 controller setter 的调用。

风险: query 固定返回、条件反向、setter no-op 或参数错误都可能漏检。

建议: 用 fake 覆盖 vendor true/false，断言 common device 调用 setter 一次、vendor device 跳过及地址一致。

### 文件: test/unittest/services_test/service_test/icce_test/icce_service_test.cpp
修改类型: UT

结论: 有问题

理由: 地址字节转换断言有效，但 `type` 只验证未知设备默认 0，没有可配置 provider；第二个用例测试的是未修改的反向转换。

风险: 新接口调用被绕过或地址类型写死 0 时仍通过。

建议: 注入返回非零 type 的 fake，并断言调用参数和次数；保留字节序断言。

### 文件: test/unittest/services_test/service_test/nl_cloud_pair_test/mock_tws_service.cpp
修改类型: 测试桩

结论: 通过

理由: 新增 `GetReportAddr` 的签名和返回类型正确，补齐 `ProfileTws` 新纯虚方法。

风险: identity 空实现可能掩盖合作集映射；与公共 ABI 扩展的根本风险无关。

建议: 需要地址映射语义时改用可配置 stub。

### 文件: test/unittest/services_test/service_test/nl_datatransfer_test/nearlink_port_service_test.cpp
修改类型: UT

结论: 有问题

理由: `PortService::Connect` 在异步任务执行前固定返回 `PORT_SUCCESS`，新增用例只断言该返回值并 sleep，没有验证 `SetConnDirectActive` 或后续栈连接。

风险: 新接口完全未调用时测试仍通过；固定 100 ms 等待存在 flaky。

建议: 使用同步点和 spy，断言调用次数、地址、先后顺序及连接状态分支。

### 文件: test/unittest/services_test/service_test/tws_test/nearlink_tws_test.cpp
修改类型: UT

结论: 有问题

理由: `GetReportAddr` 只覆盖 fallback identity；三个能力更新用例没有任何结果或副作用断言。

风险: ASC NULL、返回值错误、能力索引错误或 `SetLocalAbility` 未执行时仍通过。

建议: 注入 ProfileASC fake 和 ability loader spy，覆盖 NULL、true、false、不同 feature index，并验证写入次数和值；补合作集 report 地址映射。

---

## 问题清单

| # | 严重级别 | 文件:行 | 问题描述 | 影响 | 修复建议 |
|---|---------|--------|---------|------|---------|
| 1 | Critical | `services/service/include/SleInterfaceProfileASC.h:191-198`; `services/service/include/SleInterfaceProfileTws.h:160-161`; `services/service/src/ccp/CcpService.h:34`; `services/service/src/adapter/SleRemoteDeviceAdapter.h:38`; `services/service/BUILD.gn:351-409`; `services/service/src/mcp/McpServerServiceManager.h:44,58,74,92`; `services/service/src/mcp/McpServerServiceManagerLoader.cpp:35-45` | 扩展既有纯虚接口并改变 concrete 继承/vtable 契约；`SleRemoteDeviceAdapter` 还新增 vptr，明确改变 C++ ABI，且 Profile 指针通过独立 DSO 的 manager 接口传递。 | 按本次严重级别定义，明确 ABI 变化属于 Critical，并直接违反“不改变 ABI/API”目标；若存在未锁步重编译的旧实现、插件或 mock，则可能不兼容。 | 新增独立窄接口/facade，不修改既有 vtable；执行 ABI diff，并确认是否存在混合版本部署。 |
| 2 | Minor | `services/service/include/IRemoteDeviceQuery.h:19-20,34` | 公共头使用 `uint8_t` 但未包含 `<cstdint>`。 | 依赖 include 顺序，新的独立消费者可能编译失败。 | 显式包含 `<cstdint>`并增加 header self-contained 编译测试。 |
| 3 | Major | `services/service/src/tws/TwsHiBoxParser.h:28-33,52,57,60,104,121-125,132` | 删除反向 include 后未补直接类型依赖，头文件不自包含。 | 独立 include 或 include 顺序变化时编译失败；当前生产/UT 被前置头掩盖。 | 直接包含 `TwsMessage.h`、独立电量契约头和所需定义头。 |
| 4 | Minor | `services/service/src/adapter/SleRemoteDeviceAdapter.cpp:77-80`; `services/service/src/cdsm/CdsmService.cpp:582,591`; `services/service/src/common/DeviceBatteryManager.cpp:45`; `services/service/src/icce/icce_utils.cpp:29`; `services/service/src/port/PortService.cpp:122` | `IRemoteDeviceQuery` 静态工厂硬编码 concrete adapter，只移除了 include 边，没有解除 provider 符号耦合。 | 当前单一 target 可解析，不存在已确认的 GN target/link 环；模块拆分后消费者仍必须显式链接 adapter，无法证明 target 级解耦。 | 在 composition root 注册/注入低层 provider，拆独立 contract target，并验证拆分链接。 |
| 5 | Major | `services/service/src/common/DeviceBatteryManager.h:22`; `services/service/src/tws/TwsDefines.h:402-420`; `services/service/src/tws/TwsHiBoxParser.cpp:30` | 内部共享 `BatteryInfo` 被迁入 TWS 私有聚合头，common↔TWS 的既有模块环未消除。 | common/BAS 依赖 TWS、hibox、SDF 和 stack 细节，产品裁剪及独立 target 受阻。 | 抽取到低层、自包含的 battery contract 头/target。 |
| 6 | Major | `services/service/src/asc/ASCService.cpp:31,128,890,3680,6647`; `services/service/src/tws/TwsHiBoxParser.cpp:27`; `services/service/src/ccp/CcpService.cpp:18`; `services/service/src/audio/SleAudioFrameworkAdapter.h:30` | ASC/CCP 仍通过 TWS 私有定义或传递 include 依赖具体实现，既有循环治理不完整。 | 模块级 ASC↔TWS、CCP→ASC concrete 边仍在，增量编译和裁剪收益有限；这些反向边并非本提交新引入。 | 将契约枚举下沉到接口头，移除公共头对 concrete ASC 的 include。 |
| 7 | Minor | `services/service/BUILD.gn:97-100,231-235`; `services/service/include/IRemoteDeviceQuery.h:20`; `utils/BUILD.gn:28-37` | 新公共头依赖 utils 头，但 service 只对 utils 使用 private deps；既有 utils 配置又反向公开 service/include。 | 当前同 target 消费者未证实失败；独立下游 target 可能缺 include path，反向 public include 会掩盖真实依赖。 | 建立独立 contract target和明确 `public_deps`，移除 utils→service 的反向公共 include。 |
| 8 | Minor | `services/service/include/SleInterfaceProfileASC.h:191-198`; `test/unittest/services_test/service_test/vcp_test/mock_asc_service.cpp:235-290` | 新增三个纯虚方法后未补共用 ASC mock；该 mock 在父提交已存在其他虚方法依赖生产 DSO的问题。 | 本次进一步扩大 mock 对生产实现的依赖，隔离性下降；隐藏/静态链接形态需验证。 | 补齐全部方法或改成直接实现窄接口的 fake；检查每个 target 的未解析符号。 |
| 9 | Major | `test/unittest/services_test/service_test/asc_test/nearlink_asc_service_test.cpp:5245-5323` | 新增 NULL 用例没有断言，也未建立服务为 NULL；TWS 用例还未建立 vendor=true。 | 关键 NULL/非 NULL 行为变化无法被测试发现，部分用例可能根本不查询 TWS。 | 显式控制 vendor 和 manager 注册状态，并断言返回、调用次数和副作用。 |
| 10 | Major | `test/unittest/services_test/service_test/battery_manager_test/device_battery_manager_test.cpp:171-179`; `services/service/src/common/DeviceBatteryManager.cpp:45-53`; `services/device_manager/src/SleRemoteDeviceManager.cpp:525-532` | TwsNull 用例未注册音频设备，未知地址的 audio 判断固定为 false，确定在目标分支前返回。 | TWS NULL、report 地址、电量缓存和事件路径实际未覆盖。 | 注入 audio=true fake并验证完整副作用。 |
| 11 | Major | `test/unittest/services_test/service_test/ccp_test/ccp_service_test.cpp:385-408`; `test/unittest/services_test/service_test/cdsm_test/nearlink_cdsm_test.cpp:732-755`; `test/unittest/services_test/service_test/nl_datatransfer_test/nearlink_port_service_test.cpp:173-181`; `test/unittest/services_test/service_test/tws_test/nearlink_tws_test.cpp:1244-1278` | 多数新增 UT 仅验证不崩溃、对象非空或固定返回，不观察接口分派和副作用。 | 测试不能证明重构前后行为等价。 | 使用可配置 fake/spy，覆盖 NULL、成功、失败、参数、调用次数和异步完成。 |
| 12 | Minor | `services/service/include/IRemoteDeviceQuery.h:27-29` | public virtual destructor + 裸 singleton 指针允许调用者删除静态对象。 | 误用时非法释放、后续 UAF 或重复析构。 | 返回引用/非 owning handle，或把析构设为 protected 并明确所有权。 |
| 13 | Minor | `services/service/src/tws/TwsDefines.h:407-419` | 类型迁移同时新增默认成员初始化器。 | `BatteryInfo info;` 的运行时初始化语义改变，超出纯依赖治理；仓内当前未观察到负面结果。 | 保持原定义语义，或将行为改动拆分并说明/测试。 |
| 14 | Minor | `test/unittest/services_test/service_test/asc_test/nearlink_asc_service_test.cpp:90-93,5261` | 该 fixture 原有 QosM 清理不足，本次新用例继续写入一条未清理状态。 | 扩大既有隔离问题；用例顺序变化、repeat/shuffle 时可能交叉污染。 | TearDown 调用 `ClearQosM` 并清理 profile/设备缓存。 |
| 15 | Minor | `services/service/src/ccp/CcpService.cpp:30` | 新增 `TwsDefines.h` 但本文件没有直接使用其中符号。 | 扩散 TWS 私有依赖，可能掩盖其他头缺失 include。 | 删除无用 include，按 IWYU 修复真实依赖。 |

---

## 行为等价性判断

- 本次修改是否可以认为是纯循环依赖治理？不能。多数调用替换属于依赖治理，但公共虚接口/vtable、concrete 类层次和 `BatteryInfo` 默认初始化语义均发生变化。
- 是否发现明确业务行为变化？同版本、全量重编译的仓内主路径未发现明确返回值、错误码、状态机或副作用变化；普通默认初始化的 `BatteryInfo` 语义已改变。若支持混合版本/旧实现，接口 ABI 还存在条件性不兼容风险。
- 是否新增 NULL / 未注册 / 空实现路径？Profile 查询的 NULL 状态与旧 `GetService()` 相同，不是新增；`IRemoteDeviceQuery::GetInstance()` 当前恒非 NULL。测试名称声称的 NULL 前置多数没有真正建立。
- 新增路径与原逻辑是否等价？锁步版本下，已追踪的 ASC/TWS/CCP/adapter 调用目标和 NULL fallback 等价；跨 ABI 版本不等价，且测试不足以形成回归证明。
- 是否有运行时初始化顺序风险？没有新增注册/注销时序；存在 public 可删除 singleton 的所有权风险，以及既有 ProfileManager raw pointer 停服窗口。
- 是否有并发读写风险？本提交没有新增共享可变函数指针或 setter，未发现新增并发读写风险；既有 stop/delete 与业务查询竞态仍需系统生命周期保证。
- 是否有 ABI / API / 结构体布局变化？有。ProfileASC/ProfileTws vtable、CcpService 的继承/vtable 契约以及 SleRemoteDeviceAdapter 的类层次/对象布局发生变化；BatteryInfo 字段布局保持 13 字节，但默认初始化语义变化。
- 是否有构建依赖或链接风险？有。公共头不自包含、public/private 依赖不匹配、静态工厂保留 adapter 链接依赖、version script 未覆盖新符号、ASC mock 未补齐。

---

## 建议补充的验证

1. 执行 CleanArch/架构扫描，保存 parent 与当前提交的完整依赖边，逐条证明“17→11”，并区分 include 环、target 环和 link 环。
2. 用 include graph 工具检查 `ASC/TWS/CCP/CDSM/common/adapter/PORT/ICCE` 的强连通分量，确认没有通过私有定义头或传递 include 隐藏环。
3. 对 `IRemoteDeviceQuery.h`、`SleInterfaceProfileCcp.h`、`SleInterfaceProfileASC.h`、`SleInterfaceProfileTws.h`、`TwsHiBoxParser.h` 做单头 `-fsyntax-only` 自包含编译。
4. 使用 `gn desc ... deps --all` 和 `gn path` 检查 service、utils、PORT、ICCE、CDSM、battery 的真实依赖路径；临时拆分 target 做独立链接验证。
5. 执行全量编译，并覆盖 `nearlink_deps_audio_exist` / `nearlink_deps_telephony_exist` 的四种组合以及 PHONE/WATCH/TV/PC/TABLET 产品形态。
6. 独立构建 `nearlink_service_impl`、`nearlink_mcp_manager`、server、ASC/CCP/TWS/CDSM/ICCE/PORT/battery UT，检查 undefined/duplicate symbol。
7. 恢复或模拟 version script/symbol hiding，使用 `llvm-nm -D -C` 检查 `IRemoteDeviceQuery::GetInstance`、`ProfileCcp`、新增虚方法和 vtable 导出。
8. 使用 header ABI dumper/ABI compliance checker 对 parent 与当前产物做 ABI diff，重点检查 ProfileASC、ProfileTws、CcpService、SleRemoteDeviceAdapter 和 BatteryInfo。
9. 执行 cross-DSO CFI、LTO、user/release 构建，并验证新旧 `nearlink_service_impl`/`nearlink_mcp_manager` 混合版本；若不支持混合版本，需形成明确升级约束。
10. 重写新增 UT，使用可配置 fake/spy 覆盖 query/profile NULL、非 NULL、成功、失败、true/false、非零地址类型、调用次数、参数和副作用。
11. 增加电量集成测试，验证音频设备判断、合作集 report 地址、缓存更新、当前设备选择、上报值和 common event。
12. 增加 PORT 异步同步点测试，验证 `SetConnDirectActive` 和栈连接顺序，不使用固定 sleep 作为完成条件。
13. 执行全部受影响 UT/IT、`--gtest_shuffle --gtest_repeat=50`，确认 QosM、ProfileManager、DeviceBatteryManager 等全局状态隔离。
14. 执行 init/deinit/reload、重复 Start/Stop、停服后调用及在途异步任务测试，验证 manager raw pointer 生命周期。
15. 执行失败路径、不同配置文件/能力开关、profile 缺失和动态库加载失败回归测试。
16. 执行 clang-tidy/IWYU/静态分析，清理未使用 include、间接 include 和错误层次依赖。
17. 执行 ASan/UBSan/CFI；对 ProfileManager Stop 与业务调用、动态库卸载场景补 TSan 或专门线程安全检查。

当前已执行的验证仅为静态 diff/调用链审查和 `git diff --check`。当前工作环境缺少 GN、Ninja、C/C++ 编译器及产品 out 目录，因此未执行编译、UT、ABI 工具和架构扫描。

---

## 总体评价

本提交确实移除了多条 concrete include，并且在同版本全量构建条件下，已追踪调用大多仍落到原服务/adapter，未发现明确的树内业务结果变化；但既有模块环未全部消除，部分 concrete include 仅被替换为静态 provider 耦合。更关键的是，新增/扩展纯虚接口及 concrete 类层次明确违反本次“不改变 ABI”的硬约束；公共头自包含、契约类型分层和行为等价测试也有实质缺口。基于该明确约束而非未证实的混合版本崩溃，结论：**3. 暂不建议合入，需要先修复风险点**。
