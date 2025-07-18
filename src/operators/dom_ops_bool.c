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


#ifndef DOM_OPS_BOOL
#define DOM_OPS_BOOL(TYPE, BL)                                                  \
                                                                                \
/*   Performs multiplication/AND logic on two masked shares    */               \
/*   using the DOM-independent secure gadget as described by   */               \
/*   Gross et al. in “Domain-Oriented Masking” (CHES 2016).    */               \
/*   Link: https://eprint.iacr.org/2016/486.pdf                */               \
int FN(dom_bool_and, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                     \
{                                                                               \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    int rc = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                     \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    const uint32_t pair_count = out->share_count * out->order / 2;              \
    const uint32_t pair_bytes = pair_count * sizeof(TYPE);                      \
    TYPE rnd[pair_count];                                                       \
                                                                                \
    rc = csprng_read_array((uint8_t*)rnd, pair_bytes);                          \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    TYPE sh_out[out->share_count];                                              \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] = a->shares[i] & b->shares[i];                                \
    }                                                                           \
    uint16_t r_idx = 0;                                                         \
    for (uint8_t i = 0; i < out->order; ++i) {                                  \
        for (uint8_t j = i + 1; j < out->share_count; ++j) {                    \
            const TYPE r = rnd[r_idx++];                                        \
            sh_out[i] ^= (a->shares[i] & b->shares[j]) ^ r;                     \
            sh_out[j] ^= (a->shares[j] & b->shares[i]) ^ r;                     \
        }                                                                       \
    }                                                                           \
    memcpy(out->shares, sh_out, out->share_bytes);                              \
    rc = FN(dom_refresh, BL)(out);                                              \
                                                                                \
    secure_memzero(rnd, pair_bytes);                                            \
    secure_memzero(sh_out, out->share_bytes);                                   \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_or, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                      \
{                                                                               \
    int rc = FN(dom_bool_and, BL)(a, b, out);                                   \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    TYPE* sh_out = out->shares;                                                 \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] ^= a->shares[i] ^ b->shares[i];                               \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_xor, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                     \
{                                                                               \
    MTP(BL) mvs[] = { a, b, out };                                              \
    int rc = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                     \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    TYPE* sh_out = out->shares;                                                 \
                                                                                \
    for (uint8_t i = 0; i < out->share_count; ++i) {                            \
        sh_out[i] = a->shares[i] ^ b->shares[i];                                \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_not, BL)(MTP(BL) mv) {                                          \
    int rc = FN(dom_conv_atob, BL)(mv);                                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    mv->shares[0] = ~mv->shares[0];                                             \
                                                                                \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_shr, BL)(MTP(BL) mv, uint8_t n) {                               \
    int rc = FN(dom_conv_atob, BL)(mv);                                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    bit_length_t bl = mv->bit_length;                                           \
    n %= bl;                                                                    \
    if (n == 0)                                                                 \
        return 0;                                                               \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        mv->shares[i] >>= n;                                                    \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_shl, BL)(MTP(BL) mv, uint8_t n) {                               \
    int rc = FN(dom_conv_atob, BL)(mv);                                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    bit_length_t bl = mv->bit_length;                                           \
    n %= bl;                                                                    \
    if (n == 0)                                                                 \
        return 0;                                                               \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        mv->shares[i] <<= n;                                                    \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_rotr, BL)(MTP(BL) mv, uint8_t n) {                              \
    int rc = FN(dom_conv_atob, BL)(mv);                                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    bit_length_t bl = mv->bit_length;                                           \
    n %= bl;                                                                    \
    if (n == 0)                                                                 \
        return 0;                                                               \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        const TYPE v = mv->shares[i];                                           \
        mv->shares[i] = (v >> n) | (v << (bl - n));                             \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_rotl, BL)(MTP(BL) mv, uint8_t n)                                \
{                                                                               \
    int rc = FN(dom_conv_atob, BL)(mv);                                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    bit_length_t bl = mv->bit_length;                                           \
    n %= bl;                                                                    \
    if (n == 0)                                                                 \
        return 0;                                                               \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        const TYPE v = mv->shares[i];                                           \
        mv->shares[i] = (v << n) | (v >> (bl - n));                             \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_sub, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                     \
{                                                                               \
    MTP(BL) borrow = FN(dom_alloc, BL)(out->order, out->domain);                \
    if (!borrow)                                                                \
        return -1;                                                              \
    int rc = FN(dom_ksa_borrow, BL)(a, b, borrow);                              \
    if (!rc) {  /* no error */                                                  \
        FN(dom_bool_xor, BL)(a, b, out);                                        \
        FN(dom_bool_xor, BL)(out, borrow, out);                                 \
    }                                                                           \
    FN(dom_free, BL)(borrow);                                                   \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_bool_add, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out)                     \
{                                                                               \
    MTP(BL) carry = FN(dom_alloc, BL)(out->order, out->domain);                 \
    if (!carry)                                                                 \
        return -1;                                                              \
    int rc = FN(dom_ksa_carry, BL)(a, b, carry);                                \
    if (!rc) {  /* no error */                                                  \
        FN(dom_bool_xor, BL)(a, b, out);                                        \
        FN(dom_bool_xor, BL)(out, carry, out);                                  \
    }                                                                           \
    FN(dom_free, BL)(carry);                                                    \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \

#endif //DOM_OPS_BOOL


DOM_OPS_BOOL(uint8_t, 8)
DOM_OPS_BOOL(uint16_t, 16)
DOM_OPS_BOOL(uint32_t, 32)
DOM_OPS_BOOL(uint64_t, 64)
