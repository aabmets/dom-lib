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


#define DOM_CORE_MEMORY(TYPE, BL)                                               \
                                                                                \
static const size_t ALIGN(BL) =                                                 \
    (alignof(TYPE) < sizeof(void*)) ? sizeof(void*) : alignof(TYPE);            \
                                                                                \
                                                                                \
RES_MTP(BL) FN(dom_alloc, BL)(const uint8_t order, const domain_t domain)       \
{                                                                               \
    INIT_RES_MTP(BL)                                                            \
    VALIDATE_DOM_ORDER(DOM_FUNC_ALLOC, 33)                                      \
    VALIDATE_DOM_DOMAIN(DOM_FUNC_ALLOC, 34)                                     \
                                                                                \
    const uint8_t share_count = order + 1;                                      \
    const uint16_t share_bytes = share_count * sizeof(TYPE);                    \
    size_t total_bytes = share_bytes + sizeof(MT(BL));                          \
    total_bytes = COMPUTE_MTP_ALLOC_SIZE(total_bytes, BL);                      \
                                                                                \
    MTP(BL) mv = aligned_alloc(ALIGN(BL), total_bytes);                         \
    IF_NO_MEM_RETURN_ERES(mv, DOM_FUNC_ALLOC, 42)                               \
                                                                                \
    mv->bit_length = BL_ENUM(BL);                                               \
    mv->total_bytes = total_bytes;                                              \
    mv->domain = domain;                                                        \
    mv->order = order;                                                          \
    mv->share_count = share_count;                                              \
    mv->share_bytes = share_bytes;                                              \
    mv->sig = (uint16_t)order << 8 | BL_ENUM(BL);                               \
    secure_memzero(mv->shares, share_bytes);                                    \
                                                                                \
    res.mv = mv;                                                                \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTPA(BL) FN(dom_alloc_many, BL)(                                            \
        const uint8_t count,                                                    \
        const uint8_t order,                                                    \
        const domain_t domain                                                   \
) {                                                                             \
    INIT_RES_MTPA(BL)                                                           \
    IF_COND_RETURN_ERES(count < 1, DOM_FUNC_ALLOC_MANY, 64)                     \
                                                                                \
    MTPA(BL) mvs;                                                               \
    size_t array_bytes = count * sizeof(*mvs);                                  \
    array_bytes = COMPUTE_ARR_ALLOC_SIZE(array_bytes);                          \
                                                                                \
    mvs = aligned_alloc(SOV, array_bytes);                                      \
    IF_NO_MEM_RETURN_ERES(mvs, DOM_FUNC_ALLOC_MANY, 71)                         \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const RES_MTP(BL) mtp = FN(dom_alloc, BL)(order, domain);               \
        if (mtp.error) {                                                        \
            FN(alloc_many_error_cleanup, BL)(mvs, i);                           \
            res.error = set_dom_error_location(                                 \
                mtp.error, DOM_FUNC_ALLOC_MANY, 78                              \
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
RES_MTP(BL) FN(dom_clone, BL)(MTP(BL) mv, const bool clear_shares)              \
{                                                                               \
    INIT_RES_MTP(BL)                                                            \
    IF_NULL_PTR_RETURN_ERES(mv, DOM_FUNC_CLONE, 93)                             \
                                                                                \
    MTP(BL) clone = aligned_alloc(ALIGN(BL), mv->total_bytes);                  \
    IF_NO_MEM_RETURN_ERES(clone, DOM_FUNC_CLONE, 96)                            \
                                                                                \
    memcpy(clone, mv, mv->total_bytes);                                         \
    if (clear_shares)                                                           \
        secure_memzero(clone->shares, clone->share_bytes);                      \
    res.mv = clone;                                                             \
    return res;                                                                 \
}                                                                               \
                                                                                \
                                                                                \
RES_MTPA(BL) FN(dom_clone_many, BL)(                                            \
        MTP(BL) mv,                                                             \
        const uint8_t count,                                                    \
        const bool clear_shares                                                 \
) {                                                                             \
    INIT_RES_MTPA(BL)                                                           \
    IF_NULL_PTR_RETURN_ERES(mv, DOM_FUNC_CLONE_MANY, 112)                       \
    IF_COND_RETURN_ERES(count < 1, DOM_FUNC_CLONE_MANY, 113)                    \
                                                                                \
    MTPA(BL) mvs;                                                               \
    size_t array_bytes = count * sizeof(*mvs);                                  \
    array_bytes = COMPUTE_ARR_ALLOC_SIZE(array_bytes);                          \
                                                                                \
    mvs = aligned_alloc(SOV, array_bytes);                                      \
    IF_NO_MEM_RETURN_ERES(mvs, DOM_FUNC_CLONE_MANY, 120)                        \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const RES_MTP(BL) mtp = FN(dom_clone, BL)(mv, clear_shares);            \
        if (mtp.error) {                                                        \
            FN(alloc_many_error_cleanup, BL)(mvs, i);                           \
            res.error = set_dom_error_location(                                 \
                mtp.error, DOM_FUNC_CLONE_MANY, 127                             \
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
ECODE FN(dom_free, BL)(MTP(BL) mv)                                              \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_FREE, 141)                            \
                                                                                \
    secure_memzero((void*)mv, mv->total_bytes);                                 \
    aligned_free((void*)mv);                                                    \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_free_many, BL)(                                                    \
        MTPA(BL) mvs,                                                           \
        const uint8_t count,                                                    \
        const bool free_array                                                   \
) {                                                                             \
    IF_NULL_PTR_RETURN_ECODE(mvs, DOM_FUNC_FREE_MANY, 154)                      \
    IF_COND_RETURN_ECODE(count < 1, DOM_FUNC_FREE_MANY, 155)                    \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const ECODE ecode = FN(dom_free, BL)(mvs[i]);                           \
        IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_FREE_MANY, 159)                  \
    }                                                                           \
    if (free_array)                                                             \
        aligned_free(mvs);                                                      \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_clear, BL)(MTP(BL) mv)                                             \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_CLEAR, 169)                           \
                                                                                \
    secure_memzero(mv->shares, mv->share_bytes);                                \
    return DOM_OK;                                                              \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_clear_many, BL)(MTPA(BL) mvs, const uint8_t count)                 \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mvs, DOM_FUNC_CLEAR_MANY, 178)                     \
    IF_COND_RETURN_ECODE(count < 1, DOM_FUNC_CLEAR_MANY, 179)                   \
                                                                                \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const ECODE ecode = FN(dom_clear, BL)(mvs[i]);                          \
        IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CLEAR_MANY, 183)                 \
    }                                                                           \
    return DOM_OK;                                                              \
}                                                                               \


DOM_CORE_MEMORY(uint8_t, 8)
DOM_CORE_MEMORY(uint16_t, 16)
DOM_CORE_MEMORY(uint32_t, 32)
DOM_CORE_MEMORY(uint64_t, 64)
