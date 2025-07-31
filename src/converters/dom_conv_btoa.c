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
#include "internal/dom_internal_funcs.h"


#define DOM_CONV_BTOA(TYPE, BL)                                                 \
                                                                                \
static inline TYPE FN(psi, BL)(const TYPE masked, const TYPE mask) {            \
    return (masked ^ mask) - mask;                                              \
}                                                                               \
                                                                                \
/* NOLINTNEXTLINE(bugprone-macro-parentheses, misc-no-recursion) */             \
static RES_VAP(BL) FN(convert, BL)(const TYPE* x, const uint8_t n_plus1)        \
{                                                                               \
    INIT_RES_VAP(BL)                                                            \
                                                                                \
    const uint8_t n = n_plus1 - 1;                                              \
    if (n == 1) {                                                               \
        TYPE* out = (TYPE*)malloc(sizeof(TYPE));                                \
        IF_NO_MEM_RETURN_ERES(out, DOM_FUNC_CONV_BTOA, 0xAA00)                  \
        *out = x[0] ^ x[1];                                                     \
        res.vls = out;                                                          \
        return res;                                                             \
    }                                                                           \
    const uint16_t n_bytes = n * sizeof(TYPE);                                  \
    const uint16_t np1_bytes = n_plus1 * sizeof(TYPE);                          \
                                                                                \
    TYPE rnd[n];                                                                \
    ECODE ecode = csprng_read_array((uint8_t*)rnd, n_bytes);                    \
    if (ecode) {                                                                \
        res.error = set_dom_error_location(                                     \
            ecode, DOM_FUNC_CONV_BTOA, 0xAA11                                   \
        );                                                                      \
        return res;                                                             \
    }                                                                           \
                                                                                \
    TYPE x_mut[n_plus1];                                                        \
    memcpy(x_mut, x, np1_bytes);                                                \
                                                                                \
    for (uint8_t i = 1; i < n_plus1; ++i) {                                     \
        TYPE r = rnd[i - 1];                                                    \
        x_mut[i] ^= r;                                                          \
        x_mut[0] ^= r;                                                          \
    }                                                                           \
                                                                                \
    TYPE y[n];                                                                  \
    TYPE first_term = ((n - 1) & 1U) ? x_mut[0] : (TYPE)0;                      \
    y[0] = first_term ^ FN(psi, BL)(x_mut[0], x_mut[1]);                        \
    for (uint8_t i = 1; i < n; ++i) {                                           \
        y[i] = FN(psi, BL)(x_mut[0], x_mut[i + 1]);                             \
    }                                                                           \
                                                                                \
    RES_VAP(BL) first = FN(convert, BL)(&x_mut[1], n);                          \
    IF_ERES_GOTO_CLEANUP(first, DOM_FUNC_CONV_BTOA, 0xAA22)                     \
                                                                                \
    RES_VAP(BL) second = FN(convert, BL)(y, n);                                 \
    IF_ERES_GOTO_CLEANUP(second, DOM_FUNC_CONV_BTOA, 0xAA33)                    \
                                                                                \
    TYPE* out = (TYPE*)malloc(n_bytes);                                         \
    if (!out) {                                                                 \
        res.error = get_dom_error_code(                                         \
            DOM_ERROR_OUT_OF_MEMORY, DOM_FUNC_CONV_BTOA, 0xAA44                 \
        );                                                                      \
        goto cleanup;                                                           \
    }                                                                           \
                                                                                \
    for (uint8_t i = 0; i < n - 2; ++i) {                                       \
        out[i] = first.vls[i] + second.vls[i];                                  \
    }                                                                           \
    out[n - 2] = first.vls[n - 2];                                              \
    out[n - 1] = second.vls[n - 2];                                             \
    res.vls = out;                                                              \
                                                                                \
    cleanup:                                                                    \
    secure_memzero(rnd, n_bytes);                                               \
    secure_memzero(x_mut, np1_bytes);                                           \
    size_t buf_size = (size_t)(n - 1) * sizeof(TYPE);                           \
    if (first.vls) {                                                            \
        secure_memzero(first.vls, buf_size);                                    \
        free(first.vls);                                                        \
    }                                                                           \
    if (second.vls) {                                                           \
        secure_memzero(second.vls, buf_size);                                   \
        free(second.vls);                                                       \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
/*   Converts masked shares from boolean to arithmetic domain using        */   \
/*   the affine psi recursive decomposition method of Bettale et al.,      */   \
/*   "Improved High-Order Conversion From Boolean to Arithmetic Masking"   */   \
/*   Link: https://eprint.iacr.org/2018/328.pdf                            */   \
ECODE FN(dom_conv_btoa, BL)(MTP(BL) mv)                                         \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_CONV_BTOA, 0xAA55)                    \
    if (mv->domain == DOMAIN_ARITHMETIC) {                                      \
        return DOM_OK;                                                          \
    }                                                                           \
                                                                                \
    uint8_t share_bytes = mv->share_bytes;                                      \
    uint8_t sc_extra = mv->share_count + 1;                                     \
    uint8_t sce_bytes = sc_extra * sizeof(TYPE);                                \
                                                                                \
    TYPE* tmp = (TYPE*)malloc(sce_bytes);                                       \
    IF_NO_MEM_RETURN_ECODE(tmp, DOM_FUNC_CONV_BTOA, 0xAA66)                     \
                                                                                \
    memcpy(tmp, mv->shares, share_bytes);                                       \
    tmp[mv->share_count] = (TYPE)0;                                             \
                                                                                \
    RES_VAP(BL) res = FN(convert, BL)(tmp, sc_extra);                           \
    if (!res.error) {                                                           \
        mv->domain = DOMAIN_ARITHMETIC;                                         \
        memcpy(mv->shares, res.vls, share_bytes);                               \
    }                                                                           \
    if (res.vls) {                                                              \
        secure_memzero(res.vls, share_bytes);                                   \
        free(res.vls);                                                          \
    }                                                                           \
    secure_memzero(tmp, sce_bytes);                                             \
    free(tmp);                                                                  \
    asm volatile ("" ::: "memory");                                             \
    return res.error;                                                           \
}                                                                               \


DOM_CONV_BTOA(uint8_t, 8)
DOM_CONV_BTOA(uint16_t, 16)
DOM_CONV_BTOA(uint32_t, 32)
DOM_CONV_BTOA(uint64_t, 64)
