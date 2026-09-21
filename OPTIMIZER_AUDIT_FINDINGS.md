# Optimizer Audit Findings

**Source of this report:** a from-scratch implementation of Hansen & Tong (2024), *"Option
Pricing with Time-Varying Volatility Risk Aversion"* (the DHNG model), built twice — once in
pure libtorch, once in `pluss::Series` — as a side-by-side correctness/parity exercise for
`pluss::Series` against `torch::Tensor` on real market data (S&P 500 returns, CBOE VIX,
1990–2019, ~7,559 trading days). That project is a downstream consumer of this SDK
(`PLUSS_SDK_PREFIX` install), not part of this repo; it is referenced here only as the
reproduction vehicle. Every finding below is intentionally written up as a **generic** SDK
issue — reproducible with a plain unconstrained MLE objective, nothing paper-specific — so a
fix here benefits every downstream consumer, not just that one project.

Scope: this pass covers optimizers only — `pluss::subsp::math::optim::septima`,
`pluss::subsp::math::optim::optima`, and the new `pluss::subsp::math::optim::l_sr1_trust_region`
(PlussMa), and `pluss::ai::nn::optim::*` (PlussAI/NodeK), touching both
`pluss::subsp::math::diff`'s `Trace`-based autodiff and `Series`'s own embedded reverse-mode
tape. It does not cover PlussQit, PlussMic, or PlussPl, with one narrow exception: a new
`eigh()` (symmetric eigendecomposition) primitive was added to `PlussQit/include/pluss/subsp/math/linalg.h`
purely as a building block the new L-SR1 optimizer needed and which did not exist anywhere in
this SDK before (see Finding 7).

**Reproduction case used throughout:** an unconstrained 5-parameter Gaussian quasi-MLE
(a Heston-Nandi GARCH return model), reparametrized via softplus/sigmoid for positivity —
i.e. an entirely ordinary "fit a nonlinear model by maximum likelihood" problem, the single
most common workload an optimizer will see. Two starting points are used repeatedly:

- **naive init**: `x0 = {inv_softplus(1e-6), 2.0, inv_softplus(5e-6), 50.0, 0.1}` (order:
  `u_omega, u_beta, u_alpha, u_gamma, u_lambda`) — arbitrary, not hand-tuned.
- **warm init**: the same 5 parameters set to libtorch's own converged LBFGS solution
  (`omega≈0, beta=0.807, alpha=4.2e-6, gamma=190.08, lambda=3.274`, logLik≈24994.89).

libtorch's `torch::optim::LBFGS` (`LBFGSOptions(1.0).max_iter(300).line_search_fn("strong_wolfe")`)
solves this correctly from the naive init on the first try, in about 2 seconds. That result is
the reference point for every comparison below.

---

## Finding 1 (High severity, PlussMa) — FIXED: `septima`'s LBFGS both falsely reported
convergence AND (once that was fixed) got permanently stuck, from two separate bugs plus one
performance fix

**Status: fixed in this repo.** This finding started as a single false-convergence report; the
fix for that unmasked a second, deeper bug that had been silently making every line-search
trial's gradient unusable all along. Both are fixed below, along with a wasted-backward-pass
performance issue found along the way. All three are verified: a 7/7 numeric-equivalence check
for the value-only path, direct instrumentation of the line search confirming
`grads_defined: 0 → 1` before/after the fix, and an end-to-end run that now finds and accepts
genuine steps with real, verified (non-zero, correctly-signed) progress instead of either
silently-fabricated or permanently-absent movement.

**Location:** `pluss/subsp/math/optim.h`, the `septima()` family, `septima_step_inplace_`, and
`septima_unflatten_values_`.

### Bug 1a: false convergence via a silently-zeroed disconnected gradient

