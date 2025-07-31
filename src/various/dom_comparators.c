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


#define DOM_COMPARE(BL)                                                         \
                                                                                \
ECODE FN(dom_cmp_lt, BL)(                                                       \
        MTP(BL) a,                                                              \
        MTP(BL) b,                                                              \
        MTP(BL) out,                                                            \
        const bool full_mask                                                    \
) {                                                                             \
    ECODE ecode = DOM_OK;                                                       \
                                                                                \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                      \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CMP_LT, 0xAA00)                      \
                                                                                \
    RES_MTPA(BL) res = FN(dom_alloc_many, BL)(5, out->order, DOMAIN_BOOLEAN);   \
    IF_ECODE_UPDATE_RETURN(res.error, DOM_FUNC_CMP_LT, 0xAA11)                  \
    MTPA(BL) tmp = res.mvs;                                                     \
                                                                                \
    MTP(BL) t0 = tmp[0];                                                        \
    MTP(BL) t1 = tmp[1];                                                        \
    MTP(BL) t2 = tmp[2];                                                        \
    MTP(BL) t3 = tmp[3];                                                        \
    MTP(BL) diff = tmp[4];                                                      \
                                                                                \
    ecode = FN(dom_bool_sub, BL)(a, b, diff);                                   \
    if (!ecode) {                                                               \
        FN(dom_bool_xor, BL)(a, b, t0);                                         \
        FN(dom_bool_xor, BL)(diff, b, t1);                                      \
                                                                                \
        ecode = FN(dom_bool_or, BL)(t0, t1, t2);                                \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_CMP_LT, 0xAA22)                   \
                                                                                \
        FN(dom_bool_xor, BL)(a, t2, t3);                                        \
        FN(dom_bool_shr, BL)(t3, BL - 1);                                       \
                                                                                \
        if (full_mask) {                                                        \
            RES_MTP(BL) one = FN(dom_mask, BL)(1, out->order, DOMAIN_BOOLEAN);  \
            IF_ECODE_GOTO_CLEANUP(one.error, DOM_FUNC_CMP_LT, 0xAA33)           \
                                                                                \
            ecode = FN(dom_bool_sub, BL)(t3, one.mv, t3);                       \
            FN(dom_free, BL)(one.mv);                                           \
            IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_CMP_LT, 0xAA44)               \
                                                                                \
            FN(dom_bool_not, BL)(t3);                                           \
        }                                                                       \
        memcpy(out->shares, t3->shares, t3->share_bytes);                       \
        ecode = FN(dom_refresh, BL)(out);                                       \
    }                                                                           \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(tmp, 5, true);                                        \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_cmp_le, BL)(                                                       \
        MTP(BL) a,                                                              \
        MTP(BL) b,                                                              \
        MTP(BL) out,                                                            \
        const bool full_mask                                                    \
) {                                                                             \
    const ECODE ecode = FN(dom_cmp_lt, BL)(b, a, out, full_mask);  /* NOLINT */ \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CMP_LE, 0xAA55)                      \
                                                                                \
    const UINT(BL) mask = full_mask ? (UINT(BL))0 - 1 : 1u;                     \
    out->shares[0] ^= mask;                                                     \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_cmp_gt, BL)(                                                       \
        MTP(BL) a,                                                              \
        MTP(BL) b,                                                              \
        MTP(BL) out,                                                            \
        const bool full_mask                                                    \
) {                                                                             \
    const ECODE ecode = FN(dom_cmp_lt, BL)(b, a, out, full_mask);  /* NOLINT */ \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CMP_GT, 0xAA66)                      \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_cmp_ge, BL)(                                                       \
        MTP(BL) a,                                                              \
        MTP(BL) b,                                                              \
        MTP(BL) out,                                                            \
        const bool full_mask                                                    \
) {                                                                             \
    const ECODE ecode = FN(dom_cmp_lt, BL)(a, b, out, full_mask);               \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CMP_GE, 0xAA77)                      \
                                                                                \
    const UINT(BL) mask = full_mask ? (UINT(BL))0 - 1 : 1u;                     \
    out->shares[0] ^= mask;                                                     \
    return DOM_OK;                                                              \
}                                                                               \


DOM_COMPARE(8)
DOM_COMPARE(16)
DOM_COMPARE(32)
DOM_COMPARE(64)
