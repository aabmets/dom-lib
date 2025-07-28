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


// ---------------------------------------------------------------------------------------------------------------------
#define MAX_SEC_ORDER   30      // higher orders are impractical
#define ERR_MSG_LENGTH  100     // must be kept up to date with messages in dom_errors.h

#define GLUE(a,b)           a##b
#define XGLUE(a,b)          GLUE(a,b)

#define STR(s)              #s
#define XSTR(s)             STR(s)

#define BL_ENUM(BL)         BIT_LENGTH_##BL                     // bit length enum
#define TYPE(T, BL)         XGLUE(T, XGLUE(BL, _t))             // builtin or custom type
#define UINT(BL)            TYPE(uint, BL)                      // unsigned integer type
#define MT(BL)              XGLUE(masked_, UINT(BL))            // masked type struct
#define MTP(BL)             MT(BL) *                            // pointer to a masked type struct
#define MTPA(BL)            MT(BL) **                           // array of pointers to masked type structs

#define STS(BL)             XGLUE(_u, BL)                       // short type suffix
#define FN(NAME, BL)        XGLUE(NAME, STS(BL))                // function name with type specifier
#define CONV(BL1, BL2)      XGLUE(STS(BL1), FN(_to, BL2))       // type conversion, expands to '_uXX_to_uYY'
#define FNCT(BL1, BL2)      XGLUE(dom_conv, CONV(BL1, BL2))     // type conversion function name

#define ECODE               uint32_t                            // error code type
#define RES_UINT(BL)        XGLUE(result_, UINT(BL))            // result struct for UINT(BL)
#define RES_MTP(BL)         XGLUE(result_, TYPE(mtp, BL))       // result struct for MTP(BL)
#define RES_MTPA(BL)        XGLUE(result_, TYPE(mtpa, BL))      // result struct for MTPA(BL)

#define SOV                 sizeof(void *)                      // size of void pointer
#define VAR(NAME, BL)       NAME##_##BL                         // typed variable name
#define ALIGN(BL)           VAR(ALIGNMENT, BL)                  // alignment variable name

#define INIT_RES_UINT(BL)   RES_UINT(BL) res = { .error = DOM_OK, .value = 0 };
#define INIT_RES_MTP(BL)    RES_MTP(BL) res = { .error = DOM_OK, .mv = NULL };
#define INIT_RES_MTPA(BL)   RES_MTPA(BL) res = { .error = DOM_OK, .mvs = NULL };

// ---------------------------------------------------------------------------------------------------------------------
#define IF_NO_MEM_RETURN_ECODE(ptr, func, lineno)                                                                       \
    if (!ptr) {                                                                                                         \
        return get_dom_error_code(DOM_ERROR_OUT_OF_MEMORY, func, lineno);                                               \
    }                                                                                                                   \

#define IF_NO_MEM_RETURN_ERES(ptr, func, lineno)                                                                        \
    if (!ptr) {                                                                                                         \
        res.error = get_dom_error_code(DOM_ERROR_OUT_OF_MEMORY, func, lineno);                                          \
        return res;                                                                                                     \
    }                                                                                                                   \

// ---------------------------------------------------------------------------------------------------------------------
#define IF_NULL_PTR_RETURN_ECODE(ptr, func, lineno)                                                                     \
    if (!ptr) {                                                                                                         \
        return get_dom_error_code(DOM_ERROR_NULL_POINTER, func, lineno);                                                \
    }                                                                                                                   \

#define IF_NULL_PTR_RETURN_ERES(ptr, func, lineno)                                                                      \
    if (!ptr) {                                                                                                         \
        res.error = get_dom_error_code(DOM_ERROR_NULL_POINTER, func, lineno);                                           \
        return res;                                                                                                     \
    }                                                                                                                   \

// ---------------------------------------------------------------------------------------------------------------------
#define IF_COND_RETURN_ECODE(cond, func, lineno)                                                                        \
    if (cond) {                                                                                                         \
        return get_dom_error_code(DOM_ERROR_INVALID_VALUE, func, lineno);                                               \
    }                                                                                                                   \

#define IF_COND_RETURN_ERES(cond, func, lineno)                                                                         \
    if (cond) {                                                                                                         \
        res.error = get_dom_error_code(DOM_ERROR_INVALID_VALUE, func, lineno);                                          \
        return res;                                                                                                     \
    }                                                                                                                   \

// ---------------------------------------------------------------------------------------------------------------------
#define IF_ECODE_UPDATE_RETURN(ecode, func, lineno)                                                                     \
    if (ecode) {                                                                                                        \
        return set_dom_error_location(ecode, func, lineno);                                                             \
    }                                                                                                                   \

#define IF_ERES_UPDATE_RETURN(eres, func, lineno)                                                                       \
    if (eres.error) {                                                                                                   \
        res.error = set_dom_error_location(eres.error, func, lineno);                                                   \
        return res;                                                                                                     \
    }                                                                                                                   \

// ---------------------------------------------------------------------------------------------------------------------
#define VALIDATE_DOM_ORDER(func, lineno)                                                                                \
    IF_COND_RETURN_ERES((unsigned)order > MAX_SEC_ORDER, func, lineno)                                                  \

#define VALIDATE_DOM_DOMAIN(func, lineno)                                                                               \
    IF_COND_RETURN_ERES((unsigned)domain > DOMAIN_ARITHMETIC, func, lineno)                                             \

// ---------------------------------------------------------------------------------------------------------------------
#define COMPUTE_MTP_ALLOC_SIZE(BYTES, BL)   ((BYTES + ALIGN(BL) - 1) & ~(ALIGN(BL) - 1))
#define COMPUTE_ARR_ALLOC_SIZE(BYTES)       (BYTES + (SOV - 1) & ~(SOV - 1))

// ---------------------------------------------------------------------------------------------------------------------
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
