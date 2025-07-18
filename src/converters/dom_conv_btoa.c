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


#define DOM_BTOA_HELPERS(TYPE, BL)                                              \
static inline TYPE FN(psi, BL)(TYPE masked, TYPE mask) {                        \
    return (masked ^ mask) - mask;                                              \
}                                                                               \
                                                                                \
/* NOLINTNEXTLINE(bugprone-macro-parentheses, misc-no-recursion) */             \
static TYPE* FN(convert, BL)(const TYPE* x, uint8_t n_plus1)                    \
{                                                                               \
    const uint8_t n = n_plus1 - 1;                                              \
    if (n == 1) {                                                               \
        TYPE* out = (TYPE*)malloc(sizeof(TYPE));                                \
        if (!out)                                                               \
            return NULL;                                                        \
        *out = x[0] ^ x[1];                                                     \
        return out;                                                             \
    }                                                                           \
    const uint16_t n_bytes = n * sizeof(TYPE);                                  \
    const uint16_t np1_bytes = n_plus1 * sizeof(TYPE);                          \
                                                                                \
    TYPE rnd[n];                                                                \
    int rc = csprng_read_array((uint8_t*)rnd, n_bytes);                         \
    if (rc)                                                                     \
        return NULL;                                                            \
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
    TYPE* first = FN(convert, BL)(&x_mut[1], n);                                \
    if (!first)                                                                 \
        goto cleanup;                                                           \
                                                                                \
    TYPE* second = FN(convert, BL)(y, n);                                       \
    if (!second)                                                                \
        goto cleanup;                                                           \
                                                                                \
    TYPE* out = (TYPE*)malloc(n_bytes);                                         \
    if (!out)                                                                   \
        goto cleanup;                                                           \
                                                                                \
    for (uint8_t i = 0; i < n - 2; ++i) {                                       \
        out[i] = first[i] + second[i];                                          \
    }                                                                           \
    out[n - 2] = first[n - 2];                                                  \
    out[n - 1] = second[n - 2];                                                 \
                                                                                \
    cleanup:                                                                    \
    secure_memzero(rnd, n_bytes);                                               \
    secure_memzero(x_mut, np1_bytes);                                           \
    size_t buf_size = (size_t)(n - 1) * sizeof(TYPE);                           \
    if (first) {                                                                \
        secure_memzero(first, buf_size);                                        \
        free(first);                                                            \
    }                                                                           \
    if (second) {                                                               \
        secure_memzero(second, buf_size);                                       \
        free(second);                                                           \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return out ? out : NULL;                                                    \
}                                                                               \


#ifndef DOM_CONV_BTOA
#define DOM_CONV_BTOA(TYPE, BL)                                                 \
                                                                                \
DOM_BTOA_HELPERS(TYPE, BL)                                                      \
                                                                                \
/*   Converts masked shares from boolean to arithmetic domain using        */   \
/*   the affine psi recursive decomposition method of Bettale et al.,      */   \
/*   "Improved High-Order Conversion From Boolean to Arithmetic Masking"   */   \
/*   Link: https://eprint.iacr.org/2018/328.pdf                            */   \
int FN(dom_conv_btoa, BL)(MTP(BL) mv)                                           \
{                                                                               \
    if (!mv) {                                                                  \
        return -1;                                                              \
    } else if (mv->domain == DOMAIN_ARITHMETIC)                                 \
        return 0;                                                               \
                                                                                \
    uint8_t share_bytes = mv->share_bytes;                                      \
    uint8_t sc_extra = mv->share_count + 1;                                     \
    uint8_t sce_bytes = sc_extra * sizeof(TYPE);                                \
    TYPE* shares = mv->shares;                                                  \
                                                                                \
    TYPE* tmp = (TYPE*)malloc(sce_bytes);                                       \
    if (!tmp)                                                                   \
        return -1;                                                              \
                                                                                \
    memcpy(tmp, shares, share_bytes);                                           \
    tmp[mv->share_count] = (TYPE)0;                                             \
                                                                                \
    int rc = -1;                                                                \
    TYPE* new_shares = FN(convert, BL)(tmp, sc_extra);                          \
    if (new_shares) {                                                           \
        mv->domain = DOMAIN_ARITHMETIC;                                         \
        memcpy(shares, new_shares, share_bytes);                                \
        secure_memzero(new_shares, share_bytes);                                \
        free(new_shares);                                                       \
        rc = 0;                                                                 \
    }                                                                           \
    secure_memzero(tmp, sce_bytes);                                             \
    free(tmp);                                                                  \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \

#endif //DOM_CONV_BTOA


DOM_CONV_BTOA(uint8_t, 8)
DOM_CONV_BTOA(uint16_t, 16)
DOM_CONV_BTOA(uint32_t, 32)
DOM_CONV_BTOA(uint64_t, 64)
