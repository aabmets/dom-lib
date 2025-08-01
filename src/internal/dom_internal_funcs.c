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

#include "dom_types.h"
#include "dom_errors.h"
#include "dom_internal_defs.h"
#include "dom_internal_funcs.h"


void secure_memzero(void* ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) *p++ = 0u;
}


#define DOM_INTERNAL_FUNCS_IMPL(BL)                                             \
                                                                                \
void FN(alloc_many_error_cleanup, BL)(                                          \
        MTPA(BL) mvs,                                                           \
        const uint8_t count                                                     \
) {                                                                             \
    for (uint8_t i = 0; i < count; ++i) {                                       \
        const MTP(BL) mv = mvs[i];                                              \
        secure_memzero((void*)mv, mv->total_bytes);                             \
        aligned_free((void*)mv);                                                \
    }                                                                           \
    aligned_free(mvs);                                                          \
}                                                                               \

DOM_INTERNAL_FUNCS_IMPL(8)
DOM_INTERNAL_FUNCS_IMPL(16)
DOM_INTERNAL_FUNCS_IMPL(32)
DOM_INTERNAL_FUNCS_IMPL(64)


#if defined(_WIN32) || defined(_WIN64)
    #undef UINT  // NOSONAR - conflict with wincrypt.h
    #include <windows.h>
    #include <wincrypt.h>
    ECODE csprng_read_array(uint8_t* buffer, const uint32_t length)
    {
        const unsigned long flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
        const NTSTATUS rc = BCryptGenRandom(NULL, buffer, length, flags);
        if (!rc)
            return DOM_OK;
        return get_dom_error_code(DOM_ERROR_CSPRNG_FAILED, FUNC_CSPRNG_READ_ARRAY, 31);
    }
#elif defined(__unix__)
    #include <fcntl.h>
    #include <unistd.h>
    ECODE csprng_read_array(uint8_t* buffer, const uint32_t length)
    {
        const int fd = open("/dev/urandom", O_RDONLY);
        const int bytes = fd >= 0 ? read(fd, buffer, length) : 0;
        if (fd >= 0 && bytes == length)
            return DOM_OK;
        return get_dom_error_code(DOM_ERROR_CSPRNG_FAILED, FUNC_CSPRNG_READ_ARRAY, 41);
    }
#endif
