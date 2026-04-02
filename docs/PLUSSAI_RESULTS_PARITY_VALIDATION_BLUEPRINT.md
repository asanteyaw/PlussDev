# PlussAI CPU-First Results Parity Validation Blueprint

## Objective
Build systematic model-validation parity against Torch references with CPU as the correctness benchmark, then promote the same parity gates to MPS/CUDA.

## Doctrine Locks
- Keep Pluss-native API names and semantics.
- Validate results parity, not source-level imitation.
- CPU parity is the baseline contract for MPS/CUDA.
- Fail loud on unsupported routes or graph misuse.

## Patch Plan

### Patch R1: Expand CPU Training Parity Lanes
Status: **Completed**

Scope:
- Extend `nn_training_parity_test.cpp` beyond Fusion/LSTM to include:
  - RNN training trajectory lane
  - Schedule training trajectory lane
- Keep deterministic seeds and synthetic datasets.
- Enforce:
  - monotonic loss improvement checks
  - CPU vs MPS trajectory parity envelopes
  - CPU vs CUDA trajectory parity envelopes

Patched file:
- `PlussAI/test/nn_training_parity_test.cpp`

### Patch R2: Torch Reference Generation Pipeline
Status: **Completed**

Scope:
- Add reproducible Torch reference generators and one-shot runner for CSV baselines:
  - linear optimizer matrix
  - LSTM
  - RNN
  - schedule-like stack
- Wire environment-variable contracts consumed by Catch2 parity tests.

Patched files:
- `PlussAI/test/torch_rnn_reference.py`
- `PlussAI/test/torch_schedule_reference.py`
- `PlussAI/test/generate_torch_training_references.sh`

### Patch R3: Conv/Transformer Training Parity Lanes
Status: **Completed**

Scope:
- Add CPU training traces for Conv and Transformer families.
- Add Torch CSV hooks:
  - `PLUSS_TORCH_CONV_REF_CSV`
  - `PLUSS_TORCH_TRANSFORMER_REF_CSV`
- Add CPU vs backend trajectory parity thresholds.

Current progress:
- CPU Conv and CPU Transformer training traces are added to `nn_training_parity_test.cpp`.
- Torch reference generators for both lanes are added and wired into `generate_torch_training_references.sh`.
- CPU-vs-Torch loss trajectory checks for both env hooks now execute in Catch2.

### Patch R4: GRU + Custom Objective Parity and CPU Replay Gates
Status: **Completed**

Scope:
- Add GRU training trajectory parity lane.
- Add user-defined objective lane built from direct `Series` ops with tracked parameters.
- Add deterministic CPU replay checks across all CPU benchmark lanes.
- Add Torch CSV hooks:
  - `PLUSS_TORCH_GRU_REF_CSV`
  - `PLUSS_TORCH_OBJECTIVE_REF_CSV`

Patched files:
- `PlussAI/test/nn_training_parity_test.cpp`
- `PlussAI/test/torch_gru_reference.py`
- `PlussAI/test/torch_objective_reference.py`
- `PlussAI/test/generate_torch_training_references.sh`

### Patch R5: Sensitivity Analysis + Edge Cases
Status: **Planned**

Scope:
- Add finite-difference/perturbation sensitivity checks per model family.
- Add edge-case suites:
  - tiny batch, long sequence, constant target, high/low scale inputs
  - explicit NaN/Inf injection policy checks

### Patch R6: Stress + Backtesting Matrix
Status: **Planned**

Scope:
- Long-horizon training stress tests with drift windows and replay determinism.
- Backtesting-style rolling windows for sequence models.
- Artifact outputs for trajectory diagnostics and regression triage.

### Patch R7: CPU Benchmark Promotion to MPS/CUDA
Status: **In Progress**

Scope:
- Promote all CPU-validated model lanes to MPS/CUDA gating.
- Require parity on:
  - loss trajectories
  - parameter summaries
  - fail-loud policy behavior

## Execution Commands

1. Generate Torch references (cpp_env torch):
```bash
PlussAI/test/generate_torch_training_references.sh /tmp/pluss_torch_refs
```

2. Run parity test with references:
```bash
PLUSS_TORCH_LINEAR_REF_CSV=/tmp/pluss_torch_refs/linear_adam.csv \
PLUSS_TORCH_LINEAR_ADAMW_REF_CSV=/tmp/pluss_torch_refs/linear_adamw.csv \
PLUSS_TORCH_LINEAR_SGD_REF_CSV=/tmp/pluss_torch_refs/linear_sgd.csv \
PLUSS_TORCH_LINEAR_RMSPROP_REF_CSV=/tmp/pluss_torch_refs/linear_rmsprop.csv \
PLUSS_TORCH_LINEAR_ADAGRAD_REF_CSV=/tmp/pluss_torch_refs/linear_adagrad.csv \
PLUSS_TORCH_LINEAR_NADAM_REF_CSV=/tmp/pluss_torch_refs/linear_nadam.csv \
PLUSS_TORCH_LSTM_REF_CSV=/tmp/pluss_torch_refs/lstm.csv \
PLUSS_TORCH_RNN_REF_CSV=/tmp/pluss_torch_refs/rnn.csv \
PLUSS_TORCH_GRU_REF_CSV=/tmp/pluss_torch_refs/gru.csv \
PLUSS_TORCH_SCHEDULE_REF_CSV=/tmp/pluss_torch_refs/schedule.csv \
PLUSS_TORCH_CONV_REF_CSV=/tmp/pluss_torch_refs/conv.csv \
PLUSS_TORCH_TRANSFORMER_REF_CSV=/tmp/pluss_torch_refs/transformer.csv \
PLUSS_TORCH_OBJECTIVE_REF_CSV=/tmp/pluss_torch_refs/objective.csv \
./build_cpp_env/PlussAI/test/nn_training_parity_test
```
