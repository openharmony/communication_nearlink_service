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
#ifndef DLI_ERRNO_H
#define DLI_ERRNO_H

#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Error code of the host protocol stack.
    Currently, the error code is the same as that of the controller. */
#define    DLI_SUCCESS                                              0x00
#define    DLI_UNKNOWN_COMMAND                                      0x01
#define    DLI_UNKNOWN_CONNECTION_IDENTIFIER                        0x02
#define    DLI_HARDWARE_FAILURE                                     0x03
#define    DLI_AUTHENTICATION_FAILURE                               0x04
#define    DLI_PIN_OR_KEY_MISSING                                   0x05
#define    DLI_MEMORY_CAPACITY_EXCEEDED                             0x06
#define    DLI_CONNECTION_TIMEOUT                                   0x07
#define    DLI_CONNECTION_LIMIT_EXCEEDED                            0x08
#define    DLI_SYNC_CONNECTION_LIMIT_EXCEEDED                       0x09
#define    DLI_CONNECTION_ALREADY_EXISTS                            0x0A
#define    DLI_COMMAND_DISALLOWED                                   0x0B
#define    DLI_UNACCEPTADLI_BDADDR                                  0x0C
#define    DLI_COMMAND_TIMEOUT                                      0x0D
#define    DLI_UNSUPPORTED_REMOTE_FEATURE                           0x0E
#define    DLI_INVALID_PARAMETERS                                   0x0F
#define    DLI_REMOTE_USER_TERMINATED_CONNECTION                    0x10
#define    DLI_CONNECTION_TERMINATED_BY_LOCAL_HOST                  0x11
#define    DLI_ROLE_CHANGE_NOT_ALLOWED                              0x12
#define    DLI_ENCRYPTION_MODE_NOT_ACCEPTAGLE                       0x13
#define    DLI_LINK_KEY_CANNOT_BE_CHANGED                           0x14
#define    DLI_INSTANT_PASSED                                       0x15
#define    DLI_CHANNEL_CLASSIFICATION_NOT_SUPPORTED                 0x16
#define    DLI_INSUFFICIENT_SECURITY                                0x17
#define    DLI_ROLE_SWITCH_FAILED                                   0x18
#define    DLI_CONTROLLER_BUSY                                      0x19
#define    DLI_ADVERTISING_TIMEOUT                                  0x1A
#define    DLI_CONNECTION_TERMINATED_MIC_FAILURE                    0x1B
#define    DLI_CONNECTION_FAILED_TO_BE_ESTABLISHED                  0x1C
#define    DLI_CCA_REJECTED_BUT_ADJUST_USING_CLOCK_DRAGGING         0x1D
#define    DLI_UNKNOWN_ADVERTISING_IDENTIFIER                       0x1E
#define    DLI_PACKET_TOO_LONG                                      0x1F
#define    DLI_UNSPECIFIED_ERROR                                    0x20

#define DLI_MAKE_ERRNO(id) SDF_MAKE_ERRNO(COMP_DLI, 1, 1, (id))
#define DLI_STACK_NOINIT_ERRNO                 DLI_MAKE_ERRNO(101)
#define DLI_STACK_INITED_ERRNO                 DLI_MAKE_ERRNO(102)
#define DLI_STACK_PARAMS_ERRNO                 DLI_MAKE_ERRNO(103)
#define DLI_STACK_MEM_ERRNO                    DLI_MAKE_ERRNO(104)
#define DLI_STACK_EVC_CREATE_ERRNO             DLI_MAKE_ERRNO(105)
#define DLI_STACK_WORKER_CREATE_ERRNO          DLI_MAKE_ERRNO(106)
#define DLI_STACK_EVENT_ADD_ERRNO              DLI_MAKE_ERRNO(107)
#define DLI_STACK_INIT_SEM_ERRNO               DLI_MAKE_ERRNO(108)
#define DLI_STACK_HAL_INIT_ERRNO               DLI_MAKE_ERRNO(109)
#define DLI_STACK_INIT_TIMEOUT_ERRNO           DLI_MAKE_ERRNO(110)
#define DLI_STACK_INIT_LOCK_ERRNO              DLI_MAKE_ERRNO(111)
#define DLI_STACK_READ_COMM_CONFIG_VAL_ERRNO   DLI_MAKE_ERRNO(112)
#define DLI_STACK_POST_BLOCK_ERROR             DLI_MAKE_ERRNO(113)
#define DLI_STACK_TASK_TIMEOUT                 DLI_MAKE_ERRNO(114)
#define DLI_STACK_OUT_MAXCNT_ERRNO             DLI_MAKE_ERRNO(115)
#ifdef __cplusplus
}
#endif
#endif // DLI_ERRNO_H