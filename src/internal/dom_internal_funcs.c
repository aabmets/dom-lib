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

#include "dom_errors.h"


void secure_memzero(void* ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) *p++ = 0u;
}


#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <wincrypt.h>
    uint32_t csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const unsigned long flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
        const NTSTATUS rc = BCryptGenRandom(NULL, buffer, length, flags);
        if (!rc)
            return DOM_OK;
        return get_dom_error_code(DOM_ERROR_CSPRNG_FAILED, FUNC_CSPRNG_READ_ARRAY, 30);
    }
#elif defined(__unix__)
    #include <fcntl.h>
    #include <unistd.h>
    uint32_t csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const int fd = open("/dev/urandom", O_RDONLY);
        const int bytes = fd >= 0 ? read(fd, buffer, length) : 0;
        if (fd >= 0 && bytes == length)
            return DOM_OK;
        return get_dom_error_code(DOM_ERROR_CSPRNG_FAILED, FUNC_CSPRNG_READ_ARRAY, 41);
    }
#endif
