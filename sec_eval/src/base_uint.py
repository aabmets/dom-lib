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
import typing as t
from abc import ABC, abstractmethod


__all__ = ["IterNum", "ByteOrder", "BaseUint"]


T = t.TypeVar("T", bound="BaseUint")
ByteOrder = t.Literal["little", "big"]
IterNum = t.Union[t.Iterable[t.SupportsIndex], t.SupportsBytes]


class BaseUint(ABC):
    @staticmethod
    @abstractmethod
    def bit_length() -> int: ...

    @staticmethod
    @abstractmethod
    def max_value() -> int: ...

    @property
    def value(self) -> int:
        return self._value

    @property
    def hamming_weight(self) -> int:
        return self._value.bit_count()

    @value.setter
    def value(self, value: int) -> None:
        if isinstance(value, BaseUint):
            value = value.value
        elif not isinstance(value, int):
            raise TypeError(f"Cannot set {self.__class__.__name__} value from {value}")
        self._value = value & self.max_value()

    def __init__(self, value: int | BaseUint = 0) -> None:
        if isinstance(value, int):
            self.value = value & self.max_value()  # clamp value to bit length
        elif isinstance(getattr(value, "value", None), int):
            self.value = value.value
        else:
            raise TypeError(f"BaseUint value cannot be of type '{type(value)}'")

    @classmethod
    def from_bytes(cls: t.Type[T], data: IterNum, *, byteorder: ByteOrder = "big") -> T:
        return cls(int.from_bytes(data, byteorder, signed=False))

    def to_bytes(self, *, byteorder: ByteOrder = "big") -> bytes:
        return self.value.to_bytes(self.bit_length() // 8, byteorder)

    def _operate_binary(self, operator: t.Callable, other: int | BaseUint, cls: t.Type = None) -> t.Any:
        other_val = other.value if isinstance(other, BaseUint) else other
        result_val = operator(self._value, other_val)
        cls = cls or self.__class__
        return cls(result_val)

    def _operate_unary(self, operator: t.Callable[[int], int]) -> BaseUint:
        result_val = operator(self._value) & self.max_value()
        return self.__class__(result_val)

    def __index__(self) -> int:
        return self._value

    def __int__(self) -> int:
        return self._value

    def __str__(self) -> str:
        return str(self._value)

    def __add__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.add, other)

    def __sub__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.sub, other)

    def __mul__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.mul, other)

    def __pow__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.pow, other)

    def __mod__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.mod, other)

    def __and__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.and_, other)

    def __or__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.or_, other)

    def __xor__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.xor, other)

    def __rshift__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.rshift, other)

    def __lshift__(self, other: int | BaseUint) -> BaseUint:
        return self._operate_binary(opr.lshift, other)

    def __neg__(self) -> BaseUint:
        return self._operate_unary(opr.neg)

    def __invert__(self) -> BaseUint:
        return self._operate_unary(opr.invert)

    def rotl(self, n: int) -> BaseUint:
        """Rotates bits out from the left and back into the right"""
        distance = n % self.bit_length()
        rs = self._value >> (self.bit_length() - distance)
        ls = self._value << distance
        res = (rs | ls) & self.max_value()
        return self.__class__(res)

    def rotr(self, n: int) -> BaseUint:
        """Rotates bits out from the right and back into the left"""
        distance = n % self.bit_length()
        rs = self._value >> distance
        ls = self._value << (self.bit_length() - distance)
        res = (rs | ls) & self.max_value()
        return self.__class__(res)

    def __eq__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.eq, other, bool)

    def __ne__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.ne, other, bool)

    def __gt__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.gt, other, bool)

    def __lt__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.lt, other, bool)

    def __ge__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.ge, other, bool)

    def __le__(self, other: int | BaseUint) -> bool:
        return self._operate_binary(opr.le, other, bool)
