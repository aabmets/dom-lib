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
    using mskd_t = MT(BL);                                                                                              \
    using mtp = MTP(BL);                                                                                                \
    using mtpa = MTPA(BL);                                                                                              \
    using uint = UINT(BL);                                                                                              \
                                                                                                                        \
    static void    dom_free          (mtp mv)                           { FN(dom_free, BL)(mv); }                       \
    static mtp     dom_mask          (uint v, uint8_t o, domain_t d)    { return FN(dom_mask, BL)(v, o, d); }           \
    static uint    dom_unmask        (mtp mv, uint* o, uint8_t i)      { return FN(dom_unmask, BL)(mv, o, i); }         \
    static int     dom_bool_and      (mtp a, mtp b, mtp o)              { return FN(dom_bool_and, BL)(a, b, o); }       \
    static int     dom_bool_or       (mtp a, mtp b, mtp o)              { return FN(dom_bool_or, BL)(a, b, o); }        \
    static int     dom_bool_xor      (mtp a, mtp b, mtp o)              { return FN(dom_bool_xor, BL)(a, b, o); }       \
    static int     dom_bool_not      (mtp mv)                           { return FN(dom_bool_not, BL)(mv); }            \
    static int     dom_bool_shr      (mtp mv, uint8_t n)                { return FN(dom_bool_shr, BL)(mv, n); }         \
    static int     dom_bool_shl      (mtp mv, uint8_t n)                { return FN(dom_bool_shl, BL)(mv, n); }         \
    static int     dom_bool_rotr     (mtp mv, uint8_t n)                { return FN(dom_bool_rotr, BL)(mv, n); }        \
    static int     dom_bool_rotl     (mtp mv, uint8_t n)                { return FN(dom_bool_rotl, BL)(mv, n); }        \
    static int     dom_bool_add      (mtp a, mtp b, mtp o)              { return FN(dom_bool_add, BL)(a, b, o); }       \
    static int     dom_bool_sub      (mtp a, mtp b, mtp o)              { return FN(dom_bool_sub, BL)(a, b, o); }       \
    static int     dom_arith_add     (mtp a, mtp b, mtp o)              { return FN(dom_arith_add, BL)(a, b, o); }      \
    static int     dom_arith_sub     (mtp a, mtp b, mtp o)              { return FN(dom_arith_sub, BL)(a, b, o); }      \
    static int     dom_arith_mult    (mtp a, mtp b, mtp o)              { return FN(dom_arith_mult, BL)(a, b, o); }     \
    static int     dom_conv          (mtp mv, domain_t td)              { return FN(dom_conv, BL)(mv, td); }            \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


template<typename T>
void test_binary_operation(
        int (*masked_op)(
            typename dom_traits<T>::mskd_t*,
            typename dom_traits<T>::mskd_t*,
            typename dom_traits<T>::mskd_t*
        ),
        const std::function<T(T, T)>& unmasked_op,
        domain_t domain
) {
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[2];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto* mv_a = traits::dom_mask(values[0], order, domain);
    auto* mv_b = traits::dom_mask(values[1], order, domain);
    auto* mv_out = traits::dom_mask(0, order, domain);
    T unmasked[1];

    REQUIRE(masked_op(mv_a, mv_b, mv_out) == 0);

    T expected = unmasked_op(values[0], values[1]);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    traits::dom_conv(mv_a, counter_domain);
    REQUIRE(mv_a->domain == counter_domain);
    REQUIRE(masked_op(mv_a, mv_b, mv_out) == 0);
    REQUIRE(mv_a->domain == domain);

    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == 0);
    REQUIRE(unmasked[0] == expected);

    traits::dom_free(mv_a);
    traits::dom_free(mv_b);
    traits::dom_free(mv_out);
}


template<typename T>
void test_unary_operation(
        int (*masked_op)(typename dom_traits<T>::mskd_t*),
        const std::function<T(T)>& unmasked_op,
        domain_t domain
) {
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[1];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto* mv = traits::dom_mask(values[0], order, domain);
    T unmasked[1];

    REQUIRE(masked_op(mv) == 0);

    T expected = unmasked_op(values[0]);
    REQUIRE(traits::dom_unmask(mv, unmasked, 0) == 0);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    traits::dom_conv(mv, counter_domain);
    REQUIRE(mv->domain == counter_domain);
    REQUIRE(masked_op(mv) == 0);
    REQUIRE(mv->domain == domain);

    traits::dom_free(mv);
}


