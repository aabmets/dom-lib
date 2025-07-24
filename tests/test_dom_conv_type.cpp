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
    using large_mtp = MTP(BLL);                                                                                         \
    using large_mtpa = MTPA(BLL);                                                                                       \
    using large_uint = UINT(BLL);                                                                                       \
                                                                                                                        \
    using small_mtp = MTP(BLS);                                                                                         \
    using small_mtpa = MTPA(BLS);                                                                                       \
    using small_uint = UINT(BLS);                                                                                       \
                                                                                                                        \
    static small_mtp     mask_small         (small_uint v, uint8_t o, domain_t d)                                       \
                                            { return FN(dom_mask, BLS)(v, o, d); }                                      \
                                                                                                                        \
    static small_uint    unmask_small       (small_mtp mv, small_uint* o, uint8_t i)                                    \
                                            { return FN(dom_unmask, BLS)(mv, o, i); }                                   \
                                                                                                                        \
    static large_uint    unmask_large       (large_mtp mv, large_uint* o, uint8_t i)                                    \
                                            { return FN(dom_unmask, BLL)(mv, o, i); }                                   \
                                                                                                                        \
    static void          free_small_many    (small_mtpa mvs, uint8_t c, bool fa)                                        \
                                            { FN(dom_free_many, BLS)(mvs, c, fa); }                                     \
                                                                                                                        \
    static void          free_small         (small_mtp mv)        { FN(dom_free, BLS)(mv); }                            \
    static void          free_large         (large_mtp mv)        { FN(dom_free, BLL)(mv); }                            \
    static large_mtp     to_large           (small_mtpa parts)    { return FNCT(BLS, BLL)(parts); }                     \
    static small_mtpa    to_small           (large_mtp mv)        { return FNCT(BLL, BLS)(mv); }                        \
};                                                                                                                      \

DEFINE_DOM_TRAITS(64, 32)   // 2/1 ratio
DEFINE_DOM_TRAITS(32, 16)   // 2/1 ratio
DEFINE_DOM_TRAITS(16, 8)    // 2/1 ratio
DEFINE_DOM_TRAITS(64, 16)   // 4/1 ratio
DEFINE_DOM_TRAITS(32, 8)    // 4/1 ratio
DEFINE_DOM_TRAITS(64, 8)    // 8/1 ratio

#undef DEFINE_DOM_TRAITS


template<typename L, typename S>
static void roundtrip(uint8_t order)
{
    using traits = dom_traits<L, S>;
    constexpr size_t PARTS = sizeof(L) / sizeof(S);

    L original;
    csprng_read_array(reinterpret_cast<uint8_t*>(&original), sizeof(original));

    constexpr unsigned DIST_BITS = sizeof(S) * 8u;
    std::array<S, PARTS> chunks{};
    for (size_t i = 0; i < PARTS; ++i)
        chunks[i] = static_cast<S>(original >> (i * DIST_BITS));

    std::array<typename traits::small_mtp, PARTS> parts{};
    for (size_t i = 0; i < PARTS; ++i) {
        parts[i] = traits::mask_small(chunks[i], order, DOMAIN_BOOLEAN);
        REQUIRE(parts[i] != nullptr);
    }

    auto* mv_large = traits::to_large(parts.data());
    REQUIRE(mv_large != nullptr);

    L unmasked_large[1];
    REQUIRE(traits::unmask_large(mv_large, unmasked_large, 0) == 0);
    REQUIRE(unmasked_large[0] == original);

    auto** back = traits::to_small(mv_large);
    REQUIRE(back != nullptr);

    S unmasked_small[1];
    for (size_t i = 0; i < PARTS; ++i) {
        REQUIRE(back[i] != nullptr);
        REQUIRE(traits::unmask_small(back[i], unmasked_small, 0) == 0);
        REQUIRE(unmasked_small[0] == chunks[i]);
    }

    for (auto* mv : parts)
        traits::free_small(mv);
    traits::free_large(mv_large);
    traits::free_small_many(back, static_cast<uint8_t>(PARTS), true);
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
