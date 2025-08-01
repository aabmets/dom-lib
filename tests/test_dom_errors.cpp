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

#include <catch2/catch_all.hpp>
#include <cstring>

#include "dom_api.h"
#include "dom_errors.h"
#include "internal/dom_internal_defs.h"


TEST_CASE(
    "get_dom_error_code encodes error components into a 32-bit value",
    "[unittest][dom_errors]"
) {
    constexpr error_code_t code = DOM_ERROR_OUT_OF_MEMORY;
    constexpr func_id_t func = DOM_FUNC_ALLOC;
    constexpr uint16_t line_id = 0x1234;

    constexpr uint32_t expected =
        static_cast<uint32_t>(code) << 24 |
        static_cast<uint32_t>(func) << 16 |
        line_id;

    REQUIRE(get_dom_error_code(code, func, line_id) == expected);
}


TEST_CASE(
    "get_dom_error_message returns \"No error\" for DOM_OK",
    "[unittest][dom_errors]"
) {
    const char* msg = get_dom_error_message(DOM_OK);

    REQUIRE(std::strcmp(msg, "No error") == 0);
    REQUIRE(std::strlen(msg) < ERR_MSG_LENGTH);
}


TEST_CASE(
    "get_dom_error_message returns formatted string for known error and function",
    "[unittest][dom_errors]"
) {
    error_code_t code = DOM_ERROR_OUT_OF_MEMORY;
    func_id_t func = DOM_FUNC_ALLOC;
    uint16_t line_id = 0x1234;

    uint32_t error  = get_dom_error_code(code, func, line_id);
    const char* msg = get_dom_error_message(error);
    std::string s(msg);

    REQUIRE(s.find("out of memory")  != std::string::npos);
    REQUIRE(s.find("dom_alloc")      != std::string::npos);
    REQUIRE(s.find("(code 0x0C)")    != std::string::npos);
    REQUIRE(s.find("(id 0x00)")      != std::string::npos);
    REQUIRE(s.find("line id 0x1234") != std::string::npos);
    REQUIRE(std::strlen(msg) < ERR_MSG_LENGTH);
}


TEST_CASE(
    "get_dom_error_message handles unknown error and function codes",
    "[unittest][dom_errors]"
) {
    uint8_t code = 0xFD;
    uint8_t func = 0xEE;
    uint16_t line_id = 0x1234;

    uint32_t error =
        static_cast<uint32_t>(code) << 24 |
        static_cast<uint32_t>(func) << 16 |
        line_id;

    const char* msg = get_dom_error_message(error);
    std::string s(msg);

    REQUIRE(s.find("unknown")        != std::string::npos);
    REQUIRE(s.find("(code 0xFD)")    != std::string::npos);
    REQUIRE(s.find("(id 0xEE)")      != std::string::npos);
    REQUIRE(s.find("line id 0x1234") != std::string::npos);
    REQUIRE(std::strlen(msg) < ERR_MSG_LENGTH);
}
