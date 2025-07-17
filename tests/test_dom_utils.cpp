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
#include <climits>
#include <cstring>
#include <vector>

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
    /* Single-instance helpers */                                                                                       \
    static void    dom_free       (mtp mv)                           { FN(dom_free, BL)(mv); }                          \
    static void    dom_clear      (mtp mv)                           { FN(dom_clear, BL)(mv); }                         \
    static mtp     dom_alloc      (uint8_t o, domain_t d)            { return FN(dom_alloc, BL)(o, d); }                \
    static mtp     dom_mask       (uint v, uint8_t o, domain_t d)    { return FN(dom_mask, BL)(v, o, d); }              \
    static uint    dom_unmask     (mtp mv)                           { return FN(dom_unmask, BL)(mv); }                 \
    static void    dom_refresh    (mtp mv)                           { FN(dom_refresh, BL)(mv); }                       \
    static mtp     dom_clone      (mtp mv, bool z)                   { return FN(dom_clone, BL)(mv, z); }               \
                                                                                                                        \
    /* Array helpers */                                                                                                 \
    static void    dom_free_many       (mtpa mvs, uint8_t count)     { FN(dom_free_many, BL)(mvs, count); }             \
    static void    dom_clear_many      (mtpa mvs, uint8_t count)     { FN(dom_clear_many, BL)(mvs, count); }            \
                                                                                                                        \
    static mtpa    dom_alloc_many      (uint8_t count, uint8_t order, domain_t domain)                                  \
                                       { return FN(dom_alloc_many, BL)(count, order, domain); }                         \
                                                                                                                        \
    static mtpa    dom_mask_many       (const uint* values, uint8_t count, uint8_t order, domain_t domain)              \
                                       { return FN(dom_mask_many, BL)(values, count, order, domain); }                  \
                                                                                                                        \
    static void    dom_unmask_many     (mtpa mvs, uint* out, uint8_t count)                                             \
                                       { FN(dom_unmask_many, BL)(mvs, out, count); }                                    \
                                                                                                                        \
    static void    dom_refresh_many    (mtpa mvs, uint8_t count)                                                        \
                                       { FN(dom_refresh_many, BL)(mvs, count); }                                        \
                                                                                                                        \
    static mtpa    dom_clone_many      (mtp mv, uint8_t count, bool zero_shares)                                        \
                                       { return FN(dom_clone_many, BL)(mv, count, zero_shares); }                       \
};                                                                                                                      \

DEFINE_DOM_TRAITS(8)
DEFINE_DOM_TRAITS(16)
DEFINE_DOM_TRAITS(32)
DEFINE_DOM_TRAITS(64)

#undef DEFINE_DOM_TRAITS


// -----------------------------------------------------------------------------
//  Helper type to iterate over the 6 (type, domain) combinations
// -----------------------------------------------------------------------------
template<typename T, domain_t Domain>
struct TypeDomainPair {
    using type = T;
    static constexpr domain_t domain = Domain;
};


