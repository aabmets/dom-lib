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
import secrets
import operator as opr
from src import BaseMaskedUint, BaseUint, Domain

__all__ = [
    "assert_unmasking",
    "get_randomly_masked_value",
    "get_many_randomly_masked_values",
]


def assert_unmasking(mv: BaseMaskedUint, domain: Domain, expected_value: BaseUint):
    unmasking_fn = opr.xor if domain == Domain.BOOLEAN else opr.add
    assert mv.domain == domain
    assert mv.unmasking_fn == unmasking_fn
    unmasked_value = mv.masked_value
    for mask in mv.masks:
        unmasked_value = unmasking_fn(unmasked_value, mask)
    assert expected_value == unmasked_value == mv.unmask()


def get_randomly_masked_value(
        cls: t.Type[BaseMaskedUint],
        order: int,
        domain: Domain
) -> tuple[BaseUint, BaseMaskedUint]:
    uint_cls = cls.uint_class()
    bit_length = uint_cls.bit_length()
    value = secrets.randbits(bit_length)
    masked_uint = cls(value, order, domain)
    return uint_cls(value), masked_uint


def get_many_randomly_masked_values(
        cls: t.Type[BaseMaskedUint],
        count: int,
        order: int,
        domain: Domain
) -> tuple[list[BaseUint], list[BaseMaskedUint]]:
    values, mvs = [], []
    for _ in range(count):
        value, mv = get_randomly_masked_value(cls, order, domain)
        values.append(value)
        mvs.append(mv)
    return values, mvs
