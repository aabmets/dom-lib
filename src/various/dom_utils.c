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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


#ifndef DOM_CORE
#define DOM_CORE(TYPE, BL, BIT_LENGTH)                                          \
                                                                                \
int FN(dom_free, BL)(MTP(BL) mv)                                                \
{                                                                               \
    if (!mv)                                                                    \
        return -1;                                                              \
    secure_memzero(mv, mv->total_bytes);                                        \
    aligned_free(mv);                                                           \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_free_many, BL)(                                                      \
        MTPA(BL) mvs,                                                           \
        const uint8_t count,                                                    \
        bool free_array                                                         \
) {                                                                             \
    if (!mvs || count < 2)                                                      \
        return -1;                                                              \
    for (uint8_t i = 0; i < count; ++i)                                         \
        FN(dom_free, BL)(mvs[i]);                                               \
    if (free_array)                                                             \
        aligned_free(mvs);                                                      \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_clear, BL)(MTP(BL) mv)                                               \
{                                                                               \
    if (!mv)                                                                    \
        return -1;                                                              \
    secure_memzero(mv->shares, mv->share_bytes);                                \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_clear_many, BL)(MTPA(BL) mvs, const uint8_t count)                   \
{                                                                               \
    if (!mvs || count < 2)                                                      \
        return -1;                                                              \
    for (uint8_t i = 0; i < count; ++i)                                         \
        FN(dom_clear, BL)(mvs[i]);                                              \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
MTP(BL) FN(dom_alloc, BL)(const uint8_t order, const domain_t domain)           \
{                                                                               \
    if (order == 0 || order > MAX_SEC_ORDER)                                    \
        return NULL;                                                            \
                                                                                \
    const uint8_t share_count = order + 1;                                      \
    const uint16_t share_bytes = share_count * sizeof(TYPE);                    \
    const size_t struct_size = share_bytes + sizeof(MT(BL));                    \
    const size_t align = alignof(TYPE);                                         \
    const size_t offset = align - 1;                                            \
    const size_t total_bytes = (struct_size + offset) & ~offset;                \
                                                                                \
    MTP(BL) mv = aligned_alloc(align, total_bytes);                             \
    if (!mv)                                                                    \
        return NULL;                                                            \
                                                                                \
    mv->bit_length = BIT_LENGTH;                                                \
    mv->total_bytes = total_bytes;                                              \
    mv->domain = domain;                                                        \
    mv->order = order;                                                          \
    mv->share_count = share_count;                                              \
    mv->share_bytes = share_bytes;                                              \
    mv->sig = (uint16_t)order << 8 | BIT_LENGTH;                                \
    secure_memzero(mv->shares, share_bytes);                                    \
    return mv;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
MTPA(BL) FN(dom_alloc_many, BL)(                                                \
        const uint8_t count,                                                    \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    if (count < 2 || order == 0 || order > MAX_SEC_ORDER)                       \
        return NULL;                                                            \
                                                                                \
    const size_t align = alignof(TYPE);                                         \
    MTPA(BL) mvs = aligned_alloc(align, count * sizeof(*mvs));                  \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        mvs[i] = FN(dom_alloc, BL)(order, domain);                              \
        if (!mvs[i]) {                                                          \
            FN(dom_free_many, BL)(mvs, i, true);                                \
            return NULL;                                                        \
        }                                                                       \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
MTP(BL) FN(dom_mask, BL)(                                                       \
        const TYPE value,                                                       \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    if (order == 0 || order > MAX_SEC_ORDER)                                    \
        return NULL;                                                            \
                                                                                \
    MTP(BL) mv = FN(dom_alloc, BL)(order, domain);                              \
    if (!mv)                                                                    \
        return NULL;                                                            \
                                                                                \
    TYPE* shares = (TYPE*)mv->shares;                                           \
    uint8_t order_bytes = order * sizeof(TYPE);                                 \
                                                                                \
    int rc = csprng_read_array((uint8_t*)&shares[1], order_bytes);              \
    if (rc) {                                                                   \
        FN(dom_free, BL)(mv);                                                   \
        return NULL;                                                            \
    }                                                                           \
    TYPE masked = value;                                                        \
    if (domain == DOMAIN_BOOLEAN) {  /* XOR masking */                          \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            masked ^= shares[i];                                                \
        }                                                                       \
    } else {  /* DOMAIN_ARITHMETIC - subtractive masking */                     \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            masked -= shares[i];                                                \
        }                                                                       \
    }                                                                           \
    shares[0] = masked;                                                         \
    return mv;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
MTPA(BL) FN(dom_mask_many, BL)(                                                 \
        const TYPE* values,                                                     \
        const uint8_t count,                                                    \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    if (!values || count < 2 || order == 0 || order > MAX_SEC_ORDER)            \
        return NULL;                                                            \
                                                                                \
    const size_t align = alignof(TYPE);                                         \
    MTPA(BL) mvs = aligned_alloc(align, count * sizeof(*mvs));                  \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    for (uint32_t i = 0; i < count; ++i) {                                      \
        mvs[i] = FN(dom_mask, BL)(values[i], order, domain);                    \
        if (!mvs[i]) {                                                          \
            FN(dom_free_many, BL)(mvs, i, true);                                \
            return NULL;                                                        \
        }                                                                       \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_unmask, BL)(MTP(BL) mv, TYPE* out, uint8_t index)                    \
{                                                                               \
    if (!mv || !out)                                                            \
        return -1;                                                              \
    TYPE* shares = mv->shares;                                                  \
    TYPE result = shares[0];                                                    \
    if (mv->domain == DOMAIN_BOOLEAN) {  /* XOR unmasking */                    \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            result ^= shares[i];                                                \
        }                                                                       \
    } else { /* DOMAIN_ARITHMETIC - additive unmasking */                       \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            result += shares[i];                                                \
        }                                                                       \
    }                                                                           \
    out[index] = result;                                                        \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_unmask_many, BL)(MTPA(BL) mvs, TYPE* out, uint8_t count)             \
{                                                                               \
    if (!mvs || !out || count < 2)                                              \
        return -1;                                                              \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        MTP(BL) mv = mvs[i];                                                    \
        int rc = FN(dom_unmask, BL)(mv, out, i);                                \
        if (rc)                                                                 \
            return rc;                                                          \
    }                                                                           \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_refresh, BL)(MTP(BL) mv)                                             \
{                                                                               \
    if (!mv)                                                                    \
        return -1;                                                              \
    TYPE rnd[mv->order];                                                        \
    uint8_t order_bytes = mv->order * sizeof(TYPE);                             \
                                                                                \
    int rc = csprng_read_array((uint8_t*)rnd, order_bytes);                     \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    TYPE* shares = (TYPE*)mv->shares;                                           \
    if (mv->domain == DOMAIN_BOOLEAN) {                                         \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            TYPE rand_val = rnd[i - 1];                                         \
            shares[0] ^= rand_val;                                              \
            shares[i] ^= rand_val;                                              \
        }                                                                       \
    } else { /* DOMAIN_ARITHMETIC */                                            \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            TYPE rand_val = rnd[i - 1];                                         \
            shares[0] -= rand_val;                                              \
            shares[i] += rand_val;                                              \
        }                                                                       \
    }                                                                           \
    secure_memzero(rnd, order_bytes);                                           \
    return 0;                                                                   \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_refresh_many, BL)(MTPA(BL) mvs, uint8_t count)                       \
{                                                                               \
    if (!mvs || count < 2)                                                      \
        return -1;                                                              \
    int rc = 0;                                                                 \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        rc = FN(dom_refresh, BL)(mvs[i]);                                       \
        if (rc)                                                                 \
            return rc;                                                          \
    }                                                                           \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
