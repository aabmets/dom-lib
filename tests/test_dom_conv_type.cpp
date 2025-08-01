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
#include <array>
#include <cinttypes>

#include "dom_api.h"
#include "internal/dom_internal_defs.h"
#include "internal/dom_internal_funcs.h"


template<typename L, typename S>
struct dom_traits;

#define DEFINE_DOM_TRAITS(BLL, BLS)                                                                                     \
template<>                                                                                                              \
struct dom_traits<UINT(BLL), UINT(BLS)> {                                                                               \
    using small_mtp = MTP(BLS);                                                                                         \
                                                                                                                        \
    static RES_MTP(BLS)     mask_small          (UINT(BLS) value, uint8_t order, domain_t domain)                        \
                                                { return FN(dom_mask, BLS)(value, order, domain); }                     \
                                                                                                                        \
    static ECODE            unmask_small        (MTP(BLS) mv, UINT(BLS)* out, uint8_t index)                             \
                                                { return FN(dom_unmask, BLS)(mv, out, index); }                         \
                                                                                                                        \
    static ECODE            unmask_large        (MTP(BLL) mv, UINT(BLL)* out, uint8_t index)                             \
                                                { return FN(dom_unmask, BLL)(mv, out, index); }                         \
                                                                                                                        \
    static ECODE            free_small_many     (MTPA(BLS) mvs, uint8_t count, bool free_array)                         \
                                                { return FN(dom_free_many, BLS)(mvs, count, free_array); }              \
                                                                                                                        \
    static ECODE            free_small          (MTP(BLS) mv)           { return FN(dom_free, BLS)(mv); }               \
    static ECODE            free_large          (MTP(BLL) mv)           { return FN(dom_free, BLL)(mv); }               \
    static RES_MTP(BLL)     to_large            (MTPA(BLS) parts)       { return FNCT(BLS, BLL)(parts); }               \
    static RES_MTPA(BLS)    to_small            (MTP(BLL) mv)           { return FNCT(BLL, BLS)(mv); }                  \
};                                                                                                                      \

DEFINE_DOM_TRAITS(64, 32)   // 2/1 ratio
DEFINE_DOM_TRAITS(32, 16)   // 2/1 ratio
DEFINE_DOM_TRAITS(16, 8)    // 2/1 ratio
DEFINE_DOM_TRAITS(64, 16)   // 4/1 ratio
DEFINE_DOM_TRAITS(32, 8)    // 4/1 ratio
DEFINE_DOM_TRAITS(64, 8)    // 8/1 ratio

#undef DEFINE_DOM_TRAITS


template<typename T>
uint8_t* as_byte_ptr(T* ptr) {
    return reinterpret_cast<uint8_t*>(ptr);  // NOSONAR
}


template<typename L, typename S>
static void roundtrip(uint8_t order)
{
    using traits = dom_traits<L, S>;
    constexpr size_t PARTS = sizeof(L) / sizeof(S);

    L original;
    csprng_read_array(as_byte_ptr(&original), sizeof(original));

    constexpr unsigned DIST_BITS = sizeof(S) * 8u;
    std::array<S, PARTS> chunks{};
    for (size_t i = 0; i < PARTS; ++i)
        chunks[i] = static_cast<S>(original >> (i * DIST_BITS));

    std::array<typename traits::small_mtp, PARTS> parts{};
    for (size_t i = 0; i < PARTS; ++i) {
        auto part = traits::mask_small(chunks[i], order, DOMAIN_BOOLEAN);
        REQUIRE(part.error == DOM_OK);
        REQUIRE(part.mv != nullptr);
        parts[i] = part.mv;
    }

    auto mv_large = traits::to_large(parts.data());
    REQUIRE(mv_large.error == DOM_OK);
    REQUIRE(mv_large.mv != nullptr);

    std::array<L, 1> unmasked_large_array = {};
    L* unmasked_large = unmasked_large_array.data();

    REQUIRE(traits::unmask_large(mv_large.mv, unmasked_large, 0) == DOM_OK);
    REQUIRE(unmasked_large[0] == original);

    auto back = traits::to_small(mv_large.mv);
    REQUIRE(back.error == DOM_OK);
    REQUIRE(back.mvs != nullptr);

    std::array<S, 1> unmasked_small_array = {};
    S* unmasked_small = unmasked_small_array.data();

    for (size_t i = 0; i < PARTS; ++i) {
        auto* mv = back.mvs[i];
        REQUIRE(mv != nullptr);
        REQUIRE(traits::unmask_small(mv, unmasked_small, 0) == DOM_OK);
        REQUIRE(unmasked_small[0] == chunks[i]);
    }

    for (auto* mv : parts){
        traits::free_small(mv);
    }
    traits::free_large(mv_large.mv);
    traits::free_small_many(back.mvs, static_cast<uint8_t>(PARTS), true);
}


TEST_CASE("DOM type‑converter round‑trip across ratios 2,4,8", "[unittest][dom_conv_type]")
{
    const int order = GENERATE_COPY(range(1, 4));
    INFO("security order = " << order);

    roundtrip<uint64_t, uint32_t>(static_cast<uint8_t>(order));   // 2/1 ratio
    roundtrip<uint32_t, uint16_t>(static_cast<uint8_t>(order));   // 2/1 ratio
    roundtrip<uint16_t, uint8_t >(static_cast<uint8_t>(order));   // 2/1 ratio
    roundtrip<uint64_t, uint16_t>(static_cast<uint8_t>(order));   // 4/1 ratio
    roundtrip<uint32_t, uint8_t >(static_cast<uint8_t>(order));   // 4/1 ratio
    roundtrip<uint64_t, uint8_t >(static_cast<uint8_t>(order));   // 8/1 ratio
}
