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

import typing as t
import operator as opr
import itertools as it
from src.base_masked_uint import BaseMaskedUint


__all__ = [
    "dom_ksa_carry",
    "dom_ksa_borrow",
    "dom_bool_and",
    "dom_bool_or",
    "dom_bool_xor",
    "dom_bool_not",
    "dom_bool_shr",
    "dom_bool_shl",
    "dom_bool_rotr",
    "dom_bool_rotl",
    "dom_bool_sub",
    "dom_bool_add",
    "dom_arith_add",
    "dom_arith_sub",
    "dom_arith_mult",
]


def dom_ksa_carry(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    rounds = [1, 2, 4, 8, 16, 32, 64]
    bl = a.bit_length()

    p = dom_bool_xor(a, b)
    g = dom_bool_and(a, b)

    for dist in it.takewhile(lambda x: x < bl, rounds):
        p_shift = dom_bool_shl(p, dist)
        g_shift = dom_bool_shl(g, dist)
        tmp = dom_bool_and(p, g_shift)
        g = dom_bool_xor(g, tmp)
        p = dom_bool_and(p, p_shift)

    return dom_bool_shl(g, 1)


def dom_ksa_borrow(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    rounds = [1, 2, 4, 8, 16, 32, 64]
    bl = a.bit_length()

    a_inv = dom_bool_not(a)
    p = dom_bool_xor(a_inv, b)
    g = dom_bool_and(a_inv, b)

    for dist in it.takewhile(lambda x: x < bl, rounds):
        p_shift = dom_bool_shl(p, dist)
        g_shift = dom_bool_shl(g, dist)
        tmp1 = dom_bool_and(p, g_shift)
        tmp2 = dom_bool_and(g, tmp1)
        g = dom_bool_xor(g, tmp1)
        g = dom_bool_xor(g, tmp2)
        p = dom_bool_and(p, p_shift)

    return dom_bool_shl(g, 1)


def _and_mult_helper(a: BaseMaskedUint, b: BaseMaskedUint, operator: t.Callable) -> BaseMaskedUint:
    """
    Performs multiplication/AND logic on two masked shares using the DOM-independent
    secure gadget as described by Gross et al. in “Domain-Oriented Masking” (CHES 2016).
    Link: https://eprint.iacr.org/2016/486.pdf
    """
    x, y = a.shares, b.shares
    out = [operator(x[i], y[i]) for i in range(a.share_count)]

    pair_count = a.share_count * a.order // 2
    rand_vals = iter(a.get_random_uints(pair_count))

    for i in range(a.order):
        for j in range(i + 1, a.share_count):
            rand = next(rand_vals)
            o_ji = operator(x[j], y[i])
            o_ij = operator(x[i], y[j])
            p_ji = a.masking_fn(o_ji, rand)
            p_ij = a.unmasking_fn(o_ij, rand)
            out[i] = a.unmasking_fn(out[i], p_ij)
            out[j] = a.unmasking_fn(out[j], p_ji)

    return a.clone(out)


def dom_bool_and(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    return _and_mult_helper(a, b, opr.and_)


def dom_bool_or(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    x, y = a.shares, b.shares
    out = dom_bool_and(a, b).shares

    for i in range(a.share_count):
        out[i] ^= x[i] ^ y[i]

    return a.clone(out)


def dom_bool_xor(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    x, y, out = a.shares, b.shares, []
    for i in range(a.share_count):
        out.append(x[i] ^ y[i])
    return a.clone(out)


def dom_bool_not(mv: BaseMaskedUint) -> BaseMaskedUint:
    return mv.clone([~mv.masked_value, *mv.masks])


def _shift_rotate_helper(mv: BaseMaskedUint, operation: str, distance: int = None) -> BaseMaskedUint:
    new_mv = getattr(mv.masked_value, operation)(distance)
    new_masks = [getattr(m, operation)(distance) for m in mv.masks]
    return mv.clone([new_mv, *new_masks])


def dom_bool_shr(mv: BaseMaskedUint, distance: int) -> BaseMaskedUint:
    return _shift_rotate_helper(mv, "__rshift__", distance)


def dom_bool_shl(mv: BaseMaskedUint, distance: int) -> BaseMaskedUint:
    return _shift_rotate_helper(mv, "__lshift__", distance)


def dom_bool_rotr(mv: BaseMaskedUint, distance: int) -> BaseMaskedUint:
    return _shift_rotate_helper(mv, "rotr", distance)


def dom_bool_rotl(mv: BaseMaskedUint, distance: int) -> BaseMaskedUint:
    return _shift_rotate_helper(mv, "rotl", distance)


def dom_bool_sub(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    borrow = dom_ksa_borrow(a, b)
    out = dom_bool_xor(a, b)
    return dom_bool_xor(out, borrow)


def dom_bool_add(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    carry = dom_ksa_carry(a, b)
    out = dom_bool_xor(a, b)
    return dom_bool_xor(out, carry)


def dom_arith_add(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    x, y = a.shares, b.shares
    out = [x[i] + y[i] for i in range(a.share_count)]
    return a.clone(out)


def dom_arith_sub(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    x, y = a.shares, b.shares
    out = [x[i] - y[i] for i in range(a.share_count)]
    return a.clone(out)


def dom_arith_mult(a: BaseMaskedUint, b: BaseMaskedUint) -> BaseMaskedUint:
    return _and_mult_helper(a, b, opr.mul)