MTP(BL) FN(dom_clone, BL)(const MTP(BL) mv, const bool clear_shares)            \
{                                                                               \
    if (!mv)                                                                    \
        return NULL;                                                            \
    const size_t align = sizeof(void *);                                        \
    MTP(BL) clone = aligned_alloc(align, mv->total_bytes);                      \
    if (!clone)                                                                 \
        return NULL;                                                            \
                                                                                \
    memcpy(clone, mv, mv->total_bytes);                                         \
    if (clear_shares)                                                           \
        secure_memzero(clone->shares, clone->share_bytes);                      \
    return clone;                                                               \
}                                                                               \
                                                                                \
                                                                                \
MTPA(BL) FN(dom_clone_many, BL)(                                                \
        const MTP(BL) mv,                                                       \
        const uint8_t count,                                                    \
        const bool clear_shares                                                 \
) {                                                                             \
    if (!mv || count < 2)                                                       \
        return NULL;                                                            \
                                                                                \
    const size_t align = alignof(TYPE);                                         \
    MTPA(BL) mvs = aligned_alloc(align, count * sizeof(*mvs));                  \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        mvs[i] = FN(dom_clone, BL)(mv, clear_shares);                           \
        if (!mvs[i]) {                                                          \
            FN(dom_free_many, BL)(mvs, i, true);                                \
            return NULL;                                                        \
        }                                                                       \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \

#endif //DOM_CORE


DOM_CORE(uint8_t, 8, BIT_LENGTH_8)
DOM_CORE(uint16_t, 16, BIT_LENGTH_16)
DOM_CORE(uint32_t, 32, BIT_LENGTH_32)
DOM_CORE(uint64_t, 64, BIT_LENGTH_64)
