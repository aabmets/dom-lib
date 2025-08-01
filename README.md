# dom-lib
A lightweight, portable Domain-Oriented Masking library for C and C++.  

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)<br/>
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=bugs)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=aabmets_dom-lib&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=aabmets_dom-lib)


## Table of Contents

1) [What is Domain-Oriented Masking](#what-is-domain-oriented-masking)
2) [Why use a C-based DOM library?](#why-use-a-c-based-dom-library)
3) [Security notes & caveats](#security-notes--caveats)
4) [Currently implemented API surface](#currently-implemented-api-surface)
5) [Algorithms you can mask today](#algorithms-you-can-mask-today)
6) [Road-map & help wanted](#road-map--help-wanted)

<br/>


## What is Domain-Oriented Masking
Side-channel-resistant software splits every secret value into `d + 1` random shares 
(where **_d_** is the desired security order). DOM keeps those shares in separate domains:

| Domain     | Operation style   | Intuition                                       |
|------------|-------------------|-------------------------------------------------|
| Boolean    | XOR-based masking | Used for bitwise logic (e.g., AND, XOR, shifts) |
| Arithmetic | Additive masking  | Used for addition / modular arithmetic          |

Operations are performed share-wise inside a domain; the shares are converted between the domains when an algorithm
needs to compute an operation belonging to the opposite domain. Security order 1 secures software to the first-order
against Differential Power Analysis, while higher orders secure software against higher-order DPA up to order **_d_**.

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>


## Why use a C-based DOM library?

### Pros

1) **_Portable_**: Build and run on any 32-bit or 64-bit little-endian CPU or MCU that has a C11 compiler.

2) **_Cheaper_**: FPGA / ASIC processors are more expensive for hardware-based HDL / Verilog implementations.

3) **_Faster iteration_**: Re-compile in seconds, single-step with a software debugger and unit-test on the host PC before moving to hardware.

4) **_Readable & maintainable_**: Higher-level control-flow and strong typing make the code easier to audit, extend and review.

5) **_Memory safety helpers_**: The library wipes sensitive buffers and propagates detailed error codes.

### Cons

1) **_No parallelism_**: Cannot exploit RTL-level gate-level parallelism, so peak throughput is lower than hand-tuned HDL or Verilog _(unless you apply true multiprocessing)_.

2) **_Register pressure_**: High security orders quickly exhaust general-purpose registers. When the compiler spills shares to the stack, the extra memory traffic slows down processing and may enlarge the side-channel signal.

3) **_Entropy plumbing_**: Each masked operation consumes randomness at run-time. On an SoC your RNG may be a peripheral with limited bandwidth; a Verilog design can stream masks from a dedicated TRNG FIFO in lock-step, whereas C code must poll or buffer, adding latency or blocking the core.

4) **_Audit surface_**: You must review both the C source and the disassembly for every optimization level you ship into production. With RTL you review one artifact; with C you effectively review two.

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>


## Security notes & caveats

This domain-oriented masking implemented in this library is provably **t-SNI secure** _(Strong Non-Interfering)_ 
in the standard probing model (Ishai–Sahai–Wagner, 2003). The security is estimated by analyzing the logic of the 
core gadgets and then extending their security by composability to composite gadgets. We also replicate the C-code 
logic into Python and run the following statistical analysis tests:

* **TVLA**: Test Vector Leakage Assessment — A pass/fail Welch t-test codified in ISO/IEC 17825:2024.
* **MIA**: Mutual Information Analysis — Mutual information estimation between the measured leakage **_L_** and a variable **_V_**.

The Python unittests run for orders 1–3 and only 5000 rounds for each, because the computation is
very heavy and already takes more than 20 minutes on a decent processor. If you intend to use this
library in production, we would recommend you to implement the same security unittests in low-level C
(which is challenging to do) and run them for at least `100 000 * (10 ^ (order - 1))` rounds.

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>


## Currently implemented API surface

All functions are implemented as C-preprocessor macros and expanded for 8, 16, 32 and 64-bit variants.

* **_Memory helpers_** – `dom_alloc[_many]`, `dom_clone[_many]`, `dom_free[_many]`, `dom_clear[_many]`

