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

#ifndef DOM_INTERNAL_FUNCS_H
#define DOM_INTERNAL_FUNCS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#endif


    #if defined(_WIN32) || defined(_WIN64)
        #define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
        #define aligned_free(ptr) _aligned_free(ptr)
    #else
        #define aligned_free(ptr) free(ptr)
    #endif

    void secure_memzero(void* ptr, size_t len);
    ECODE csprng_read_array(uint8_t* buffer, uint32_t length);


#ifdef __cplusplus
}
#endif

#endif //DOM_INTERNAL_FUNCS_H
