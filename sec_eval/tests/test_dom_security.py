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

import itertools
import os
import secrets
import typing as t
from functools import partial

import numpy as np
import pytest
from scipy import stats
from sklearn.feature_selection import mutual_info_regression

from src import BaseMaskedUint, Domain
from src import operators as ops
from src import converters as conv
from tests import gcmi
from tests import conftest as cfg
from dataclasses import dataclass


@dataclass
class Component:
    operator: t.Callable
    domain: Domain
    type: t.Literal["unary", "binary", "distance"]


ROUNDS = int(os.environ.get("SEC_SAMPLES", 5000))
COMPONENTS = [
    Component(ops.dom_ksa_carry, Domain.BOOLEAN, "binary"),
    Component(ops.dom_ksa_borrow, Domain.BOOLEAN, "binary"),
    Component(ops.dom_bool_and, Domain.BOOLEAN, "binary"),
    Component(ops.dom_bool_or, Domain.BOOLEAN, "binary"),
    Component(ops.dom_bool_xor, Domain.BOOLEAN, "binary"),
    Component(ops.dom_bool_not, Domain.BOOLEAN, "unary"),
    Component(ops.dom_bool_shr, Domain.BOOLEAN, "distance"),
    Component(ops.dom_bool_shl, Domain.BOOLEAN, "distance"),
    Component(ops.dom_bool_rotr, Domain.BOOLEAN, "distance"),
    Component(ops.dom_bool_rotl, Domain.BOOLEAN, "distance"),
    Component(ops.dom_bool_add, Domain.BOOLEAN, "binary"),
    Component(ops.dom_bool_sub, Domain.BOOLEAN, "binary"),
    Component(ops.dom_arith_add, Domain.ARITHMETIC, "binary"),
    Component(ops.dom_arith_sub, Domain.ARITHMETIC, "binary"),
    Component(ops.dom_arith_mult, Domain.ARITHMETIC, "binary"),
    Component(conv.dom_conv_btoa, Domain.BOOLEAN, "unary"),
    Component(conv.dom_conv_atob, Domain.ARITHMETIC, "unary"),
]


def collect_traces(
        cls: t.Type[BaseMaskedUint],
        order: int,
        component: Component,
        *,
        use_fixed_secret: bool = False,
        fill_secrets_vec: bool = False,
) -> tuple[np.ndarray, np.ndarray]:
    bit_length = cls.bit_length()
    rand_val = partial(secrets.randbits, bit_length)
    create_masked_uint = partial(cls, order=order, domain=component.domain)

    fixed_secret = None
    if use_fixed_secret:
        fixed_secret = rand_val()

    traces, secrets_vec = [], []
    for _ in range(ROUNDS):
        secret = fixed_secret
        if not use_fixed_secret:
            secret = rand_val()
        a = create_masked_uint(secret)

        if component.type == "distance":
            dist = bit_length // 2
            result = component.operator(a, dist)
        elif component.type == "binary":
            b = create_masked_uint(rand_val())
            result = component.operator(a, b)
        else:  # unary
            result = component.operator(a)
        traces.append(result.hamming_weights)

        if fill_secrets_vec:
            secrets_vec.append(secret)

    traces = np.asarray(traces, dtype=np.int32)
    secrets_vec = np.asarray(secrets_vec)
    return traces, secrets_vec


def expand_traces(mat: np.ndarray, order: int) -> np.ndarray:
    out = [mat]
    for d in range(2, order + 1):
        for idx in itertools.combinations(range(mat.shape[1]), d):
            out.append(np.prod(mat[:, idx], axis=1, keepdims=True))
    return np.hstack(out).astype(np.float64)


@pytest.mark.run_tvla
@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("component", COMPONENTS)
def test_fixed_vs_random_tvla(cls, order, component):
    fixed_set = collect_traces(cls, order, component, use_fixed_secret=True)[0]
    random_set = collect_traces(cls, order, component)[0]

    all_traces = np.vstack((fixed_set, random_set))
    global_mean = all_traces.mean(axis=0, keepdims=True)
    centered_f = fixed_set - global_mean
    centered_r = random_set - global_mean

    expand_fn = partial(expand_traces, order=order)
    f_exp, r_exp = map(expand_fn, (centered_f, centered_r))

    t_stat, _ = stats.ttest_ind(f_exp, r_exp, equal_var=False)
    worst = np.nanmax(np.abs(t_stat))
    threshold = 4.5
    assert worst < threshold, f"TVLA fail: |t|={worst:.2f} ≥ {threshold}"


@pytest.mark.run_mia
@pytest.mark.parametrize("cls", cfg.MASKED_UINT_CLASSES)
@pytest.mark.parametrize("order", cfg.compute_security_orders())
@pytest.mark.parametrize("component", COMPONENTS)
def test_mutual_info_analysis(cls, order, component):
    traces, secrets_vec = collect_traces(cls, order, component, fill_secrets_vec=True)
    feats = expand_traces(traces, order)

    # Univariate analysis
    worst = max(
        [
            mutual_info_regression(
                feats[:, [col]], secrets_vec, n_neighbors=5, random_state=0, discrete_features=True
            )[0]
            for col in range(feats.shape[1])
        ]
    )
    threshold_nats = 0.05
    assert worst < threshold_nats, f"Univariate MIA fail: MI={worst:.3f} nats ≥ {threshold_nats}"

    # Multivariate analysis
    sv = secrets_vec.reshape(1, -1)
    i_bits = gcmi.gcmi_cc(feats.T, sv)
    worst = float(i_bits * np.log(2.0))  # convert from bits to nats

    threshold_nats = 0.10
    assert worst < threshold_nats, f"Multivariate MIA fail: MI={worst:.3f} nats ≥ {threshold_nats}"
