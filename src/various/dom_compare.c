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

#include <string.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"


#ifndef DOM_COMPARE
#define DOM_COMPARE(BL)                                                         \
                                                                                \
int FN(dom_cmp_lt, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    int rc = 0;                                                                 \
                                                                                \
    MTP(BL) mvs[3] = { a, b, out };                                             \
    rc = FN(dom_conv_many, BL)(mvs, 3, DOMAIN_BOOLEAN);                         \
    if (rc)                                                                     \
        return rc;                                                              \
                                                                                \
    MTPA(BL) tmp = FN(dom_alloc_many, BL)(5, out->order, DOMAIN_BOOLEAN);       \
    if (!tmp)                                                                   \
        return -1;                                                              \
                                                                                \
    MTP(BL) t0 = tmp[0];                                                        \
    MTP(BL) t1 = tmp[1];                                                        \
    MTP(BL) t2 = tmp[2];                                                        \
    MTP(BL) t3 = tmp[3];                                                        \
    MTP(BL) diff = tmp[4];                                                      \
                                                                                \
    rc = FN(dom_bool_sub, BL)(a, b, diff);                                      \
    if (!rc) {  /* no error */                                                  \
        FN(dom_bool_xor, BL)(a, b, t0);                                         \
        FN(dom_bool_xor, BL)(diff, b, t1);                                      \
                                                                                \
        rc = FN(dom_bool_or, BL)(t0, t1, t2);                                   \
        if (rc)                                                                 \
            goto cleanup;                                                       \
                                                                                \
        FN(dom_bool_xor, BL)(a, t2, t3);                                        \
        FN(dom_bool_shr, BL)(t3, BL - 1);                                       \
                                                                                \
        memcpy(out->shares, t3->shares, t3->share_bytes);                       \
        rc = FN(dom_refresh, BL)(out);                                          \
    }                                                                           \
                                                                                \
    cleanup:                                                                    \
    FN(dom_free_many, BL)(tmp, 5, true);                                        \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_cmp_le, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    int rc = FN(dom_cmp_lt, BL)(b, a, out);  /* NOLINT */                       \
    if (!rc)                                                                    \
        out->shares[0] ^= 0x1;                                                  \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_cmp_gt, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    return FN(dom_cmp_lt, BL)(b, a, out);  /* NOLINT */                         \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_cmp_ge, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    int rc = FN(dom_cmp_lt, BL)(a, b, out);                                     \
    if (!rc)                                                                    \
        out->shares[0] ^= 0x1;                                                  \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_cmp_ne, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    MTPA(BL) tmp = FN(dom_alloc_many, BL)(2, out->order, DOMAIN_BOOLEAN);       \
    if (!tmp)                                                                   \
        return -1;                                                              \
                                                                                \
    MTP(BL) lt_ab = tmp[0];                                                     \
    MTP(BL) lt_ba = tmp[1];                                                     \
                                                                                \
    int rc  = FN(dom_cmp_lt, BL)(a, b, lt_ab);                                  \
    if (!rc) {                                                                  \
        rc = FN(dom_cmp_lt, BL)(b, a, lt_ba);  /* NOLINT */                     \
        if (!rc) {                                                              \
            rc = FN(dom_bool_xor, BL)(lt_ab, lt_ba, out);                       \
        }                                                                       \
    }                                                                           \
    FN(dom_free_many, BL)(tmp, 2, true);                                        \
    asm volatile ("" ::: "memory");                                             \
    return rc;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
int FN(dom_cmp_eq, BL)(MTP(BL) a, MTP(BL) b, MTP(BL) out) {                     \
    int rc = FN(dom_cmp_ne, BL)(a, b, out);                                     \
    if (!rc)                                                                    \
        out->shares[0] ^= 0x1;                                                  \
    return rc;                                                                  \
}                                                                               \

#endif //DOM_COMPARE


DOM_COMPARE(8)
DOM_COMPARE(16)
DOM_COMPARE(32)
DOM_COMPARE(64)
