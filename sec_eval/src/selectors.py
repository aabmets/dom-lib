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
    "dom_select",
    "dom_select_lt",
    "dom_select_le",
    "dom_select_gt",
    "dom_select_ge",
]


def dom_cmp_lt(a: BaseMaskedUint, b: BaseMaskedUint, full_mask: bool = False) -> BaseMaskedUint:
    diff = ops.dom_bool_sub(a, b)
    t0 = ops.dom_bool_xor(a, b)
    t1 = ops.dom_bool_xor(diff, b)
    tmp = ops.dom_bool_or(t0, t1)
    tmp = ops.dom_bool_xor(a, tmp)
    out = ops.dom_bool_shr(tmp, tmp.bit_length() - 1)
    if full_mask:
        c = a.__class__(1, a.order, a.domain)
        out = ops.dom_bool_sub(out, c)
        out = ops.dom_bool_not(out)
    out.refresh_masks()
    return out


def dom_cmp_le(a: BaseMaskedUint, b: BaseMaskedUint, full_mask: bool = False) -> BaseMaskedUint:
    out = dom_cmp_lt(b, a, full_mask)
    mask = a.max_value() if full_mask else 0x1
    out.masked_value ^= mask
    return out


def dom_cmp_gt(a: BaseMaskedUint, b: BaseMaskedUint, full_mask: bool = False) -> BaseMaskedUint:
    return dom_cmp_lt(b, a, full_mask)


def dom_cmp_ge(a: BaseMaskedUint, b: BaseMaskedUint, full_mask: bool = False) -> BaseMaskedUint:
    out = dom_cmp_lt(a, b, full_mask)
    mask = a.max_value() if full_mask else 0x1
    out.masked_value ^= mask
    return out


def dom_select(a: BaseMaskedUint, b: BaseMaskedUint, mask: BaseMaskedUint) -> BaseMaskedUint:
    diff = ops.dom_bool_xor(a, b)
    diff = ops.dom_bool_and(mask, diff)
    out = ops.dom_bool_xor(diff, b)
    out.refresh_masks()
    return out


def dom_select_lt(
        a_cmp: BaseMaskedUint,
        b_cmp: BaseMaskedUint,
        truth_sel: BaseMaskedUint,
        false_sel: BaseMaskedUint
) -> BaseMaskedUint:
    mask = dom_cmp_lt(a_cmp, b_cmp, full_mask=True)
    return dom_select(truth_sel, false_sel, mask)


def dom_select_le(
        a_cmp: BaseMaskedUint,
        b_cmp: BaseMaskedUint,
        truth_sel: BaseMaskedUint,
        false_sel: BaseMaskedUint
) -> BaseMaskedUint:
    mask = dom_cmp_le(a_cmp, b_cmp, full_mask=True)
    return dom_select(truth_sel, false_sel, mask)


def dom_select_gt(
        a_cmp: BaseMaskedUint,
        b_cmp: BaseMaskedUint,
        truth_sel: BaseMaskedUint,
        false_sel: BaseMaskedUint
) -> BaseMaskedUint:
    mask = dom_cmp_gt(a_cmp, b_cmp, full_mask=True)
    return dom_select(truth_sel, false_sel, mask)


def dom_select_ge(
        a_cmp: BaseMaskedUint,
        b_cmp: BaseMaskedUint,
        truth_sel: BaseMaskedUint,
        false_sel: BaseMaskedUint
) -> BaseMaskedUint:
    mask = dom_cmp_ge(a_cmp, b_cmp, full_mask=True)
    return dom_select(truth_sel, false_sel, mask)
