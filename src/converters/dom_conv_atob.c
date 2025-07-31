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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"


#define DOM_CONV_ATOB(BL)                                                       \
                                                                                \
static ECODE FN(csa, BL)(                                                       \
        MTP(BL) x,                                                              \
        MTP(BL) y,                                                              \
        MTP(BL) z,                                                              \
        MTPA(BL) s_res,                                                         \
        MTPA(BL) c_res                                                          \
) {                                                                             \
    RES_MTPA(BL) res = FN(dom_alloc_many, BL)(5, x->order, x->domain);          \
    IF_ECODE_UPDATE_RETURN(res.error, DOM_FUNC_CONV_ATOB, 0xAA00)               \
    MTPA(BL) tmp = res.mvs;                                                     \
                                                                                \
    /*  a = x ^ y  */                                                           \
    MTP(BL) a = tmp[0];                                                         \
    FN(dom_bool_xor, BL)(x, y, a);                                              \
                                                                                \
    /*  s = a ^ z  */                                                           \
    MTP(BL) s = tmp[1];                                                         \
    FN(dom_bool_xor, BL)(a, z, s);                                              \
                                                                                \
    /*  w = x ^ z  */                                                           \
    MTP(BL) w = tmp[2];                                                         \
    FN(dom_bool_xor, BL)(x, z, w);                                              \
                                                                                \
    /*  v = a & w  */                                                           \
    MTP(BL) v = tmp[3];                                                         \
    ECODE ecode = FN(dom_bool_and, BL)(a, w, v);                                \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CONV_ATOB, 0xAA11)                   \
                                                                                \
    /*  c = x ^ v  */                                                           \
    MTP(BL) c = tmp[4];                                                         \
    FN(dom_bool_xor, BL)(x, v, c);                                              \
    FN(dom_bool_shl, BL)(c, 1);                                                 \
                                                                                \
    *s_res = s;                                                                 \
    *c_res = c;                                                                 \
                                                                                \
    MTP(BL) mvs[3] = { a, w, v };                                               \
    FN(dom_free_many, BL)(mvs, 3, false);                                       \
    asm volatile ("" ::: "memory");                                             \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
/* NOLINTNEXTLINE(misc-no-recursion) */                                         \
static ECODE FN(csa_tree, BL)(                                                  \
        MTP(BL) vals[],                                                         \
        MTPA(BL) s_res,                                                         \
        MTPA(BL) c_res,                                                         \
        const uint8_t len                                                       \
) {                                                                             \
    if (len == 3) {                                                             \
        return FN(csa, BL)(vals[0], vals[1], vals[2], s_res, c_res);            \
    }                                                                           \
    const uint8_t len_min1 = len - 1;                                           \
    MTP(BL) mv = vals[0];                                                       \
                                                                                \
    RES_MTPA(BL) res = FN(dom_alloc_many, BL)(2, mv->order, mv->domain);        \
    IF_ECODE_UPDATE_RETURN(res.error, DOM_FUNC_CONV_ATOB, 0xAA22)               \
    MTPA(BL) tmp = res.mvs;                                                     \
                                                                                \
    ECODE ecode = FN(csa_tree, BL)(vals, &tmp[0], &tmp[1], len_min1);           \
    if (ecode) {                                                                \
        ecode = set_dom_error_location(                                         \
            ecode, DOM_FUNC_CONV_ATOB, 0xAA33                                   \
        );                                                                      \
    } else {                                                                    \
        ecode = FN(csa, BL)(tmp[0], tmp[1], vals[len_min1], s_res, c_res);      \
        if (ecode) {                                                            \
            ecode = set_dom_error_location(                                     \
                ecode, DOM_FUNC_CONV_ATOB, 0xAA44                               \
            );                                                                  \
        }                                                                       \
    }                                                                           \
                                                                                \
    FN(dom_free_many, BL)(tmp, 2, true);                                        \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \
                                                                                \
                                                                                \
/*   Converts masked shares from arithmetic to boolean domain using        */   \
/*   the high-order recursive carry-save-adder method of Liu et al.,       */   \
/*   “A Low-Latency High-Order Arithmetic to Boolean Masking Conversion”   */   \
/*   Link: https://eprint.iacr.org/2024/045.pdf                            */   \
ECODE FN(dom_conv_atob, BL)(MTP(BL) mv)                                         \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_CONV_ATOB, 0xAA55)                    \
    if (mv->domain == DOMAIN_BOOLEAN) {                                         \
        return DOM_OK;                                                          \
    }                                                                           \
                                                                                \
    RES_MTPA(BL) vals_res = FN(dom_mask_many, BL)(                              \
        mv->shares, mv->share_count, mv->order, DOMAIN_BOOLEAN                  \
    );                                                                          \
    IF_ECODE_UPDATE_RETURN(vals_res.error, DOM_FUNC_CONV_ATOB, 0xAA66)          \
    MTPA(BL) vals = vals_res.mvs;                                               \
                                                                                \
    ECODE ecode = DOM_OK;                                                       \
    MTP(BL) s_res = NULL;                                                       \
    MTP(BL) c_res = NULL;                                                       \
    MTP(BL) k_out = NULL;                                                       \
    RES_MTP(BL) k_res = FN(dom_alloc, BL)(mv->order, DOMAIN_BOOLEAN);           \
    IF_ECODE_GOTO_CLEANUP(k_res.error, DOM_FUNC_CONV_ATOB, 0xAA77)              \
    k_out = k_res.mv;                                                           \
                                                                                \
    if (mv->share_count == 2) {                                                 \
        s_res = vals[0];                                                        \
        c_res = vals[1];                                                        \
    } else {                                                                    \
        ecode = FN(csa_tree, BL)(vals, &s_res, &c_res, mv->share_count);        \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_CONV_ATOB, 0xAA88)                \
    }                                                                           \
    ecode = FN(dom_ksa_carry, BL)(s_res, c_res, k_out);                         \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_CONV_ATOB, 0xAA99)                    \
                                                                                \
    FN(dom_bool_xor, BL)(s_res, k_out, k_out);                                  \
    FN(dom_bool_xor, BL)(c_res, k_out, k_out);                                  \
                                                                                \
    memcpy(mv->shares, k_out->shares, mv->share_bytes);                         \
    mv->domain = DOMAIN_BOOLEAN;                                                \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(vals, mv->share_count, true);                         \
    if (mv->share_count > 2) {                                                  \
        if (s_res)                                                              \
            FN(dom_free, BL)(s_res);                                            \
        if (c_res)                                                              \
            FN(dom_free, BL)(c_res);                                            \
    }                                                                           \
    if (k_out)                                                                  \
        FN(dom_free, BL)(k_out);                                                \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \


DOM_CONV_ATOB(8)
DOM_CONV_ATOB(16)
DOM_CONV_ATOB(32)
DOM_CONV_ATOB(64)
