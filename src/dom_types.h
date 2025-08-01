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

#ifndef DOM_TYPES_H
#define DOM_TYPES_H

#include <limits.h>
#include <stdint.h>

#include "internal/dom_internal_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    BIT_LENGTH_8 = CHAR_BIT * sizeof(uint8_t),
    BIT_LENGTH_16 = CHAR_BIT * sizeof(uint16_t),
    BIT_LENGTH_32 = CHAR_BIT * sizeof(uint32_t),
    BIT_LENGTH_64 = CHAR_BIT * sizeof(uint64_t)
} bit_length_t;

typedef enum {
    DOMAIN_BOOLEAN = 0,
    DOMAIN_ARITHMETIC = 1
} domain_t;

#define MASKED_TYPE(BL)             \
typedef struct {                    \
    uint16_t sig;                   \
    bit_length_t bit_length;        \
    uint32_t total_bytes;           \
    domain_t domain;                \
    uint8_t order;                  \
    uint8_t share_count;            \
    uint16_t share_bytes;           \
    UINT(BL) shares[];              \
} MT(BL);                           \
                                    \
typedef struct {                    \
    UINT(BL)* vls;                  \
    uint32_t error;                 \
} RES_VAP(BL);                      \
                                    \
typedef struct {                    \
    MTP(BL) mv;                     \
    uint32_t error;                 \
} RES_MTP(BL);                      \
                                    \
typedef struct {                    \
    MTPA(BL) mvs;                   \
    uint32_t count;                 \
    uint32_t error;                 \
} RES_MTPA(BL);                     \


MASKED_TYPE(8)
MASKED_TYPE(16)
MASKED_TYPE(32)
MASKED_TYPE(64)


#ifdef __cplusplus
}
#endif

#undef MASKED_TYPE
#include "internal/dom_internal_undefs.h"  // NOSONAR

#endif //DOM_TYPES_H
