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

#ifndef DOM_API_H
#define DOM_API_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "dom_types.h"
#include "dom_errors.h"
#include "internal/dom_internal_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


static_assert(
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
    "Target platform must be little-endian"
);


// ---------------------------------------------------------------------------------------------------------------------
#define MASKING_FUNCTIONS(TYPE, BL)                                                                                     \
                                                                                                                        \
RES_MTP(BL)     FN(dom_alloc, BL)           (uint8_t order, domain_t domain);                                           \
RES_MTPA(BL)    FN(dom_alloc_many, BL)      (uint8_t count, uint8_t order, domain_t domain);                            \
RES_MTP(BL)     FN(dom_clone, BL)           (MTP(BL) mv, bool clear_shares);                                            \
RES_MTPA(BL)    FN(dom_clone_many, BL)      (MTP(BL) mv, uint8_t count, bool clear_shares);                             \
ECODE           FN(dom_free, BL)            (MTP(BL) mv);                                                               \
ECODE           FN(dom_free_many, BL)       (MTPA(BL) mvs, uint8_t count, bool free_array);                             \
ECODE           FN(dom_clear, BL)           (MTP(BL) mv);                                                               \
ECODE           FN(dom_clear_many, BL)      (MTPA(BL) mvs, uint8_t count);                                              \
                                                                                                                        \
RES_MTP(BL)     FN(dom_mask, BL)            (TYPE value, uint8_t order, domain_t domain);                               \
RES_MTPA(BL)    FN(dom_mask_many, BL)       (TYPE* values, uint8_t count, uint8_t order, domain_t domain);              \
ECODE           FN(dom_unmask, BL)          (MTP(BL) mv, TYPE* out, uint8_t index);                                     \
ECODE           FN(dom_unmask_many, BL)     (MTPA(BL) mvs, TYPE* out, uint8_t count);                                   \
ECODE           FN(dom_refresh, BL)         (MTP(BL) mv);                                                               \
ECODE           FN(dom_refresh_many, BL)    (MTPA(BL) mvs, uint8_t count);                                              \
                                                                                                                        \
ECODE           FN(dom_conv, BL)            (MTP(BL) mv, domain_t target_domain);                                       \
ECODE           FN(dom_conv_many, BL)       (MTPA(BL) mvs, uint8_t count, domain_t target_domain);                      \
ECODE           FN(dom_conv_btoa, BL)       (MTP(BL) mv);                                                               \
ECODE           FN(dom_conv_atob, BL)       (MTP(BL) mv);                                                               \
                                                                                                                        \
ECODE           FN(dom_ksa_carry, BL)       (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_ksa_borrow, BL)      (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
                                                                                                                        \
ECODE           FN(dom_bool_and, BL)        (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_bool_or, BL)         (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_bool_xor, BL)        (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_bool_not, BL)        (MTP(BL) mv);                                                               \
ECODE           FN(dom_bool_shr, BL)        (MTP(BL) mv, uint8_t n);                                                    \
ECODE           FN(dom_bool_shl, BL)        (MTP(BL) mv, uint8_t n);                                                    \
ECODE           FN(dom_bool_rotr, BL)       (MTP(BL) mv, uint8_t n);                                                    \
ECODE           FN(dom_bool_rotl, BL)       (MTP(BL) mv, uint8_t n);                                                    \
ECODE           FN(dom_bool_add, BL)        (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_bool_sub, BL)        (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
                                                                                                                        \
ECODE           FN(dom_arith_add, BL)       (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_arith_sub, BL)       (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
ECODE           FN(dom_arith_mult, BL)      (MTP(BL) a, MTP(BL) b, MTP(BL) out);                                        \
                                                                                                                        \
ECODE           FN(dom_cmp_lt, BL)          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask);                        \
ECODE           FN(dom_cmp_le, BL)          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask);                        \
ECODE           FN(dom_cmp_gt, BL)          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask);                        \
ECODE           FN(dom_cmp_ge, BL)          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask);                        \
                                                                                                                        \
RES_MTP(BL)     FN(dom_select_lt, BL)       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel);       \
RES_MTP(BL)     FN(dom_select_le, BL)       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel);       \
RES_MTP(BL)     FN(dom_select_gt, BL)       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel);       \
RES_MTP(BL)     FN(dom_select_ge, BL)       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel);       \


// ---------------------------------------------------------------------------------------------------------------------
#define MASKING_FUNCTIONS_CONV_TYPE(BLL, BLS)                                                                           \
                                                                                                                        \
RES_MTP(BLL)    FNCT(BLS, BLL)              (MTPA(BLS) mvs);                                                            \
RES_MTPA(BLS)   FNCT(BLL, BLS)              (MTP(BLL) mv);                                                              \


// ---------------------------------------------------------------------------------------------------------------------
MASKING_FUNCTIONS(uint8_t, 8)
MASKING_FUNCTIONS(uint16_t, 16)
MASKING_FUNCTIONS(uint32_t, 32)
MASKING_FUNCTIONS(uint64_t, 64)

MASKING_FUNCTIONS_CONV_TYPE(64, 32)   // 2/1 ratio
MASKING_FUNCTIONS_CONV_TYPE(32, 16)   // 2/1 ratio
MASKING_FUNCTIONS_CONV_TYPE(16, 8)    // 2/1 ratio
MASKING_FUNCTIONS_CONV_TYPE(64, 16)   // 4/1 ratio
MASKING_FUNCTIONS_CONV_TYPE(32, 8)    // 4/1 ratio
MASKING_FUNCTIONS_CONV_TYPE(64, 8)    // 8/1 ratio


#ifdef __cplusplus
}
#endif

#undef MASKING_FUNCTIONS
#undef MASKING_FUNCTIONS_CONV_TYPE
#include "internal/dom_internal_undefs.h"

#endif //DOM_API_H
