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
    static RES_MTP(BL)      dom_alloc           (uint8_t order, domain_t domain)                                        \
                                                { return FN(dom_alloc, BL)(order, domain); }                            \
                                                                                                                        \
    static RES_MTPA(BL)     dom_alloc_many      (uint8_t count, uint8_t order, domain_t domain)                         \
                                                { return FN(dom_alloc_many, BL)(count, order, domain); }                \
                                                                                                                        \
    static RES_MTP(BL)      dom_clone           (MTP(BL) mv, bool clear_shares)                                         \
                                                { return FN(dom_clone, BL)(mv, clear_shares); }                         \
                                                                                                                        \
    static RES_MTPA(BL)     dom_clone_many      (MTP(BL) mv, uint8_t count, bool clear_shares)                          \
                                                { return FN(dom_clone_many, BL)(mv, count, clear_shares); }             \
                                                                                                                        \
    static ECODE            dom_free            (MTP(BL) mv)                                                            \
                                                { return FN(dom_free, BL)(mv); }                                        \
                                                                                                                        \
    static ECODE            dom_free_many       (MTPA(BL) mvs, uint8_t count, bool free_array)                          \
                                                { return FN(dom_free_many, BL)(mvs, count, free_array); }               \
                                                                                                                        \
    static ECODE            dom_clear           (MTP(BL) mv)                                                            \
                                                { return FN(dom_clear, BL)(mv); }                                       \
                                                                                                                        \
    static ECODE            dom_clear_many      (MTPA(BL) mvs, uint8_t count)                                           \
                                                { return FN(dom_clear_many, BL)(mvs, count); }                          \
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
    static ECODE            dom_unmask_many     (MTPA(BL) mvs, UINT(BL)* out, uint8_t count)                            \
                                                { return FN(dom_unmask_many, BL)(mvs, out, count); }                    \
                                                                                                                        \
    static ECODE            dom_refresh         (MTP(BL) mv)                                                            \
                                                { return FN(dom_refresh, BL)(mv); }                                     \
                                                                                                                        \
    static ECODE            dom_refresh_many    (MTPA(BL) mvs, uint8_t count)                                           \
                                                { return FN(dom_refresh_many, BL)(mvs, count); }                        \
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


