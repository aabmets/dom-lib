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

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


#define DOM_CORE_MASKING(TYPE, BL)                                              \
                                                                                \
RES_MTP(BL) FN(dom_mask, BL)(                                                   \
        const TYPE value,                                                       \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    RES_MTP(BL) res = FN(dom_alloc, BL)(order, domain);                         \
    IF_ERES_UPDATE_RETURN(res, DOM_FUNC_MASK, 32)                               \
                                                                                \
    TYPE* shares = res.mv->shares;                                              \
    const uint8_t share_count = res.mv->share_count;                            \
    const uint32_t order_bytes = order * sizeof(TYPE);                          \
                                                                                \
    const ECODE ecode = csprng_read_array((uint8_t*)&shares[1], order_bytes);   \
    if (ecode) {                                                                \
        FN(dom_free, BL)(res.mv);                                               \
        res.error = set_dom_error_location(ecode, DOM_FUNC_MASK, 41);           \
        return res;                                                             \
    }                                                                           \
    TYPE masked = value;                                                        \
    if (domain == DOMAIN_BOOLEAN) {  /* XOR masking */                          \
        for (uint8_t i = 1; i < share_count; ++i) {                             \
            masked ^= shares[i];                                                \
        }                                                                       \
    } else {  /* DOMAIN_ARITHMETIC - subtractive masking */                     \
        for (uint8_t i = 1; i < share_count; ++i) {                             \
            masked -= shares[i];                                                \
        }                                                                       \
    }                                                                           \
    shares[0] = masked;                                                         \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTPA(BL) FN(dom_mask_many, BL)(                                             \
        TYPE* values,                                                           \
        const uint8_t count,                                                    \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    INIT_RES_MTPA(BL)                                                           \
    IF_NULL_PTR_RETURN_ERES(values, DOM_FUNC_MASK_MANY, 66)                     \
    IF_COND_RETURN_ERES(count < 1, DOM_FUNC_MASK_MANY, 67)                      \
                                                                                \
    MTPA(BL) mvs;                                                               \
    size_t array_bytes = count * sizeof(*mvs);                                  \
    array_bytes = COMPUTE_ARR_ALLOC_SIZE(array_bytes);                          \
                                                                                \
    mvs = aligned_alloc(SOV, array_bytes);                                      \
    IF_NO_MEM_RETURN_ERES(mvs, DOM_FUNC_MASK_MANY, 74)                          \
                                                                                \
    for (uint32_t i = 0; i < count; ++i) {                                      \
        const RES_MTP(BL) mtp = FN(dom_mask, BL)(values[i], order, domain);     \
        if (mtp.error) {                                                        \
            FN(alloc_many_error_cleanup, BL)(mvs, i);                           \
            res.error = set_dom_error_location(                                 \
                mtp.error, DOM_FUNC_MASK_MANY, 81                               \
            );                                                                  \
            return res;                                                         \
        }                                                                       \
        mvs[i] = mtp.mv;                                                        \
    }                                                                           \
    res.count = count;                                                          \
    res.mvs = mvs;                                                              \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_unmask, BL)(MTP(BL) mv, TYPE* out, const uint8_t index)            \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_UNMASK, 95)                           \
    IF_NULL_PTR_RETURN_ECODE(out, DOM_FUNC_UNMASK, 96)                          \
                                                                                \
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
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_unmask_many, BL)(MTPA(BL) mvs, TYPE* out, const uint8_t count)     \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mvs, DOM_FUNC_UNMASK_MANY, 116)                    \
    IF_NULL_PTR_RETURN_ECODE(out, DOM_FUNC_UNMASK_MANY, 117)                    \
    IF_COND_RETURN_ECODE(count < 1, DOM_FUNC_UNMASK_MANY, 118)                  \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        MTP(BL) mv = mvs[i];                                                    \
        const ECODE ecode = FN(dom_unmask, BL)(mv, out, i);                     \
        if (ecode)                                                              \
            return ecode;                                                       \
    }                                                                           \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_refresh, BL)(MTP(BL) mv)                                           \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_REFRESH, 130)                         \
                                                                                \
    TYPE rnd[mv->order];                                                        \
    uint8_t order_bytes = mv->order * sizeof(TYPE);                             \
                                                                                \
    ECODE ecode = csprng_read_array((uint8_t*)rnd, order_bytes);                \
    if (ecode)                                                                  \
        return ecode;                                                           \
                                                                                \
    TYPE* shares = mv->shares;                                                  \
    if (mv->domain == DOMAIN_BOOLEAN) {                                         \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            const TYPE rand_val = rnd[i - 1];                                   \
            shares[0] ^= rand_val;                                              \
            shares[i] ^= rand_val;                                              \
        }                                                                       \
    } else { /* DOMAIN_ARITHMETIC */                                            \
        for (uint8_t i = 1; i < mv->share_count; ++i) {                         \
            const TYPE rand_val = rnd[i - 1];                                   \
            shares[0] -= rand_val;                                              \
            shares[i] += rand_val;                                              \
        }                                                                       \
    }                                                                           \
    secure_memzero(rnd, order_bytes);                                           \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_refresh_many, BL)(MTPA(BL) mvs, uint8_t count)                     \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mvs, DOM_FUNC_REFRESH_MANY, 160)                   \
    IF_COND_RETURN_ECODE(count < 1, DOM_FUNC_REFRESH_MANY, 161)                 \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const ECODE ecode = FN(dom_refresh, BL)(mvs[i]);                        \
        if (ecode)                                                              \
            return ecode;                                                       \
    }                                                                           \
    return DOM_OK;                                                              \
}                                                                               \


DOM_CORE_MASKING(uint8_t, 8)
DOM_CORE_MASKING(uint16_t, 16)
DOM_CORE_MASKING(uint32_t, 32)
DOM_CORE_MASKING(uint64_t, 64)