template<typename T>
void test_shift_rotate_operation(
        int (*masked_op)(typename dom_traits<T>::mskd_t*, uint8_t),
        const std::function<T(T, T)>& unmasked_op,
        domain_t domain
) {
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[1];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto* mv = traits::dom_mask(values[0], order, DOMAIN_BOOLEAN);
    uint8_t offset = static_cast<uint8_t>(mv->bit_length / 2) - 1;
    T unmasked[1];

    REQUIRE(masked_op(mv, offset) == 0);

    T expected = unmasked_op(values[0], offset);
    REQUIRE(traits::dom_unmask(mv, unmasked, 0) == 0);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    traits::dom_conv(mv, counter_domain);
    REQUIRE(mv->domain == counter_domain);
    REQUIRE(masked_op(mv, offset) == 0);
    REQUIRE(mv->domain == domain);

    traits::dom_free(mv);
}


TEMPLATE_TEST_CASE("Assert DOM operations work correctly",
        "[unittest][dom_ops]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    SECTION("boolean AND") {
        auto masked_op = traits::dom_bool_and;
        auto unmasked_op = [](TestType a, TestType b) { return a & b; };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean OR") {
        auto masked_op = traits::dom_bool_or;
        auto unmasked_op = [](TestType a, TestType b) { return a | b; };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean XOR") {
        auto masked_op = traits::dom_bool_xor;
        auto unmasked_op = [](TestType a, TestType b) { return a ^ b; };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean NOT") {
        auto masked_op = traits::dom_bool_not;
        auto unmasked_op = [](TestType a) { return static_cast<TestType>(~a); };
        test_unary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean SHR") {
        auto masked_op = traits::dom_bool_shr;
        auto unmasked_op = [](TestType a, uint8_t b) { return static_cast<TestType>(a >> b); };
        test_shift_rotate_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean SHL") {
        auto masked_op = traits::dom_bool_shl;
        auto unmasked_op = [](TestType a, uint8_t b) { return static_cast<TestType>(a << b); };
        test_shift_rotate_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean ROTR") {
        auto masked_op = traits::dom_bool_rotr;
        auto unmasked_op = [](TestType a, uint8_t b) {
            const uint8_t w = sizeof(TestType) * 8;
            return static_cast<TestType>((a >> b) | (a << (w - b)));
        };
        test_shift_rotate_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean ROTL") {
        auto masked_op = traits::dom_bool_rotl;
        auto unmasked_op = [](TestType a, uint8_t b) {
            const uint8_t w = sizeof(TestType) * 8;
            return static_cast<TestType>((a << b) | (a >> (w - b)));
        };
        test_shift_rotate_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean ADD") {
        auto masked_op = traits::dom_bool_add;
        auto unmasked_op = [](TestType a, TestType b) { return static_cast<TestType>(a + b); };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("boolean SUB") {
        auto masked_op = traits::dom_bool_sub;
        auto unmasked_op = [](TestType a, TestType b) { return static_cast<TestType>(a - b); };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_BOOLEAN);
    }

    SECTION("arithmetic ADD") {
        auto masked_op = traits::dom_arith_add;
        auto unmasked_op = [](TestType a, TestType b) { return static_cast<TestType>(a + b); };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_ARITHMETIC);
    }

    SECTION("arithmetic SUB") {
        auto masked_op = traits::dom_arith_sub;
        auto unmasked_op = [](TestType a, TestType b) { return static_cast<TestType>(a - b); };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_ARITHMETIC);
    }

    SECTION("arithmetic MULT") {
        auto masked_op = traits::dom_arith_mult;
        auto unmasked_op = [](TestType a, TestType b) { return static_cast<TestType>(a * b); };
        test_binary_operation<TestType>(masked_op, unmasked_op, DOMAIN_ARITHMETIC);
    }
}
