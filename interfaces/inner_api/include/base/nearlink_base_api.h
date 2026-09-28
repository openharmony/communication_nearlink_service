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

/**
 * @file nearlink_base_api.h
 *
 * @brief Aggregate entry of base.so public API for external repositories.
 *
 * base.so implementation classes carry the "Core" suffix (e.g. NearlinkHostCore)
 * while the framework.so mirrors keep the public names (e.g. NearlinkHost).
 * External repositories may depend on base.so directly and use these classes,
 * or may keep using the framework public API which forwards to base internally.
 *
 * Only headers located under interfaces/inner_api/include are publishable;
 * internal base headers (sa_manager/proxy/stub/switch_module/host_load_callback)
 * are not listed here and are consumed inside this repository only.
 */

#ifndef NEARLINK_BASE_API_H
#define NEARLINK_BASE_API_H

#include "base/nearlink_host_core.h"
#include "base/nearlink_remote_device_core.h"
#include "base/nearlink_sle_controller_core.h"

#endif  // NEARLINK_BASE_API_H