* **_Masking core_** – `dom_mask[_many]`, `dom_unmask[_many]`, `dom_refresh[_many]`

* **_Domain converters_** – `dom_conv`, `dom_conv_btoa`, `dom_conv_atob` + type-ratio converters (e.g., 32→16)

* **_Boolean operations_** – AND/OR/XOR/NOT, bit-shifts and rotations, boolean add/sub using Kogge-Stone algorithm.

* **_Arithmetic operations_** – add, sub, multiply

* **_Comparators & selectors_** – <, ≤, >, ≥ plus constant-time conditional select helpers

* **_Error handling_** – structured 32-bit codes and human-readable strings via `get_dom_error_message`

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>


## Algorithms you can mask today

* AES-128 / 192 / 256

* SHA-2 (224 / 256 / 384 / 512) and SHA-3 / Keccak

* ChaCha20 / XChaCha20 and Poly1305

* BLAKE2 / BLAKE3 compression function

* Ed25519 / X25519 field arithmetic

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>


## Road-map & help wanted

This library still requires a lot of work to be usable for Post-Quantum Cryptography algorithms:

| Category             | Gadget                                                     | Needed by                 |
|----------------------|------------------------------------------------------------|---------------------------|
| Modulo arithmetic    | constant-time cond-sub q / cond-add q                      | all lattice / PKE schemes |
| Modulo arithmetic    | Montgomery & Barrett reduction (16-bit & 32-bit)           | all lattice / PKE schemes |
| Modulo arithmetic    | masked modular inversion (extended Euclid)                 | all lattice / PKE schemes |
| Polynomial math      | masked schoolbook & Toom-Cook multiply                     | Kyber, Dilithium, Saber   |
| Polynomial math      | coefficient compression / decompression                    | Kyber, Dilithium, Saber   |
| Polynomial math      | polynomial NTT-friendly bit-rev shuffle                    | Kyber, Dilithium, Saber   |
| Transforms           | masked forward & inverse NTT (256-point, q = 3329/8380417) | Kyber, Dilithium, Falcon  |
| Transforms           | masked FFT over ℤ[ω]/(ω^N + 1), N = 1024                   | Kyber, Dilithium, Falcon  |
| Sampling             | centered binomial sampler (η = 2/3/4)                      | Kyber, Saber, Falcon      |
| Sampling             | discrete Gaussian sampler (σ ≈ 1.2 – 1.6 for Falcon)       | Kyber, Saber, Falcon      |
| Sampling             | uniform rejection sampler mod q                            | Kyber, Saber, Falcon      |
| Code-based helpers   | masked GF(2^m) mult & inv (bit-serial / LUT)               | Classic McEliece, BIKE    |
| Code-based helpers   | masked Berlekamp–Massey / Patterson decoder                | Classic McEliece, BIKE    |
| Code-based helpers   | masked syndrome computation for binary Goppa codes         | Classic McEliece, BIKE    |
| Code-based helpers   | dense & sparse binary matrix–vector multiply               | Classic McEliece, BIKE    |
| Code-based helpers   | constant-time random weight-t error‐vector sampler         | Classic McEliece, BIKE    |
| Hash-based helpers   | masked Merkle-tree node hash (H = H(parent ‖ child))       | SPHINCS+, XMSS, LMS/HSS   |
| Hash-based helpers   | masked tweakable hash/WOTS+ chain function                 | SPHINCS+, XMSS, LMS/HSS   |
| Hash-based helpers   | masked FORS subtree hashing                                | SPHINCS+, XMSS, LMS/HSS   |
| Scheme-specific glue | masked point-wise Montgomery mult + add                    | Kyber, Dilithium          |
| Scheme-specific glue | masked matrix–vector multiply                              | Dilithium                 |
| Scheme-specific glue | masked poly mult in R₃[X]/(X²⁵⁶+1)                         | Saber                     |
| Scheme-specific glue | masked BCH/RS syndrome calculation                         | HQC                       |
| Scheme-specific glue | masked hint generation & usage                             | Dilithium                 |
| Scheme-specific glue | lattice basis conversion helpers                           | Falcon                    |

[⬆️ Back to top](#table-of-contents)
<br/>
<br/>
