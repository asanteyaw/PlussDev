# ML/DL Autodiff and Optimizer Maturity Blueprint

## Scope

This document defines the execution plan for the **ML/DL path** (PlussAI-facing training stack) that sits on top of:

- `pluss::codem::Series` runtime
- `pluss::subsp::math::diff` derivative transforms

This is explicitly different from the `pluss::subsp::math::optim` solver path (JAXopt/fmincon-style numerical optimization).

## Doctrine Constraints

This plan follows the project doctrine:

- no unnecessary API sugar / alias bloat
- C++20 style, correctness first
- Pluss-native naming (avoid copying Torch method names at the user surface)
- modular and scalable structure

## Reference Files Audited

Primary reference files read from the upstream PyTorch source tree:

- `torch/csrc/autograd/engine.h`
- `torch/csrc/autograd/engine.cpp`
- `torch/csrc/autograd/function.h`
- `torch/csrc/autograd/variable.h`
- `torch/autograd/functional.py`
- `torch/autograd/forward_ad.py`
- `torch/autograd/graph.py`
- `torch/optim/optimizer.py`
- `torch/optim/adam.py`
- `torch/optim/sgd.py`
- `torch/csrc/api/include/torch/optim/optimizer.h`
- `torch/csrc/api/include/torch/optim/adam.h`

## Parity Estimate (As of March 14, 2026)

This is a weighted estimate against "Torch-level maturity" for ML/DL autodiff + optimizer workflows.

Weights:

- `25%` Autodiff engine semantics and safety
- `25%` Differentiable op coverage breadth
- `20%` ML/DL optimizer stack maturity
- `20%` runtime/backend performance maturity
- `10%` test rigor and failure diagnostics

Current scoring:

- Engine semantics/safety: `47 / 100`
- Op coverage breadth: `38 / 100`
- Optimizer stack maturity: `47 / 100`
- Runtime/backend maturity for DL workloads: `29 / 100`
- Test rigor/diagnostics: `57 / 100`

Weighted maturity score:

- **`42 / 100` overall parity with Torch-level maturity**

Notes:

- Transform APIs (`grad`, `value_and_grad`, `vjp`, `jvp`, `jacrev`, `jacfwd`, `hvp`) remain materially ahead of toy AD.
- DL optimizer core now includes param-grouped `SGD`, `Adam`, `AdamW`, `RMSprop`, `NAdam`, and `Minimax` with restart parity checks.
- Adam/AdamW now include batched update lanes, capturable bias-correction mode, and strict fail-loud lane policy for unsupported fused requests.
- SGD/RMSprop/NAdam now include batched update lanes and strict fail-loud lane policy for unsupported fused requests.
- Diff transform layer now covers additional unary families (`asin`, `acos`, `atan`, `sinh`, `cosh`, `log1p`, `expm1`, `erf`) and explicit HVP mode policy.
- Added high-use autodiff families for ML objectives: `softmax`, `log_softmax`, and native loss transforms (`squared_error_loss`, `absolute_error_loss`, `huber_error_loss`, `logistic_binary_loss`) with reduction policies and transform tests.
- Fixed reverse-mode derivative propagation for `abs` at tape level, removing a silent gradient-gap in piecewise objectives.
- Remaining gap is mostly true backend fused kernels, broader op families for full DL model breadth, and deeper engine internals (hooks/reentrancy/profiling parity).

## High-Impact Families (Execution Order)

## Family A: Engine Correctness and Graph Semantics

Goal: remove silent gradient corruption paths before scaling feature surface.

Tasks:

- formalize graph lifetime and reentrancy rules for nested `reverse()` calls
- harden mutation/version invariants across views and aliasing
- add explicit anomaly diagnostics path for backward failures
- validate deterministic behavior under repeated backward on retained graphs

Gate:

- no stale-saved-value regressions under mutation stress suites
- deterministic repeated reverse on fixed seeds and inputs

## Family B: Operator Derivative Coverage Expansion

Goal: maximize practical DL model coverage quickly.

Tasks:

- prioritize derivative kernels for: normalization, convolution-adjacent primitives, loss families, broadcasting edge ops
- consolidate derivative rules behind a central registration pattern
- define unsupported-op behavior contract (clear runtime errors, no silent fallback corruption)
- add finite-difference audit harness for every new derivative rule

Gate:

- audited derivative parity for targeted op tranche
- all new ops pass scalar, vector, batched, and shape-broadcast tests

## Family C: ML/DL Optimizer Core (Other Path)

Goal: first serious training optimizer surface for PlussAI.

Tasks:

- [x] introduce base optimizer abstraction (param groups, states, zeroing behavior)
- [x] implement production set:
  - [x] SGD (momentum + nesterov)
  - [x] Adam
  - [x] AdamW
  - [x] RMSprop
  - [x] NAdam
  - [x] Minimax (constrained primal-dual family for DL-facing constrained training use-cases)
