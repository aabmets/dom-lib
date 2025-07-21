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

from functools import partial

from src.base_uint import BaseUint
from src import operators as ops
from src.base_masked_uint import Domain, BaseMaskedUint


__all__ = ["dom_conv_btoa", "dom_conv_atob"]


def dom_conv_btoa(mv: BaseMaskedUint) -> BaseMaskedUint:
    """
    Converts masked shares from boolean to arithmetic domain using
    the affine psi recursive decomposition method of Bettale et al.,
    "Improved High-Order Conversion From Boolean to Arithmetic Masking"
    Link: https://eprint.iacr.org/2018/328.pdf
    """
    if mv.domain != Domain.BOOLEAN:
        return mv
    uint = mv.uint_class()

    def psi(masked: BaseUint, mask: BaseUint) -> BaseUint:
        return (masked ^ mask) - mask

    def convert(x: list[BaseUint], n_plus1: int) -> list[BaseUint]:
        n = n_plus1 - 1
        if n == 1:
            return [x[0] ^ x[1]]

        new_masks = mv.get_random_uints(len(x) - 1)
        for index, mask in enumerate(new_masks, start=1):
            x[index] ^= mask
            x[0] ^= mask

        first_term = x[0] if (n - 1) & 1 else uint(0)
        y: list[BaseUint] = [first_term ^ psi(x[0], x[1])]
        y.extend([psi(x[0], x[i + 1]) for i in range(1, n)])

        first = convert(x[1:], n)
        second = convert(y, n)

        out = [first[i] + second[i] for i in range(n - 2)]
        out.extend([first[n - 2], second[n - 2]])
        return out

    bool_shares = [mv.masked_value, *mv.masks, uint(0)]
    arith_shares = convert(bool_shares, mv.share_count + 1)
    return mv.update(arith_shares, Domain.ARITHMETIC)


def dom_conv_atob(mv: BaseMaskedUint) -> BaseMaskedUint:
    """
    Converts masked shares from arithmetic to Boolean domain using
    the high-order recursive carry-save-adder method of Liu et al.,
    “A Low-Latency High-Order Arithmetic to Boolean Masking Conversion”
    Link: https://eprint.iacr.org/2024/045.pdf
    """
    if mv.domain != Domain.ARITHMETIC:
        return mv
    bmu_tuple = tuple[BaseMaskedUint, BaseMaskedUint]

    def csa(x: BaseMaskedUint, y: BaseMaskedUint, z: BaseMaskedUint) -> bmu_tuple:
        a = ops.dom_bool_xor(x, y)
        s = ops.dom_bool_xor(a, z)
        w = ops.dom_bool_xor(x, z)
        v = ops.dom_bool_and(a, w)
        c = ops.dom_bool_xor(x, v)
        c = ops.dom_bool_shl(c, 1)
        return s, c

    def csa_tree(vals: list[BaseMaskedUint]) -> bmu_tuple:
        if len(vals) == 3:
            return csa(vals[0], vals[1], vals[2])
        s, c = csa_tree(vals[:-1])
        return csa(s, c, vals[-1])

    cls = partial(mv.__class__, order=mv.order, domain=Domain.BOOLEAN)
    shares = [cls(v) for v in [mv.masked_value, *mv.masks]]

    if mv.share_count == 2:
        s_, c_ = shares[0], shares[1]
    else:
        s_, c_ = csa_tree(shares)

    carry = ops.dom_ksa_carry(s_, c_)
    result = ops.dom_bool_xor(s_, c_)
    result = ops.dom_bool_xor(result, carry)
    return mv.update(result.shares, Domain.BOOLEAN)
