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


#ifndef DOM_TYPE_CONV_2TO1_1TO2
#define DOM_TYPE_CONV_2TO1_1TO2(BLL, BLS)                                       \
                                                                                \
/* dom_conv_type_2to1 */                                                        \
MTP(BLL) FNCT(BLS, BLL)(MTPA(BLS) mvs)                                          \
{                                                                               \
    MTP(BLL) mv = FN(dom_alloc, BLL)(mvs[0]->order, mvs[0]->domain);            \
    if (!mv)                                                                    \
        return NULL;                                                            \
                                                                                \
    UINT(BLL)* out = mv->shares;                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
                                                                                \
    const uint8_t dist = mvs[0]->bit_length;                                    \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        out[i] = (UINT(BLL))s1[i] << dist | s0[i];                              \
    }                                                                           \
    return mv;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
/* dom_conv_type_1to2 */                                                        \
MTPA(BLS) FNCT(BLL, BLS)(MTP(BLL) mv)                                           \
{                                                                               \
    MTPA(BLS) mvs = FN(dom_alloc_many, BLS)(2, mv->order, mv->domain);          \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
                                                                                \
    const uint8_t* p = (uint8_t*)mv->shares;                                    \
    const uint8_t stride = sizeof(UINT(BLL));                                   \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i, p += stride) {                \
        s0[i] = *(UINT(BLS)*)(p);                                               \
        s1[i] = *(UINT(BLS)*)(p + sizeof(UINT(BLS)));                           \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \

#endif //DOM_TYPE_CONV_2TO1_1TO2


#ifndef DOM_TYPE_CONV_4TO1_1TO4
#define DOM_TYPE_CONV_4TO1_1TO4(BLL, BLS)                                       \
                                                                                \
/* dom_conv_type_4to1 */                                                        \
MTP(BLL) FNCT(BLS, BLL)(MTPA(BLS) mvs)                                          \
{                                                                               \
    MTP(BLL) mv = FN(dom_alloc, BLL)(mvs[0]->order, mvs[0]->domain);            \
    if (!mv)                                                                    \
        return NULL;                                                            \
                                                                                \
    UINT(BLL)* out = mv->shares;                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
    UINT(BLS)* s2 = mvs[2]->shares;                                             \
    UINT(BLS)* s3 = mvs[3]->shares;                                             \
                                                                                \
    const uint8_t dist = mvs[0]->bit_length;                                    \
                                                                                \
    uint8_t offset1 = 1 * dist;                                                 \
    uint8_t offset2 = 2 * dist;                                                 \
    uint8_t offset3 = 3 * dist;                                                 \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        out[i] = ((UINT(BLL))s3[i] << offset3)                                  \
               | ((UINT(BLL))s2[i] << offset2)                                  \
               | ((UINT(BLL))s1[i] << offset1)                                  \
               | ((UINT(BLL))s0[i]);                                            \
    }                                                                           \
    return mv;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
/* dom_conv_type_1to4 */                                                        \
MTPA(BLS) FNCT(BLL, BLS)(MTP(BLL) mv)                                           \
{                                                                               \
    MTPA(BLS) mvs = FN(dom_alloc_many, BLS)(4, mv->order, mv->domain);          \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
    UINT(BLS)* s2 = mvs[2]->shares;                                             \
    UINT(BLS)* s3 = mvs[3]->shares;                                             \
                                                                                \
    const uint8_t* p = (uint8_t*)mv->shares;                                    \
    const uint8_t stride = sizeof(UINT(BLL));                                   \
                                                                                \
    uint8_t offset1 = 1 * sizeof(UINT(BLS));                                    \
    uint8_t offset2 = 2 * sizeof(UINT(BLS));                                    \
    uint8_t offset3 = 3 * sizeof(UINT(BLS));                                    \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i, p += stride) {                \
        s0[i] = *(UINT(BLS)*)(p);                                               \
        s1[i] = *(UINT(BLS)*)(p + offset1);                                     \
        s2[i] = *(UINT(BLS)*)(p + offset2);                                     \
        s3[i] = *(UINT(BLS)*)(p + offset3);                                     \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \

#endif //DOM_TYPE_CONV_4TO1_1TO4


#ifndef DOM_TYPE_CONV_8TO1_1TO8
#define DOM_TYPE_CONV_8TO1_1TO8(BLL, BLS)                                       \
                                                                                \
/* dom_conv_type_8to1 */                                                        \
MTP(BLL) FNCT(BLS, BLL)(MTPA(BLS) mvs)                                          \
{                                                                               \
    MTP(BLL) mv = FN(dom_alloc, BLL)(mvs[0]->order, mvs[0]->domain);            \
    if (!mv)                                                                    \
        return NULL;                                                            \
                                                                                \
    UINT(BLL)* out = mv->shares;                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
    UINT(BLS)* s2 = mvs[2]->shares;                                             \
    UINT(BLS)* s3 = mvs[3]->shares;                                             \
    UINT(BLS)* s4 = mvs[4]->shares;                                             \
    UINT(BLS)* s5 = mvs[5]->shares;                                             \
    UINT(BLS)* s6 = mvs[6]->shares;                                             \
    UINT(BLS)* s7 = mvs[7]->shares;                                             \
                                                                                \
    const uint8_t dist = mvs[0]->bit_length;                                    \
                                                                                \
    uint8_t offset1 = 1 * dist;                                                 \
    uint8_t offset2 = 2 * dist;                                                 \
    uint8_t offset3 = 3 * dist;                                                 \
    uint8_t offset4 = 4 * dist;                                                 \
    uint8_t offset5 = 5 * dist;                                                 \
    uint8_t offset6 = 6 * dist;                                                 \
    uint8_t offset7 = 7 * dist;                                                 \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i) {                             \
        out[i] = ((UINT(BLL))s7[i] << offset7)                                  \
               | ((UINT(BLL))s6[i] << offset6)                                  \
               | ((UINT(BLL))s5[i] << offset5)                                  \
               | ((UINT(BLL))s4[i] << offset4)                                  \
               | ((UINT(BLL))s3[i] << offset3)                                  \
               | ((UINT(BLL))s2[i] << offset2)                                  \
               | ((UINT(BLL))s1[i] << offset1)                                  \
               | (UINT(BLL))s0[i];                                              \
    }                                                                           \
    return mv;                                                                  \
}                                                                               \
                                                                                \
                                                                                \
/* dom_conv_type_1to8 */                                                        \
MTPA(BLS) FNCT(BLL, BLS)(MTP(BLL) mv)                                           \
{                                                                               \
    MTPA(BLS) mvs = FN(dom_alloc_many, BLS)(8, mv->order, mv->domain);          \
    if (!mvs)                                                                   \
        return NULL;                                                            \
                                                                                \
    UINT(BLS)* s0 = mvs[0]->shares;                                             \
    UINT(BLS)* s1 = mvs[1]->shares;                                             \
    UINT(BLS)* s2 = mvs[2]->shares;                                             \
    UINT(BLS)* s3 = mvs[3]->shares;                                             \
    UINT(BLS)* s4 = mvs[4]->shares;                                             \
    UINT(BLS)* s5 = mvs[5]->shares;                                             \
    UINT(BLS)* s6 = mvs[6]->shares;                                             \
    UINT(BLS)* s7 = mvs[7]->shares;                                             \
                                                                                \
    const uint8_t* p = (uint8_t*)mv->shares;                                    \
    const uint8_t stride = sizeof(UINT(BLL));                                   \
                                                                                \
    uint8_t offset1 = 1 * sizeof(UINT(BLS));                                    \
    uint8_t offset2 = 2 * sizeof(UINT(BLS));                                    \
    uint8_t offset3 = 3 * sizeof(UINT(BLS));                                    \
    uint8_t offset4 = 4 * sizeof(UINT(BLS));                                    \
    uint8_t offset5 = 5 * sizeof(UINT(BLS));                                    \
    uint8_t offset6 = 6 * sizeof(UINT(BLS));                                    \
    uint8_t offset7 = 7 * sizeof(UINT(BLS));                                    \
                                                                                \
    for (uint8_t i = 0; i < mv->share_count; ++i, p += stride) {                \
        s0[i] = *(UINT(BLS)*)(p);                                               \
        s1[i] = *(UINT(BLS)*)(p + offset1);                                     \
        s2[i] = *(UINT(BLS)*)(p + offset2);                                     \
        s3[i] = *(UINT(BLS)*)(p + offset3);                                     \
        s4[i] = *(UINT(BLS)*)(p + offset4);                                     \
        s5[i] = *(UINT(BLS)*)(p + offset5);                                     \
        s6[i] = *(UINT(BLS)*)(p + offset6);                                     \
        s7[i] = *(UINT(BLS)*)(p + offset7);                                     \
    }                                                                           \
    return mvs;                                                                 \
}                                                                               \

#endif //DOM_TYPE_CONV_8TO1_1TO8


DOM_TYPE_CONV_2TO1_1TO2(64, 32)   // 2/1 ratio
DOM_TYPE_CONV_2TO1_1TO2(32, 16)   // 2/1 ratio
DOM_TYPE_CONV_2TO1_1TO2(16, 8)    // 2/1 ratio
DOM_TYPE_CONV_4TO1_1TO4(64, 16)   // 4/1 ratio
DOM_TYPE_CONV_4TO1_1TO4(32, 8)    // 4/1 ratio
DOM_TYPE_CONV_8TO1_1TO8(64, 8)    // 8/1 ratio
