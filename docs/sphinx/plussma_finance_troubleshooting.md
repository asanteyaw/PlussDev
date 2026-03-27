# PlussMa Finance Troubleshooting

This page focuses on practical failure modes, diagnosis, and direct fixes for PlussMa Finance workflows.

## 1) Implied Vol Fails (NaN / no root)

### Symptom
- `pricing::implied_vol(...)` returns non-finite value or solver status indicates failure.

### Common causes
- option market price is outside no-arbitrage bounds
- bad initial bracket (`vol_lower`, `vol_upper`)
- maturity close to zero with noisy quote

### Fixes
1. Validate bounds first:
   - Call: `max(0, S*exp(-qT) - K*exp(-rT)) <= C <= S*exp(-qT)` for calls.
2. Expand root bracket in `ImpliedVolOptions`.
3. Increase `max_iterations`, tighten/relax `tolerance` appropriately.
4. For short maturities, clip or exclude quotes below a minimum `T`.

### Example

```cpp
fin::pricing::ImpliedVolOptions iv_opts{};
iv_opts.method = fin::pricing::ImpliedVolMethod::Brent;
iv_opts.vol_lower = 1e-6;
iv_opts.vol_upper = 5.0;
iv_opts.max_iterations = 200;
iv_opts.tolerance = 1e-10;
```

## 2) Heston/Bates Calibration Diverges

### Symptom
- optimizer oscillates or parameters explode
- poor fit on wings or specific maturities

### Common causes
- unconstrained search region
- inconsistent initial guesses
- overfitting noisy strikes with too-small regularization/robust loss control

### Fixes
1. Constrain admissible region (especially `kappa, theta, sigma, rho, v0`).
2. Seed from robust defaults, then local-refine.
3. Fit by maturity buckets first, then global fit.
4. Use deterministic seed for reproducible debug.
5. Compare FFT vs COS pricing during calibration to detect engine artifacts.

### Diagnostics checklist
- residual heatmap by (maturity, strike)
- signed residual mean per maturity
- tail-residual percentile summaries

## 3) FFT Price Artifacts

### Symptom
- negative prices on deep OTM/ITM or jagged strike curves

### Common causes
- poor FFT damping factor
- insufficient grid (`num_fft`) or bad step size
- quadrature mismatch

### Fixes
1. Increase `num_fft` (for example 2048 -> 4096 or 8192).
2. Tune `damping_factor` (commonly around 1.0 to 2.0 for calls).
3. Reduce `characteristic_fcn_step`.
4. Cross-check with COS or MC for a few anchor contracts.

## 4) Monte Carlo Too Noisy / Slow

### Symptom
- unstable estimates across runs
- runtime too high

### Common causes
- too few paths
- unsuitable scheme for the SDE stiffness
- no variance reduction

### Fixes
1. Increase paths and use convergence checkpoints.
2. Choose stable scheme for process type.
3. Enable variance reduction where available.
4. Use two-pass approach:
   - coarse paths for tuning
   - large paths for final report.

### Recommended progression
- development: `5k-20k` paths
- candidate verification: `50k-100k`
- final benchmark: `100k+` with repeated seed blocks

## 5) CIR Paths Become Unstable

### Symptom
- negative/near-negative rates from discretization artifacts

### Common causes
- parameters violate or nearly violate Feller region
- coarse time step

### Fixes
1. Check `2*kappa*theta >= sigma^2` (guideline for positivity behavior).
2. Reduce time step.
3. Use a positivity-preserving discretization where supported.
4. Validate against known moments / bond prices.

## 6) module::Field Objective Looks Wrong

### Symptom
- objective scale unexpected
- mismatch between expected and computed residual/loss

### Common causes
- wrong instrument type or maturity unit mismatch
- price units mismatch (percent vs absolute)
- wrong `controls.loss`

### Fixes
1. Confirm maturity in years (not days) where required.
2. Validate strike/spot units.
3. Print intermediate outputs:
   - `module::price(field)`
   - `module::residual(field, observed)`
   - `module::loss(field, observed)`
4. Start with a tiny known dataset and hand-check values.

## 7) Filtering (KF/EKF/UKF/PF) Drifts

### Symptom
- latent state diverges or lags sharply

### Common causes
- poor process/observation noise initialization
- model mismatch in nonlinear transitions
- too few particles for PF

### Fixes
1. Tune noise covariances from residual diagnostics.
2. For EKF/UKF, validate Jacobian/sigma-point scale behavior.
3. Increase particle count and resampling quality for PF.
4. Run one-step-ahead residual whiteness checks.

## 8) Hedge PnL Unrealistic

### Symptom
- PnL too optimistic compared with market expectations

### Common causes
- ignoring transaction costs / slippage
- rebalance schedule too idealized
- stale volatility input

### Fixes
1. Add explicit cost model assumptions.
2. Sweep rebalance frequency and compare stability.
3. Recompute greeks under consistent market state snapshots.
4. Stress test on gap moves and volatility shocks.

## 9) Non-Deterministic Results Across Runs

### Symptom
- outputs differ between repeated runs

### Common causes
- seed not pinned
- deterministic mode disabled
- platform-specific reduction order differences

### Fixes
1. Set fixed random seed in options.
2. Enable deterministic mode where supported.
3. Use same dtype/platform during parity tests.
4. Compare with tolerances, not exact equality, for floating-point workloads.

## 10) Shape / Type Mismatch With Series Inputs

### Symptom
- runtime errors for dimension mismatch

### Common causes
- mixed scalar and vector lengths
- using row-major assumptions inconsistently
- unexpected dtype promotion

### Fixes
1. Print shape of every input Series before pricing/calibration.
2. Ensure batch dimensions align.
3. Force explicit dtype and platform using `Config`.
4. Validate contiguity where performance-sensitive kernels require it.

## 11) Performance Regressions

### Symptom
- recent code changes slow key workloads

### Common causes
- accidental host materialization
- repeated reallocation in loops
- repeated model/object construction in tight loops

### Fixes
1. Hoist reusable option/model objects outside loops.
2. Avoid repeated conversions for the same tensors.
3. Benchmark by stage: data prep vs pricing vs calibration.
4. Keep a stable benchmark set and compare per-commit.

## 12) Minimal Debug Protocol

When a finance workflow fails, use this exact sequence:

1. Reduce to one contract and one maturity.
2. Use Black-Scholes scalar as baseline sanity check.
3. Switch advanced model engine to COS for cross-check.
4. Pin seed and deterministic options.
5. Validate bounds/units and print shapes.
6. Re-introduce vectorization and full dataset gradually.

## 13) Escalation Template For Bug Reports

Provide these fields to make fixes fast:

- API called
- full option struct values
- dtype/platform
- random seed + deterministic flag
- sample input shapes and 3-5 input rows
- expected vs actual output
- whether BS/COS/MC parity check passed

This is sufficient for reproducible diagnosis in most finance issues.

