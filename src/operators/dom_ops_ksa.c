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


#ifndef DOM_KSA
#define DOM_KSA(BL)                                                             \
                                                                                \
int FN(dom_ksa_carry, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                  \
    MTP(BL) mvs[] = { a, b, out };                                              \
    int rc = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                     \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    MTPA(BL) clones = FN(dom_clone_many, BL)(a, 5, true);                       \
    if (!clones)                                                                \
        return -1;                                                              \
                                                                                \
    MTP(BL) p = clones[0];                                                      \
    MTP(BL) g = clones[1];                                                      \
    MTP(BL) tmp = clones[2];                                                    \
    MTP(BL) p_shift = clones[3];                                                \
    MTP(BL) g_shift = clones[4];                                                \
                                                                                \
    FN(dom_bool_xor, BL)(a, b, p);                                              \
    rc = FN(dom_bool_and, BL)(a, b, g);                                         \
    if (rc)                                                                     \
        goto cleanup;                                                           \
                                                                                \
    const uint8_t bl = (uint8_t)a->bit_length;                                  \
    for (uint8_t dist = 1; dist < bl; dist <<= 1) {                             \
        memcpy(p_shift->shares, p->shares, p->share_bytes);                     \
        memcpy(g_shift->shares, g->shares, g->share_bytes);                     \
                                                                                \
        FN(dom_bool_shl, BL)(p_shift, dist);                                    \
        FN(dom_bool_shl, BL)(g_shift, dist);                                    \
                                                                                \
        rc = FN(dom_bool_and, BL)(p, g_shift, tmp);                             \
        if (rc)                                                                 \
            goto cleanup;                                                       \
                                                                                \
        FN(dom_bool_xor, BL)(g, tmp, g);                                        \
        secure_memzero(tmp->shares, tmp->share_bytes);                          \
                                                                                \
        rc = FN(dom_bool_and, BL)(p, p_shift, p);                               \
        if (rc)                                                                 \
            goto cleanup;                                                       \
    }                                                                           \
    FN(dom_bool_shl, BL)(g, 1);                                                 \
    memcpy(out->shares, g->shares, g->share_bytes);                             \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(clones, 5);                                           \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_ksa_borrow, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                 \
    MTP(BL) mvs[] = { a, b, out };                                              \
    int rc = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                     \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    MTPA(BL) clones = FN(dom_clone_many, BL)(a, 6, false);                      \
    if (!clones)                                                                \
        return -1;                                                              \
                                                                                \
    MTP(BL) p = clones[0];                                                      \
    MTP(BL) g = clones[1];                                                      \
    MTP(BL) tmp = clones[2];                                                    \
    MTP(BL) p_shift = clones[3];                                                \
    MTP(BL) g_shift = clones[4];                                                \
    MTP(BL) a_inv = clones[5];                                                  \
                                                                                \
    FN(dom_bool_not, BL)(a_inv);                                                \
    FN(dom_bool_xor, BL)(a_inv, b, p);                                          \
    rc = FN(dom_bool_and, BL)(a_inv, b, g);                                     \
    if (rc)                                                                     \
        goto cleanup;                                                           \
                                                                                \
    const uint8_t bl = (uint8_t)a->bit_length;                                  \
    for (uint8_t dist = 1; dist < bl; dist <<= 1) {                             \
        memcpy(p_shift->shares, p->shares, p->share_bytes);                     \
        memcpy(g_shift->shares, g->shares, g->share_bytes);                     \
                                                                                \
        FN(dom_bool_shl, BL)(p_shift, dist);                                    \
        FN(dom_bool_shl, BL)(g_shift, dist);                                    \
                                                                                \
        rc = FN(dom_bool_and, BL)(p, g_shift, tmp);                             \
        if (rc)                                                                 \
            goto cleanup;                                                       \
                                                                                \
        rc = FN(dom_bool_and, BL)(g, tmp, g_shift);                             \
        if (rc)                                                                 \
            goto cleanup;                                                       \
                                                                                \
        FN(dom_bool_xor, BL)(g, tmp, g);                                        \
        secure_memzero(tmp->shares, tmp->share_bytes);                          \
        FN(dom_bool_xor, BL)(g, g_shift, g);                                    \
                                                                                \
        rc = FN(dom_bool_and, BL)(p, p_shift, p);                               \
        if (rc)                                                                 \
            goto cleanup;                                                       \
    }                                                                           \
    FN(dom_bool_shl, BL)(g, 1);                                                 \
    memcpy(out->shares, g->shares, g->share_bytes);                             \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(clones, 6);                                           \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \

#endif //DOM_KSA


DOM_KSA(8)
DOM_KSA(16)
DOM_KSA(32)
DOM_KSA(64)
