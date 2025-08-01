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

#include "internal/dom_internal_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    DOM_OK                      = 0x0,
    DOM_ERROR_OUT_OF_MEMORY     = ENOMEM,
    DOM_ERROR_NULL_POINTER      = EFAULT,
    DOM_ERROR_INVALID_VALUE     = EINVAL,
    DOM_ERROR_CSPRNG_FAILED     = EIO,
    DOM_ERROR_SIG_MISMATCH      = 0xAA,
} error_code_t;


typedef enum {
    // Group 0x0: Core memory functions
    DOM_FUNC_ALLOC              = 0x00,
    DOM_FUNC_ALLOC_MANY         = 0x01,
    DOM_FUNC_CLONE              = 0x02,
    DOM_FUNC_CLONE_MANY         = 0x03,
    DOM_FUNC_FREE               = 0x04,
    DOM_FUNC_FREE_MANY          = 0x05,
    DOM_FUNC_CLEAR              = 0x06,
    DOM_FUNC_CLEAR_MANY         = 0x07,

    // Group 0x1: Core masking functions
    DOM_FUNC_MASK               = 0x10,
    DOM_FUNC_MASK_MANY          = 0x11,
    DOM_FUNC_UNMASK             = 0x12,
    DOM_FUNC_UNMASK_MANY        = 0x13,
    DOM_FUNC_REFRESH            = 0x14,
    DOM_FUNC_REFRESH_MANY       = 0x15,

    // Group 0x2: Converter functions
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

    // Group 0x3: Boolean math functions
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

    // Group 0x4: Arithmetic math functions
    DOM_FUNC_ARITH_ADD          = 0x40,
    DOM_FUNC_ARITH_SUB          = 0x41,
    DOM_FUNC_ARITH_MULT         = 0x42,

    // Group 0x5: Selector functions
    DOM_FUNC_CMP_LT             = 0x50,
    DOM_FUNC_CMP_LE             = 0x51,
    DOM_FUNC_CMP_GT             = 0x52,
    DOM_FUNC_CMP_GE             = 0x53,
    DOM_FUNC_SELECT             = 0x54,
    DOM_FUNC_SELECT_LT          = 0x55,
    DOM_FUNC_SELECT_LE          = 0x56,
    DOM_FUNC_SELECT_GT          = 0x57,
    DOM_FUNC_SELECT_GE          = 0x58,

    // Group 0xF: Internal functions
    FUNC_CSPRNG_READ_ARRAY      = 0xFA,
} func_id_t;


ECODE   get_dom_error_code          (error_code_t code, func_id_t func, uint16_t line_id);
ECODE   set_dom_error_location      (ECODE error, func_id_t func, uint16_t line_id);
char*   get_dom_error_message       (ECODE error);


#ifdef __cplusplus
}
#endif

#include "internal/dom_internal_undefs.h"  // NOSONAR

#endif //DOM_ERRORS_H
