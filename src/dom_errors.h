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

#ifndef DOM_ERRORS_H
#define DOM_ERRORS_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "internal/dom_internal_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


static THREAD_LOCAL char error_message[ERR_MSG_LENGTH];


typedef enum {
    DOM_OK = 0,
    DOM_ERROR_OUT_OF_MEMORY = ENOMEM,
    DOM_ERROR_NULL_POINTER = EFAULT,
    DOM_ERROR_SIG_MISMATCH = EINVAL,
    DOM_ERROR_CSPRNG_FAILED = EIO,
} error_code_t;


static const char* dom_error_enum_to_str(const uint8_t err)
{
    switch (err) {
        case DOM_OK:                        return "no error";
        case DOM_ERROR_OUT_OF_MEMORY:       return "out of memory";
        case DOM_ERROR_NULL_POINTER:        return "null pointer";
        case DOM_ERROR_SIG_MISMATCH:        return "signature mismatch";
        case DOM_ERROR_CSPRNG_FAILED:       return "csprng failed";
        default:                            return "unknown error";
    }
}


typedef enum {
    // Group 0x0: Singular utilities
    DOM_FUNC_FREE               = 0x00,
    DOM_FUNC_CLEAR              = 0x01,
    DOM_FUNC_ALLOC              = 0x02,
    DOM_FUNC_MASK               = 0x03,
    DOM_FUNC_UNMASK             = 0x04,
    DOM_FUNC_REFRESH            = 0x05,
    DOM_FUNC_CLONE              = 0x06,

    // Group 0x1: Plural utilities
    DOM_FUNC_FREE_MANY          = 0x10,
    DOM_FUNC_CLEAR_MANY         = 0x11,
    DOM_FUNC_ALLOC_MANY         = 0x12,
    DOM_FUNC_MASK_MANY          = 0x13,
    DOM_FUNC_UNMASK_MANY        = 0x14,
    DOM_FUNC_REFRESH_MANY       = 0x15,
    DOM_FUNC_CLONE_MANY         = 0x16,

    // Group 0x2: Converters
    DOM_FUNC_CONV               = 0x20,
    DOM_FUNC_CONV_MANY          = 0x21,
    DOM_FUNC_CONV_BTOA          = 0x22,
    DOM_FUNC_CONV_ATOB          = 0x23,
    DOM_FUNC_CONV_TYPE_2TO1     = 0x24,
    DOM_FUNC_CONV_TYPE_1TO2     = 0x25,
    DOM_FUNC_CONV_TYPE_4TO1     = 0x26,
    DOM_FUNC_CONV_TYPE_1TO4     = 0x27,
    DOM_FUNC_CONV_TYPE_8TO1     = 0x28,
    DOM_FUNC_CONV_TYPE_1TO8     = 0x29,

    // Group 0x3: Boolean math
    DOM_FUNC_KSA_CARRY          = 0x30,
    DOM_FUNC_KSA_BORROW         = 0x31,
    DOM_FUNC_BOOL_AND           = 0x32,
    DOM_FUNC_BOOL_OR            = 0x33,
    DOM_FUNC_BOOL_XOR           = 0x34,
    DOM_FUNC_BOOL_NOT           = 0x35,
    DOM_FUNC_BOOL_SHR           = 0x36,
    DOM_FUNC_BOOL_SHL           = 0x37,
    DOM_FUNC_BOOL_ROTR          = 0x38,
    DOM_FUNC_BOOL_ROTL          = 0x39,
    DOM_FUNC_BOOL_ADD           = 0x3A,
    DOM_FUNC_BOOL_SUB           = 0x3B,

    // Group 0x4: Arithmetic math
    DOM_FUNC_ARITH_ADD          = 0x40,
    DOM_FUNC_ARITH_SUB          = 0x41,
    DOM_FUNC_ARITH_MULT         = 0x42,

    // Group 0x5: Selectors
    DOM_FUNC_CMP_LT             = 0x50,
    DOM_FUNC_CMP_LE             = 0x51,
    DOM_FUNC_CMP_GT             = 0x52,
    DOM_FUNC_CMP_GE             = 0x53,
    DOM_FUNC_SELECT             = 0x54,
    DOM_FUNC_SELECT_LT          = 0x55,
    DOM_FUNC_SELECT_LE          = 0x56,
    DOM_FUNC_SELECT_GT          = 0x57,
    DOM_FUNC_SELECT_GE          = 0x58,
} func_id_t;


