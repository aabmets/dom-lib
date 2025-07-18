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


#ifndef DOM_CONV
#define DOM_CONV(BL)                                                            \
                                                                                \
int FN(dom_conv, BL)(MTP(BL) mv, domain_t target_domain)                        \
{                                                                               \
    if (mv) {                                                                   \
        if (mv->domain == target_domain)                                        \
            return 0;                                                           \
        int(*conv)(MTP(BL)) = target_domain == DOMAIN_BOOLEAN                   \
            ? FN(dom_conv_atob, BL)                                             \
            : FN(dom_conv_btoa, BL);                                            \
        return conv(mv);                                                        \
    }                                                                           \
    return -1;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_conv_many, BL)(                                                      \
        MTPA(BL) mvs,                                                           \
        uint8_t count,                                                          \
        domain_t target_domain                                                  \
) {                                                                             \
    if (!mvs || count < 2)                                                      \
        return -1;                                                              \
                                                                                \
    int rc = 0;                                                                 \
    const uint8_t pair_count = count - 1;                                       \
    int(*conv)(MTP(BL)) = target_domain == DOMAIN_BOOLEAN                       \
        ? FN(dom_conv_atob, BL)                                                 \
        : FN(dom_conv_btoa, BL);                                                \
                                                                                \
    for (uint8_t i = 0; i < pair_count; ++i) {                                  \
        MTP(BL) a = mvs[i];                                                     \
        MTP(BL) b = mvs[i+1];                                                   \
                                                                                \
        if (!a || !b || a->sig != b->sig)                                       \
            return -1;                                                          \
                                                                                \
        rc = conv(a);                                                           \
        if (rc)                                                                 \
            return rc;                                                          \
    }                                                                           \
    return conv(mvs[pair_count]);                                               \
}                                                                               \

#endif //DOM_CONV


DOM_CONV(8)
DOM_CONV(16)
DOM_CONV(32)
DOM_CONV(64)
