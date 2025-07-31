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
    using mtp = MTP(BL);                                                                                                \
                                                                                                                        \
    static ECODE            dom_free            (MTP(BL) mv)                                                            \
                                                { return FN(dom_free, BL)(mv); }                                        \
                                                                                                                        \
    static RES_MTP(BL)      dom_mask            (UINT(BL) value, uint8_t order, domain_t domain)                        \
                                                { return FN(dom_mask, BL)(value, order, domain); }                      \
                                                                                                                        \
    static ECODE            dom_unmask          (MTP(BL) mv, UINT(BL)* out, uint8_t index)                              \
                                                { return FN(dom_unmask, BL)(mv, out, index); }                          \
                                                                                                                        \
    static ECODE            dom_conv            (MTP(BL) mv, domain_t target_domain)                                    \
                                                { return FN(dom_conv, BL)(mv, target_domain); }                         \
                                                                                                                        \
    static ECODE            dom_cmp_lt          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask)                     \
                                                { return FN(dom_cmp_lt, BL)(a, b, out, full_mask); }                    \
                                                                                                                        \
    static ECODE            dom_cmp_le          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask)                     \
                                                { return FN(dom_cmp_le, BL)(a, b, out, full_mask); }                    \
                                                                                                                        \
    static ECODE            dom_cmp_gt          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask)                     \
                                                { return FN(dom_cmp_gt, BL)(a, b, out, full_mask); }                    \
                                                                                                                        \
    static ECODE            dom_cmp_ge          (MTP(BL) a, MTP(BL) b, MTP(BL) out, bool full_mask)                     \
                                                { return FN(dom_cmp_ge, BL)(a, b, out, full_mask); }                    \
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

    auto zero_res = traits::dom_mask(static_cast<TestType>(0), order, domain);
    auto max_res  = traits::dom_mask(std::numeric_limits<TestType>::max(), order, domain);
    auto out_res  = traits::dom_mask(0, order, DOMAIN_BOOLEAN);
    REQUIRE(zero_res.error == DOM_OK);
    REQUIRE(max_res.error == DOM_OK);
    REQUIRE(out_res.error == DOM_OK);
    REQUIRE(zero_res.mv != nullptr);
    REQUIRE(max_res.mv != nullptr);
    REQUIRE(out_res.mv != nullptr);
    auto* mv_zero = zero_res.mv;
    auto* mv_max  = max_res.mv;
    auto* mv_out  = out_res.mv;
    TestType unmasked[1];

    /* 0 < MAX ⇒ true */
    REQUIRE(traits::dom_cmp_lt(mv_zero, mv_max, mv_out, full_mask) == DOM_OK);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
    const TestType exp_true = full_mask
        ? std::numeric_limits<TestType>::max()
        : static_cast<TestType>(1);
    REQUIRE(unmasked[0] == exp_true);

    /* MAX < 0 ⇒ false */
    REQUIRE(traits::dom_cmp_lt(mv_max, mv_zero, mv_out, full_mask) == DOM_OK);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
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
        ECODE (*masked_cmp)(
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
            auto a_res = traits::dom_mask(values[0], order, domain);
            auto b_res = traits::dom_mask(values[1], order, domain);
            auto out_res = traits::dom_mask(0, order, DOMAIN_BOOLEAN);
            REQUIRE(a_res.error == DOM_OK);
            REQUIRE(b_res.error == DOM_OK);
            REQUIRE(out_res.error == DOM_OK);
            REQUIRE(a_res.mv != nullptr);
            REQUIRE(b_res.mv != nullptr);
            REQUIRE(out_res.mv != nullptr);
            auto* mv_a = a_res.mv;
            auto* mv_b = b_res.mv;
            auto* mv_out = out_res.mv;
            TestType unmasked[1];

            REQUIRE(masked_cmp(mv_a, mv_b, mv_out, full_mask) == DOM_OK);

            const TestType expected = unmasked_op(values[0], values[1])
                ? (full_mask ? std::numeric_limits<TestType>::max() : static_cast<TestType>(1))
                : static_cast<TestType>(0);
            REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
            REQUIRE(unmasked[0] == expected);

            // Assert automatic domain conversion
            domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
            REQUIRE(traits::dom_conv(mv_a, counter_domain) == DOM_OK);
            REQUIRE(mv_a->domain == counter_domain);
            REQUIRE(masked_cmp(mv_a, mv_b, mv_out, full_mask) == DOM_OK);
            REQUIRE(mv_a->domain == DOMAIN_BOOLEAN);

            REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
            REQUIRE(unmasked[0] == expected);

            traits::dom_free(mv_a);
            traits::dom_free(mv_b);
            traits::dom_free(mv_out);
        }
    }
}