static const char* dom_func_enum_to_str(const uint8_t err)
{
    switch (err) {
        // Group 0x0: Singular utilities
        case DOM_FUNC_FREE:             return "dom_free";
        case DOM_FUNC_CLEAR:            return "dom_clear";
        case DOM_FUNC_ALLOC:            return "dom_alloc";
        case DOM_FUNC_MASK:             return "dom_mask";
        case DOM_FUNC_UNMASK:           return "dom_unmask";
        case DOM_FUNC_REFRESH:          return "dom_refresh";
        case DOM_FUNC_CLONE:            return "dom_clone";

        // Group 0x1: Plural utilities
        case DOM_FUNC_FREE_MANY:        return "dom_free_many";
        case DOM_FUNC_CLEAR_MANY:       return "dom_clear_many";
        case DOM_FUNC_ALLOC_MANY:       return "dom_alloc_many";
        case DOM_FUNC_MASK_MANY:        return "dom_mask_many";
        case DOM_FUNC_UNMASK_MANY:      return "dom_unmask_many";
        case DOM_FUNC_REFRESH_MANY:     return "dom_refresh_many";
        case DOM_FUNC_CLONE_MANY:       return "dom_clone_many";

        // Group 0x2: Converters
        case DOM_FUNC_CONV:             return "dom_conv";
        case DOM_FUNC_CONV_MANY:        return "dom_conv_many";
        case DOM_FUNC_CONV_BTOA:        return "dom_conv_btoa";
        case DOM_FUNC_CONV_ATOB:        return "dom_conv_atob";
        case DOM_FUNC_CONV_TYPE_2TO1:   return "dom_conv_type_2to1";
        case DOM_FUNC_CONV_TYPE_1TO2:   return "dom_conv_type_1to2";
        case DOM_FUNC_CONV_TYPE_4TO1:   return "dom_conv_type_4to1";
        case DOM_FUNC_CONV_TYPE_1TO4:   return "dom_conv_type_1to4";
        case DOM_FUNC_CONV_TYPE_8TO1:   return "dom_conv_type_8to1";
        case DOM_FUNC_CONV_TYPE_1TO8:   return "dom_conv_type_1to8";

        // Group 0x3: Boolean math
        case DOM_FUNC_KSA_CARRY:        return "dom_ksa_carry";
        case DOM_FUNC_KSA_BORROW:       return "dom_ksa_borrow";
        case DOM_FUNC_BOOL_AND:         return "dom_bool_and";
        case DOM_FUNC_BOOL_OR:          return "dom_bool_or";
        case DOM_FUNC_BOOL_XOR:         return "dom_bool_xor";
        case DOM_FUNC_BOOL_NOT:         return "dom_bool_not";
        case DOM_FUNC_BOOL_SHR:         return "dom_bool_shr";
        case DOM_FUNC_BOOL_SHL:         return "dom_bool_shl";
        case DOM_FUNC_BOOL_ROTR:        return "dom_bool_rotr";
        case DOM_FUNC_BOOL_ROTL:        return "dom_bool_rotl";
        case DOM_FUNC_BOOL_ADD:         return "dom_bool_add";
        case DOM_FUNC_BOOL_SUB:         return "dom_bool_sub";

        // Group 0x4: Arithmetic math
        case DOM_FUNC_ARITH_ADD:        return "dom_arith_add";
        case DOM_FUNC_ARITH_SUB:        return "dom_arith_sub";
        case DOM_FUNC_ARITH_MULT:       return "dom_arith_mult";

        // Group 0x5: Selectors
        case DOM_FUNC_CMP_LT:           return "dom_cmp_lt";
        case DOM_FUNC_CMP_LE:           return "dom_cmp_le";
        case DOM_FUNC_CMP_GT:           return "dom_cmp_gt";
        case DOM_FUNC_CMP_GE:           return "dom_cmp_ge";
        case DOM_FUNC_SELECT:           return "dom_select";
        case DOM_FUNC_SELECT_LT:        return "dom_select_lt";
        case DOM_FUNC_SELECT_LE:        return "dom_select_le";
        case DOM_FUNC_SELECT_GT:        return "dom_select_gt";
        case DOM_FUNC_SELECT_GE:        return "dom_select_ge";

        // Fallback
        default:                        return "unknown function";
    }
}


static uint32_t get_dom_error_code(
        const error_code_t code,
        const func_id_t func,
        const uint16_t lineno
) {
    return (uint32_t)code << 24 | (uint32_t)func << 16 | (uint32_t)lineno;
}


static char* get_dom_error_message(const uint32_t error)
{
    if (error == DOM_OK) {
        snprintf(error_message, sizeof(error_message), "No error");
        return error_message;
    }
    const uint8_t code = (uint8_t)(error >> 24);
    const uint8_t func = (uint8_t)(error >> 16);
    const uint16_t lineno = (uint16_t)error;

    const char* code_str = dom_error_enum_to_str(code);
    const char* func_str = dom_func_enum_to_str(func);

    if (func_str == "unknown function") {
        snprintf(error_message, sizeof(error_message),
            "DOM error: %s (code 0x%02X) in %s (id 0x%02X) at line %u",
            code_str, code, func_str, func, lineno
        );
    } else {
        snprintf(error_message, sizeof(error_message),
            "DOM error: %s (code 0x%02X) in function %s (id 0x%02X) at line %u",
            code_str, code, func_str, func, lineno
        );
    }
    return error_message;
}


#ifdef __cplusplus
}
#endif

#include "internal/dom_internal_undefs.h"

#endif //DOM_ERRORS_H
