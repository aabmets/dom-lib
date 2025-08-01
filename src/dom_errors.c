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
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"


static THREAD_LOCAL char error_message[ERR_MSG_LENGTH];


static const char* dom_error_enum_to_str(const uint8_t err)
{
    switch (err) {
        case DOM_OK:                        return "no error";
        case DOM_ERROR_OUT_OF_MEMORY:       return "out of memory";
        case DOM_ERROR_NULL_POINTER:        return "null pointer";
        case DOM_ERROR_INVALID_VALUE:       return "invalid argument";
        case DOM_ERROR_CSPRNG_FAILED:       return "csprng failed";
        case DOM_ERROR_SIG_MISMATCH:        return "signature mismatch";
        default:                            return "unknown error";
    }
}


static const char* dom_func_enum_to_str(const uint8_t err)
{
    switch (err) {
        // Group 0x0: Core memory functions
        case DOM_FUNC_ALLOC:            return "dom_alloc";
        case DOM_FUNC_ALLOC_MANY:       return "dom_alloc_many";
        case DOM_FUNC_CLONE:            return "dom_clone";
        case DOM_FUNC_CLONE_MANY:       return "dom_clone_many";
        case DOM_FUNC_FREE:             return "dom_free";
        case DOM_FUNC_FREE_MANY:        return "dom_free_many";
        case DOM_FUNC_CLEAR:            return "dom_clear";
        case DOM_FUNC_CLEAR_MANY:       return "dom_clear_many";

        // Group 0x1: Core masking functions
        case DOM_FUNC_MASK:             return "dom_mask";
        case DOM_FUNC_MASK_MANY:        return "dom_mask_many";
        case DOM_FUNC_UNMASK:           return "dom_unmask";
        case DOM_FUNC_UNMASK_MANY:      return "dom_unmask_many";
        case DOM_FUNC_REFRESH:          return "dom_refresh";
        case DOM_FUNC_REFRESH_MANY:     return "dom_refresh_many";

        // Group 0x2: Converter functions
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
        default:                        break;
    }

    switch (err) {
        // Group 0x3: Boolean math functions
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

        // Group 0x4: Arithmetic math functions
        case DOM_FUNC_ARITH_ADD:        return "dom_arith_add";
        case DOM_FUNC_ARITH_SUB:        return "dom_arith_sub";
        case DOM_FUNC_ARITH_MULT:       return "dom_arith_mult";

        // Group 0x5: Selector functions
        case DOM_FUNC_CMP_LT:           return "dom_cmp_lt";
        case DOM_FUNC_CMP_LE:           return "dom_cmp_le";
        case DOM_FUNC_CMP_GT:           return "dom_cmp_gt";
        case DOM_FUNC_CMP_GE:           return "dom_cmp_ge";
        case DOM_FUNC_SELECT:           return "dom_select";
        case DOM_FUNC_SELECT_LT:        return "dom_select_lt";
        case DOM_FUNC_SELECT_LE:        return "dom_select_le";
        case DOM_FUNC_SELECT_GT:        return "dom_select_gt";
        case DOM_FUNC_SELECT_GE:        return "dom_select_ge";

        // Group 0xF: Internal functions
        case FUNC_CSPRNG_READ_ARRAY:    return "csprng_read_array";
        default:                        return "unknown function";
    }
}


/*
 *  ║ Byte 1  ║ Byte 2  ║ Byte 3  ║ Byte 4  ║
 *  ╠═════════╬═════════╬═════════╬═════════╣
 *   0000 0000 0000 0000 0000 0000 0000 0000
 *  ├─────────┼─────────┼───────────────────┤
 *  │ A       │ B       │ C                 │
 *
 *    A) 1 byte  - Error reason (error_code_t)
 *    B) 1 byte  - Function ID where error occurred (unique across files)
 *    C) 2 bytes - Source code line ID (unique for a file, but not across files)
 */
ECODE get_dom_error_code(const error_code_t code, const func_id_t func, const uint16_t line_id)
{
    return (ECODE)code << 24 | (ECODE)func << 16 | (ECODE)line_id;
}


ECODE set_dom_error_location(const ECODE error, const func_id_t func, const uint16_t line_id)
{
    return error & 0xFF000000u | (ECODE)func << 16 | (ECODE)line_id;
}


char* get_dom_error_message(const ECODE error)
{
    if (error == DOM_OK) {
        snprintf(error_message, sizeof(error_message), "No error");
        return error_message;
    }
    const uint8_t code = (uint8_t)(error >> 24);
    const uint8_t func = (uint8_t)(error >> 16);
    const uint16_t line_id = (uint16_t)error;

    const char* code_str = dom_error_enum_to_str(code);
    const char* func_str = dom_func_enum_to_str(func);

    if (func_str == "unknown function") {
        snprintf(error_message, sizeof(error_message),
            "DOM error: %s (code 0x%02X) in %s (id 0x%02X) at line id 0x%04X",
            code_str, code, func_str, func, line_id
        );
    } else {
        snprintf(error_message, sizeof(error_message),
            "DOM error: %s (code 0x%02X) in function %s (id 0x%02X) at line id 0x%04X",
            code_str, code, func_str, func, line_id
        );
    }
    return error_message;
}
