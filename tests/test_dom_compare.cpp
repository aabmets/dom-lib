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
    static void    dom_free        (mtp mv)                           { FN(dom_free, BL)(mv); }                         \
    static mtp     dom_mask        (uint v, domain_t d, uint8_t o)    { return FN(dom_mask, BL)(v, d, o); }             \
    static uint    dom_unmask      (mtp mv)                           { return FN(dom_unmask, BL)(mv); }                \
    static int     dom_bool_and    (mtp a, mtp b, mtp o)              { return FN(dom_bool_and, BL)(a, b, o); }         \
    static int     dom_bool_or     (mtp a, mtp b, mtp o)              { return FN(dom_bool_or, BL)(a, b, o); }          \
    static int     dom_bool_xor    (mtp a, mtp b, mtp o)              { return FN(dom_bool_xor, BL)(a, b, o); }         \
    static int     dom_bool_not    (mtp mv)                           { return FN(dom_bool_not, BL)(mv); }              \
    static int     dom_conv        (mtp mv, domain_t td)              { return FN(dom_conv, BL)(mv, td); }              \
    static int     dom_cmp_lt      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_lt, BL)(a, b, o); }           \
    static int     dom_cmp_le      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_le, BL)(a, b, o); }           \
    static int     dom_cmp_gt      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_gt, BL)(a, b, o); }           \
    static int     dom_cmp_ge      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_ge, BL)(a, b, o); }           \
    static int     dom_cmp_eq      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_eq, BL)(a, b, o); }           \
    static int     dom_cmp_ne      (mtp a, mtp b, mtp o)              { return FN(dom_cmp_ne, BL)(a, b, o); }           \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


template<typename T>
void test_compare_operation(
        int (*masked_cmp)(
                typename dom_traits<T>::mskd_t*,
                typename dom_traits<T>::mskd_t*,
                typename dom_traits<T>::mskd_t*
        ),
        const std::function<bool(T, T)>& unmasked_op,
        domain_t domain)
{
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[2];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto* mv_a = traits::dom_mask(values[0], domain, order);
    auto* mv_b = traits::dom_mask(values[1], domain, order);
    auto* mv_out = traits::dom_mask(0, DOMAIN_BOOLEAN, order);

    REQUIRE(masked_cmp(mv_a, mv_b, mv_out) == 0);

    T unmasked = traits::dom_unmask(mv_out);
    T expected = unmasked_op(values[0], values[1]);
    REQUIRE(unmasked == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    traits::dom_conv(mv_a, counter_domain);
    REQUIRE(mv_a->domain == counter_domain);
    REQUIRE(masked_cmp(mv_a, mv_b, mv_out) == 0);
    REQUIRE(mv_a->domain == DOMAIN_BOOLEAN);

    unmasked = traits::dom_unmask(mv_out);
    REQUIRE(unmasked == expected);

    traits::dom_free(mv_a);
    traits::dom_free(mv_b);
    traits::dom_free(mv_out);
}


TEMPLATE_TEST_CASE("dom_cmp_lt handles boundary values",
        "[unittest][dom]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;
    const int order = GENERATE_COPY(range(1, 4));

    for (domain_t domain_in : { DOMAIN_ARITHMETIC, DOMAIN_BOOLEAN }) {
        auto* mv_zero = traits::dom_mask(static_cast<TestType>(0), domain_in, order);
        auto* mv_max  = traits::dom_mask(std::numeric_limits<TestType>::max(), domain_in, order);
        auto* mv_out  = traits::dom_mask(0, DOMAIN_BOOLEAN, order);

        /* 0 < MAX ⇒ 1 */
        REQUIRE(traits::dom_cmp_lt(mv_zero, mv_max, mv_out) == 0);
        REQUIRE(traits::dom_unmask(mv_out) == 1u);

        /* MAX < 0 ⇒ 0 */
        REQUIRE(traits::dom_cmp_lt(mv_max, mv_zero, mv_out) == 0);
        REQUIRE(traits::dom_unmask(mv_out) == 0u);

        traits::dom_free(mv_zero);
        traits::dom_free(mv_max);
        traits::dom_free(mv_out);
    }
}


TEMPLATE_TEST_CASE("Assert DOM comparison operations work correctly",
        "[unittest][dom][compare]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    const struct {
        const char* name;
        int (*fn)(typename traits::mtp, typename traits::mtp, typename traits::mtp);
        std::function<bool(TestType, TestType)> pred;
    } cases[] = {
        { "LT", traits::dom_cmp_lt, [](TestType a, TestType b){ return a <  b; } },
        { "LE", traits::dom_cmp_le, [](TestType a, TestType b){ return a <= b; } },
        { "GT", traits::dom_cmp_gt, [](TestType a, TestType b){ return a >  b; } },
        { "GE", traits::dom_cmp_ge, [](TestType a, TestType b){ return a >= b; } },
        { "EQ", traits::dom_cmp_eq, [](TestType a, TestType b){ return a == b; } },
        { "NE", traits::dom_cmp_ne, [](TestType a, TestType b){ return a != b; } },
    };

    for (const auto& c : cases) {
        SECTION(c.name) {
            for (domain_t domain_in : { DOMAIN_BOOLEAN, DOMAIN_ARITHMETIC }) {
                test_compare_operation<TestType>(c.fn, c.pred, domain_in);
            }
        }
    }
}
