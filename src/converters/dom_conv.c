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

#include "dom_api.h"
#include "internal/dom_internal_defs.h"


#define DOM_CONV(BL)                                                            \
                                                                                \
ECODE FN(dom_conv, BL)(MTP(BL) mv, const domain_t target_domain)                \
{                                                                               \
    IF_NULL_PTR_RETURN_ECODE(mv, DOM_FUNC_CONV, 0xAA00)                         \
    if (mv->domain == target_domain) {                                          \
        return DOM_OK;                                                          \
    }                                                                           \
    ECODE (*conv)(MTP(BL)) = target_domain == DOMAIN_BOOLEAN                    \
        ? FN(dom_conv_atob, BL)                                                 \
        : FN(dom_conv_btoa, BL);                                                \
    return conv(mv);                                                            \
}                                                                               \
                                                                                \
                                                                                \
ECODE FN(dom_conv_many, BL)(                                                    \
        MTPA(BL) mvs,                                                           \
        const uint8_t count,                                                    \
        const domain_t target_domain                                            \
) {                                                                             \
    IF_NULL_PTR_RETURN_ECODE(mvs, DOM_FUNC_CONV_MANY, 0xAA11)                   \
    IF_NULL_PTR_RETURN_ECODE(mvs[0], DOM_FUNC_CONV_MANY, 0xAA22)                \
    IF_COND_RETURN_ECODE(count < 1, DOM_FUNC_CONV_MANY, 0xAA33)                 \
                                                                                \
    const uint8_t pair_count = count - 1;                                       \
    ECODE (*conv)(MTP(BL)) = target_domain == DOMAIN_BOOLEAN                    \
        ? FN(dom_conv_atob, BL)                                                 \
        : FN(dom_conv_btoa, BL);                                                \
                                                                                \
    ECODE ecode = DOM_OK;                                                       \
    for (uint8_t i = 0; i < pair_count; ++i) {                                  \
        MTP(BL) a = mvs[i];                                                     \
        MTP(BL) b = mvs[i+1];                                                   \
                                                                                \
        IF_NULL_PTR_RETURN_ECODE(b, DOM_FUNC_CONV_MANY, 0xAA44)                 \
        if (a->sig != b->sig) {                                                 \
            return get_dom_error_code(                                          \
                DOM_ERROR_SIG_MISMATCH, DOM_FUNC_CONV_MANY, 0xAA55              \
            );                                                                  \
        }                                                                       \
                                                                                \
        ecode = conv(a);                                                        \
        IF_ECODE_UPDATE_RETURN(ecode, DOM_FUNC_CONV_MANY, 0xAA66)               \
    }                                                                           \
    return conv(mvs[pair_count]);                                               \
}                                                                               \


DOM_CONV(8)
DOM_CONV(16)
DOM_CONV(32)
DOM_CONV(64)