**Symptom:** Running `septima()` in `Lbfgs` mode with `StrongWolfe` line search
(`learning_rate=1.0`, matching libtorch's own LBFGS default) on the reproduction case, from the
naive init:

```
exit_flag = Converged, message = "converged"
iterations = 2, function_evaluations = 15
gradient_inf_norm = 0.0, step_inf_norm = 0.0
logLik = 24775.99   (true optimum: 24994.89 — off by ~220 nats)
gamma  = 50.00275   (barely moved from its init of 50.0)
lambda = 0.10388    (barely moved from its init of 0.1)
```

From the warm init, the *same* call reproduces libtorch's answer almost exactly
(`logLik=24994.84`, all 5 parameters within 1e-3 of libtorch's), confirming the LBFGS math and
line search are fundamentally sound — this is specifically a cold-start failure.

**Root cause (confirmed by reading `diff.h` directly, not inferred):**

1. At the naive init, the raw gradient of the 5 packed parameters is
   `(-71.6, -802.3, -767.3, -5.63, -7.94)` — `u_beta`/`u_alpha`'s components are ~100–140×
   larger than `u_gamma`/`u_lambda`'s. A full Newton-ish LBFGS step with `learning_rate=1.0`
   pushes `u_beta` by roughly +800, which drives `sigmoid(u_beta)` to exactly `1.0` at
   double-precision — almost certainly producing a NaN/Inf somewhere downstream (e.g. a
   `log(1-beta)`-shaped term) at that trial point.
2. `diff::value_and_grad` (`diff.h`, the loop building `grads` from `vars`) checks
   `v.has_sensitivity()` per parameter **block**. When the reverse pass can't establish a link
   (which a NaN in the backward graph will do), the default policy — silently, because
   `strict_dependency` defaults to `false` — **materializes a zero-valued gradient** for that
   block and continues:
   ```cpp
   if (!v.has_sensitivity()) {
     if (policy.strict_dependency) { throw_error(...); }
     if (policy.warn_on_disconnected && !disconnected_warned) {
       PLUSS_WARN("... materializing disconnected sensitivities according to policy "
                  "(set strict_dependency=true to fail loud).");
     }
     grads.push_back(detail::zeros_like_(v.value()));
     continue;
   }
   ```
3. Because all 5 raw parameters are packed into **one** flat `Series` block (the common,
   idiomatic pattern used everywhere else in this SDK's own examples), a disconnection
   anywhere zeroes the **entire** flat gradient at once, not just the offending component.
4. LBFGS's own convergence check reads `gradient_inf_norm` — which is now exactly `0.0`,
   a fabricated stand-in for "this evaluation was invalid," not evidence of a stationary
   point — and reports `Converged`.

This is a genuine **line-search robustness gap**, not a numerical-conditioning issue in the
objective: a NaN-producing trial step should be *rejected* (shrink and retry, matching what
`MinimaxOptions::nan_recovery_attempts`/`nan_recovery_shrink` already does one layer up in
PlussAI — see Finding 3) and re-tried with a smaller step, not silently laundered into a
"gradient is zero" signal that the outer optimizer has no way to distinguish from genuine
convergence.

**Fix applied:** both call sites that consume `value_and_grad`'s output for a line-search
decision — the strong-Wolfe trial-point evaluation inside `septima_step_inplace_`, and the
top-level per-iteration gradient computation inside `septima()` itself (which feeds both the
convergence check *and* the line search's search direction, and had the identical bug
independently) — now pass an explicit `DerivativePolicy` with
`disconnected_sensitivity = DisconnectedSensitivity::Undefined` instead of relying on the
library default (`Zero`). Both sites then explicitly check `all_of(..., [](const Series& g) {
return g.defined(); })` before trusting `septima_finite_`, and treat "any block undefined"
exactly like "non-finite": reject and shrink in the line search, and report
`SeptimaExitFlag::NonFiniteEvaluation` (reusing the existing exit flag and
`stop_on_nonfinite_objective` option, which already existed for the objective-non-finite case)
at the top level, with `out.gradient_inf_norm` set to `+infinity` rather than a fabricated
`0.0` when the caller opts out of stopping.

### Bug 1b (the actual root cause once 1a's masking was removed): `septima_unflatten_values_`
hands the line search's own trial points to the objective as gradient-tracking leaves without
severing their existing graph connection, so **every** trial's gradient comes back disconnected

Fixing Bug 1a exposed a much more consequential problem: with the fabricated `Zero` gradient no
longer masking it, `septima`'s LBFGS ran its full iteration budget but made **zero movement at
all** — `gradient_inf_norm` correctly reported the real, non-zero gradient (`58.48`) every
single iteration, yet `step_inf_norm` was `0.0` every time, eventually exiting
`NoProgress`. Instrumenting the line search directly (temporarily) showed every one of the 25
shrinking trial step sizes — from the full step down to `step_size ≈ 6e-8`, a genuinely
microscopic perturbation — reporting `grads_defined=0`, even though the objective *value* at
those same points was perfectly finite from the fourth attempt onward.

**Root cause:** `septima_unflatten_values_` builds each trial parameter block via
`flat.slice(...)`, `.contiguous()`, and `.reshape(...)` on `flat_values` — which, inside the
line search, is `flat_params.add(direction.mul(step_size))`, i.e. a freshly computed
*non-leaf* tensor. The resulting pieces are then handed straight to
`septima_augmented_value_grad_` → `value_and_grad`, which wraps each one as an independent
`Trace` and calls `ensure_tracks_slope_` on it — treating it as a **fresh leaf** for this
evaluation. But the piece still carries its slice/reshape lineage back to that non-leaf
computation. The autodiff engine ends up reconciling "this is a fresh tracked leaf" against
"this value's own graph says it's derived from something else," and the reverse pass never
re-establishes a sensitivity link back to it — at *any* step size, since shrinking the step
doesn't change the fact that the piece is still a view chained off a computed tensor.

This means the disconnection was never really about a wild step overflowing a sigmoid — that
theory was disproven directly (a standalone `value_and_grad` call at an equally tiny
perturbation returns a perfectly well-defined gradient). It was a structural gap in how
`septima_unflatten_values_`'s output gets reused as a leaf, present for *every* trial
regardless of step size, silently invisible before Bug 1a's fix because a fabricated zero
gradient trivially satisfies the curvature condition (`|0| <= anything non-negative`) and so
occasionally let a step through anyway.

**Fix applied:** `septima_unflatten_values_` now calls
`pluss::codem::series_tape::clear_source_link(piece)` on each reconstructed block before
returning it — the exact pattern `pluss::ai::nn::optim::Optimizer::assign_parameter_` already
uses for the same reason ("Optimizer outputs become fresh leaf parameters for the next
iteration"). Verified directly: after this fix, the same instrumented run shows
`grads_defined=1` throughout, and at the fourth finite-value attempt (`step_size ≈ 7.8e-3`)
both the Armijo and curvature conditions are satisfied, producing a genuine accepted step with
`step_inf_norm ≈ 0.457` — real, verified progress, not a fabricated report of it.

Because `septima_unflatten_values_` is the general-purpose reconstruction helper (used for
*any* multi-block parameter vector, not just this reproduction case's single packed block),
this bug affected every `septima` LBFGS call using the strong-Wolfe line search, independent of
problem — the fix is fully generic.

### Performance fix found in the same code path: a wasted backward pass on every outer iteration

While fixing the above, the LBFGS line search's base-point evaluation was found to call
`septima_augmented_value_grad_` (a full forward *and* backward pass) purely to read the scalar
objective value, discarding the gradient half immediately (`(void)base_grad;`). A new
`septima_value_only_`/`septima_augmented_value_only_` pair builds the same `Trace` inputs and
calls the objective, but skips `.reverse()` entirely, since only the value is needed here.
Verified via 7 numeric-equivalence checks (4 unconstrained points, 3 constraint-augmented
points) that the value-only path returns bit-for-bit the same scalar as
`value_and_grad`'s value half. This removes one full unnecessary backward pass per outer LBFGS
iteration, for every user of the strong-Wolfe line search.

### A related, smaller memory-hygiene fix: `DerivativePolicy::keep_graph`

Both of the `DerivativePolicy` constructions added for Bug 1a's fix also set `keep_graph =
false`: each of these evaluations is used immediately and never revisited, so retaining the
backward graph afterward (the library default) serves no purpose and, once Bug 1a's fix lets
`septima` actually run its full iteration budget instead of stopping after ~2 iterations,
accumulates real, avoidable memory pressure across hundreds of iterations. This is necessary
but — see Finding 1c below — **not sufficient** to keep memory bounded at realistic problem
scale.

**Suggested further work (not done here):**

1. Expose a `DerivativePolicy` (or at minimum a `strict_dependency` toggle) through
   `SeptimaOptions`/`SeptimaSolveOptions` directly, so callers can opt into fail-loud behavior
   without bypassing `septima()` (there is still no way to do this through the public surface;
   the fixes above hardcode the `Undefined`/`keep_graph=false` choice internally, which is the
   right default but not user-configurable).
2. Consider whether `pluss::subsp::math::diff::gradient_audit` (present in `diff.h`, not
   investigated in depth here) is intended to catch problems like Bug 1b, and if so, whether it
   should run automatically (at least in a debug build) before trusting a `Converged` exit
   flag.
3. Audit other call sites in this SDK that reconstruct a parameter Series via slicing/reshaping
   a computed tensor and then hand it to `value_and_grad`/`Trace` as an independent leaf —
   `septima_unflatten_values_` is unlikely to be the only place this pattern appears, and every
   occurrence is a candidate for the same silent-disconnection bug.

---

## Finding 1c (High severity, unresolved, PlussMa/PlussQit): `septima`'s memory usage grows
roughly linearly across iterations at realistic problem scale, independent of the Finding 1
fixes above, and does not plateau

**Status: confirmed, root cause NOT found.** This surfaced only when testing Finding 1's fixes
at the reproduction case's full scale (the 7,559-observation sequential recursion) rather than
the smaller subsamples used to verify correctness quickly. A run that correctly makes real
optimization progress (per Finding 1's fixes) was killed by the OS (`SIGKILL`, exit 137) after
climbing past ~3 GB RSS with no sign of leveling off.

**What was ruled out:**

- **Not caused by Bug 1b's disconnection issue or the retries it caused.** Repeating the same
  full-scale run *after* Bug 1b's fix (so trials are no longer being uselessly retried 25 times
  every iteration) reproduces an essentially identical growth trajectory and rate (measured via
  `ps -o rss=` at ~20 second intervals: `675 → 1236 → 1524 → 1799 MB`, versus `960 → 1235 →
  1523 → 1798 → 2073 MB` before that fix) — the growth is not attributable to wasted retry
  volume.
- **Not (fully) `DerivativePolicy::keep_graph` defaulting to `true`.** Setting
  `keep_graph = false` on every `value_and_grad` call `septima` makes internally (see above)
  did not stop the growth; it may have delayed it slightly, but the linear trend and rate look
  the same with or without it.
- **Not LBFGS history retention.** `history_size` defaults to 20 and bounds the *count* of
  retained `(s, y)` pairs; each pair is parameter-sized, not graph-sized, and the growth
  continues long past `history_size` iterations without leveling off at any point consistent
  with a bounded 20-pair cache.

**What this suggests, not yet confirmed:** the growth pattern (steady, close to linear, no
plateau) is more consistent with something in `pluss::codem::series_tape`'s underlying node
storage not being reclaimed once a `Series`/`Trace` value goes out of scope and its local
references drop to zero — i.e. a genuinely global or session-scoped tape structure that
requires an explicit clear/prune between independent evaluations, rather than being cleaned up
by ordinary RAII/reference-counting as each trial's temporary `Series` objects go out of scope.
This is consistent with, but not proven by, the available evidence — confirming it needs
someone with visibility into `SeriesTape.h`'s actual node lifetime/GC model, which was beyond
what could be reliably diagnosed by testing from outside the tape's own implementation in the
time available for this pass.

**Suggested next steps for whoever picks this up:**

1. Instrument `pluss::codem::series_tape` itself (node count, live-node count, or an
   allocation counter) across a long-running `septima` optimization loop like this reproduction
   case, to see directly whether node count grows unboundedly or whether the growth is
   elsewhere (e.g. in `Series`'s own storage/allocator, unrelated to the tape).
2. Check whether there is (or should be) an explicit "clear the tape between independent
   evaluations" call that a long-running optimizer loop like `septima()`'s is expected to make
   but currently doesn't — if such a facility exists, this may be a one-line fix in `septima()`
   itself (call it once per outer iteration, after the line search's trial evaluations are
   done with) rather than a deeper engine change.
3. If no such facility exists, this likely needs to be designed: any observation-driven /
   iterative-optimization consumer of `value_and_grad` over a long sequential computation is at
   risk of the same unbounded growth, so this is not specific to `septima`.

**Reproduction:** the exact `x0`, objective (a straightforward Gaussian QMLE — the same shape
as any nonlinear-regression likelihood), and `SeptimaSolveOptions` used above are enough to
reproduce this deterministically; no paper-specific code is needed.

---

## Finding 2 (Medium severity, PlussMa): `optima()`'s constrained trust-region presets converge
to a materially worse point than LBFGS on the same unconstrained problem

**Location:** `pluss/subsp/math/optim.h`, `optim::optima()` with the `internal_protocol()` /
`robust_internal_protocol()` / `sequential_qp()` presets (`problem_scaling=InitialPoint`,
trust-region-based).

**Symptom:** applied to the identical reproduction case (via `ProblemBuilder`/
`optim::problem().objective(fn).build()`, reparametrizing the same 5 raw parameters,
i.e. an unconstrained problem run through a constrained solver with an empty constraint set —
a supported and common use pattern), from the naive init:

- Genuinely converges by its own criteria (`message="barrier optimality and feasibility
  tolerances satisfied"`, not a spurious zero-gradient exit like Finding 1) to
  `gamma≈50.003, lambda≈0.104, logLik≈24776.3` — i.e. the same basin as Finding 1's failure,
  reached by an entirely different mechanism.
- Manually rescaling the initial point with the scaling heuristic used in this SDK's own
  `heston_test.cpp` example (`scaler = 10^ceil(log10(|x0|))`) made this **worse**, not better —
  the solver then reported convergence after only 3 iterations with `stationarity=32561` (a
  large residual, i.e. nowhere near a real stationary point), with every parameter frozen
  exactly at its initial value. This suggests the preset's own automatic
  `problem_scaling=InitialPoint` is actively fighting a manually-supplied scaler rather than
  composing safely with it.

**Assessment:** this is a genuinely constrained-QP/trust-region solver being asked to solve an
unconstrained problem via reparametrization — a supported pattern, but one where a
well-conditioned quasi-Newton method (LBFGS) has a natural advantage. The finding here is
narrower than Finding 1: it's not misreporting convergence (its own diagnostics are internally
consistent), but its automatic scaling does not handle multi-scale unconstrained problems
(here, softplus-transformed parameters ~O(1e-6) alongside identity-transformed ones ~O(50-200))
as gracefully as LBFGS does on the identical starting point.

**Suggested generic fixes:**

1. Document explicitly that `internal_protocol()`/`sequential_qp()` are tuned for genuinely
   constrained problems, and that `septima()`'s LBFGS mode (once Finding 1 is fixed) is the
   recommended default for unconstrained MLE-shaped problems.
2. Investigate whether `problem_scaling=InitialPoint` can use a **per-parameter** log-scale
   normalization instead of a single global scale, for exactly this multi-order-of-magnitude
   scenario.
3. Either detect and warn when a caller supplies an external scaler on top of
   `InitialPoint` auto-scaling (since the combination measurably made things worse here), or
   make the two compose safely instead of double-transforming.

---

## Finding 3 (High severity — FIXED, PlussAI): `pluss::ai::nn::optim::*` (Adam, AdamW, SGD,
RMSprop, Adagrad, NAdam, **Minimax**) hard-rejected every floating dtype except
Float32/Float16/BFloat16 — including Float64 — while `Series`'s own autodiff no longer has this
restriction

**Status: fixed in this repo (since this report was first written).** `is_training_float_dtype_`
now includes `Float64`, and two stale test contracts that had asserted the old Float32-only
behavior were updated to match. Verified via cross-precision tests across all affected
optimizers on CPU. MPS correctly, loudly rejects Float64 via a separate, pre-existing,
unrelated CodeM guard (confirmed intentional, not a bug). While this was being verified, a
**second, independent gap in the same area** was found and fixed too: `Series`'s core autodiff
also gained complex-number support since this report's original scope, and
`pluss::ai::nn::optim`'s NaN-recovery path called `isfinite()` directly with no complex-dtype
handling (CodeM's `isfinite()` predicate has no complex implementation at all) — fixed via a
`view_as_real_`/`view_as_complex_` pair (mirroring torch's own `_view_as_real` convention:
treat a complex parameter's real/imaginary channels as independent through the entire per-step
Adam-family formula, not just the second-moment squaring, since complex division mixes
components) wired into all five moment-tracking optimizers, plus the identical fix applied to
Minimax's own NaN-recovery path and its `has_non_finite_` diagnostic. Minimax's constraint
guards were also refined from a blanket "no complex" rule to the precise per-mechanism one
actually required: box constraints (an ordering on the parameter) correctly reject complex,
equality constraints (pure add/mul, no ordering) never should have, and inequality constraints
only need the *residual* (not the parameter) to be real. The full verification record for this
work (cross-precision + live-torch-numeric comparisons across all six optimizer families,
norm-clipping, and value-clipping) exists in the author's own working notes from that session
but is **not re-derived here**, since it predates and is independent of the `septima`/L-SR1 work
below.

**Original finding, preserved for context:**

**Location:** `pluss/nn_detail.h`:

```cpp
inline bool is_training_float_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32 ||
         dtype == ScalarType::Float16 ||
         dtype == ScalarType::BFloat16;
}
```

...checked at parameter-registration time in `NodeK/NN/optim/optim_base.h`
(`Optimizer::validate_parameters_()`) and again at every `step()` call
(`Optimizer::step(closure)`, both the `Series`-returning and
`pair<Series, vector<Series>>`-returning overloads, via `require_training_float_dtype_`).

**Symptom:** constructing e.g. `pluss::ai::nn::optim::Minimax` with a Float64 parameter throws
immediately:
```
pluss::ai::nn::optim::Optimizer: training expects Float32/Float16/BFloat16 parameters; got
unsupported dtype for param[0]
```
and even a correctly-Float32 parameter set still throws at `step()` time if *any* scalar
constant inside the objective was built without an explicit `Config` and silently promoted the
loss back to Float64 (see Finding 4 — that's how this was first hit in practice).

**Context (from the author, recorded here for the fix record):** this Float32-only
restriction was modeled on a mistaken belief that libtorch's optimizers/autograd are
Float32-only — actually a historical, cuDNN-specific constraint on certain RNN paths, not a
general libtorch limitation. `Series`'s own core autodiff has since been extended to support
all floating dtypes to match libtorch's actual behavior; `pluss::ai::nn::optim` was not yet
updated to match, so this allowlist is now stale relative to what the engine it sits on top of
actually supports.

**Impact (as originally observed, now resolved):** the entire `pluss::ai::nn::optim` family was
unusable for any Float64 training/optimization workload — every non-toy scientific/econometric
optimization problem in this project's reproduction case, and any other double-precision
workload — even though `Series::reverse()` itself has no such restriction.

**Suggested generic fix (status: all three items below are DONE):**

1. Relax `is_training_float_dtype_` to whatever set of floating dtypes `Series`'s own
   `.reverse()` autodiff actually supports today (per the author, that's now "all float
   types"), rather than a hardcoded three-member allowlist that has to be manually kept in
   sync by hand whenever `Series`'s own capability changes.
2. Add end-to-end Float64 test coverage for at least one representative optimizer (e.g. `Adam`)
   — parameter dtype, gradient dtype, and loss dtype all Float64 — to lock the fix in and catch
   any future regression of this allowlist.
3. Audit the other three `is_training_float_dtype_`/`require_training_float_dtype_` call sites
   in `optim_base.h` for the same relaxation (`validate_parameters_`, and both `step(closure)`
   overloads) — they all currently share the same stale allowlist.

**Positive corroborating note, worth preserving alongside the fix:** once worked around by
testing at Float32, `Minimax`'s actual optimization mechanics performed *well* on the identical
adversarial cold-start case from Finding 1 — smooth, monotonic progress
(`logLik: 24587.8 → 24895.0` over 300 iterations, `gamma: 50.05 → 63.34`, `lambda: 0.15 → 5.89`),
with no false-convergence or NaN-trap behavior. At the time this was written, the working theory
was that `Minimax`'s explicit `nan_recover_step_` (shrink-and-retry on a non-finite post-step
parameter) was doing something `septima`'s line search lacked, and that porting it would be the
fix for Finding 1. **That theory turned out to be only half right, and the actual fix ended up
elsewhere:** `septima` already had an equivalent shrink-and-retry mechanism
(`nan_recovery_attempts`/`nan_recovery_shrink`) that handles genuine NaN/Inf correctly — the
real bugs were (a) not distinguishing a *disconnected* gradient from a legitimately-zero one
(Bug 1a), and (b) `septima_unflatten_values_` handing the line search's own trial points to the
objective without severing their graph lineage, so *every* trial's gradient came back
disconnected regardless of step size (Bug 1b) — see Finding 1 above for both, now fixed. The
comparison to `Minimax` was a useful prompt for digging deeper, even though the eventual fix
wasn't the one it suggested.

**Performance note — retracted after a controlled re-measurement:** an earlier draft of this
report claimed `Minimax` ran ~3× slower per iteration than an equivalent hand-written Adam
update on `pluss::subsp::math::diff::value_and_grad` (~2.9s/iter vs. ~1s/iter). That
comparison was contaminated: the ~2.9s/iter figure came from a 300-iteration run sharing the
machine with other background work in a long session, not a clean measurement. Three
follow-up checks all say there is no real gap:

1. A fresh, single-process, 60-iteration `Minimax` run showed a flat ~0.85–0.90s/iteration
   with no growth or drift across the run (ruling out a leak or unbounded-tape-growth
   explanation for the earlier number).
2. An isolated phase breakdown (forward-only, forward+`Series::reverse()`, forward+
   `value_and_grad`, full `Minimax.step()`) found `Minimax`'s own bookkeeping adds negligible
   cost on top of `Series::reverse()` (0.90s vs. 0.86s/iter) — i.e. whatever cost exists is in
   the shared autodiff machinery underneath both approaches, not in `Minimax`'s update logic.
3. A final interleaved comparison in one process with nothing else running —
   trace-based (0.90s/iter) → `Minimax` (0.96s/iter) → trace-based again (1.09s/iter) — put
   `Minimax` *inside* the noise band of the same trace-based method measured twice in a row.

**Conclusion: no performance defect in `Minimax` is substantiated by this audit.** The
original claim is left here, struck through in spirit, specifically so a maintainer re-reading
old discussion of this report doesn't chase a regression that isn't real. If a genuine
`Series::reverse()`-vs-`Trace` performance question is worth investigating on its own merits,
it should be profiled with dedicated, repeated, contention-free benchmarks (e.g. `google
benchmark`-style repeated trials with warm-up) rather than a single wall-clock reading in a
loop that shares the machine with other work — the ~20% run-to-run drift seen even *within* the
same algorithm in check 3 above shows how easily a single reading misleads on this machine.

---

## Finding 4 (Low severity / footgun, cross-cutting): `pluss::make_series(scalar)` with no
explicit `Config` silently defaults to Float64 and promotes the whole downstream graph

**Location:** general `pluss::make_series(double)` single-argument factory behavior; caught via
a downstream call in the reproduction project's own code
(`hng_unconditional_variance`'s `pluss::make_series(1.0)`), not in this SDK's source — but the
*mechanism* is a property of the SDK's factory/promotion rules and will bite any caller who
mixes an explicitly-configured `Series` (e.g. Float32, built for `pluss::ai::nn::optim`
compatibility) with a bare `pluss::make_series(scalar)` literal anywhere in the same
expression.

**Symptom:** a Float32 parameter combined with one bare `pluss::make_series(1.0)` scalar
silently produces a Float64 result several operations downstream, with no warning — surfacing,
in this case, only because `pluss::ai::nn::optim::Optimizer::step()` happens to hard-check the
loss dtype (Finding 3) and threw. In any context *without* such a downstream check, this would
silently cost precision/memory or fail far away from the actual cause.

**Suggested generic fixes (either would help; both would help more):**

1. A debug-mode/opt-in diagnostic that warns when an operation's result dtype differs from
   *every* operand's dtype specifically because of an implicit-default scalar (as distinct from
   an intentional promotion between two explicitly-typed operands, which is presumably fine and
   should stay silent).
2. An idiomatic "scalar shaped like this other Series" factory (mirroring libtorch's
   `tensor.new_tensor`/`new_zeros` convention, e.g. `x.new_scalar(value)`) promoted as the
   preferred way to introduce a constant inside dtype-generic code, with the bare
   `pluss::make_series(scalar)` overload's own doc comment flagged as easy to misuse for this
   reason.

---

## Finding 5 (Scope note, not a bug, PlussMa): DFO population minimizers are impractical for
sequential (non-batchable) objectives at their documented defaults

**Location:** `PlussMa/Math/Numerical/optimization/dfo_minimizers.h`
(`differential_evolution_minimizer`, `differential_evolution_prmm_minimizer`,
`particle_swarm_minimizer`, `particle_swarm_dv_minimizer`).

**Observation:** these default to `population_size=200`, up to `max_generations=1000`, and are
designed around `PopulationSolverExecutionMode::Vectorized` — i.e. batched evaluation of the
whole population per generation. For an objective that is an inherently sequential recursion
over a long time series (this project's ~7,559-step likelihood, evaluated one path at a time),
the effective cost is `population_size × generations` full sequential evaluations — tens of
thousands at the defaults, each costing on the order of ~1 second on this project's objective —
which was judged impractical to run within this audit and was not attempted.

This is not a defect in the DFO implementations; it's a documentation/guidance gap. Consider
adding an explicit note (and ideally a worked example) on how to reshape a sequential
per-path recursion into a single batched pass across the population dimension, since that is
the difference between these solvers being practical or not for this entire class of
objective (any time-series likelihood, not just this one).

---

## Finding 6 (High severity — FIXED, PlussMa): `septima`'s LBFGS StrongWolfe line search
enforced a curvature condition that backtracking cannot reliably satisfy, causing false
"no progress" stalls independent of Findings 1a/1b

**Status: fixed in this repo.** Distinct from Finding 1's disconnected-gradient bugs (which
fabricated a zero gradient and caused *false convergence*), this is a structural limitation of
the line search's *acceptance test* that caused genuine, repeated *false stalls* even with a
correct, non-zero gradient every iteration.

**Location:** `pluss/subsp/math/optim.h`, `septima_step_inplace_`'s `StrongWolfe` branch (LBFGS
mode).

**Symptom:** on the reproduction case's naive init, `septima()` in `Lbfgs`/`StrongWolfe` mode
ran 28 iterations (719 function evaluations) and exited via its own no-progress path:
`logLik=24808.23`, with `gamma` and `lambda` barely moved from their initial values (`50.002`,
`0.104` vs. init `50.0`, `0.1`) — a materially worse result than either Adam-family method on
the same problem, despite LBFGS being the theoretically stronger method here.

**Root cause:** the line search required *both* the Armijo (sufficient-decrease) condition
*and* the strong-Wolfe curvature condition (`|gtd_new| <= -wolfe_c2*gtd`) before accepting a
trial step, backtracking geometrically on failure. This is structurally unable to reliably
satisfy curvature: as `step_size` shrinks toward 0, the trial gradient's directional derivative
`gtd_new` converges *to* the base directional derivative `gtd` itself, so the curvature
condition (which requires `|gtd_new|` to be meaningfully *smaller* than `|gtd|`, since
`wolfe_c2 < 1`) becomes *harder* to satisfy the more the search backtracks, not easier. A
backtracking-only search can therefore exhaust its entire budget without ever finding a step
that satisfies both conditions simultaneously, even when perfectly good, Armijo-satisfying
steps exist at several of the tested step sizes. This is exactly the same structural failure
mode found and fixed in this SDK's own L-SR1 trust-region optimizer (`optim_lsr1.h`, added in
this same pass — see Finding 7) while verifying *that* implementation against Rosenbrock and
this same reproduction case, and confirmed here to affect `septima`'s pre-existing LBFGS too,
not just the new addition.

**Why dropping curvature enforcement from the line search is safe here:**
`update_lbfgs_history_` (the function that actually accumulates the `(s, y)` pairs LBFGS's
inverse-Hessian approximation is built from) already applies its own, independent curvature
skip — `lbfgs_curvature_ok_`, checking `s^Ty > tolerance * max(1, ||s||*||y||)` — before adding
any pair to history. The line search enforcing curvature too was redundant "belt and
suspenders," not the only thing keeping the LBFGS approximation well-defined; a
curvature-violating step can still be accepted by the line search and simply won't contribute a
pair to history, exactly as designed.

**Fix applied:** the line search now accepts on the Armijo condition alone. A second layer
mirrors L-SR1's own fix: if no trial anywhere in the backtracking budget satisfies Armijo's
strict margin, the best (smallest-objective) *finite* trial seen is accepted anyway, on the
reasoning that once `step_size` shrinks far enough the *required* decrease
(`wolfe_c1*step_size*gtd`) can fall below the floating-point noise floor of evaluating the
objective, so a genuinely improving trial can still fail the strict inequality — discarding
real, if small, progress over an increasingly unattainable margin is not a genuine failure. The
now-unused `wolfe_c2` field is kept in `SeptimaOptions::LbfgsOptions` for API/ABI compatibility
(unlike the brand-new `optim_lsr1.h`, where the equivalent dead option was removed outright,
`septima` is an established, likely-already-consumed public surface), documented in place as no
longer read.

**Verified:** re-ran the identical reproduction case after the fix. `septima` LBFGS+StrongWolfe
now runs 107 iterations (321 function evaluations) and reaches `logLik=24994.88804218`
(`omega≈0, beta=0.806784, alpha=4.19e-6, gamma=190.0754, lambda=3.274511`) — matching the
literature reference (`24994.89`) closely and landing on essentially the *same* optimum L-SR1
independently reaches (Finding 7: `logLik=24994.888`), a useful cross-validation that both
fixes converge to the true answer rather than two different local artifacts. This replaces the
previous stall at `24808.23` after 28 iterations with gamma/lambda barely moved from init.

---

## Finding 7 (New capability, PlussMa): L-SR1 (limited-memory Symmetric Rank-1 with
trust-region globalization) added as a third `septima`-family method — the first in this SDK
that supports indefinite curvature

**Location:** new file `pluss/subsp/math/optim_lsr1.h`, plus a new `eigh()` (symmetric
eigendecomposition) primitive added to `pluss/subsp/math/linalg.h` (no such primitive existed
anywhere in this SDK before — confirmed via a repo-wide search; `LYMPH_GENERAL_ML_EXPANSION_PLAN.md`
even lists it as a known gap).

**Why this is a "finding" and not just a feature note:** unlike BFGS/LBFGS (used by `septima`)
and Adam-family methods (used by `septima` and `Minimax`), SR1 does not force
positive-definiteness onto its Hessian approximation, so it can represent indefinite curvature
(saddle points, negative-curvature directions) that the rest of this SDK's optimizer surface
structurally cannot. Implements Erway, Griffin, Marcia, Omheni, *"Trust-Region Algorithms for
Training Responses..."* (arXiv:1807.00251v3), Algorithm 1 (outer trust-region loop) and
Algorithm 2 (exact orthonormal-basis subproblem solver, including the "hard case") verbatim
against the paper.

**Four real bugs were found and fixed during implementation**, each caught by an independent
verification method (a different code path or a direct optimality check, not just re-reading
the same derivation):

1. A hard-case eigenvector formula error in the subproblem solver (`Q[:,i]` used where
   `Q*U[:,i]` — the actual eigenvector of the compact Hessian restricted to `span(Q)` — was
   required), caught by an independent KKT-optimality check.
2. A numerically unstable `gamma` (initial Hessian scale) clamp: the paper's own recommended
   scale-selection procedure, computed via a Cholesky-congruence reduction, produced wild
   garbage values (observed: ~-1.5e17 from a matrix whose true eigenvalues were O(1-1e3)) when
   the small generalized eigenproblem it depends on was ill-conditioned. Fixed with a
   condition-number guard, mirroring an existing degeneracy-guard pattern already used elsewhere
   in the same file.
3. The same backtracking-cannot-enforce-curvature structural issue as Finding 6 above (found
   here *first*, then confirmed to also affect `septima`'s pre-existing LBFGS).
4. Even with curvature dropped, backtracking-only search can still hard-fail if the trust-region
   step itself travels into a numerically pathological region for a specific objective — observed
   directly on this report's reproduction case (a 7,559-step sequential recurrence): a full step
   was non-finite, a half step was an astronomical-but-technically-finite blowup (~4e161), and
   every smaller step approached the current objective from the *worse* side without ever
   crossing below it. Fixed by treating an entirely-unimproving line search exactly like any
   other badly-rejected trust-region step (shrink the radius and retry, bounded by a
   no-progress iteration cap) instead of an unconditional hard stop.

**Verified:** compact SR1 representation vs. an independent dense recursive reference (exact
match); subproblem solver vs. an independent brute-force full-dimension solver across interior,
boundary, and both hard-case variants (primary check: direct KKT-optimality, not raw-vector
comparison, since the hard case's solution is only unique up to the choice of degenerate
eigenvector); full algorithm on 2D/10D Rosenbrock to machine precision (~1e-21 objective). On
this report's own reproduction case, L-SR1 now reaches `logLik=24994.888` (273 iterations,
1373 function evaluations) — matching the literature reference (`24994.89`) almost exactly,
with fitted parameters essentially identical to it, and is now the best result of the three
PlussMa/PlussAI methods compared here (L-SR1, `septima` LBFGS, `septima` Adam), not just the
fastest.

---

## Finding 8 (Environment/tooling, unresolved in the project's actual build directories):
`<complex>` cannot be compiled by this machine's configured `cpp_env` conda clang toolchain

**Discovered:** while verifying the complex-dtype support added per Finding 3 above (any file
including `<complex>` is affected, not something specific to the optimizer changes).

**Symptom:** the `cpp_env` conda environment's clang cannot compile *any* translation unit that
includes `<complex>`, as of the date this was found — a real SDK/toolchain incompatibility, not
a code bug: Xcode on this machine now ships only a beta `MacOSX27.sdk`, ahead of the running
macOS version, whose `math.h` expects clang module support that the conda-provided clang does
not correctly resolve for that SDK. This blocks every existing `build-*` directory that links
against a target including `<complex>`, not just files touched by this pass.

**Status:** worked around for verification purposes only (a disposable build directory using
Apple's own `/usr/bin/clang++`, matched to the SDK, compiles cleanly) — the project's real,
persistent build directories are still pointed at the now-broken conda clang, since switching
the whole project's compiler is a call for whoever owns the build configuration, not something
to change silently as a side effect of an optimizer audit.

**Suggested fix:** either pin `CMAKE_CXX_COMPILER` to Apple's clang (keeping
`CMAKE_PREFIX_PATH`/dependency resolution on `cpp_env`) for build directories that need
`<complex>`, or update/pin the conda clang toolchain to one that resolves the currently-installed
Xcode SDK correctly.

---

## Summary table

| # | Component | Severity | Type | Status | One-line fix direction |
|---|---|---|---|---|---|
| 1a | `septima` (PlussMa) | High | Correctness (false convergence) | **Fixed** | `DerivativePolicy{Undefined, keep_graph=false}` at both call sites; treat "any block undefined" like non-finite in the line search and the outer convergence check |
| 1b | `septima_unflatten_values_` (PlussMa) | High | Correctness (permanently stuck, root cause of 1a's symptom) | **Fixed** | `clear_source_link()` each reconstructed block before handing it to `value_and_grad` as a fresh leaf |
| 1 (perf) | `septima` LBFGS line search (PlussMa) | — | Performance (wasted backward pass) | **Fixed** | New `septima_value_only_`/`septima_augmented_value_only_` skip `.reverse()` for the base-point evaluation, which only ever needed the scalar value |
| 1c | `septima` / `pluss::codem::series_tape` (PlussMa/PlussQit) | High | Unbounded memory growth at realistic scale | **Unresolved** | Needs someone with tape-internals visibility; likely a missing explicit clear/prune between independent evaluations in a long optimization loop |
| 2 | `optima` trust-region presets (PlussMa) | Medium | Robustness / scaling | Not fixed | Per-parameter scaling for multi-order-of-magnitude unconstrained problems; document LBFGS as the unconstrained-problem default |
| 3 | `pluss::ai::nn::optim::*` incl. Minimax (PlussAI) | High | Stale capability gate | **Fixed** | Float64 allowlisted; complex-dtype `isfinite()` gap in NaN-recovery found+fixed alongside (Minimax + all 5 moment-tracking optimizers); Minimax's constraint guards refined to precise per-mechanism rules |
| 4 | `pluss::make_series(scalar)` (cross-cutting) | Low | Footgun / DX | Not fixed | Warn on implicit-default-scalar-driven dtype promotion; provide a `new_scalar`-style idiom |
| 5 | DFO minimizers (PlussMa) | Info | Documentation | Not fixed | Guidance for batching sequential objectives across the population dimension |
| 6 | `septima` LBFGS StrongWolfe line search (PlussMa) | High | Correctness (false "no progress" stall, distinct from 1a/1b) | **Fixed** | Armijo-only acceptance + best-improving-trial fallback; `update_lbfgs_history_`'s own independent curvature skip already protects the LBFGS update, so the line search enforcing curvature too was redundant and unenforceable via backtracking |
| 7 | New: `optim_lsr1.h` L-SR1 trust-region optimizer (PlussMa) | — | New capability | **Added, 4 bugs found+fixed during implementation** | First indefinite-curvature-aware method in this SDK; reaches the reproduction case's literature-reference optimum almost exactly (best of the 3 PlussMa/PlussAI methods compared) |
| 8 | `<complex>` vs. this machine's `cpp_env` conda clang (environment) | — | Toolchain incompatibility | Unresolved in the project's real build dirs | Pin `CMAKE_CXX_COMPILER` to Apple clang (keep `cpp_env` for dependency resolution) for build directories needing `<complex>`, or fix the conda toolchain's SDK resolution |

All of these are reproducible with the plain 5-parameter Gaussian-QMLE reproduction case
described at the top of this report — no access to the originating downstream project is
required to verify any of them. Findings 1a/1b/1-perf/6 are landed in this repo's
`PlussMa/include/pluss/subsp/math/optim.h` (verified via a fast isolated numeric-equivalence
test for the value-only path, direct line-search instrumentation for 1a/1b/6, and end-to-end
runs reaching genuine accepted steps and, for Finding 6, the literature-reference optimum);
Finding 1c was discovered while verifying those fixes at realistic scale and remains open.
Finding 3 is fixed per the author's own prior-session record (see that finding for details).
Finding 7 (L-SR1) is a new addition, not a fix to existing code, landed in the new
`PlussMa/include/pluss/subsp/math/optim_lsr1.h` plus a new `eigh()` in
`PlussQit/include/pluss/subsp/math/linalg.h`.
