/*
 *   Apache License 2.0
 *
 *   Copyright (c) 2025, Mattias Aabmets
 *
 *   The contents of this file are subject to the terms and conditions defined in the License.
 *   You may not use, modify, or distribute this file except in compliance with the License.
 *
 *   SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


#define DOM_SELECT(BL)                                                          \
                                                                                \
ECODE FN(dom_select, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) mask, MTP(BL) out)       \
{                                                                               \
    ECODE ecode = DOM_OK;                                                       \
                                                                                \
    MTP(BL) mvs[4] = { a, b, mask, out };                                       \
    ecode = FN(dom_conv_many, BL)(mvs, 4, DOMAIN_BOOLEAN);                      \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_SELECT, 0xAA00)                      \
                                                                                \
    RES_MTPA(BL) res = FN(dom_alloc_many, BL)(3, out->order, DOMAIN_BOOLEAN);   \
    IF_ECODE_UPDATE_RETURN(res.error, DOM_FUNC_SELECT, 0xAA11)                  \
    MTPA(BL) tmp = res.mvs;                                                     \
                                                                                \
    MTP(BL) t0 = tmp[0];                                                        \
    MTP(BL) t1 = tmp[1];                                                        \
    MTP(BL) t2 = tmp[2];                                                        \
                                                                                \
    FN(dom_bool_xor, BL)(a, b, t0);                                             \
    ecode = FN(dom_bool_and, BL)(mask, t0, t1);                                 \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_SELECT, 0xAA22)                       \
                                                                                \
    FN(dom_bool_xor, BL)(t1, b, t2);                                            \
                                                                                \
    memcpy(out->shares, t2->shares, t2->share_bytes);                           \
    ecode = FN(dom_refresh, BL)(out);                                           \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(tmp, 3, true);                                        \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \
                                                                                \
                                                                                \
RES_MTP(BL) FN(dom_select_lt, BL)(                                              \
        MTP(BL) a_cmp,                                                          \
        MTP(BL) b_cmp,                                                          \
        MTP(BL) truth_sel,                                                      \
        MTP(BL) false_sel                                                       \
) {                                                                             \
    INIT_RES_MTP(BL)                                                            \
                                                                                \
    const RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a_cmp, 2, true);         \
    IF_ERES_UPDATE_RETURN(clones, DOM_FUNC_SELECT_LT, 0xAA33)                   \
                                                                                \
    MTP(BL) tmp = clones.mvs[0];                                                \
    MTP(BL) out = clones.mvs[1];                                                \
                                                                                \
    res.error = FN(dom_cmp_lt, BL)(a_cmp, b_cmp, tmp, true);                    \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_LT, 0xAA44)                       \
                                                                                \
    res.error = FN(dom_select, BL)(truth_sel, false_sel, tmp, out);             \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_LT, 0xAA55)                       \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free, BL)(tmp);                                                      \
    if (res.error) {                                                            \
        FN(dom_free, BL)(out);                                                  \
    } else {                                                                    \
        res.mv = out;                                                           \
    }                                                                           \
    aligned_free(clones.mvs);                                                   \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTP(BL) FN(dom_select_le, BL)(                                              \
        MTP(BL) a_cmp,                                                          \
        MTP(BL) b_cmp,                                                          \
        MTP(BL) truth_sel,                                                      \
        MTP(BL) false_sel                                                       \
) {                                                                             \
    INIT_RES_MTP(BL)                                                            \
                                                                                \
    const RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a_cmp, 2, true);         \
    IF_ERES_UPDATE_RETURN(clones, DOM_FUNC_SELECT_LE, 0xAA66)                   \
                                                                                \
    MTP(BL) tmp = clones.mvs[0];                                                \
    MTP(BL) out = clones.mvs[1];                                                \
                                                                                \
    res.error = FN(dom_cmp_le, BL)(a_cmp, b_cmp, tmp, true);                    \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_LE, 0xAA77)                       \
                                                                                \
    res.error = FN(dom_select, BL)(truth_sel, false_sel, tmp, out);             \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_LE, 0xAA88)                       \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free, BL)(tmp);                                                      \
    if (res.error) {                                                            \
        FN(dom_free, BL)(out);                                                  \
    } else {                                                                    \
        res.mv = out;                                                           \
    }                                                                           \
    aligned_free(clones.mvs);                                                   \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTP(BL) FN(dom_select_gt, BL)(                                              \
        MTP(BL) a_cmp,                                                          \
        MTP(BL) b_cmp,                                                          \
        MTP(BL) truth_sel,                                                      \
        MTP(BL) false_sel                                                       \
) {                                                                             \
    INIT_RES_MTP(BL)                                                            \
                                                                                \
    const RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a_cmp, 2, true);         \
    IF_ERES_UPDATE_RETURN(clones, DOM_FUNC_SELECT_GT, 0xAA99)                   \
                                                                                \
    MTP(BL) tmp = clones.mvs[0];                                                \
    MTP(BL) out = clones.mvs[1];                                                \
                                                                                \
    res.error = FN(dom_cmp_gt, BL)(a_cmp, b_cmp, tmp, true);                    \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_GT, 0xBB00)                       \
                                                                                \
    res.error = FN(dom_select, BL)(truth_sel, false_sel, tmp, out);             \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_GT, 0xBB11)                       \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free, BL)(tmp);                                                      \
    if (res.error) {                                                            \
        FN(dom_free, BL)(out);                                                  \
    } else {                                                                    \
        res.mv = out;                                                           \
    }                                                                           \
    aligned_free(clones.mvs);                                                   \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTP(BL) FN(dom_select_ge, BL)(                                              \
        MTP(BL) a_cmp,                                                          \
        MTP(BL) b_cmp,                                                          \
        MTP(BL) truth_sel,                                                      \
        MTP(BL) false_sel                                                       \
) {                                                                             \
    INIT_RES_MTP(BL)                                                            \
                                                                                \
    const RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a_cmp, 2, true);         \
    IF_ERES_UPDATE_RETURN(clones, DOM_FUNC_SELECT_GE, 0xBB22)                   \
                                                                                \
    MTP(BL) tmp = clones.mvs[0];                                                \
    MTP(BL) out = clones.mvs[1];                                                \
                                                                                \
    res.error = FN(dom_cmp_ge, BL)(a_cmp, b_cmp, tmp, true);                    \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_GE, 0xBB33)                       \
                                                                                \
    res.error = FN(dom_select, BL)(truth_sel, false_sel, tmp, out);             \
    IF_ERES_GOTO_CLEANUP(res, DOM_FUNC_SELECT_GE, 0xBB44)                       \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free, BL)(tmp);                                                      \
    if (res.error) {                                                            \
        FN(dom_free, BL)(out);                                                  \
    } else {                                                                    \
        res.mv = out;                                                           \
    }                                                                           \
    aligned_free(clones.mvs);                                                   \
    return res;                                                                 \
}                                                                               \


DOM_SELECT(8)
DOM_SELECT(16)
DOM_SELECT(32)
DOM_SELECT(64)
