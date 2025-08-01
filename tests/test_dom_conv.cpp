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

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


template<typename T>
struct dom_traits;

#define DEFINE_DOM_TRAITS(BL)                                                                                           \
template<>                                                                                                              \
struct dom_traits<UINT(BL)> {                                                                                           \
    static ECODE            dom_free            (MTP(BL) mv)                                                            \
                                                { return FN(dom_free, BL)(mv); }                                        \
                                                                                                                        \
    static ECODE            dom_free_many       (MTPA(BL) mvs, uint8_t count, bool free_array)                          \
                                                { return FN(dom_free_many, BL)(mvs, count, free_array); }               \
                                                                                                                        \
    static RES_MTP(BL)      dom_mask            (UINT(BL) value, uint8_t order, domain_t domain)                        \
                                                { return FN(dom_mask, BL)(value, order, domain); }                      \
                                                                                                                        \
    static RES_MTPA(BL)     dom_mask_many       (UINT(BL)* value, uint8_t count, uint8_t order, domain_t domain)        \
                                                { return FN(dom_mask_many, BL)(value, count, order, domain); }          \
                                                                                                                        \
    static ECODE            dom_unmask          (MTP(BL) mv, UINT(BL)* out, uint8_t index)                              \
                                                { return FN(dom_unmask, BL)(mv, out, index); }                          \
                                                                                                                        \
    static ECODE            dom_conv_btoa       (MTP(BL) mv)                                                            \
                                                { return FN(dom_conv_btoa, BL)(mv); }                                   \
                                                                                                                        \
    static ECODE            dom_conv_atob       (MTP(BL) mv)                                                            \
                                                { return FN(dom_conv_atob, BL)(mv); }                                   \
                                                                                                                        \
    static ECODE            dom_conv_many       (MTPA(BL) mvs, uint8_t count, domain_t target_domain)                   \
                                                { return FN(dom_conv_many, BL)(mvs, count, target_domain); }            \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


template<typename T>
uint8_t* as_byte_ptr(T* ptr) {
    return reinterpret_cast<uint8_t*>(ptr);  // NOSONAR
}


TEMPLATE_TEST_CASE(
        "Assert DOM converter functions work correctly",
        "[unittest][dom_conv]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    std::array<TestType, 1> value_array = {};
    TestType* value = value_array.data();

    csprng_read_array(as_byte_ptr(value), sizeof(value));
    auto expected = static_cast<TestType>(value[0]);

    // Mask expected value with boolean domain
    auto res = traits::dom_mask(expected, order, DOMAIN_BOOLEAN);
    REQUIRE(res.error == DOM_OK);
    REQUIRE(res.mv != nullptr);
    REQUIRE(res.mv->domain == DOMAIN_BOOLEAN);

    std::array<TestType, 1> unmasked_array = {};
    TestType* unmasked = unmasked_array.data();

    REQUIRE(traits::dom_conv_btoa(res.mv) == DOM_OK);

    // Check unmasking from the arithmetic domain
    REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);
    REQUIRE(res.mv->domain == DOMAIN_ARITHMETIC);

    REQUIRE(traits::dom_conv_atob(res.mv) == DOM_OK);

    // Check unmasking back in the boolean domain
    REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);
    REQUIRE(res.mv->domain == DOMAIN_BOOLEAN);

    traits::dom_free(res.mv);
}


TEMPLATE_TEST_CASE(
        "dom_conv_many preserves values across domains",
        "[unittest][dom_conv]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;
    constexpr uint8_t COUNT = 6;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    std::array<TestType, COUNT> texts_array = {};
    TestType* texts = texts_array.data();

    csprng_read_array(as_byte_ptr(texts), sizeof(texts));

    auto res = traits::dom_mask_many(
        texts, COUNT, static_cast<uint8_t>(order), DOMAIN_BOOLEAN
    );
    REQUIRE(res.error == DOM_OK);
    REQUIRE(res.mvs != nullptr);
    for (uint8_t i = 0; i < COUNT; ++i)
        REQUIRE(res.mvs[i]->domain == DOMAIN_BOOLEAN);

    std::array<TestType, 1> unmasked_array = {};
    TestType* unmasked = unmasked_array.data();

    REQUIRE(traits::dom_conv_many(res.mvs, COUNT, DOMAIN_ARITHMETIC) == DOM_OK);
    for (uint8_t i = 0; i < COUNT; ++i) {
        REQUIRE(res.mvs[i]->domain == DOMAIN_ARITHMETIC);
        REQUIRE(traits::dom_unmask(res.mvs[i], unmasked, 0) == DOM_OK);
        REQUIRE(unmasked[0] == texts[i]);
    }

    REQUIRE(traits::dom_conv_many(res.mvs, COUNT, DOMAIN_BOOLEAN) == DOM_OK);
    for (uint8_t i = 0; i < COUNT; ++i) {
        REQUIRE(res.mvs[i]->domain == DOMAIN_BOOLEAN);
        REQUIRE(traits::dom_unmask(res.mvs[i], unmasked, 0) == DOM_OK);
        REQUIRE(unmasked[0] == texts[i]);
    }

    traits::dom_free_many(res.mvs, COUNT, true);
}
