/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef DPFWK_ERRCODE_H
#define DPFWK_ERRCODE_H

#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DP_Module_E {
    DP_MOD_CM = 1,
    DP_MOD_MM,
    DP_MOD_QOSM,
    DP_MOD_DTAP,
    DP_MOD_TRANSPORT,
} DP_Module_E;

typedef enum DP_CM_SubModule_E {
    DP_CM_SUBMOD_TCM = 1,
    DP_CM_SUBMOD_RTCM,
    DP_CM_SUBMOD_CTCM,
    DP_CM_SUBMOD_CTRL_SIG,
} DP_CM_SubModule_E;

typedef enum DP_MM_SubModule_E {
    // mm submodule
    DP_MM_UNKNOWN,
} DP_MM_SubModule_E;

typedef enum DP_QOSM_SubModule_E {
    // qosm submodule
    DP_QOSM_UNKNOWN,
} DP_QOSM_SubModule_E;

typedef enum DP_DTAP_SubModule_E {
    DP_DTAP_SUBMOD_PERF = 1,
    DP_DTAP_SUBMOD_CONV,
    DP_DTAP_SUBMOD_RELAY,
    DP_DTAP_SUBMOD_TRANS,
    DP_DTAP_SUBMOD_FRAME,
} DP_DTAP_SubModule_E;

typedef enum DP_TRANS_SubModule_E {
    DP_TRANS_SUBMOD_FRAMEWORK = 1,
    DP_TRANS_SUBMOD_LWCLTP,
} DP_TRANS_SubModule_E;

/*  error code id transferred when an error code is printed, for example:
 *  log("error code = %d", DP_MAKE_CM_TCM_ERRNO(1))
 *  success: directly print 0, for example:
 *  log("success = %d", 0)
 *  error codes without prefixes are used internally. */
#define DP_MAKE_CM_TCM_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_CM, DP_CM_SUBMOD_TCM, (id))
#define DP_MAKE_CM_RTCM_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_CM, DP_CM_SUBMOD_RTCM, (id))
#define DP_MAKE_CM_CTCM_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_CM, DP_CM_SUBMOD_CTCM, (id))
#define DP_MAKE_CM_CTRL_SIG_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_CM, DP_CM_SUBMOD_CTRL_SIG, (id))

#define DP_MAKE_DTAP_PERF_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_DTAP, DP_DTAP_SUBMOD_PERF, (id))
#define DP_MAKE_DTAP_CONV_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_DTAP, DP_DTAP_SUBMOD_CONV, (id))
#define DP_MAKE_DTAP_RELAY_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_DTAP, DP_DTAP_SUBMOD_RELAY, (id))
#define DP_MAKE_DTAP_TRANS_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_DTAP, DP_DTAP_SUBMOD_TRANS, (id))
#define DP_MAKE_DTAP_FRAME_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_DTAP, DP_DTAP_SUBMOD_FRAME, (id))

#define DP_MAKE_TRANS_FWK_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_TRANSPORT, DP_TRANS_SUBMOD_FRAMEWORK, (id))
#define DP_MAKE_TRANS_LWCLTP_ERRNO(id) SDF_MAKE_ERRNO(COMP_DP, DP_MOD_TRANSPORT, DP_TRANS_SUBMOD_LWCLTP, (id))

#ifdef __cplusplus
}
#endif

#endif  // DPFWK_ERRCODE_H