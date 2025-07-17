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
#include "dom_internal_funcs.h"
#include "dom_internal_defs.h"


template<typename T>
struct dom_traits;

#define DEFINE_DOM_TRAITS(BL)                                                                                           \
template<>                                                                                                              \
struct dom_traits<UINT(BL)> {                                                                                           \
    using mskd_t = MT(BL);                                                                                              \
    using mtp = MTP(BL);                                                                                                \
    using mtpa = MTPA(BL);                                                                                              \
    using uint = UINT(BL);                                                                                              \
                                                                                                                        \
    static void    dom_free         (mtp mv)                           { FN(dom_free, BL)(mv); }                        \
    static mtp     dom_mask         (uint v, domain_t d, uint8_t o)    { return FN(dom_mask, BL)(v, d, o); }            \
    static uint    dom_unmask       (mtp mv)                           { return FN(dom_unmask, BL)(mv); }               \
    static int     dom_conv_btoa    (mtp mv)                           { return FN(dom_conv_btoa, BL)(mv); }            \
    static int     dom_conv_atob    (mtp mv)                           { return FN(dom_conv_atob, BL)(mv); }            \
                                                                                                                        \
    static int     dom_conv_many    (mtpa mvs, uint8_t c, domain_t td)                                                  \
                                    { return FN(dom_conv_many, BL)(mvs, c, td); }                                       \
                                                                                                                        \
    static mtpa    dom_mask_many    (const uint* values, domain_t doman, uint8_t order, uint32_t count)                 \
                                    { return FN(dom_mask_many, BL)(values, doman, order, count); }                      \
                                                                                                                        \
    static void    dom_free_many    (mtpa mvs, uint8_t count, uint32_t skip_mask)                                       \
                                    { FN(dom_free_many, BL)(mvs, count, skip_mask); }                                   \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


TEMPLATE_TEST_CASE(
        "Assert DOM converter functions work correctly",
        "[unittest][dom]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    TestType value[1];
    csprng_read_array(reinterpret_cast<uint8_t*>(value), sizeof(value));
    auto expected = static_cast<TestType>(value[0]);

    // Mask expected value with boolean domain
    auto* mv = traits::dom_mask(expected, DOMAIN_BOOLEAN, order);
    REQUIRE(mv->domain == DOMAIN_BOOLEAN);

    REQUIRE(traits::dom_conv_btoa(mv) == 0);

    // Check unmasking from the arithmetic domain
    TestType unmasked_1 = traits::dom_unmask(mv);
    REQUIRE(unmasked_1 == expected);
    REQUIRE(mv->domain == DOMAIN_ARITHMETIC);

    REQUIRE(traits::dom_conv_atob(mv) == 0);

    // Check unmasking back in the boolean domain
    TestType unmasked_2 = traits::dom_unmask(mv);
    REQUIRE(unmasked_2 == expected);
    REQUIRE(mv->domain == DOMAIN_BOOLEAN);

    traits::dom_free(mv);
}


TEMPLATE_TEST_CASE(
        "dom_conv_many preserves values across domains",
        "[unittest][dom]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;
    constexpr uint8_t COUNT = 6;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    TestType texts[COUNT];
    csprng_read_array(reinterpret_cast<uint8_t*>(texts), sizeof(texts));

    auto** mvs = traits::dom_mask_many(
        texts, DOMAIN_BOOLEAN, static_cast<uint8_t>(order), COUNT
    );
    REQUIRE(mvs != nullptr);
    for (uint8_t i = 0; i < COUNT; ++i)
        REQUIRE(mvs[i]->domain == DOMAIN_BOOLEAN);

    REQUIRE(traits::dom_conv_many(mvs, COUNT, DOMAIN_ARITHMETIC) == 0);
    for (uint8_t i = 0; i < COUNT; ++i) {
        REQUIRE(mvs[i]->domain == DOMAIN_ARITHMETIC);
        CHECK(traits::dom_unmask(mvs[i]) == texts[i]);
    }

    REQUIRE(traits::dom_conv_many(mvs, COUNT, DOMAIN_BOOLEAN) == 0);
    for (uint8_t i = 0; i < COUNT; ++i) {
        REQUIRE(mvs[i]->domain == DOMAIN_BOOLEAN);
        CHECK(traits::dom_unmask(mvs[i]) == texts[i]);
    }

    traits::dom_free_many(mvs, COUNT, 0);
}
