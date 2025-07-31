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


#define DOM_KSA(BL)                                                             \
                                                                                \
ECODE FN(dom_ksa_carry, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                \
    ECODE ecode = DOM_OK;                                                       \
    uint8_t state = 0x0;                                                        \
                                                                                \
    MTP(BL) mvs[] = { a, b, out };                                              \
    ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                      \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA00)                    \
                                                                                \
    RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a, 5, true);                   \
    IF_ECODE_GOTO_CLEANUP(clones.error, DOM_FUNC_KSA_CARRY, 0xAA11)             \
                                                                                \
    state = 0xFF;                                                               \
    MTPA(BL) c_mvs = clones.mvs;                                                \
    MTP(BL) p = c_mvs[0];                                                       \
    MTP(BL) g = c_mvs[1];                                                       \
    MTP(BL) tmp = c_mvs[2];                                                     \
    MTP(BL) p_shift = c_mvs[3];                                                 \
    MTP(BL) g_shift = c_mvs[4];                                                 \
                                                                                \
    FN(dom_bool_xor, BL)(a, b, p);                                              \
    ecode = FN(dom_bool_and, BL)(a, b, g);                                      \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA22)                    \
                                                                                \
    const uint8_t bl = (uint8_t)a->bit_length;                                  \
    for (uint8_t dist = 1; dist < bl; dist <<= 1) {                             \
        memcpy(p_shift->shares, p->shares, p->share_bytes);                     \
        memcpy(g_shift->shares, g->shares, g->share_bytes);                     \
                                                                                \
        FN(dom_bool_shl, BL)(p_shift, dist);                                    \
        FN(dom_bool_shl, BL)(g_shift, dist);                                    \
                                                                                \
        ecode = FN(dom_bool_and, BL)(p, g_shift, tmp);                          \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA33)                \
                                                                                \
        FN(dom_bool_xor, BL)(g, tmp, g);                                        \
        secure_memzero(tmp->shares, tmp->share_bytes);                          \
                                                                                \
        ecode = FN(dom_bool_and, BL)(p, p_shift, p);                            \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA44)                \
    }                                                                           \
    FN(dom_bool_shl, BL)(g, 1);                                                 \
    memcpy(out->shares, g->shares, g->share_bytes);                             \
                                                                                \
    cleanup:                                                                    \
    if (state == 0xFF) {                                                        \
        FN(dom_free_many, BL)(c_mvs, 5, true);                                  \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_ksa_borrow, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {               \
    ECODE ecode = DOM_OK;                                                       \
    uint8_t state = 0x0;                                                        \
                                                                                \
    MTP(BL) mvs[] = { a, b, out };                                              \
    ecode = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                      \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_BORROW, 0xAA55)                   \
                                                                                \
    RES_MTPA(BL) clones = FN(dom_clone_many, BL)(a, 6, false);                  \
    IF_ECODE_GOTO_CLEANUP(clones.error, DOM_FUNC_KSA_BORROW, 0xAA66)            \
                                                                                \
    state = 0xFF;                                                               \
    MTPA(BL) c_mvs = clones.mvs;                                                \
    MTP(BL) p = c_mvs[0];                                                       \
    MTP(BL) g = c_mvs[1];                                                       \
    MTP(BL) tmp = c_mvs[2];                                                     \
    MTP(BL) p_shift = c_mvs[3];                                                 \
    MTP(BL) g_shift = c_mvs[4];                                                 \
    MTP(BL) a_inv = c_mvs[5];                                                   \
                                                                                \
    FN(dom_bool_not, BL)(a_inv);                                                \
    FN(dom_bool_xor, BL)(a_inv, b, p);                                          \
    ecode = FN(dom_bool_and, BL)(a_inv, b, g);                                  \
    IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA77)                    \
                                                                                \
    const uint8_t bl = (uint8_t)a->bit_length;                                  \
    for (uint8_t dist = 1; dist < bl; dist <<= 1) {                             \
        memcpy(p_shift->shares, p->shares, p->share_bytes);                     \
        memcpy(g_shift->shares, g->shares, g->share_bytes);                     \
                                                                                \
        FN(dom_bool_shl, BL)(p_shift, dist);                                    \
        FN(dom_bool_shl, BL)(g_shift, dist);                                    \
                                                                                \
        ecode = FN(dom_bool_and, BL)(p, g_shift, tmp);                          \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA88)                \
                                                                                \
        ecode = FN(dom_bool_and, BL)(g, tmp, g_shift);                          \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xAA99)                \
                                                                                \
        FN(dom_bool_xor, BL)(g, tmp, g);                                        \
        secure_memzero(tmp->shares, tmp->share_bytes);                          \
        FN(dom_bool_xor, BL)(g, g_shift, g);                                    \
                                                                                \
        ecode = FN(dom_bool_and, BL)(p, p_shift, p);                            \
        IF_ECODE_GOTO_CLEANUP(ecode, DOM_FUNC_KSA_CARRY, 0xBB00)                \
    }                                                                           \
    FN(dom_bool_shl, BL)(g, 1);                                                 \
    memcpy(out->shares, g->shares, g->share_bytes);                             \
                                                                                \
    cleanup:                                                                    \
    if (state == 0xFF) {                                                        \
        FN(dom_free_many, BL)(c_mvs, 6, true);                                  \
    }                                                                           \
    asm volatile ("" ::: "memory");                                             \
    return ecode;                                                               \
}                                                                               \


DOM_KSA(8)
DOM_KSA(16)
DOM_KSA(32)
DOM_KSA(64)
