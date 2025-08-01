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
    static RES_MTP(BL)      dom_alloc           (uint8_t order, domain_t domain)                                        \
                                                { return FN(dom_alloc, BL)(order, domain); }                            \
                                                                                                                        \
    static ECODE            dom_free            (MTP(BL) mv)                                                            \
                                                { return FN(dom_free, BL)(mv); }                                        \
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
    static ECODE            dom_carry           (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_ksa_carry, BL)(a, b, out); }                            \
                                                                                                                        \
    static ECODE            dom_borrow          (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_ksa_borrow, BL)(a, b, out); }                           \
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
        const auto ai = static_cast<int>(a >> i & 1u);
        const auto bi = static_cast<int>(b >> i & 1u);

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

    const int order = GENERATE(range(1, 4));
    INFO("security order = " << order);

    std::array<T, 2> values_array = {};
    T* values = values_array.data();

    csprng_read_array(as_byte_ptr(values), sizeof(values));

    auto a_res = traits::dom_mask(values[0], order, DOMAIN_BOOLEAN);
    auto b_res = traits::dom_mask(values[1], order, DOMAIN_BOOLEAN);
    auto g_res = traits::dom_alloc(order, DOMAIN_BOOLEAN);

    REQUIRE(a_res.error == DOM_OK);
    REQUIRE(b_res.error == DOM_OK);
    REQUIRE(g_res.error == DOM_OK);

    auto* mv_a = a_res.mv;
    auto* mv_b = b_res.mv;
    auto* mv_g = g_res.mv;

    std::array<T, 1> unmasked_array = {};
    T* unmasked = unmasked_array.data();

    REQUIRE(ksa_fn(mv_a, mv_b, mv_g) == DOM_OK);

    T expected = ref_fn(values[0], values[1]);
    REQUIRE(traits::dom_unmask(mv_g, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);

    traits::dom_free(mv_a);
    traits::dom_free(mv_b);
    traits::dom_free(mv_g);
}


TEMPLATE_TEST_CASE("Assert KSA carry / borrow gadgets work correctly",
        "[unittest][dom_ksa]", uint8_t, uint16_t, uint32_t, uint64_t
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
