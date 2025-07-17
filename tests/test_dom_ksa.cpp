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
    static void    dom_free      (mtp mv)                           { FN(dom_free, BL)(mv); }                           \
    static mtp     dom_mask      (uint v, domain_t d, uint8_t o)    { return FN(dom_mask, BL)(v, d, o); }               \
    static uint    dom_unmask    (mtp mv)                           { return FN(dom_unmask, BL)(mv); }                  \
    static mtp     dom_carry     (mtp a, mtp b)                     { return FN(dom_ksa_carry, BL)(a, b); }             \
    static mtp     dom_borrow    (mtp a, mtp b)                     { return FN(dom_ksa_borrow, BL)(a, b); }            \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


template<typename T>
static T ref_carry_word_shifted(T a, T b) {
    const unsigned bits = sizeof(T) * 8;
    T carry_word = 0;
    unsigned carry = 0;

    for (unsigned i = 0; i < bits; ++i) {
        const unsigned ai = (a >> i) & 1u;
        const unsigned bi = (b >> i) & 1u;

        const unsigned sum = ai + bi + carry;
        carry = sum >> 1;

        if (carry && i + 1 < bits)
            carry_word |= T(1) << (i + 1);
    }
    return carry_word;
}


template<typename T>
static T ref_borrow_word_shifted(T a, T b) {
    const unsigned bits = sizeof(T) * 8;
    T borrow_word = 0;
    unsigned borrow = 0;

    for (unsigned i = 0; i < bits; ++i) {
        const int ai = static_cast<int>(a >> i & 1u);
        const int bi = static_cast<int>(b >> i & 1u);

        const int diff = ai - bi - static_cast<int>(borrow);
        borrow = diff < 0;

        if (borrow && i + 1 < bits)
            borrow_word |= T(1) << (i + 1);
    }
    return borrow_word;
}

template<typename T, typename RefFn, typename KsaFn>
static void test_ksa_operation(const RefFn& ref_fn, const KsaFn& ksa_fn) {
    using traits = dom_traits<T>;
    using mskd_t = typename traits::mskd_t;

    const int order = GENERATE(range(1, 4));
    INFO("security order = " << order);

    T vals[2];
    csprng_read_array(reinterpret_cast<uint8_t*>(vals), sizeof(vals));

    mskd_t* mv_a = traits::dom_mask(vals[0], DOMAIN_BOOLEAN, order);
    mskd_t* mv_b = traits::dom_mask(vals[1], DOMAIN_BOOLEAN, order);

    mskd_t* mv_g = ksa_fn(mv_a, mv_b);
    REQUIRE(mv_g != nullptr);

    T g_unmasked = traits::dom_unmask(mv_g);
    T g_expected = ref_fn(vals[0], vals[1]);
    REQUIRE(g_unmasked == g_expected);

    traits::dom_free(mv_a);
    traits::dom_free(mv_b);
    traits::dom_free(mv_g);
}


TEMPLATE_TEST_CASE("Assert KSA carry / borrow gadgets work correctly",
        "[unittest][dom][ksa]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    SECTION("Kogge‑Stone carry (addition prefix)") {
        auto ref_fn = ref_carry_word_shifted<TestType>;
        auto ksa_fn = dom_traits<TestType>::dom_carry;
        test_ksa_operation<TestType>(ref_fn, ksa_fn);
    }

    SECTION("Kogge‑Stone borrow (subtraction prefix)") {
        auto ref_fn = ref_borrow_word_shifted<TestType>;
        auto ksa_fn = dom_traits<TestType>::dom_borrow;
        test_ksa_operation<TestType>(ref_fn, ksa_fn);
    }
}
