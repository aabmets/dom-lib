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

#include <limits>
#include <functional>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


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
    static void    dom_free        (mtp mv)                           { FN(dom_free, BL)(mv); }                         \
    static mtp     dom_mask        (uint v, uint8_t o, domain_t d)    { return FN(dom_mask, BL)(v, o, d); }             \
    static uint    dom_unmask      (mtp mv, uint* o, uint8_t i)       { return FN(dom_unmask, BL)(mv, o, i); }          \
    static int     dom_conv        (mtp mv, domain_t td)              { return FN(dom_conv, BL)(mv, td); }              \
    static int     dom_cmp_lt      (mtp a, mtp b, mtp o, bool fm)     { return FN(dom_cmp_lt, BL)(a, b, o, fm); }       \
    static int     dom_cmp_le      (mtp a, mtp b, mtp o, bool fm)     { return FN(dom_cmp_le, BL)(a, b, o, fm); }       \
    static int     dom_cmp_gt      (mtp a, mtp b, mtp o, bool fm)     { return FN(dom_cmp_gt, BL)(a, b, o, fm); }       \
    static int     dom_cmp_ge      (mtp a, mtp b, mtp o, bool fm)     { return FN(dom_cmp_ge, BL)(a, b, o, fm); }       \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


TEMPLATE_TEST_CASE("dom_cmp_lt handles boundary values",
        "[unittest][dom_cmp]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    const int order = GENERATE_COPY(range(1, 4));
    const domain_t domain = GENERATE_COPY(DOMAIN_ARITHMETIC, DOMAIN_BOOLEAN);
    const bool full_mask = GENERATE_COPY(true, false);

    INFO("security order = " << order);
    INFO("masking domain = " << (domain == DOMAIN_ARITHMETIC ? "arithmetic" : "boolean"));
    INFO("full_mask = " << std::boolalpha << full_mask);

    auto* mv_zero = traits::dom_mask(static_cast<TestType>(0), order, domain);
    auto* mv_max  = traits::dom_mask(std::numeric_limits<TestType>::max(), order, domain);
    auto* mv_out  = traits::dom_mask(0, order, DOMAIN_BOOLEAN);
    TestType unmasked[1];

    /* 0 < MAX ⇒ true */
    REQUIRE(traits::dom_cmp_lt(mv_zero, mv_max, mv_out, full_mask) == 0);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
    const TestType exp_true = full_mask
        ? std::numeric_limits<TestType>::max()
        : static_cast<TestType>(1);
    REQUIRE(unmasked[0] == exp_true);

    /* MAX < 0 ⇒ false */
    REQUIRE(traits::dom_cmp_lt(mv_max, mv_zero, mv_out, full_mask) == 0);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
    REQUIRE(unmasked[0] == static_cast<TestType>(0));

    traits::dom_free(mv_zero);
    traits::dom_free(mv_max);
    traits::dom_free(mv_out);
}


TEMPLATE_TEST_CASE("Assert DOM comparison operations work correctly",
        "[unittest][dom_cmp]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    const int order = GENERATE_COPY(range(1, 4));
    const domain_t domain = GENERATE_COPY(DOMAIN_ARITHMETIC, DOMAIN_BOOLEAN);
    const bool full_mask = GENERATE_COPY(true, false);

    INFO("security order = " << order);
    INFO("masking domain = " << (domain == DOMAIN_ARITHMETIC ? "arithmetic" : "boolean"));
    INFO("full_mask = " << std::boolalpha << full_mask);

    const struct {
        const char* name;
        int (*masked_cmp)(
            typename traits::mtp,
            typename traits::mtp,
            typename traits::mtp,
            bool);
        std::function<bool(TestType, TestType)> unmasked_op;
    } cases[] = {
        { "LT", traits::dom_cmp_lt, [](TestType a, TestType b){ return a <  b; } },
        { "LE", traits::dom_cmp_le, [](TestType a, TestType b){ return a <= b; } },
        { "GT", traits::dom_cmp_gt, [](TestType a, TestType b){ return a >  b; } },
        { "GE", traits::dom_cmp_ge, [](TestType a, TestType b){ return a >= b; } },
    };

    for (const auto& [name, masked_cmp, unmasked_op] : cases) {
        SECTION(name) {
            TestType values[2];
            csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
            auto* mv_a = traits::dom_mask(values[0], order, domain);
            auto* mv_b = traits::dom_mask(values[1], order, domain);
            auto* mv_out = traits::dom_mask(0, order, DOMAIN_BOOLEAN);
            TestType unmasked[1];

            REQUIRE(masked_cmp(mv_a, mv_b, mv_out, full_mask) == 0);

            const TestType expected = unmasked_op(values[0], values[1])
                ? (full_mask ? std::numeric_limits<TestType>::max() : static_cast<TestType>(1))
                : static_cast<TestType>(0);
            REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
            REQUIRE(unmasked[0] == expected);

            // Assert automatic domain conversion
            domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
            traits::dom_conv(mv_a, counter_domain);
            REQUIRE(mv_a->domain == counter_domain);
            REQUIRE(masked_cmp(mv_a, mv_b, mv_out, full_mask) == 0);
            REQUIRE(mv_a->domain == DOMAIN_BOOLEAN);

            REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
            REQUIRE(unmasked[0] == expected);

            traits::dom_free(mv_a);
            traits::dom_free(mv_b);
            traits::dom_free(mv_out);
        }
    }
}