// -----------------------------------------------------------------------------
//  Comprehensive test‑suite that exercises *all* public utilities
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE(
        "DOM utility functions - full coverage", "[unittest][dom_utils]",
        (TypeDomainPair<uint8_t, DOMAIN_BOOLEAN>),
        (TypeDomainPair<uint8_t, DOMAIN_ARITHMETIC>),
        (TypeDomainPair<uint16_t, DOMAIN_BOOLEAN>),
        (TypeDomainPair<uint16_t, DOMAIN_ARITHMETIC>),
        (TypeDomainPair<uint32_t, DOMAIN_BOOLEAN>),
        (TypeDomainPair<uint32_t, DOMAIN_ARITHMETIC>),
        (TypeDomainPair<uint64_t, DOMAIN_BOOLEAN>),
        (TypeDomainPair<uint64_t, DOMAIN_ARITHMETIC>)
) {
    using DataType = typename TestType::type;
    using MaskedType = typename dom_traits<DataType>::mskd_t;
    using traits = dom_traits<typename TestType::type>;
    constexpr domain_t domain = TestType::domain;

    const uint8_t order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    // ---------------------------------------------------------------------
    SECTION("Single allocation initialises all meta‑data and zeroes shares")
    {
        MaskedType* mv = traits::dom_alloc(order, domain);
        REQUIRE(mv != nullptr);
        REQUIRE(mv->domain == domain);
        REQUIRE(mv->order == order);
        REQUIRE(mv->share_count == order + 1);
        REQUIRE(mv->bit_length == sizeof(DataType) * CHAR_BIT);

        auto* shares = reinterpret_cast<DataType*>(mv->shares);
        for (uint8_t i = 0; i < mv->share_count; ++i)
            REQUIRE(shares[i] == static_cast<DataType>(0));

        traits::dom_free(mv);
    }

    // ---------------------------------------------------------------------
    SECTION("Bulk allocation produces *count* valid, independent objects")
    {
        constexpr uint8_t count = 4;
        MaskedType** mvs = traits::dom_alloc_many(count, order, domain);
        REQUIRE(mvs != nullptr);
        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(mvs[i] != nullptr);
            REQUIRE(mvs[i]->domain == domain);
            REQUIRE(mvs[i]->order == order);
        }
        traits::dom_free_many(mvs, count);
    }

    // ---------------------------------------------------------------------
    SECTION("Mask / unmask round‑trip retains original value")
    {
        DataType value;
        csprng_read_array(reinterpret_cast<uint8_t*>(&value), sizeof(value));

        MaskedType* mv = traits::dom_mask(value, order, domain);
        REQUIRE(traits::dom_unmask(mv) == value);

        traits::dom_free(mv);
    }

    // ---------------------------------------------------------------------
    SECTION("mask_many & unmask_many handle arrays consistently")
    {
        constexpr uint8_t count = 5;
        std::vector<DataType> values(count);
        csprng_read_array(reinterpret_cast<uint8_t*>(values.data()), count * sizeof(DataType));

        MaskedType** mvs = traits::dom_mask_many(values.data(), count, order, domain);
        REQUIRE(mvs != nullptr);

        std::vector<DataType> out(count, {});
        traits::dom_unmask_many(mvs, out.data(), count);
        REQUIRE(out == values);

        traits::dom_free_many(mvs, count);
    }

    // ---------------------------------------------------------------------
    SECTION("clear zeroes all shares while keeping meta‑data intact")
    {
        DataType value{};
        csprng_read_array(reinterpret_cast<uint8_t*>(&value), sizeof(value));

        MaskedType* mv = traits::dom_mask(value, order, domain);
        traits::dom_clear(mv);

        auto* shares = reinterpret_cast<DataType*>(mv->shares);
        for (uint8_t i = 0; i < mv->share_count; ++i)
            REQUIRE(shares[i] == static_cast<DataType>(0));

        traits::dom_free(mv);
    }

    // ---------------------------------------------------------------------
    SECTION("refresh keeps logical value but changes at least one share")
    {
        DataType value{};
        csprng_read_array(reinterpret_cast<uint8_t*>(&value), sizeof(value));
        MaskedType* mv = traits::dom_mask(value, order, domain);
        const auto* after = reinterpret_cast<const DataType*>(mv->shares);

        // Snapshot previous shares
        std::vector<DataType> before(mv->share_count);
        std::memcpy(before.data(), after, mv->share_bytes);

        uint8_t retries = 3;
        bool changed = false;

        while (retries) {
            traits::dom_refresh(mv);
            REQUIRE(traits::dom_unmask(mv) == value);

            for (uint8_t i = 0; i < mv->share_count; ++i)
                changed |= (after[i] != before[i]);

            if (changed)
                break;
            --retries;
        }
        REQUIRE(changed);
        traits::dom_free(mv);
    }

    // ---------------------------------------------------------------------
    SECTION("refresh_many updates every member in array")
    {
        constexpr uint8_t count = 3;
        std::vector<DataType> vals(count);
        csprng_read_array(reinterpret_cast<uint8_t*>(vals.data()), count * sizeof(DataType));

        MaskedType** mvs = traits::dom_mask_many(vals.data(), count, order, domain);
        REQUIRE(mvs != nullptr);

        // Preserve old shares for later comparison
        std::vector<std::vector<DataType>> snapshots(count);
        for (uint8_t i = 0; i < count; ++i) {
            snapshots[i].resize(mvs[i]->share_count);
            std::memcpy(snapshots[i].data(), mvs[i]->shares, mvs[i]->share_bytes);
        }

        traits::dom_refresh_many(mvs, count);

        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(traits::dom_unmask(mvs[i]) == vals[i]);
            bool changed = false;
            const auto* after = reinterpret_cast<const DataType*>(mvs[i]->shares);
            for (uint8_t j = 0; j < mvs[i]->share_count; ++j)
                changed |= (after[j] != snapshots[i][j]);
            REQUIRE(changed);  // at least one share altered
        }

        traits::dom_free_many(mvs, count);
    }

    // ---------------------------------------------------------------------
    SECTION("clone performs a deep copy with and without clear_shares")
    {
        DataType value{};
        csprng_read_array(reinterpret_cast<uint8_t*>(&value), sizeof(value));

        MaskedType *orig        = traits::dom_mask(value, order, domain);
        MaskedType *clone_full  = traits::dom_clone(orig, false);
        MaskedType *clone_zero  = traits::dom_clone(orig, true);

        // ---- zero_shares == false ----
        REQUIRE(clone_full != nullptr);
        REQUIRE(clone_full != orig);  // different memory
        REQUIRE(std::memcmp(clone_full, orig, orig->total_bytes) == 0); // identical content

        // Mutate clone, orig must stay intact
        auto* c_shares = reinterpret_cast<DataType*>(clone_full->shares);
        c_shares[0] ^= static_cast<DataType>(1);
        REQUIRE(traits::dom_unmask(orig) == value);

        // ---- zero_shares == true ----
        REQUIRE(clone_zero != nullptr);
        REQUIRE(clone_zero != orig);
        REQUIRE(clone_zero->order == orig->order);
        auto* z_shares = reinterpret_cast<DataType*>(clone_zero->shares);
        for (uint8_t i = 0; i < clone_zero->share_count; ++i)
            REQUIRE(z_shares[i] == static_cast<DataType>(0));

        traits::dom_free(clone_full);
        traits::dom_free(clone_zero);
        traits::dom_free(orig);
    }

    // ---------------------------------------------------------------------
    SECTION("clone_many replicates semantics across array")
    {
        DataType value{};
        csprng_read_array(reinterpret_cast<uint8_t*>(&value), sizeof(value));
        MaskedType* orig = traits::dom_mask(value, order, domain);
        constexpr uint8_t count = 4;

        // ---- zero_shares == false ----
        MaskedType** full_clones = traits::dom_clone_many(orig, count, false);
        REQUIRE(full_clones != nullptr);
        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(full_clones[i] != nullptr);
            REQUIRE(full_clones[i] != orig);
            for (uint8_t j = i + 1; j < count; ++j)
                REQUIRE(full_clones[i] != full_clones[j]);
            REQUIRE(std::memcmp(full_clones[i], orig, orig->total_bytes) == 0);
        }

        // mutate one clone to ensure independence
        auto* shares0 = reinterpret_cast<DataType*>(full_clones[0]->shares);
        shares0[0] ^= static_cast<DataType>(1);
        REQUIRE(traits::dom_unmask(orig) == value);
        for (uint8_t i = 1; i < count; ++i)
            REQUIRE(std::memcmp(full_clones[i], orig, orig->total_bytes) == 0);

        // ---- zero_shares == true ----
        MaskedType** zero_clones = traits::dom_clone_many(orig, count, true);
        REQUIRE(zero_clones != nullptr);
        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(zero_clones[i] != nullptr);
            auto* shares = reinterpret_cast<DataType*>(zero_clones[i]->shares);
            for (uint8_t s = 0; s < zero_clones[i]->share_count; ++s)
                REQUIRE(shares[s] == static_cast<DataType>(0));
        }

        traits::dom_free_many(full_clones, count);
        traits::dom_free_many(zero_clones, count);
        traits::dom_free(orig);
    }
}