- [x] support closure-based step where needed
- [x] define optimizer state serialization contract

Gate:

- [x] converges on canonical toy problems (linear/classification and constrained smoke coverage)
- [x] deterministic resume from serialized state

## Family D: Numerical Stability and Failure Policy

Goal: preserve training continuity under hostile numeric conditions.

Tasks:

- NaN/Inf detection policy and configurable handling per optimizer step
- epsilon and clamp policies for denominator-sensitive updates
- gradient clipping primitives (value and norm modes)
- failure telemetry hooks for step diagnostics

Gate:

- stability suite passes extreme-scale input tests without crashes
- controlled failure behavior under injected NaN/Inf gradients

## Family E: Backend Throughput and Dispatch Maturity

Goal: close practical perf gap without embedding backend policy in autodiff.

Tasks:

- keep autodiff backend-agnostic; optimize runtime dispatch layer
- prioritize BLAS/Accelerate pathways for dense update math
- integrate MPS-specific fast paths for optimizer update kernels
- define backend capability matrix per optimizer mode

Gate:

- measurable speedup on representative batch training workloads
- no backend-specific correctness regressions vs CPU reference

## Test Strategy (cpp_env/cache2)

Targeted suites:

- edge-case tests: shape mismatch, scalar/non-scalar contracts, aliasing mutations
- numerical-instability tests: overflow-prone, underflow-prone, and near-singular update paths
- stress tests: long-iteration training loops with retention/reuse patterns
- crash tests: invalid inputs, bad hyperparameters, and injected NaN/Inf

Execution commands (current):

- build AD tests:
  - `cmake --build build_cpp_env --target plussqit_codem_autodiff_tests -j6`
- run AD tests:
  - `./build_cpp_env/PlussQit/plussqit_codem_autodiff_tests`

Current status:

- `plussqit_codem_autodiff_tests` passes after CodeM-only test dependency cleanup and added stability/validation coverage.
- Added deterministic CPU autodiff parity harness (`test_patch167_cpu_autodiff_parity.cpp`) covering indexing/gather/select/take, reduction objective gradients, and a simple training lane with fixed reference vectors/tolerances.
- Added engine integrity stress gates for mutation/versioning and one-shot graph misuse (`SeriesTape` + `test_patch67_autodiff_transforms.cpp`).
- Added initial device-lift autodiff parity lanes in MPS/CUDA gate suites (`test_patch149_mps_torch_family_parity.cpp`, `test_patch148_cuda_torch_family_parity.cpp`) using CPU as value/sensitivity baseline.
- Expanded `nn_training_parity_test` CPU-first Torch-validation lanes to include RNN, Schedule, Conv, and Transformer trajectory checks (CSV-env gated).
- Added GRU and custom-Series-objective CPU-vs-Torch trajectory lanes plus CPU replay-determinism gates in `nn_training_parity_test`.
- `plussqit_codem_optim_tests` currently fails in this build due unresolved `math::optim` visibility/linking in the CodeM-only test target and should be split/relinked by ownership boundary.

## Phase Plan With Mini TODO Lists

## Phase 1 (Immediate, 1-2 patches)

- [ ] keep `diff` internals free of Torch-named calls at implementation touchpoints
- [ ] expand AD validation and stability tests for transform misuse and long-run loops
- [ ] isolate CodeM autodiff tests from `math::optim` dependencies

Deliverable:

- stable AD baseline with passing dedicated AD suite and cleaner subsystem boundary.

## Phase 2 (Near-Term, 3-5 patches)

- [x] introduce ML optimizer base interface (param groups + state + step contract)
- [x] add SGD and Adam with deterministic state behavior
- [x] add AdamW/RMSprop/NAdam coverage in the same stateful surface
- [x] add gradient clipping and constrained-step recovery controls
- [x] add optimizer convergence and restart tests

Deliverable:

- first production ML optimizer stack ready for PlussAI builders.

## Phase 3 (Mid-Term, 5+ patches)

- [ ] broaden derivative op tranche needed by practical MLP/CNN/RNN stacks
- [ ] add anomaly/debug tooling and richer failure diagnostics
- [ ] backend performance tuning across CPU BLAS + MPS pathways

Deliverable:

- meaningful narrowing of Torch maturity gap with reproducible benchmark gains.

## Definition of "Torch-Level Mature Enough to Claim"

Minimum bar to claim near-Torch maturity for this path:

- robust graph/mutation semantics under stress
- broad derivative op coverage for mainstream DL workloads
- stable and tested SGD/Adam/AdamW stack with serialization and restart parity
- backend acceleration with predictable correctness/performance behavior
- comprehensive failure and diagnostics story (not just happy-path correctness)
