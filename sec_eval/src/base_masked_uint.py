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

import operator as opr
import secrets
import typing as t
from abc import ABC, abstractmethod
from copy import deepcopy
from enum import Enum

from src.base_uint import BaseUint

__all__ = ["Domain", "BaseMaskedUint"]


class Domain(Enum):
    BOOLEAN = 0
    ARITHMETIC = 1


class BaseMaskedUint(ABC):
    @staticmethod
    @abstractmethod
    def uint_class() -> t.Type[BaseUint]: ...

    @staticmethod
    @abstractmethod
    def bit_length() -> int: ...

    @staticmethod
    @abstractmethod
    def max_value() -> int: ...

    @classmethod
    def get_random_uints(cls, count: int) -> list[BaseUint]:
        uint, bl = cls.uint_class(), cls.bit_length()
        return [uint(secrets.randbits(bl)) for _ in range(count)]

    @property
    def shares(self) -> list[BaseUint]:
        for index, uint in enumerate([self.masked_value, *self.masks]):
            self._shares[index] = uint
        return self._shares

    @shares.setter
    def shares(self, shares: list[BaseUint]) -> None:
        for index, uint in enumerate(shares):
            self._shares[index] = uint
        self.masked_value = shares[0]
        self.masks = shares[1:]

    @property
    def hamming_weights(self) -> list[int]:
        return [s.hamming_weight for s in self.shares]

    def __init__(self, value: int | BaseUint, order: int, domain: Domain) -> None:
        self.order = order
        self.domain = domain
        self.share_count = order + 1
        self.masking_fn = opr.xor if domain == Domain.BOOLEAN else opr.sub
        self.unmasking_fn = opr.xor if domain == Domain.BOOLEAN else opr.add
        self.masks = self.get_random_uints(order)
        self.masked_value: BaseUint = self.uint_class()(value)
        self._shares: list[BaseUint] = [self.masked_value, *self.masks]
        for mask in self.masks:
            self.masked_value = self.masking_fn(self.masked_value, mask)

    def unmask(self) -> BaseUint:
        masked_value = self.masked_value
        for mask in self.masks:
            masked_value = self.unmasking_fn(masked_value, mask)
        return masked_value

    def refresh_masks(self) -> None:
        new_masks = self.get_random_uints(self.order)
        for index, mask in enumerate(new_masks):
            self.masks[index] = self.unmasking_fn(self.masks[index], mask)
            self.masked_value = self.masking_fn(self.masked_value, mask)

    def update(self, shares: list[BaseUint], domain: Domain = None) -> BaseMaskedUint:
        self.shares = shares
        self.domain = domain or self.domain
        self.masking_fn = opr.xor if self.domain == Domain.BOOLEAN else opr.sub
        self.unmasking_fn = opr.xor if self.domain == Domain.BOOLEAN else opr.add
        return self

    def clone(self, shares: list[BaseUint], domain: Domain = None) -> BaseMaskedUint:
        clone = deepcopy(self)
        clone.update(shares, domain)
        return clone
