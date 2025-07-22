#
#   Apache License 2.0
#
#   Copyright (c) 2025, Mattias Aabmets
#
#   The contents of this file are subject to the terms and conditions defined in the License.
#   You may not use, modify, or distribute this file except in compliance with the License.
#
#   SPDX-License-Identifier: Apache-2.0
#

from __future__ import annotations

from src import operators as ops
from src.base_masked_uint import BaseMaskedUint

__all__ = [
    "dom_cmp_lt",
    "dom_cmp_le",
    "dom_cmp_gt",
    "dom_cmp_ge",
    "dom_cmp_ne",
    "dom_cmp_eq",
]


def dom_cmp_lt(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    diff = ops.dom_bool_sub(a, b)
    t0 = ops.dom_bool_xor(a, b)
    t1 = ops.dom_bool_xor(diff, b)
    t2 = ops.dom_bool_or(t0, t1)
    t3 = ops.dom_bool_xor(a, t2)
    out = ops.dom_bool_shr(t3, t3.bit_length() - 1)
    out.refresh_masks()
    return out


def dom_cmp_le(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    out = dom_cmp_lt(b, a)
    out.masked_value ^= 0x1
    return out


def dom_cmp_gt(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    return dom_cmp_lt(b, a)


def dom_cmp_ge(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    out = dom_cmp_lt(a, b)
    out.masked_value ^= 0x1
    return out


def dom_cmp_ne(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    lt_ab = dom_cmp_lt(a, b)
    lt_ba = dom_cmp_lt(b, a)
    return ops.dom_bool_xor(lt_ab, lt_ba)


def dom_cmp_eq(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    out = dom_cmp_ne(a, b)
    out.masked_value ^= 0x1
    return out
