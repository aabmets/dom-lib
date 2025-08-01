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

#include <stdint.h>
#include <string.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


#define DOM_OPS_ARITH(TYPE, BL)                                                 \
                                                                                \
ECODE FN(dom_arith_add, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                  \
{                                                                               \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    ECODE ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_ARITHMETIC);             \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_ARITH_ADD, 0xAA00)                   \
                                                                                \
    TYPE* sh_out = out->shares;                                                 \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] = a->shares[i] + b->shares[i];                                \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_arith_sub, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                  \
{                                                                               \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    ECODE ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_ARITHMETIC);             \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_ARITH_SUB, 0xAA11)                   \
                                                                                \
    TYPE* sh_out = out->shares;                                                 \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] = a->shares[i] - b->shares[i];                                \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
/*   Performs multiplication/AND logic on two masked shares    */               \
/*   using the DOM-independent secure gadget as described by   */               \
/*   Gross et al. in “Domain-Oriented Masking” (CHES 2016).    */               \
/*   Link: https://eprint.iacr.org/2016/486.pdf                */               \
ECODE FN(dom_arith_mult, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                 \
{                                                                               \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    ECODE ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_ARITHMETIC);             \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_ARITH_MULT, 0xAA22)                  \
                                                                                \
    const uint32_t pair_count = out->share_count * out->order / 2;              \
    const uint32_t pair_bytes = pair_count * sizeof(TYPE);                      \
    TYPE rnd[pair_count];                                                       \
                                                                                \
    ecode = csprng_read_array((uint8_t*)rnd, pair_bytes);                       \
    IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_ARITH_MULT, 0xAA33)                  \
                                                                                \
    TYPE sh_out[out->share_count];                                              \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] = a->shares[i] * b->shares[i];                                \
    }                                                                           \
    uint16_t r_idx = 0;                                                         \
    for (uint8_t i = 0; i < out->order; ++i) {                                  \
        for (uint8_t j = i + 1; j < out->share_count; ++j) {                    \
            const TYPE r = rnd[r_idx++];                                        \
            sh_out[i] += (a->shares[i] * b->shares[j]) + r;                     \
            sh_out[j] += (a->shares[j] * b->shares[i]) - r;                     \
        }                                                                       \
    }                                                                           \
    memcpy(out->shares, sh_out, out->share_bytes);                              \
    ecode = FN(dom_refresh, BL)(out);                                           \
                                                                                \
    secure_memzero(rnd, pair_bytes);                                            \
    secure_memzero(sh_out, out->share_bytes);                                   \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \


DOM_OPS_ARITH(uint8_t, 8)
DOM_OPS_ARITH(uint16_t, 16)
DOM_OPS_ARITH(uint32_t, 32)
DOM_OPS_ARITH(uint64_t, 64)
