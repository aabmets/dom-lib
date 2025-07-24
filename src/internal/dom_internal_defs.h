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

#ifndef DOM_INTERNAL_DEFS_H
#define DOM_INTERNAL_DEFS_H


#include <stdint.h>

#define MAX_SEC_ORDER   30      // higher orders are impractical
#define ERR_MSG_LENGTH  100     // must be kept up to date with messages in dom_errors.h

#define GLUE(a,b)           a##b
#define XGLUE(a,b)          GLUE(a,b)

#define STR(s)              #s
#define XSTR(s)             STR(s)

#define TYPE(T, BL)         XGLUE(T, XGLUE(BL, _t))             // builtin or custom type
#define UINT(BL)            TYPE(uint, BL)                      // unsigned integer type
#define MT(BL)              XGLUE(masked_, UINT(BL))            // masked type struct
#define MTP(BL)             MT(BL) *                            // pointer to a masked type struct
#define MTPA(BL)            MT(BL) **                           // array of pointers to masked type structs

#define RES_UINT(BL)        XGLUE(result_, UINT(BL))            // result struct for UINT(BL)
#define RES_MTP(BL)         XGLUE(result_, TYPE(mtp, BL))       // result struct for MTP(BL)
#define RES_MTPA(BL)        XGLUE(result_, TYPE(mtpa, BL))      // result struct for MTPA(BL)

#define STS(BL)             XGLUE(_u, BL)                       // short type suffix
#define FN(NAME, BL)        XGLUE(NAME, STS(BL))                // function name with type specifier
#define CONV(BL1, BL2)      XGLUE(STS(BL1), FN(_to, BL2))       // type conversion, expands to '_uXX_to_uYY'
#define FNCT(BL1, BL2)      XGLUE(dom_conv, CONV(BL1, BL2))     // type conversion function name

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define THREAD_LOCAL _Thread_local
#elif defined(__cplusplus)
    #define THREAD_LOCAL thread_local
#elif defined(__GNUC__) || defined(__clang__)
    #define THREAD_LOCAL __thread
#else
    #error "No thread-local storage keyword available"
#endif


#endif //DOM_INTERNAL_DEFS_H
