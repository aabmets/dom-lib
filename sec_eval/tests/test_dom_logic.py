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

import operator as opr
import secrets
import typing as t

import pytest

from src import Domain, BaseUint
from src import operators as ops
from src import converters as conv
from tests import helpers as hlp
from tests import conftest as cfg

__all__ = [
    "test_masking_unmasking",
    "test_domain_converters",
    "test_dom_ksa_carry_borrow",
    "test_binary_operations",
    "test_boolean_shift_rotate",
    "test_boolean_inversion"
]


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("domain", [Domain.BOOLEAN, Domain.ARITHMETIC])
def test_masking_unmasking(cls, order, domain):
    value, mv = hlp.get_randomly_masked_value(cls, order, domain)
    hlp.assert_unmasking(mv, domain, value)
    mv.refresh_masks()
    hlp.assert_unmasking(mv, domain, value)


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
def test_domain_converters(cls, order):
    value, mv = hlp.get_randomly_masked_value(cls, order, Domain.BOOLEAN)
    hlp.assert_unmasking(mv, Domain.BOOLEAN, value)
    conv.dom_conv_btoa(mv)
    hlp.assert_unmasking(mv, Domain.ARITHMETIC, value)
    conv.dom_conv_atob(mv)
    hlp.assert_unmasking(mv, Domain.BOOLEAN, value)


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("test_set", [
    (ops.dom_ksa_carry, lambda ai, bi, carry: (ai + bi + carry) >> 1),
    (ops.dom_ksa_borrow, lambda ai, bi, borrow: (ai - bi - borrow) < 0)
])
def test_dom_ksa_carry_borrow(cls, order, test_set) -> None:
    ksa_fn, ref_fn = test_set
    values, mvs = hlp.get_many_randomly_masked_values(cls, 2, order, Domain.BOOLEAN)
    uint, bits = cls.uint_class(), cls.bit_length()

    def ref_ksa_word_shifted(a: BaseUint, b: BaseUint) -> BaseUint:
        ksa, ksa_word = 0, 0
        for i in range(bits):
            ai = int(a >> i & 1)
            bi = int(b >> i & 1)
            ksa = ref_fn(ai, bi, ksa)
            if ksa and i + 1 < bits:
                ksa_word |= 1 << (i + 1)
        return uint(ksa_word)

    expected = ref_ksa_word_shifted(values[0], values[1])
    result = ksa_fn(mvs[0], mvs[1])
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("test_set", [
    (opr.and_, ops.dom_bool_and, Domain.BOOLEAN),
    (opr.or_, ops.dom_bool_or, Domain.BOOLEAN),
    (opr.xor, ops.dom_bool_xor, Domain.BOOLEAN),
    (opr.sub, ops.dom_bool_sub, Domain.BOOLEAN),
    (opr.add, ops.dom_bool_add, Domain.BOOLEAN),
    (opr.sub, ops.dom_arith_sub, Domain.ARITHMETIC),
    (opr.add, ops.dom_arith_add, Domain.ARITHMETIC),
    (opr.mul, ops.dom_arith_mult, Domain.ARITHMETIC),
])
def test_binary_operations(cls, order, test_set: tuple[t.Callable, t.Callable, Domain]):
    ref_fn, masked_fn, domain = test_set
    values, mvs = hlp.get_many_randomly_masked_values(cls, 3, order, domain)
    expected = ref_fn(values[0], values[1])
    expected = ref_fn(expected, values[2])
    result = masked_fn(mvs[0], mvs[1])
    result = masked_fn(result, mvs[2])
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("test_set", [
    ("__rshift__", ops.dom_bool_shr),
    ("__lshift__", ops.dom_bool_shl),
    ("rotr", ops.dom_bool_rotr),
    ("rotl", ops.dom_bool_rotl),
])
def test_boolean_shift_rotate(cls, order, test_set: tuple[str, t.Callable]):
    ref_fn_name, masked_fn = test_set
    value, mv = hlp.get_randomly_masked_value(cls, order, Domain.BOOLEAN)
    distance = secrets.choice(range(1, value.bit_length()))
    expected = getattr(value, ref_fn_name)(distance)
    result = masked_fn(mv, distance)
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
def test_boolean_inversion(cls, order):
    value, mv = hlp.get_randomly_masked_value(cls, order, Domain.BOOLEAN)
    expected = ~value
    result = ops.dom_bool_not(mv)
    assert expected == result.unmask()
