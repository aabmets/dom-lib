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


template<typename T>
struct dom_traits;

#define DEFINE_DOM_TRAITS(BL)                                                                                           \
template<>                                                                                                              \
struct dom_traits<UINT(BL)> {                                                                                           \
    using mtp = MTP(BL);                                                                                                \
    using res_mtp = RES_MTP(BL);                                                                                        \
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
                                                                                                                        \
    static RES_MTP(BL)      dom_select_lt       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel)    \
                                                { return FN(dom_select_lt, BL)(a_cmp, b_cmp, truth_sel, false_sel); }   \
                                                                                                                        \
    static RES_MTP(BL)      dom_select_le       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel)    \
                                                { return FN(dom_select_le, BL)(a_cmp, b_cmp, truth_sel, false_sel); }   \
                                                                                                                        \
    static RES_MTP(BL)      dom_select_gt       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel)    \
                                                { return FN(dom_select_gt, BL)(a_cmp, b_cmp, truth_sel, false_sel); }   \
                                                                                                                        \
    static RES_MTP(BL)      dom_select_ge       (MTP(BL) a_cmp, MTP(BL) b_cmp, MTP(BL) truth_sel, MTP(BL) false_sel)    \
                                                { return FN(dom_select_ge, BL)(a_cmp, b_cmp, truth_sel, false_sel); }   \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


TEMPLATE_TEST_CASE(
        "Assert DOM selector operations work correctly",
        "[unittest][dom_select]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    const uint8_t order = GENERATE_COPY(range(1, 4));
    const domain_t domain = GENERATE_COPY(DOMAIN_ARITHMETIC, DOMAIN_BOOLEAN);

    INFO("security order = " << order);
    INFO("masking domain = " << (domain == DOMAIN_ARITHMETIC ? "arithmetic" : "boolean"));

    const auto max_v   = std::numeric_limits<TestType>::max();
    const auto half_v  = static_cast<TestType>(max_v / 2);
    const auto zero_v  = static_cast<TestType>(0);
    const auto truth_v = static_cast<TestType>(0xAA);
    const auto false_v = static_cast<TestType>(0xBB);

    auto max_res   = traits::dom_mask(max_v,   order, domain);
    auto half_res  = traits::dom_mask(half_v,  order, domain);
    auto zero_res  = traits::dom_mask(zero_v,  order, domain);
    auto truth_res = traits::dom_mask(truth_v, order, DOMAIN_BOOLEAN);
    auto false_res = traits::dom_mask(false_v, order, DOMAIN_BOOLEAN);

    REQUIRE(max_res.error   == DOM_OK);
    REQUIRE(half_res.error  == DOM_OK);
    REQUIRE(zero_res.error  == DOM_OK);
    REQUIRE(truth_res.error == DOM_OK);
    REQUIRE(false_res.error == DOM_OK);

    auto* mv_max   = max_res.mv;
    auto* mv_half  = half_res.mv;
    auto* mv_zero  = zero_res.mv;
    auto* mv_truth = truth_res.mv;
    auto* mv_false = false_res.mv;

    struct selector_case {
        const char* name;
        typename traits::res_mtp (*masked_select)(
            typename traits::mtp,
            typename traits::mtp,
            typename traits::mtp,
            typename traits::mtp);
        std::function<bool(TestType, TestType)> unmasked_select;
    };

    const std::array<selector_case, 4> cases = {{
        { "LT", traits::dom_select_lt, [](TestType a, TestType b){ return a <  b; } },
        { "LE", traits::dom_select_le, [](TestType a, TestType b){ return a <= b; } },
        { "GT", traits::dom_select_gt, [](TestType a, TestType b){ return a >  b; } },
        { "GE", traits::dom_select_ge, [](TestType a, TestType b){ return a >= b; } },
    }};

    struct pair_case {
        const char* desc;
        typename traits::mtp lhs;
        typename traits::mtp rhs;
        TestType lhs_val;
        TestType rhs_val;
    };

    const std::array<pair_case, 4> pairs = {{
        { "half vs max" , mv_half, mv_max , half_v , max_v  },
        { "max  vs half", mv_max , mv_half, max_v  , half_v },
        { "zero vs half", mv_zero, mv_half, zero_v , half_v },
        { "half vs zero", mv_half, mv_zero, half_v , zero_v },
    }};

    std::array<TestType, 1> unmasked_array = {};
    TestType* unmasked = unmasked_array.data();

    for (const auto& c : cases) {
        SECTION(c.name) {
            for (const auto& p : pairs) {
                DYNAMIC_SECTION(p.desc) {
                    auto res = c.masked_select(p.lhs, p.rhs, mv_truth, mv_false);
                    REQUIRE(res.error == DOM_OK);
                    REQUIRE(res.mv != nullptr);

                    const TestType expected = c.unmasked_select(p.lhs_val, p.rhs_val) ? truth_v : false_v;
                    REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
                    REQUIRE(unmasked[0] == expected);

                    traits::dom_free(res.mv);
                }
            }

            // Assert automatic domain conversion
            domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
            REQUIRE(traits::dom_conv(mv_half, counter_domain) == DOM_OK);
            REQUIRE(mv_half->domain == counter_domain);
            auto res = c.masked_select(mv_half, mv_max, mv_truth, mv_false);
            REQUIRE(res.error == DOM_OK);
            REQUIRE(mv_half->domain == DOMAIN_BOOLEAN);
            REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
            REQUIRE(unmasked[0] == (c.unmasked_select(half_v, max_v) ? truth_v : false_v));
            traits::dom_free(res.mv);
        }
    }

    traits::dom_free(mv_max);
    traits::dom_free(mv_half);
    traits::dom_free(mv_zero);
    traits::dom_free(mv_truth);
    traits::dom_free(mv_false);
}
