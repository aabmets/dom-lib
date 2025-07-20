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

from src import BaseMaskedUint, BaseUint, Domain, MaskedUint8, MaskedUint32, MaskedUint64

__all__ = [
    "test_masking_unmasking",
    "test_domain_conversion",
    "test_boolean_and",
    "test_boolean_or",
    "test_boolean_xor",
    "test_boolean_not",
    "test_boolean_rshift",
    "test_boolean_lshift",
    "test_boolean_rotr",
    "test_boolean_rotl",
    "test_arithmetic_neg",
    "test_arithmetic_add",
    "test_arithmetic_sub",
    "test_arithmetic_mul",
]


def get_randomly_masked_uint(
    cls: t.Type[BaseMaskedUint], domain: Domain, order: int
) -> tuple[BaseUint, BaseMaskedUint]:
    uint_cls = cls.uint_class()
    bit_length = uint_cls.bit_length()
    value = secrets.randbits(bit_length)
    masked_uint = cls(value, domain, order)
    return uint_cls(value), masked_uint


def get_many_randomly_masked_uints(
    cls: t.Type[BaseMaskedUint], domain: Domain, order: int, *, count: int = 3
) -> tuple[list[BaseUint], list[BaseMaskedUint]]:
    values, mvs = [], []
    for _ in range(count):
        value, mv = get_randomly_masked_uint(cls, domain, order)
        values.append(value)
        mvs.append(mv)
    return values, mvs


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("domain", [Domain.BOOLEAN, Domain.ARITHMETIC])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_masking_unmasking(cls, domain, order):
    value, mv = get_randomly_masked_uint(cls, domain, order)

    def assert_unmasking():
        assert len(mv.masks) == order
        unmasking_fn = opr.xor if domain == Domain.BOOLEAN else opr.add
        unmasked_value = mv.masked_value
        for mask in mv.masks:
            unmasked_value = unmasking_fn(unmasked_value, mask)
        assert value == unmasked_value == mv.unmask()

    assert_unmasking()
    mv.refresh_masks()
    assert_unmasking()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_domain_conversion(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)

    def assert_unmasking(domain: Domain, unmasking_fn: t.Callable):
        assert mv.domain == domain
        assert mv.unmasking_fn == unmasking_fn
        unmasked_value = mv.masked_value
        for mask in mv.masks:
            unmasked_value = unmasking_fn(unmasked_value, mask)
        assert value == unmasked_value == mv.unmask()

    assert_unmasking(Domain.BOOLEAN, opr.xor)
    mv.btoa()
    assert_unmasking(Domain.ARITHMETIC, opr.add)
    mv.atob()
    assert_unmasking(Domain.BOOLEAN, opr.xor)


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_and(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.BOOLEAN, order)
    expected = values[0] & values[1] & values[2]
    result = mvs[0] & mvs[1] & mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_or(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.BOOLEAN, order)
    expected = values[0] | values[1] | values[2]
    result = mvs[0] | mvs[1] | mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_xor(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.BOOLEAN, order)
    expected = values[0] ^ values[1] ^ values[2]
    result = mvs[0] ^ mvs[1] ^ mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_not(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)
    assert ~value == ~mv.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_rshift(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)
    distance = secrets.choice(range(1, value.bit_length()))
    assert (value >> distance) == (mv >> distance).unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_lshift(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)
    distance = secrets.choice(range(1, value.bit_length()))
    assert (value << distance) == (mv << distance).unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_rotr(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)
    distance = secrets.choice(range(1, value.bit_length()))
    assert value.rotr(distance) == mv.rotr(distance).unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_boolean_rotl(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.BOOLEAN, order)
    distance = secrets.choice(range(1, value.bit_length()))
    assert value.rotl(distance) == mv.rotl(distance).unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_arithmetic_neg(cls, order):
    value, mv = get_randomly_masked_uint(cls, Domain.ARITHMETIC, order)
    assert -value == -mv.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_arithmetic_add(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.ARITHMETIC, order)
    expected = values[0] + values[1] + values[2]
    result = mvs[0] + mvs[1] + mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_arithmetic_sub(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.ARITHMETIC, order)
    expected = values[0] - values[1] - values[2]
    result = mvs[0] - mvs[1] - mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_arithmetic_mul(cls, order):
    values, mvs = get_many_randomly_masked_uints(cls, Domain.ARITHMETIC, order)
    expected = values[0] * values[1] * values[2]
    result = mvs[0] * mvs[1] * mvs[2]
    assert expected == result.unmask()


@pytest.mark.parametrize("cls", [MaskedUint8, MaskedUint32, MaskedUint64])
@pytest.mark.parametrize("domain", [Domain.BOOLEAN, Domain.ARITHMETIC])
@pytest.mark.parametrize("order", list(range(1, 11)))
def test_cross_domain_behavior(cls, domain, order):
    values, mvs = get_many_randomly_masked_uints(cls, domain, order)
    dist = MaskedUint32.uint_class().bit_length() // 2

    either = BaseUint | BaseMaskedUint

    def formula(a: either, b: either, c: either) -> either:
        return (a + b) ^ (b * (a | c).rotr(dist)) - (c & (a << dist))

    result1 = formula(*values)
    result2 = formula(*mvs)

    assert result1 == result2.unmask()
