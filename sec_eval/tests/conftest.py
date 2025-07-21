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

import os
import pytest
import warnings
import functools
from src import MaskedUint8, MaskedUint32, MaskedUint64

__all__ = ["compute_security_orders"]


MASKED_UINT_CLASSES = [MaskedUint8, MaskedUint32, MaskedUint64]


def pytest_addoption(parser):
    parser.addoption(
        "--run-tvla",
        action="store_true",
        default=False,
        help="Run TVLA side-channel security evaluation tests",
    )
    parser.addoption(
        "--run-mia",
        action="store_true",
        default=False,
        help="Run MIA side-channel security evaluation tests",
    )


def pytest_runtest_setup(item):
    for mark in ["run_tvla", "run_mia"]:
        opt = f"--{mark.replace('_', '-')}"
        if mark in item.keywords and not item.config.getoption(opt):
            pytest.skip(f"need {opt} option to run")


@functools.cache
def compute_security_orders() -> list[int]:
    default_orders = [1, 2, 3]
    upper_bound = 10
    lower_bound = 1

    if order := int(os.environ.get("SEC_ORDER", 0)):
        clamped_order = max(min(order, upper_bound), lower_bound)
        if not (lower_bound <= order <= upper_bound):
            msg = f"SEC_ORDER {order} is out of range, clamped to {clamped_order}"
            warnings.warn(msg, stacklevel=1)
        if clamped_order > 3:
            msg = f"Security evaluation can take a very long time due to SEC_ORDER {order}"
            warnings.warn(msg, stacklevel=1)
        return [clamped_order]
    return default_orders
