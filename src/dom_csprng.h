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

#ifndef DOM_CSPRNG_H
#define DOM_CSPRNG_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif


    #if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <wincrypt.h>

    inline int csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const NTSTATUS status = BCryptGenRandom(
            NULL, buffer, length,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );
        return status == 0;
    }

    #else
    #include <fcntl.h>
    #include <unistd.h>

    inline int csprng_read_array(uint8_t* buffer, const uint32_t length) {
        const int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            return 1;
        }
        const ssize_t ret = read(fd, buffer, length);
        close(fd);
        return ret == length;
    }

    #endif


#ifdef __cplusplus
}
#endif

#endif //DOM_CSPRNG_H