TEMPLATE_TEST_CASE(
        "DOM core functions - full coverage",
        "[unittest][dom_core]", uint8_t, uint16_t, uint32_t, uint64_t
) {
    using traits = dom_traits<TestType>;

    const uint8_t order = GENERATE_COPY(range(1, 4));
    const domain_t domain = GENERATE_COPY(DOMAIN_ARITHMETIC, DOMAIN_BOOLEAN);

    INFO("security order = " << order);
    INFO("masking domain = " << (domain == DOMAIN_ARITHMETIC ? "arithmetic" : "boolean"));

    // ---------------------------------------------------------------------
    SECTION("Single allocation initialises all meta‑data and zeroes shares")
    {
        auto res = traits::dom_alloc(order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.mv != nullptr);
        REQUIRE(res.mv->domain == domain);
        REQUIRE(res.mv->order == order);
        REQUIRE(res.mv->share_count == order + 1);
        REQUIRE(res.mv->bit_length == sizeof(TestType) * CHAR_BIT);

        auto* shares = res.mv->shares;
        for (uint8_t i = 0; i < res.mv->share_count; ++i)
            REQUIRE(shares[i] == static_cast<TestType>(0));

        traits::dom_free(res.mv);
    }

    // ---------------------------------------------------------------------
    SECTION("Bulk allocation produces *count* valid, independent objects")
    {
        constexpr uint8_t count = 4;
        auto res = traits::dom_alloc_many(count, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.count == count);
        REQUIRE(res.mvs != nullptr);

        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(res.mvs[i] != nullptr);
            REQUIRE(res.mvs[i]->domain == domain);
            REQUIRE(res.mvs[i]->order == order);
        }

        traits::dom_free_many(res.mvs, count, true);
    }

    // ---------------------------------------------------------------------
    SECTION("Mask / unmask round‑trip retains original value")
    {
        TestType value;
        csprng_read_array(as_byte_ptr(&value), sizeof(value));

        auto res = traits::dom_mask(value, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.mv != nullptr);

        std::array<TestType, 1> unmasked_array = {};
        TestType* unmasked = unmasked_array.data();

        REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
        REQUIRE(unmasked[0] == value);

        traits::dom_free(res.mv);
    }

    // ---------------------------------------------------------------------
    SECTION("mask_many & unmask_many handle arrays consistently")
    {
        constexpr uint8_t count = 5;
        std::vector<TestType> values(count);
        csprng_read_array(as_byte_ptr(values.data()), count * sizeof(TestType));

        auto res = traits::dom_mask_many(values.data(), count, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.count == count);
        REQUIRE(res.mvs != nullptr);

        std::vector<TestType> out(count, {});
        REQUIRE(traits::dom_unmask_many(res.mvs, out.data(), count) == DOM_OK);
        REQUIRE(out == values);

        traits::dom_free_many(res.mvs, count, true);
    }

    // ---------------------------------------------------------------------
    SECTION("clear zeroes all shares while keeping meta‑data intact")
    {
        TestType value{};
        csprng_read_array(as_byte_ptr(&value), sizeof(value));

        auto res = traits::dom_mask(value, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.mv != nullptr);

        REQUIRE(traits::dom_clear(res.mv) == DOM_OK);

        auto* shares = res.mv->shares;
        for (uint8_t i = 0; i < res.mv->share_count; ++i)
            REQUIRE(shares[i] == static_cast<TestType>(0));

        traits::dom_free(res.mv);
    }

    // ---------------------------------------------------------------------
    SECTION("refresh keeps logical value but changes at least one share")
    {
        TestType value{};
        csprng_read_array(as_byte_ptr(&value), sizeof(value));

        auto res = traits::dom_mask(value, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.mv != nullptr);

        const auto* after = res.mv->shares;

        // Snapshot previous shares
        std::vector<TestType> before(res.mv->share_count);
        std::memcpy(before.data(), after, res.mv->share_bytes);

        std::array<TestType, 1> unmasked_array = {};
        TestType* unmasked = unmasked_array.data();

        uint8_t retries = 5;
        bool changed = false;

        while (retries) {
            REQUIRE(traits::dom_refresh(res.mv) == DOM_OK);
            REQUIRE(traits::dom_unmask(res.mv, unmasked, 0) == DOM_OK);
            REQUIRE(unmasked[0] == value);

            for (uint8_t i = 0; i < res.mv->share_count; ++i)
                changed |= (after[i] != before[i]);

            if (changed)
                break;
            --retries;
        }
        REQUIRE(changed);
        traits::dom_free(res.mv);
    }

    // ---------------------------------------------------------------------
    SECTION("refresh_many updates every member in array")
    {
        constexpr uint8_t count = 5;
        std::vector<TestType> vals(count);
        csprng_read_array(as_byte_ptr(vals.data()), count * sizeof(TestType));

        auto res = traits::dom_mask_many(vals.data(), count, order, domain);
        REQUIRE(res.error == DOM_OK);
        REQUIRE(res.count == count);
        REQUIRE(res.mvs != nullptr);

        // Preserve old shares for later comparison
        std::vector<std::vector<TestType>> snapshots(count);
        for (uint8_t i = 0; i < count; ++i) {
            snapshots[i].resize(res.mvs[i]->share_count);
            std::memcpy(
                snapshots[i].data(),
                res.mvs[i]->shares,
                res.mvs[i]->share_bytes
            );
        }

        traits::dom_refresh_many(res.mvs, count);

        std::array<TestType, 1> unmasked_array = {};
        TestType* unmasked = unmasked_array.data();
        bool changed = false;

        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(traits::dom_unmask(res.mvs[i], unmasked, 0) == 0);
            REQUIRE(unmasked[0] == vals[i]);
            const auto* after = res.mvs[i]->shares;
            for (uint8_t j = 0; j < res.mvs[i]->share_count; ++j)
                changed |= (after[j] != snapshots[i][j]);
        }
        REQUIRE(changed);  // at least one share altered

        traits::dom_free_many(res.mvs, count, true);
    }

    // ---------------------------------------------------------------------
    SECTION("clone performs a deep copy with and without clear_shares")
    {
        TestType value{};
        csprng_read_array(as_byte_ptr(&value), sizeof(value));

        auto orig        = traits::dom_mask(value, order, domain);
        auto clone_full  = traits::dom_clone(orig.mv, false);
        auto clone_zero  = traits::dom_clone(orig.mv, true);

        // ---- zero_shares == false ----
        REQUIRE(clone_full.error == DOM_OK);
        REQUIRE(clone_full.mv != nullptr);
        REQUIRE(clone_full.mv != orig.mv);  // different memory
        REQUIRE(std::memcmp(
            clone_full.mv,
            orig.mv,
            orig.mv->total_bytes
        ) == 0); // identical content

        // Mutate clone, orig must stay intact
        auto* c_shares = clone_full.mv->shares;
        c_shares[0] ^= static_cast<TestType>(1);

        std::array<TestType, 1> unmasked_array = {};
        TestType* unmasked = unmasked_array.data();

        REQUIRE(traits::dom_unmask(orig.mv, unmasked, 0) == DOM_OK);
        REQUIRE(unmasked[0] == value);

        // ---- zero_shares == true ----
        REQUIRE(clone_zero.error == DOM_OK);
        REQUIRE(clone_zero.mv != nullptr);
        REQUIRE(clone_zero.mv != orig.mv);
        REQUIRE(clone_zero.mv->order == orig.mv->order);

        auto* z_shares = clone_zero.mv->shares;
        for (uint8_t i = 0; i < clone_zero.mv->share_count; ++i)
            REQUIRE(z_shares[i] == static_cast<TestType>(0));

        traits::dom_free(clone_full.mv);
        traits::dom_free(clone_zero.mv);
        traits::dom_free(orig.mv);
    }

    // ---------------------------------------------------------------------
    SECTION("clone_many replicates semantics across array")
    {
        TestType value;
        csprng_read_array(as_byte_ptr(&value), sizeof(value));

        auto orig = traits::dom_mask(value, order, domain);
        constexpr uint8_t count = 4;

        // ---- zero_shares == false ----
        auto full_clones = traits::dom_clone_many(orig.mv, count, false);
        REQUIRE(full_clones.error == DOM_OK);
        REQUIRE(full_clones.count == count);
        REQUIRE(full_clones.mvs != nullptr);

        for (uint8_t i = 0; i < count; ++i) {
            auto mv = full_clones.mvs[i];
            REQUIRE(mv != nullptr);
            REQUIRE(mv != orig.mv);
            for (uint8_t j = i + 1; j < count; ++j)
                REQUIRE(mv != full_clones.mvs[j]);
            REQUIRE(std::memcmp(
                full_clones.mvs[i],
                orig.mv,
                orig.mv->total_bytes
            ) == 0);
        }

        // mutate one clone to ensure independence
        auto* shares0 = full_clones.mvs[0]->shares;
        shares0[0] ^= static_cast<TestType>(1);

        std::array<TestType, 1> unmasked_array = {};
        TestType* unmasked = unmasked_array.data();

        REQUIRE(traits::dom_unmask(orig.mv, unmasked, 0) == DOM_OK);
        REQUIRE(unmasked[0] == value);

        for (uint8_t i = 1; i < count; ++i) {
            REQUIRE(std::memcmp(
                full_clones.mvs[i],
                orig.mv,
                orig.mv->total_bytes
            ) == 0);
        }

        // ---- zero_shares == true ----
        auto zero_clones = traits::dom_clone_many(orig.mv, count, true);
        REQUIRE(zero_clones.error == DOM_OK);
        REQUIRE(zero_clones.count == count);
        REQUIRE(zero_clones.mvs != nullptr);

        for (uint8_t i = 0; i < count; ++i) {
            auto mv = zero_clones.mvs[i];
            REQUIRE(mv != nullptr);
            auto* shares = mv->shares;
            for (uint8_t s = 0; s < mv->share_count; ++s)
                REQUIRE(shares[s] == static_cast<TestType>(0));
        }

        traits::dom_free_many(full_clones.mvs, count, true);
        traits::dom_free_many(zero_clones.mvs, count, true);
        traits::dom_free(orig.mv);
    }
}
