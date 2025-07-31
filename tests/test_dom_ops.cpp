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
    static ECODE            dom_bool_and        (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_bool_and, BL)(a, b, out); }                             \
                                                                                                                        \
    static ECODE            dom_bool_or         (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_bool_or, BL)(a, b, out); }                              \
                                                                                                                        \
    static ECODE            dom_bool_xor        (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_bool_xor, BL)(a, b, out); }                             \
                                                                                                                        \
    static ECODE            dom_bool_not        (MTP(BL) mv)                                                            \
                                                { return FN(dom_bool_not, BL)(mv); }                                    \
                                                                                                                        \
    static ECODE            dom_bool_shr        (MTP(BL) mv, uint8_t n)                                                 \
                                                { return FN(dom_bool_shr, BL)(mv, n); }                                 \
                                                                                                                        \
    static ECODE            dom_bool_shl        (MTP(BL) mv, uint8_t n)                                                 \
                                                { return FN(dom_bool_shl, BL)(mv, n); }                                 \
                                                                                                                        \
    static ECODE            dom_bool_rotr       (MTP(BL) mv, uint8_t n)                                                 \
                                                { return FN(dom_bool_rotr, BL)(mv, n); }                                \
                                                                                                                        \
    static ECODE            dom_bool_rotl       (MTP(BL) mv, uint8_t n)                                                 \
                                                { return FN(dom_bool_rotl, BL)(mv, n); }                                \
                                                                                                                        \
    static ECODE            dom_bool_add        (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_bool_add, BL)(a, b, out); }                             \
                                                                                                                        \
    static ECODE            dom_bool_sub        (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_bool_sub, BL)(a, b, out); }                             \
                                                                                                                        \
    static ECODE            dom_arith_add       (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_arith_add, BL)(a, b, out); }                            \
                                                                                                                        \
    static ECODE            dom_arith_sub       (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_arith_sub, BL)(a, b, out); }                            \
                                                                                                                        \
    static ECODE            dom_arith_mult      (MTP(BL) a, MTP(BL) b, MTP(BL) out)                                     \
                                                { return FN(dom_arith_mult, BL)(a, b, out); }                           \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


template<typename T>
void test_binary_operation(
        ECODE (*masked_op)(
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
    auto a_res = traits::dom_mask(values[0], order, domain);
    auto b_res = traits::dom_mask(values[1], order, domain);
    auto out_res = traits::dom_mask(0, order, domain);
    REQUIRE(a_res.error == DOM_OK);
    REQUIRE(b_res.error == DOM_OK);
    REQUIRE(out_res.error == DOM_OK);
    REQUIRE(a_res.mv != nullptr);
    REQUIRE(b_res.mv != nullptr);
    REQUIRE(out_res.mv != nullptr);
    auto* mv_a = a_res.mv;
    auto* mv_b = b_res.mv;
    auto* mv_out = out_res.mv;
    T unmasked[1];

    REQUIRE(masked_op(mv_a, mv_b, mv_out) == DOM_OK);

    T expected = unmasked_op(values[0], values[1]);
    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    REQUIRE(traits::dom_conv(mv_a, counter_domain) == DOM_OK);
    REQUIRE(mv_a->domain == counter_domain);
    REQUIRE(masked_op(mv_a, mv_b, mv_out) == DOM_OK);
    REQUIRE(mv_a->domain == domain);

    REQUIRE(traits::dom_unmask(mv_out, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);

    traits::dom_free(mv_a);
    traits::dom_free(mv_b);
    traits::dom_free(mv_out);
}


template<typename T>
void test_unary_operation(
        ECODE (*masked_op)(typename dom_traits<T>::mskd_t*),
        const std::function<T(T)>& unmasked_op,
        domain_t domain
) {
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[1];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto res = traits::dom_mask(values[0], order, domain);
    REQUIRE(res.error == DOM_OK);
    REQUIRE(res.mv != nullptr);
    auto* mv = res.mv;
    T unmasked[1];

    REQUIRE(masked_op(mv) == DOM_OK);

    T expected = unmasked_op(values[0]);
    REQUIRE(traits::dom_unmask(mv, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    REQUIRE(traits::dom_conv(mv, counter_domain) == DOM_OK);
    REQUIRE(mv->domain == counter_domain);
    REQUIRE(masked_op(mv) == DOM_OK);
    REQUIRE(mv->domain == domain);

    traits::dom_free(mv);
}


template<typename T>
void test_shift_rotate_operation(
        ECODE (*masked_op)(typename dom_traits<T>::mskd_t*, uint8_t),
        const std::function<T(T, T)>& unmasked_op,
        domain_t domain
) {
    using traits = dom_traits<T>;
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    T values[1];
    csprng_read_array(reinterpret_cast<uint8_t*>(values), sizeof(values));
    auto res = traits::dom_mask(values[0], order, DOMAIN_BOOLEAN);
    REQUIRE(res.error == DOM_OK);
    REQUIRE(res.mv != nullptr);
    auto* mv = res.mv;
    T unmasked[1];

    uint8_t offset = static_cast<uint8_t>(mv->bit_length / 2) - 1;
    REQUIRE(masked_op(mv, offset) == DOM_OK);

    T expected = unmasked_op(values[0], offset);
    REQUIRE(traits::dom_unmask(mv, unmasked, 0) == DOM_OK);
    REQUIRE(unmasked[0] == expected);

    // Assert automatic domain conversion
    domain_t counter_domain = domain == DOMAIN_BOOLEAN ? DOMAIN_ARITHMETIC : DOMAIN_BOOLEAN;
    REQUIRE(traits::dom_conv(mv, counter_domain) == DOM_OK);
    REQUIRE(mv->domain == counter_domain);
    REQUIRE(masked_op(mv, offset) == DOM_OK);
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
