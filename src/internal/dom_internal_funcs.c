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


void secure_memzero(void* ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) *p++ = 0u;
}


#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <wincrypt.h>
    int csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const unsigned long flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
        const NTSTATUS rc = BCryptGenRandom(NULL, buffer, length, flags);
        return rc == 0 ? 0 : -1;
    }
#elif defined(__unix__)
    #include <fcntl.h>
    #include <unistd.h>
    int csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const int fd = open("/dev/urandom", O_RDONLY);
        const bytes = fd >= 0 ? read(fd, buffer, length) : 0;
        return fd >= 0 && bytes == length ? 0 : -1;
    }
#endif
