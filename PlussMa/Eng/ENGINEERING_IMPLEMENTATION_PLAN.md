# PlussMa Engineering Implementation Plan

## Purpose

`PlussMa/Eng` will provide engineering analysis and design machinery over the numerical
foundations already owned by PlussQit and PlussMa. It is not a second tensor library, a
second optimization package, or a second natural-science implementation. Engineering
modules own engineered-system models: topology, components, constraints, assembly,
responses, design objectives, diagnostics, and domain conventions.

The first goal is broad, dependable coverage of the foundations shared by electrical,
mechanical, civil, control, robotics, and communications work. Depth then grows behind
stable contracts. A domain is not considered implemented because it has a few formulas;
its first public tranche must include validation, batched execution, failure behavior,
independent numerical oracles, and a permanent verifier.

Correctness is ordered ahead of performance, and performance ahead of breadth. Reference
source and papers provide contracts, algorithms, edge cases, and test vectors. Their code
is not copied. Pluss implementations remain original, `Series`-native, and consistent with
the Pluss error, platform, dtype, and automatic-differentiation systems.

## Current State

The first shared-transform and Engineering tranche is implemented:

- `Series::fft()`, `ifft()`, `rfft()`, and `irfft()` are generated public methods backed
  by one batched CPU transform engine. Power-of-two lengths use a cached Stockham plan;
  other lengths use cached Bluestein convolution plans. Axes, requested lengths,
  normalization, strided inputs, real/complex dtypes, batching, platform restoration,
  and reverse sensitivity are covered by permanent tests.
- PlussMa Finance no longer owns a private FFT or quadratic DFT fallback. Heston/Bates
  Carr-Madan and fractional-transform convolution now delegate to the Series transform
  surface.
- The empty `signal_proc` shell has been removed. The canonical public family is
  `pluss::subsp::eng::signal`.
- `pluss::subsp::eng::signal` contains Series-vectorized boxcar, Bartlett, Blackman,
  Hamming, and Hann windows with symmetric and periodic contracts.
- `pluss::subsp::eng::lti` contains a validated, batched continuous/discrete state-space
  record, controllability and observability assembly, and continuous/discrete frequency
  response.
- `pluss::subsp::eng::signal` now owns batched linear convolution, conjugating
  correlation, causal FIR application, and FIR/IIR delay-polynomial frequency response.
- `pluss::subsp::eng::signal` now owns stateful direct-form filtering, cascaded
  second-order-section filtering, and endpoint-conditioned zero-phase filtering.
- `pluss::subsp::eng::lti` now owns a validated pole-zero-gain record, direct
  continuous/discrete response, stability classification, and stable digital SOS
  realization. `pluss::subsp::eng::signal` builds Butterworth and Chebyshev I/II
  low-pass, high-pass, band-pass, and band-stop systems through that representation.
- `pluss::subsp::eng::lti::sim()` now advances continuous and discrete state-space
  systems with explicit zero-order or linear input holds, optional initial state,
  leading-axis batch broadcasting, irregular continuous sample grids, complex values,
  and reverse sensitivity through the complete trajectory.
- `Series::linearalg` now owns batched Sylvester, continuous/discrete Lyapunov, and
  continuous/discrete algebraic Riccati equations. The stable routes use solve-based
  doubling or matrix-sign iterations and retain dtype, complex adjoints, platform
  routing, leading-axis batches, and reverse history.
- `pluss::subsp::eng::control` now assembles CARE/DARE, continuous/discrete LQR, and
  continuous/discrete LQE through those canonical kernels. Cross weights, weight
  validation, closed-loop matrices, and estimator duality are explicit. `lti::gram()`
  provides reachability and observability Gramians through the same Lyapunov owner.
- `pluss::subsp::math::state` now owns batched linear Gaussian filtering and RTS
  smoothing. Engineering, Finance, and Stats adapt to that one Series-native owner.
- `pluss::subsp::eng::control` now evaluates square MIMO loop sensitivity and
  complementary sensitivity and extracts batched SISO gain, phase, and sampled
  stability margins from continuous or discrete frequency responses.
- `pluss::subsp::eng::control` now assigns real or complex closed-loop poles through
  controllable eigenvector subspaces and conditioning-improving sweeps, and represents
  continuous or discretized parallel-form PID controllers with explicit integration and
  derivative rules.
- `pluss::subsp::math::qp` now owns dense convex quadratic programs in the canonical
  two-sided form `lower <= A*x <= upper`. Its prepared solver caches one Cholesky
  factorization, supports leading-axis batches, equality and infinite bounds, warm starts,
  residual diagnostics, and reverse history through the executed iterations.
- `pluss::subsp::eng::control::mpc` now condenses finite-horizon discrete linear systems
  into that shared QP owner. Stage, terminal and input costs; constant or staged
  references; input bounds; optional state bounds; batch scenarios; warm starts; and a
  reusable compiled plan are active.
- `pluss::subsp::eng::power` now compiles bus, generator, and branch topology into shared
  AC/DC network operators and solves batched DC and polar Newton AC power flow with
  explicit per-unit bases, branch-end powers, residuals, convergence, and reverse history.
- The same compiled power grid now produces slack-referenced PTDFs, bridge-aware LODFs,
  and batched N-1 branch-flow screens without rebuilding or scalar-looping over outages.
- It also traces batched AC loading paths with an analytic pseudo-arclength predictor-
  corrector, exact target correction, and localized voltage-stability noses.
- It solves balanced AC optimal power flow over voltage angle/magnitude and active/reactive
  generator decisions, with full complex nodal balance, optional two-ended apparent-power
  ratings, separable quadratic costs, bound and constraint multipliers, batched scenarios,
  and analytical first derivatives delegated to the shared nonlinear optimizer.
- The shared linear-algebra solve kernel now retains complex coefficient and right-hand
  side values instead of narrowing them to `Float64`; state-space response delegates to
  that existing batched, differentiable solver.
- Permanent signal and LTI executables cover valid, batched, strided, promoted, complex,
  differentiable, empty, singleton, and invalid contracts.

This is E0's transform portion, E1a through E1e, and E2a through E2d, not all of E0, E1,
or E2. Unit extraction, advanced linear-algebra ownership, elliptic/Bessel and
order-selection design, transfer coefficients, broader conversion, interconnection,
PID tuning and stateful controller execution remain subsequent tranches.
`control/lqr` and the first linear `control/mpc` tranche are active. Output bounds, soft
constraints, move suppression, sparse/partial condensing, infeasibility certificates,
polishing, nonlinear MPC and moving-horizon estimation remain later E3 work.

## Completed Tranche Audit

### E0 transform ownership

The Series family uses the established transform convention: forward transforms are
unscaled under `"backward"`, inverse transforms carry `1 / n`, `"forward"` reverses those
scales, and `"ortho"` uses reciprocal square-root scaling. `rfft()` exposes the
non-negative-frequency half, while `irfft()` reconstructs Hermitian symmetry before the
inverse transform. A requested length crops or zero-pads along the selected axis.

The implementation reuses Series shape, cast, concatenate, device-transfer, and tape
machinery. The native kernel processes independent transforms in parallel and groups
interleaved lanes so non-final axes do not become one scalar gather loop per signal.
Immutable transform plans are held in bounded, mutex-protected caches; graph-bearing
Series results are never cached.

Sources inspected for this tranche:

- PyTorch's local `torch/_refs/fft.py` and
  `torch/csrc/autograd/FunctionsManual.cpp` from the repository at
  `/Users/yawasante/Documents/Doctrate/Thesis/Python/second_project/Codes/pytorch`, for
  length/axis/normalization contracts and real/complex reverse rules;
- SciPy's `scipy/fft/_basic.py` and PocketFFT-facing contracts on the upstream `main`
  branch, inspected 2026-10-16, for public transform semantics and validation;
- FFTW 3.3.11 manual sections on one-dimensional, real-data, strided, batched, and plan
  execution, for planning and layout constraints;
- the former private transform in `PlussMa/Math/Finance/pricing/heston_sv.cpp`, audited as
  a migration target rather than treated as a correctness oracle.

Permanent verification includes direct-DFT oracles at radix and prime lengths, known
independent spectra, one- and multi-axis transforms, non-contiguous views, 2-D batches,
all normalization modes, crop/pad behavior, odd/even real round trips, dtype policy,
analytic adjoints, and invalid contracts. Heston, Bates, and COS/FFT pricing tests protect
the migrated consumer.

### E1a windows and state space

Window definitions follow the finite symmetric form over `n - 1` intervals. Periodic
mode evaluates the same family over `n` intervals so the omitted endpoint can tile the
signal without duplication. A zero-length request returns an empty Series; a singleton
is one because no taper can be defined from a zero denominator. Window arithmetic is
expressed entirely with Series operations and preserves the requested floating dtype and
platform.

`StateSpace` validates the trailing matrix contract
`A[..., n, n]`, `B[..., n, m]`, `C[..., p, n]`, and `D[..., p, m]`, then broadcasts only
the leading batch axes. All four matrices share one platform and are promoted once to a
common floating or complex dtype. A sample time of zero denotes a continuous model; a
finite positive value denotes a discrete model. Controllability is assembled as
`[B, AB, ..., A^(n-1)B]`; observability is assembled by vertically joining
`C, CA, ..., CA^(n-1)`. The small loop advances matrix powers, while every numerical
step and final assembly remains a batched Series operation and retains reverse history.

Sources inspected for this tranche:

- SciPy `scipy/signal/windows/_windows.py` and
  `scipy/signal/tests/test_windows.py` on upstream `main`, inspected 2026-10-16, for
  window equations, symmetric/periodic extension, singleton behavior, and test vectors;
- MATLAB R2025b visible sources
  `/Applications/MATLAB_R2025b.app/toolbox/signal/signal/hann.m` and `hamming.m`, for
  symmetric/periodic conventions and input validation;
- python-control `control/statesp.py` and `control/statefbk.py` on upstream `main`,
  inspected 2026-10-16, for state-space dimensions, timebase meaning, controllability,
  and observability conventions;
- MATLAB R2025b visible sources
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrlanalysis/obsv.m` and
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrlmodels/@StateSpaceModel/ctrb.m`,
  for matrix assembly and model-array behavior.

Pluss deliberately extends the common two-dimensional contracts with leading-axis batch
broadcasting and reverse sensitivity. Transfer-function and zero-pole-gain forms,
minimal-realization/rank policy, transient responses, interconnections, Gramians,
conversion, and simulation are deferred rather than approximated in E1a.

### E1b convolution, filtering, and frequency response

`signal::conv()` computes linear rather than circular convolution. A one-dimensional
kernel is aligned with the selected sample axis; a rank-matched kernel may carry its own
broadcastable batch axes. `Full`, `Same`, and `Valid` use the established centered
alignment: full length is `n + m - 1`, same length follows the first input, and valid
length is `abs(n - m) + 1`. The implementation zero-pads through the canonical Series
FFT, multiplies entire frequency batches, and transforms back without host lane loops.
Real inputs return their promoted real dtype; either complex operand retains complex
output. `corr()` reverses and conjugates the second input before using that same engine,
so there is one alignment and performance policy rather than two drifting kernels.

`signal::fir()` is the zero-state causal projection of full convolution: sample `k`
contains only input values available through `k`, and the result is cropped to the input
sample count. `signal::freqz()` evaluates coefficients ordered by increasing delay,
`b[0] + b[1] exp(-j omega) + ...`, on an explicit angular-frequency vector. The
three-Series overload divides numerator and denominator polynomials for IIR response.
Moving the coefficient axis to the trailing position lets one Series expression evaluate
individual filters, filter banks, and higher-rank parameter sweeps.

`lti::freqresp()` evaluates the resolvent without forming an inverse. Continuous systems
solve `(j omega I - A) X = B`; discrete systems solve
`(exp(j omega T) I - A) X = B`, where `T` is `sample_time`. The returned layout is
`[..., outputs, inputs, frequency]`. Frequency is therefore radians per time unit for
both models, rather than silently changing to radians per sample when a sample time is
known. Matrix batches, frequency points, state solves, input channels, and output
channels remain vectorized Series axes. The only decomposition loop is the existing
batched solver boundary.

The shared CPU solve now dispatches real systems through `dgesv` and complex or
mixed-real/complex systems through `zgesv`, retaining ordinary dtype promotion. Its
existing analytic reverse rule solves the adjoint system and forms
`dA = -dB X^H`; Engineering response therefore remains differentiable with respect to
the system matrices. Native MPS solve remains real-only and rejects complex routing so
complex response uses the established bridge instead of losing imaginary components.

Permanent verification covers full/same/valid alignment in both argument orders,
correlation conjugation, causal FIR truncation, non-contiguous samples, broadcast filter
banks, complex convolution, FIR and IIR values at DC and Nyquist, continuous and
discrete resolvents, batched systems, reverse sensitivities, and invalid dtype/rank
contracts. The pre-existing Fourier, STFT, LU, and linear-algebra suites are run as owner
regressions.

Sources inspected for this tranche:

- SciPy `scipy/signal/_signaltools.py` on upstream `main`, inspected 2026-10-16, for
  convolution/correlation definitions, first-input `same` sizing, `valid` alignment, and
  direct-versus-FFT policy;
- SciPy `scipy/signal/_filter_design.py` on upstream `main`, inspected 2026-10-16, for
  the `freqz` ascending-delay polynomial convention and frequency units;
- python-control `control/statesp.py` on upstream `main`, inspected 2026-10-16, for
  state-space frequency evaluation and solve-based resolvent handling;
- MATLAB R2025b source
  `/Applications/MATLAB_R2025b.app/toolbox/signal/signal/freqz.m`, for FIR/IIR response,
  explicit-frequency behavior, half/whole-circle conventions, and radians/sample versus
  sampled-frequency units;
- LAPACK `DGESV` and `ZGESV` driver contracts as exposed by Apple Accelerate, for pivoted
  real and complex general-system solves.

Overlap-add planning, automatic direct/FFT crossover, filter design, transfer-function
records, and time-domain state simulation remain explicit later E1 work. They are not
represented by aliases or partial options in E1b.

### E1c stateful and zero-phase filtering

`signal::filter()` implements the normalized direct-form II transposed recurrence for a
one-dimensional numerator and denominator. The selected sample axis may live anywhere in
an N-dimensional input; every remaining axis is an independent signal lane. The only
host loop advances the causally dependent sample state. Each state update operates on all
lanes through `Series` expressions, so strided inputs, dtype promotion, complex values,
platform dispatch, and reverse sensitivity continue through the canonical Series
machinery.

Both zero-state and explicit-state calls return `FilterRun{values, state}`. State is
stored as `[..., order]`, where the leading dimensions are the input dimensions with the
sample axis removed. A shorter-rank state may broadcast over those signal lanes. This
makes chunked execution exact: passing one run's final state to the next run produces the
same samples and final state as filtering the joined input once. Numerator and denominator
vectors are padded to one common order and normalized by `a[0]`; a zero leading
denominator coefficient fails before computation.

`signal::sos()` accepts `[section, 6]` rows ordered as
`[b0, b1, b2, a0, a1, a2]`. Sections are applied in row order using the same recurrence,
not a separate numerical implementation. Its state layout is
`[..., section, 2]`, allowing every section and signal lane to resume independently.
Second-order sections are the stable public route for high-order IIR filters; expanding a
high-order polynomial into one direct-form recurrence is intentionally not presented as
equivalent numerical practice.

`signal::zero_phase()` performs an endpoint-conditioned forward pass, reverses the
result, applies the same filter again, reverses once more, and removes the extension. Odd,
even, constant, and no-padding policies are explicit through `PhaseOpts`. The automatic
edge length follows local MATLAB R2025b: three times the effective filter order, with a
minimum edge of one. The sample axis must be longer than the edge. Initial state is the
direct-form II step steady state, solved with batched Series linear algebra and scaled by
the first extended sample. A denominator with `sum(a) == 0` has no finite step steady
state and therefore fails rather than fabricating an edge condition.

For SOS zero-phase filtering, each section receives the cascade's total effective-order
edge length and is filtered forward/reverse before the next section. This follows MATLAB
R2025b's `filtfilt(SOS,G,X)` execution. SciPy's `sosfiltfilt` instead initializes and
filters the complete cascade in each direction; the two are both zero-phase but may
differ near finite-signal edges. The distinction is deliberate and covered by the local
MATLAB oracle rather than being left as accidental implementation detail.

Permanent verification covers known transfer-function and SOS vectors emitted by local
MATLAB R2025b, exact chunk continuation, broadcast initial state, non-contiguous and
higher-rank sample axes, complex filtering, all padding modes, transfer and SOS
zero-phase paths, analytic input/coefficient/state sensitivities, and stable malformed
coefficient, state, section, short-signal, and no-steady-state failures.

Sources inspected for this tranche:

- SciPy `scipy/signal/_signaltools.py` and `scipy/signal/_sosfilt.pyx` on upstream
  `main`, inspected 2026-10-16, for direct-form II transposed equations, coefficient
  normalization, N-dimensional state layout, SOS cascade order, endpoint extensions,
  and steady-state construction;
- MATLAB R2025b sources
  `/Applications/MATLAB_R2025b.app/toolbox/signal/signal/sosfilt.m`,
  `filtfilt.m`, and `private/filtfilt_initial_conditions.m`, for effective filter order,
  edge-length policy, section-wise SOS forward/reverse execution, and the steady-state
  linear system;
- MATLAB R2025b executable outputs for one transfer-function recurrence, one SOS cascade,
  and both transfer-function and SOS forward/reverse paths, retained as permanent numeric
  vectors in `filter_test.cpp`.

### E1d classical IIR design and pole-zero systems

`lti::PoleZero` is the shared lossless representation for a proper SISO system. Zeros,
poles, and gain are stored as complex Series on one platform and at one precision;
`sample_time == 0` denotes a continuous model and a positive value denotes a discrete
model. Validation rejects non-vector roots, non-scalar gain, non-finite values, mixed
platforms, improper root counts, and invalid timebases before an analysis kernel can
consume them. Stability follows the appropriate domain: open left half-plane for a
continuous system and open unit disk for a discrete system.

`lti::freqresp()` evaluates the factored representation directly. Continuous response
uses `s = j omega`; discrete response uses `z = exp(j omega T)`. The frequency axis and
all root products are Series operations, avoiding polynomial expansion and its
conditioning loss. `lti::to_sos()` is deliberately discrete-only because the existing
signal SOS runner consumes delay-form sections. It keeps conjugate roots together for
real systems, orders pole groups from the unit-circle boundary inward, pairs each group
with the nearest remaining zero group, and embeds the system gain once. Odd orders gain
one cancelling origin pole/zero solely to fill the final biquad. Complex systems remain
complex instead of being silently projected to real coefficients.

`signal::butter()`, `cheby1()`, and `cheby2()` first construct the normalized analog
prototype, then apply low/high or wideband pass/stop root mappings. Digital designs
prewarp every user cutoff and apply the Tustin map; analog design is selected by omitting
`IirOptions::sample_rate`. A present sample rate therefore always has operational
meaning and is recorded as its reciprocal in the resulting model. Scalar and two-cutoff
overloads are separate contracts, so a scalar cannot accidentally request a band filter
and a pair cannot silently degrade to a low-pass design. Cutoffs use the supplied sample
rate's units for digital systems and angular frequency for analog systems.

Filter construction is setup work rather than sample-stream work. Prototype and mapping
loops scale only with filter order, execute once on the host, and materialize the final
roots once on the requested platform. Runtime response and filtering remain batched
Series paths. SOS conversion necessarily performs discrete root pairing on the host;
it transfers only the small root metadata and returns one contiguous section matrix,
never signal samples. This avoids pretending that a branch-heavy combinatorial pairing
algorithm benefits from device dispatch while keeping the high-volume path on Series.

Permanent verification compares Butterworth low/high/pass/stop and Chebyshev I/II
responses with local MATLAB R2025b at DC, interior frequencies, Nyquist, and transition
regions. Every digital oracle is checked through both factored response and a separately
evaluated SOS cascade. Analog poles/gain, odd-order realization, Float32 policy,
stability, cutoff/ripple validation, and analog-SOS rejection are also covered.

Sources inspected for this tranche:

- SciPy `scipy/signal/_filter_design.py` on upstream `main`, inspected 2026-10-16, for
  Butterworth and Chebyshev prototypes, low/high/pass/stop zero-pole mappings, digital
  prewarping, the bilinear transform, root-count policy, and nearest-root SOS pairing;
- MATLAB R2025b sources
  `/Applications/MATLAB_R2025b.app/toolbox/signal/signal/butter.m`, `cheby1.m`,
  `cheby2.m`, `bilinear.m`, `lp2lp.m`, `lp2hp.m`, `lp2bp.m`, `lp2bs.m`, and
  `zp2sos.m`, for public validation, cutoff conventions, transformation structure, and
  real-section requirements;
- local MATLAB R2025b executable outputs for six digital designs and one analog design,
  retained as response and root oracles in `design_test.cpp`.

Elliptic and Bessel prototypes, automatic order selection, gain scaling across sections,
general transfer-coefficient records, transfer-to-root conversion, and filter-design
sensitivity are deferred explicitly. The public API does not expose inert options for
those capabilities.

### E1e state-space simulation

`lti::sim()` consumes input shaped `[..., time, inputs]` and returns one `SimRun` whose
`values` and `states` are shaped `[..., time, outputs]` and
`[..., time, states]`. Leading system, input, and initial-state axes broadcast under one
central LTI shape policy. Matrix axes and the time axis never participate in that batch
broadcast. Matrices and signals are promoted once to a common floating or complex dtype,
and every operand must already share one platform; simulation does not conceal transfers
inside a causal loop.

For continuous systems, `Hold::Zero` forms the exact augmented exponential
`exp([[A, B], [0, 0]] dt)` on each interval. `Hold::Linear` forms the larger FOH
augmentation used by SciPy and python-control and recovers separate coefficients for the
left and right input samples. All interval matrices, including unequal intervals, are
assembled on a time batch and passed to one batched `Series::linearalg.matrix_exp()` call.
Only advancement from state `k` to state `k+1` remains sequential because that recurrence
is causally dependent; each advancement simultaneously processes every model, signal,
state, input, and output lane through Series matrix operations.

For discrete systems, the supplied time increments must equal `sample_time` within a
scale-aware tolerance. State advancement is exactly `x[k+1] = A x[k] + B u[k]`, and the
reported sample is `y[k] = C x[k] + D u[k]`. A discrete model accepts only zero-order
holding because interpolation between already sampled inputs would invent an unrecorded
continuous model. `Hold::Auto` deliberately selects linear holding for continuous models
and zero-order holding for discrete models. This deterministic rule differs from MATLAB's
signal-smoothness heuristic: identical values always receive identical semantics, and a
caller can request ZOH explicitly for steps and switching signals.

Continuous time must be a finite, non-empty, strictly increasing vector. Unlike the
MATLAB and python-control public calls inspected for this tranche, Pluss permits unequal
continuous intervals because each interval receives its own exact augmented exponential.
The discrete grid remains uniform by definition. Empty matrix axes, mismatched input or
initial-state widths, incompatible batch axes, integer signals, mixed platforms,
non-finite time, duplicate/reversed samples, wrong discrete spacing, and linear holding
of discrete systems fail before trajectory execution.

No detached host data participates in numerical advancement. Consequently gradients can
flow through `A`, `B`, `C`, `D`, every input sample, and the initial state; continuous
systems additionally use the canonical matrix-exponential reverse rule. Permanent tests
cover closed-form first-order ZOH and FOH responses on an irregular grid, free response,
discrete SISO and MIMO recurrence, independent system/input/initial broadcasting,
strided input, complex unitary evolution, every validation branch above, and analytic
sensitivities for all six differentiable operands.

Sources inspected for this tranche:

- SciPy `scipy/signal/_ltisys.py` on upstream `main`, inspected 2026-10-17, for `lsim`
  input/state/output layout, ZOH and linear-hold augmented exponentials, initial-state
  semantics, and time validation;
- python-control `control/timeresp.py` on upstream `main`, inspected 2026-10-17, for
  `forced_response`, exact continuous state propagation, linear interpolation, discrete
  sample handling, and returned state trajectories;
- MATLAB R2025b visible sources
  `/Applications/MATLAB_R2025b.app/toolbox/shared/controllib/engine/@DynamicSystem/lsim.m`
  and
  `/Applications/MATLAB_R2025b.app/toolbox/shared/controllib/engine/+ltipack/@ssdata/lsim.m`,
  for the public `lsim` contract, automatic/ZOH/FOH policy, state reporting, complex-data
  policy, sample-rate requirements, and continuous-to-discrete execution boundary.

Transfer-coefficient records, transfer/state conversion, model interconnection, delayed
systems, descriptor systems, time-varying systems, and event-driven simulation remain
explicit later work. They are not approximated through hidden resampling or inert options.

### E2a matrix equations, LQR and LQE

The canonical matrix-equation surface uses the final two axes as matrix axes and
broadcasts every leading axis. `sylv(A, B, C)` solves `A X + X B = C` through one
vectorized linear system and therefore covers the general, not necessarily stable,
Sylvester equation. It is retained as the small/medium dense reference route; its
Kronecker workspace scales quadratically in matrix element count and is not presented as
the future large-system solver.

Stable Lyapunov equations do not build that workspace. `dlyap(A, Q)` uses squared Smith
doubling for `A X A^H - X + Q = 0`. `lyap(A, Q)` maps the continuous equation
`A X + X A^H + Q = 0` through a Cayley transform and delegates to the same doubling
engine. Both routes square the transition each iteration, operate over complete Series
batches, check the defining residual, preserve Hermitian structure, and fail when the
stable solution does not converge instead of returning the last iterate.

`care(A, G, Q)` solves `A^H X + X A - X G X + Q = 0` with a dynamically scaled Newton
matrix-sign iteration on the Hamiltonian and extracts the stable graph subspace from its
spectral projector. `dare(A, G, Q)` uses the structure-preserving doubling recurrence for
`X = Q + A^H X (I + G X)^-1 A`. Every inverse-looking expression is a canonical batched
solve. Residual checks, Hermitian checks, finite-value checks, dtype-sensitive tolerance
floors, and iteration bounds are shared through `MatrixEqOptions`.

Engineering `care`/`dare` accept `A`, `B`, `Q`, `R`, and an optional state-input cross
weight. They use one Cholesky factor of positive-definite `R`, complete the square for the
cross term, form `G = B R^-1 B^H`, delegate the matrix equation, and recover the feedback
gain without materializing `R^-1`. `lqr(StateSpace, ...)` dispatches from the model's
continuous/discrete timebase. `lqe(...)` is implemented once through estimator-controller
duality and returns covariance, estimator gain, and error-dynamics matrix. Reachability
and observability Gramians use the same continuous/discrete Lyapunov kernels.

This tranche deliberately does not claim generalized descriptor Riccati equations,
singular `R`, ordered closed-loop poles, or indefinite-cost control. Those require the
planned generalized Schur/QZ and conditioning surface; accepting them through a regular
solve would turn a structural failure into a plausible but invalid controller.

Sources inspected for this tranche:

- SciPy `scipy/linalg/_solvers.py` on upstream `main`, inspected 2026-10-17, for
  Sylvester/Lyapunov signs, direct versus bilinear discrete policy, balanced Hamiltonian
  and symplectic pencils, and fail conditions;
- python-control `control/mateqn.py`, `control/statefbk.py`, and `control/stochsys.py` on
  upstream `main`, inspected 2026-10-17, for CARE/DARE, LQR/DLQR cost conventions,
  LQE/DLQE duality, returned gains, and timebase dispatch;
- MATLAB R2025b visible sources `toolbox/control/control/lyap.m`,
  `toolbox/shared/controllib/engine/numerics/care.m`,
  `toolbox/shared/controllib/engine/numerics/dare.m`,
  `toolbox/control/ctrldesign/lqr.m`, and `dlqr.m`, for validation, residual, stabilizing
  solution, and cross-weight contracts;
- Ralph Byers, “Solving the algebraic Riccati equation with the matrix sign function,”
  *Linear Algebra and its Applications* 85 (1987), DOI 10.1016/0024-3795(87)90222-9,
  for scaled sign iteration and stable invariant-subspace extraction; and
- E. K.-W. Chu, H.-Y. Fan, and W.-W. Lin, “A structure-preserving doubling algorithm for
  continuous-time algebraic Riccati equations,” *Linear Algebra and its Applications*
  396 (2005), DOI 10.1016/j.laa.2004.10.010, for structure-preserving doubling,
  convergence diagnostics, and the separation of iterative kernels from control policy;
  and
- E. K.-W. Chu, H.-Y. Fan, W.-W. Lin, and C.-S. Wang, “Structure-Preserving Algorithms
  for Periodic Discrete-Time Algebraic Riccati Equations,” *International Journal of
  Control* 77(8) (2004), DOI 10.1080/00207170410001714988, for the discrete
  `A_k`/`G_k`/`H_k` doubling recurrence and quadratic convergence.

Permanent verification includes analytic scalar Sylvester/Lyapunov/CARE/DARE solutions,
coupled matrix residuals, leading-axis broadcasting, complex adjoints, Float32 retention,
cross weights, continuous/discrete Gramians, LQR/LQE duality, reverse sensitivities, and
fail-closed unstable, non-Hermitian, singular-weight, and invalid-policy cases.

### E2b shared state estimation

Linear Gaussian state estimation now has one neutral owner:
`pluss::subsp::math::state`. `LinearModel` represents constant transition,
observation, process-covariance and observation-covariance matrices, an initial Gaussian
state, and an optional shared input path `B u[t]` / `D u[t]`. `filter()` returns predicted
and filtered state moments, predicted observations, innovations, innovation covariance,
gain, channel-use masks, per-step log likelihood and total log likelihood. `smooth()`
accepts either observations or a previously computed filter result and applies the
Rauch-Tung-Striebel backward pass without rerunning the forward recursion.

The time loop is necessarily recursive, but every operation within one time step is a
batched Series operation. Observations use shape `[..., T, m]`; model, initial-state and
input batch axes broadcast through the matrix operations. Missing channels are represented
by a per-lane mask: their observation rows and covariance cross terms are zeroed and a unit
diagonal is inserted into the augmented innovation system. This is algebraically identical
to solving only the observed submatrix, while permitting different missing sensors in each
batch lane without host extraction, channel loops, or graph breaks.

One Cholesky factor per time step is reused for the Kalman gain, innovation quadratic and
log determinant. No explicit inverse or pseudoinverse is formed. The covariance update uses
Joseph form, prediction/update covariances are symmetrized, optional jitter is explicit,
and the same Series graph carries filtered states, smoothed states and likelihoods. Dtype
and platform are preserved instead of forcing Float64; mismatched storage, non-finite model
matrices, asymmetric or spectrally indefinite covariances, incompatible shapes, negative
tolerances and continuous-time Engineering models fail before recurrence. The covariance
preflight uses one batched Hermitian eigensolve per supplied covariance, so singular
positive-semidefinite models remain valid while positive diagonals cannot conceal an
indefinite matrix; this setup check is never repeated inside the time recursion.

Engineering exposes `control::kalman()` and `control::smooth()` over a discrete
`lti::StateSpace`; the controlled overload uses the model's `B` and `D` matrices. Math
Finance retains its established `KalmanModel` / `kalman_filter()` contract as a conversion
adapter, including its historical Float64 result and host `double` likelihood. Stats SSM
now delegates both filtering and smoothing to the shared owner; its former host-side gain
reconstruction and independent RTS implementation have been removed.

This tranche intentionally does not claim exact diffuse initialization, time-varying model
matrices, square-root/UD filtering, nonlinear EKF/UKF smoothing, or complex Gaussian
covariances. Finance's existing EKF and UKF are distinct nonlinear algorithms and remain
in place pending their own promotion. Those capabilities require explicit contracts rather
than inert flags on the linear engine.

Sources inspected for this tranche:

- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@ssm/kalman.m`, especially
  `fKalmanMultVar`, for prediction/update timing, informative-channel selection,
  Cholesky likelihood evaluation, gain placement and missing-observation behavior;
- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@statespace/smooth.m`, especially
  its `smoother` backward recursion, for filtered/forecast moment indexing and missing-data
  smoothing semantics;
- Statsmodels upstream `statsmodels/tsa/statespace/kalman_filter.py`,
  `statsmodels/tsa/statespace/_filters/_conventional.pyx.in`, and
  `statsmodels/tsa/statespace/kalman_smoother.py`, inspected 2026-10-17, for conventional
  filter timing, solve/stability policy, stored result layout, missing-design zeroing and
  smoothing output contracts; and
- H. E. Rauch, F. Tung, and C. T. Striebel, “Maximum likelihood estimates of linear
  dynamic systems,” *AIAA Journal* 3(8), 1965, DOI 10.2514/3.3166, for the fixed-interval
  backward smoothing recurrence.

Permanent verification covers analytic scalar and MIMO updates, independent observation
and model batching, lane-specific and all-missing channels, controlled `B/D` paths,
Float32 retention, strided observations, smoother terminal identity and covariance
contraction, filtered-state and likelihood reverse sensitivities, Finance/Stats adapters,
and fail-closed covariance, dtype and continuous-time contracts. Covariance failures
include a symmetric matrix with non-negative diagonal entries but a negative eigenvalue.

### E2c loop sensitivity and classical margins

`control::loop()` evaluates a state-space or pole-zero loop on an explicit angular-
frequency grid. For a square MIMO state-space response it computes
`S = (I + L)^-1` through the canonical batched solve and obtains `T = I - S`; it does
not form an inverse. Frequency remains the final axis in the returned response, `sens`,
and `comp` fields, while system batch axes and matrix axes remain intact. Pole-zero loops
take the equivalent scalar route `S = 1 / (1 + L)` and `T = L S`. Both paths preserve
complex precision, platform, leading batches, strided grids, and reverse history.

`control::margin()` accepts either a complex response shaped `[..., frequency]` or a SISO
state-space/pole-zero system. Gain crossovers are located where adjacent magnitudes
bracket one; phase crossovers are negative-real-axis crossings. The crossing location and
complex response are linearly interpolated between the two supplied frequencies. When
several crossings exist, gain margin is selected by minimum `abs(log(gain))` and phase
margin by minimum absolute degrees, matching the established nearest-to-instability
contract. Missing gain or phase crossovers return an infinite margin and a NaN frequency.

The stability field is the minimum sampled distance `abs(1 + L)` and reports that sample's
frequency. It is deliberately a grid diagnostic rather than an undocumented continuous
optimizer. Users control its resolution through `omega`; the API does not claim an exact
polynomial stability margin. Discrete-system margin grids are bounded by Nyquist, all
grids must be finite, non-negative and strictly increasing, and classical gain/phase
margins reject MIMO systems rather than reducing a matrix response to an arbitrary lane.

Every frequency lane is processed by Series operations. There is no host response loop,
host extraction, or device round trip in loop algebra or margin selection. The only host
reads are whole-input validation predicates before numerical work. State-space and
pole-zero overloads reuse `lti::freqresp()`; no second frequency-response convention was
introduced.

Sources inspected for this tranche:

- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrlanalysis/margin.m`, for SISO scope,
  margin definitions, angular-frequency units, multiple-crossover selection and array
  behavior;
- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrlanalysis/imargin.m`, for the
  frequency-data interpolation contract and the distinction between sampled and
  model-derived margins; and
- python-control upstream `control/margins.py` on `main`, inspected 2026-10-17, for
  negative-real-axis and unit-magnitude crossover definitions, gain selection by
  `abs(log(GM))`, phase selection by absolute margin, missing-crossing values, and the
  separation between classical and disk margins.

Permanent verification covers scalar and matrix loop identities, an analytic third-order
loop with gain margin `6` at `sqrt(2)` and phase margin approximately `53.410786` degrees,
multiple batch lanes, missing crossovers, Float32 retention, strided grids, reverse
sensitivity, MIMO-margin rejection, malformed frequency data, and discrete Nyquist
enforcement.

### E2d robust pole assignment and PID forms

`control::place()` assigns the poles of `A - B K` from either explicit `A/B` matrices or
an `lti::StateSpace` model. It validates numerical controllability before synthesis and
rejects a requested pole multiplicity greater than the numerical rank of `B`. Real
systems require complete conjugate pairs and return a real gain; complex systems may use
arbitrary complex poles. One pole vector is shared over leading plant batches so the SVD,
QR, pseudoinverse, matrix products, and residual evaluation remain batched Series work.

The general multi-input route constructs each achievable eigenvector subspace from the
left nullspace of `B`, then performs Kautsky-Nichols-Van Dooren-style one-column
determinant-improving sweeps. Full-row-rank input matrices take a direct eigenstructure
route. The result exposes the gain, closed-loop matrix, eigenvector basis, relative
eigenrelation residual, basis condition number, and completed sweep count. Assignment
fails rather than returning an apparently plausible controller when controllability,
subspace rank, conjugacy, conditioning, or the final residual is unacceptable.

`control::pid()` represents the parallel form
`Kp + Ki/s + Kd*s/(filter*s + 1)`. Gains are real finite Series parameters and may carry
broadcastable leading batches. Continuous controllers use that expression directly.
Discrete controllers independently select forward Euler, backward Euler, or trapezoidal
approximations for integration and filtered differentiation. `control::freqresp()`
evaluates all parameter batches over an explicit angular-frequency vector without a host
frequency loop. Unfiltered trapezoidal derivatives and forward-Euler derivative filters
outside their stability interval are rejected at construction.

Sources inspected for this tranche:

- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrldesign/place.m`, for rank,
  multiplicity, real-conjugacy, assignable-subspace, conditioning, and assignment-accuracy
  contracts;
- MATLAB R2025b visible sources
  `/Applications/MATLAB_R2025b.app/toolbox/control/ctrlmodels/@pid/pid.m` and
  `/Applications/MATLAB_R2025b.app/toolbox/shared/controllib/engine/@StaticModel/StaticModel.m`,
  for parallel PID form, discrete integration rules, derivative-filter construction, and
  unstable discretization exclusions;
- python-control upstream `control/statefbk.py` on `main`, inspected 2026-10-17, for the
  `A - B K` convention, SISO/MISO/MIMO scope, and the rank-of-`B` repeated-pole limit;
- J. Kautsky, N. K. Nichols, and P. Van Dooren, “Robust pole assignment in linear state
  feedback,” *International Journal of Control* 41(5), 1985,
  DOI 10.1080/0020718508961188, for achievable eigenvector subspaces and conditioning as
  the robustness objective; and
- A. L. Tits and Y. Yang, “Globally convergent algorithms for robust pole assignment by
  state feedback,” *IEEE Transactions on Automatic Control* 41(10), 1996,
  DOI 10.1109/9.539425, for determinant maximization and the distinction between
  one-column KNV updates and two-column complex-pair updates. The current engine uses the
  former with explicit conjugate coupling; it does not claim the full YT algorithm.

Permanent verification covers analytic SISO gain, full-actuation conjugate pairs,
non-conjugate complex plants, repeated-pole rank limits, rank-two MIMO eigenvalue
invariants, leading batches, Float32 retention, strided matrices, state-space forwarding,
all three discrete PID rules, parameter batches, PID reverse sensitivity, and fail-closed
uncontrollable, malformed-pole, unstable-filter, and invalid-policy cases.

### E3a dense convex QP and condensed linear MPC

`math::qp` is the canonical owner of the dense convex problem
`min 0.5*x^T*P*x + q^T*x` subject to `lower <= A*x <= upper`. Equal lower and upper
bounds represent an equality; infinite endpoints leave a side unbounded. `P` must be
real, finite, symmetric and positive semidefinite. The solver validates this once, forms
`P + sigma*I + rho*A^T*A`, and factors that positive-definite system once. Every ADMM
iteration then uses the cached factor, batched matrix products and a Series box projection.
It does not refactor per iteration or iterate over variables, constraints or batch lanes
on the host.

`qp::prepare(P, A, options)` separates invariant setup from changing linear terms and
bounds. The returned solver accepts repeated `q/lower/upper` calls and carries explicit
primal and dual residuals, iteration count, convergence state, primal solution, projected
constraint state and dual variable. `Result::warm()` supplies all three evolving states
to the next call. A one-shot `qp::solve(...)` remains available and delegates to exactly
the same prepared engine. Failure to meet the requested tolerances is reported as
`converged == false`; it is never relabeled as a solution.

`control::mpc::build()` compiles the invariant portion of a discrete LTI problem. It
constructs the stacked state prediction `X = Sx*x0 + Su*U`, block-diagonal stage,
terminal and input costs, the condensed Hessian, and the input/state constraint matrix,
then prepares the shared QP solver. A control step supplies only the current state,
reference and optional warm state. Shared vectors describe a constant reference or bound;
`[..., horizon, width]` inputs describe staged and batched scenarios. Returned controls
and predicted states restore those semantic horizon axes rather than exposing the flat QP
layout.

The first tranche deliberately uses full condensing and dense Series matrices. That is an
appropriate route for small and medium horizons and provides a differentiable, backend-
routable correctness owner. It does not claim sparse scaling, adaptive `rho`, Ruiz
scaling, polishing, infeasibility certificates, output constraints, soft constraints or
partial condensing. Those are explicit E3 follow-ons; they will extend the same problem
records rather than fork a second MPC solver.

Sources inspected for this tranche:

- OSQP upstream `docs/solver/index.rst` on `master`, inspected 2026-10-20, for the
  two-sided QP form, indirect positive-definite linear system, box projection, primal and
  dual residuals, and absolute/relative stopping tests;
- B. Stellato, G. Banjac, P. Goulart, A. Bemporad and S. Boyd, “OSQP: an operator
  splitting solver for quadratic programs,” *Mathematical Programming Computation* 12,
  2020, for quasi-definite/positive-definite factorization reuse, warm starts and embedded
  repeated-problem motivation;
- acados upstream `acados/ocp_qp/ocp_qp_xcond_solver.h`,
  `interfaces/acados_c/ocp_qp_interface.h`, and the cart-pendulum OCP examples on `main`,
  inspected 2026-10-20, for the separation between horizon formulation, full/partial
  condensing, solver memory, changing right-hand sides and warm-start state; and
- MATLAB R2025b visible source
  `/Applications/MATLAB_R2025b.app/toolbox/optim/optim/quadprog.m`, for the conventional
  quadratic objective, equality/inequality/bound meanings, infinite endpoints, warm-start
  result contract and explicit non-convergence statuses.

Permanent verification covers analytic unconstrained, box-constrained and equality QPs;
leading-axis batches; prepared reuse; warm starts; reverse sensitivity through an
inactive-bound solution; indefinite-Hessian and inconsistent-bound rejection; scalar
integrator MPC with input and state bounds; plan reuse; horizon restoration; and batched
initial states.

### E4a linear modified-nodal circuit core

`eng::circuit` now owns one validated modified-nodal-analysis formulation for linear
circuits. `circuit::make()` treats node zero as the common datum, validates every endpoint
and value group, and compiles the R, C, L, independent-current-source, and ideal-voltage-
source topology into incidence matrices once. A `Circuit` is therefore reusable immutable
topology plus differentiable Series-valued device parameters, not a parsed netlist and not
a second matrix class.

`circuit::op()` uses resistor conductance stamps, current-source injections, ideal voltage
constraints, and one auxiliary branch-current unknown per inductor. Capacitors are open at
DC and inductors are ideal shorts. `circuit::ac()` uses the same MNA blocks with `j*omega*C`
nodal admittance and `-j*omega*L` branch impedance. Angular frequencies form a dedicated
axis immediately before the returned node/branch axis. Parameter sweeps occupy leading
Series axes and are solved as one batched linear system; no host loop advances frequencies,
nodes, elements, or sweep lanes. Host iteration is limited to compiling irregular endpoint
metadata into the cached incidence matrices.

The returned voltage includes the explicit zero-valued datum node. Ideal-source and
inductor currents retain their own final axes, and every solve reports the maximum absolute
MNA defect. The implementation delegates dense batched solving to
`Series::linearalg.lu_solve`, preserves Float32/Float64 promotion into the corresponding
complex AC dtype, and retains reverse sensitivity through device values and sources.

This is intentionally the linear circuit foundation, not a complete SPICE claim. E4b
extends the same block assembler with controlled sources and ideal op-amp constraints;
the next E4 work is nonlinear operating points and transient DAE integration. Netlist syntax, compact
semiconductor models, sparse scaling, continuation, noise, and harmonic balance remain
later layers and must not fork the canonical topology owner.

Sources inspected for this tranche:

- C.-W. Ho, A. E. Ruehli and P. A. Brennan, “The Modified Nodal Approach to
  Network Analysis,” *IEEE Transactions on Circuits and Systems* 22(6), 1975,
  DOI 10.1109/TCS.1975.1084079, for datum-node voltages, auxiliary branch-current
  unknowns, the partitioned MNA system, and topology-driven sparse assembly;
- Xyce upstream `src/DeviceModelPKG/OpenModels/N_DEV_Resistor.C` on `master`,
  inspected 2026-10-20, for the canonical `+G,-G;-G,+G` two-terminal resistor
  Jacobian stamp and conductance `G = 1/R`;
- Xyce upstream `src/DeviceModelPKG/OpenModels/N_DEV_Inductor.C` on `master`,
  inspected 2026-10-20, for explicit inductor branch-current unknowns, opposite KCL
  endpoint signs, the branch voltage equation, DC-short behavior, and inductance in
  the dynamic/AC branch block; and
- ngspice upstream source tree on `master`, inspected 2026-10-20, for the separation
  of resistor, capacitor, inductor, current-source, and voltage-source device loaders
  and for keeping device stamping distinct from analysis orchestration.

Permanent verification covers a DC divider with signed source current, a current-driven
resistor network with no auxiliary branch unknowns, two-lane DC parameter sweeps, a
combined parameter-by-frequency AC batch, the analytic RC low-pass phasor response at
zero and corner frequency, MNA residuals, reverse sensitivity through a voltage source,
invalid endpoints, and negative-frequency rejection.

### E4b dependent sources and ideal op amps

The linear circuit owner now supports voltage-controlled current sources, voltage-
controlled voltage sources, and infinite-gain ideal op amps without introducing another
solver. The canonical MNA assembly was generalized from the symmetric branch form
`[Y B; B^T Z]` to `[Y P; Q^T Z]`. Independent voltage sources and inductors still use
the same incidence for `P` and `Q`; a VCVS uses its output incidence for KCL and
`output - gain * control` for its voltage constraint; an ideal op amp uses its
output/reference incidence for KCL and its positive/negative input incidence for the
virtual-short constraint. A VCCS needs no auxiliary unknown and contributes the
nonsymmetric nodal block `output * gain * control^T` directly.

`Controls` keeps endpoint metadata on the host while gains remain floating Series with a
final element axis and optional leading parameter axes. `Amps` carries only topology.
Compilation caches every output and control incidence matrix. DC and AC analyses then
assemble all element lanes through broadcasted Series operations and perform one batched
LU solve. AC inserts its frequency axis without copying topology, and controlled gains
remain on the reverse graph. `Result` separates independent-source, inductor, VCCS, VCVS,
and op-amp currents so no caller must infer branch ordering.

Sources inspected for this tranche:

- ngspice upstream `src/spicelib/devices/vccs/vccsload.c` on `master`, inspected
  2026-10-20, for the four signed transconductance Jacobian entries coupling output and
  control nodes;
- ngspice upstream `src/spicelib/devices/vcvs/vcvsload.c` on `master`, inspected
  2026-10-20, for the output branch-current column, output-voltage row, and signed gain
  terms on the controlling nodes;
- ngspice User's Manual, section 4.2, inspected 2026-10-20, for the public orientation
  `i = g v`, `v = e v`, four-terminal ordering, transconductance units, and voltage-gain
  contract; and
- C.-W. Ho, A. E. Ruehli and P. A. Brennan, “The Modified Nodal Approach to Network
  Analysis,” *IEEE Transactions on Circuits and Systems* 22(6), 1975,
  DOI 10.1109/TCS.1975.1084079, for nonsymmetric controlled-source stamps, auxiliary
  branch variables, and ideal operational-amplifier constraints.

Permanent verification reconstructs a signed VCCS operating point and output current; a
loaded VCVS voltage and branch current; a two-lane gain sweep; a 10x ideal non-inverting
amplifier; controlled-source AC reuse; reverse sensitivity of output voltage to VCVS
gain; and failures for mismatched four-terminal metadata and degenerate op-amp inputs.

### E5a power-flow foundation

`eng::power` now owns the common steady-state network representation used by subsequent
power-system work. `Buses`, `Gens`, and `Lines` separate fixed bus injections and shunts,
generator injections, and branch topology. Every electrical value is per-unit; `Base`
plus `pu()`, `si()`, `zbase()`, `ybase()`, and `ibase()` provide the explicit three-phase
SI boundary using total apparent-power base and line-to-line voltage base. Node numbering
is zero-based inside Pluss rather than inheriting an external file format.

`power::make()` validates one slack bus, generator and line endpoints, finite real values,
positive voltage/tap data, nonzero impedances, shared dtype/platform, compatible scenario
axes, and connectivity to the slack island. It then compiles connection matrices once.
The same topology produces complex `Ybus`, `Yfrom`, and `Yto` operators with series
impedance, total line charging, off-nominal complex taps, phase shifts, bus shunts, and
line status, plus the lossless `Bbus`, `Bfrom`, and phase-shift injections used by DC
flow. Multiple active generators at one bus are reduced into the specified injection by
one batched matrix operation.

`power::dc()` removes the slack angle with compiled selection maps and solves all scenario
lanes in one reduced linear system. `power::ac()` uses the conventional polar Newton
balance: angles vary at PV and PQ buses, magnitudes vary only at PQ buses, and slack/PV
set points remain fixed. Its Jacobian is assembled from the complex matrix derivatives
`dS/dVa` and `dS/dVm`; no finite differencing is used. PV-only networks omit the empty
reactive block instead of submitting zero-width matrices to BLAS. Scenario, bus, branch,
and generator arithmetic stays in Series operations; only Newton iteration and immutable
topology validation are sequential. Both solvers retain reverse history through compiled
parameters and solves.

This tranche deliberately stops at unconstrained balanced positive-sequence power flow.
Reactive-limit switching, distributed slack, multiple islands, continuation, state
estimation, contingencies, OPF, sparse factorization, and three-phase unbalanced models
remain E5 follow-ons. A failed Newton solve returns `converged=false` with its residual;
invalid topology fails during compilation.

Sources inspected for this tranche:

- MATPOWER upstream `lib/makeYbus.m` on `master`, inspected 2026-10-20, for branch
  admittance, line-charging, complex-tap, phase-shift, shunt, `Yf`, `Yt`, and `Ybus`
  conventions;
- MATPOWER upstream `lib/makeBdc.m` on `master`, inspected 2026-10-20, for `Bbus`,
  `Bf`, tap-adjusted susceptance, and phase-shift injections;
- MATPOWER upstream `lib/newtonpf.m` and `lib/dSbus_dV.m` on `master`, inspected
  2026-10-20, for PV/PQ state selection, polar mismatch ordering, infinity-norm stopping,
  and analytic complex Jacobian identities;
- R. D. Zimmerman, “AC Power Flows, Generalized OPF Costs and their Derivatives using
  Complex Matrix Notation,” MATPOWER Technical Note 2, revision 7, 2019, for the compact
  injection and derivative derivations; and
- R. D. Zimmerman, C. E. Murillo-Sanchez and R. J. Thomas, “MATPOWER: Steady-State
  Operations, Planning, and Analysis Tools for Power Systems Research and Education,”
  *IEEE Transactions on Power Systems* 26(1), 2011,
  DOI 10.1109/TPWRS.2010.2051168, for network-model and AC/DC power-flow architecture.

Permanent verification covers three-phase per-unit bases and round trips; exact line,
charging, shunt, and phase-shifting-transformer stamps; analytical two-bus DC, PQ, and PV
solutions; explicit generator aggregation; batched load scenarios; DC and AC reverse
sensitivities; residual and convergence reporting; and failures for missing slack,
zero impedance, disconnected islands, and invalid Newton policy.

### E5b contingency factors and N-1 screening

`power::ptdf()` forms the complete branch-by-bus power-transfer distribution matrix for
the slack bus already compiled into `Grid`. It removes the slack row and column through
the existing angle maps, solves the reduced `Bbus` system against every bus selector in
one multi-right-hand-side operation, and maps the resulting angle responses through
`Bfrom`. The operation therefore supports arbitrary leading scenario axes and retains
reverse history through line reactances and tap ratios without computing a matrix inverse
or looping over injection buses.

`power::lodf()` composes that PTDF with the compiled from-minus-to incidence matrix. Its
result uses the conventional layout `[..., monitored_line, outage_line]`, sets the
self-outage factor to `-1`, and returns a separate boolean validity Series. A line is not
a valid outage when it is already inactive or when `1 - H[k,k]` is within the requested
tolerance, which identifies a bridge under the DC model. Every invalid LODF column is
filled with `NaN`; no epsilon regularization is allowed to turn an islanding event into a
finite but fictitious redistribution.

`power::outages()` applies all valid factors simultaneously. It returns the base DC flow
and a `[..., outage_line, monitored_line]` Series, so a caller can compare limits along
the final line axis while preserving parameter and operating-scenario axes. Invalid
outage rows are `NaN`, matching the LODF availability contract. The kernel reuses one DC
solve and one PTDF solve, then evaluates the complete N-1 set with broadcasted Series
arithmetic; it never rebuilds one `Grid` per outage. These linearized factors retain the
assumptions of the DC model and are screening quantities, not replacements for post-
contingency AC power flow.

Sources inspected for this tranche:

- MATPOWER 8.1 `makePTDF` source and reference entry, inspected 2026-10-20, for the
  reduced-`Bbus` multi-right-hand-side construction, reference-bus convention, and
  branch-by-bus orientation;
- MATPOWER 8.1 `makeLODF` source and reference entry, inspected 2026-10-20, for
  `H = PTDF * Cft`, column normalization by `1 - diag(H)`, the `-1` self-outage
  diagonal, and `NaN` masking of bridge columns;
- MATPOWER 8.1 `t_makePTDF` and `t_makeLODF`, inspected 2026-10-20, for transfer,
  slack, direct-recomputation, and bridge test structure; and
- MATPOWER User's Manual 8.1, section 4.5, inspected 2026-10-20, for the linear shift-
  factor equations and their DC-model interpretation.

Permanent verification covers an analytical three-bus mesh PTDF and LODF, all three
direct branch-removal solutions, exact N-1 post-outage flows, leading scenario batches,
radial bridge masking, already-inactive lines, axis layout, tolerance rejection, and
reverse sensitivity of a post-outage flow to bus injections.

### E5c economic and preventive secure dispatch

`power::dispatch()` now owns the convex DC dispatch contract. Generator decisions use
one final generator axis; fixed bus injections remain in `Grid`; and quadratic and
linear costs, generator bounds, and branch ratings may either be shared across all
scenarios or carry the Grid's exact leading scenario axes. The objective is
`sum(quadratic * generation^2 + linear * generation)`. Inactive generators are fixed to
zero regardless of their supplied bounds, one-sided infinite generator bounds are
supported, `NaN` bounds are rejected, and an entirely inactive fleet fails before
optimization.

The formulation reuses the compiled generator-to-bus map, the private DC solve, `ptdf()`,
and the shared `math::qp` owner. It imposes one system balance equality, generator bounds,
and both signs of every base branch rating in a single `lower <= A generation <= upper`
system. `DispatchOptions::secure` augments that same system with every valid preventive
N-1 line-outage constraint. The post-contingency matrix is assembled as
`base_flow + LODF * outaged_flow` directly against generation, so scenarios, outages,
monitored lines, and generators are evaluated by broadcasted Series operations rather
than by rebuilding or solving one network per outage. Bridge and inactive-line outages
remain explicitly invalid: they contribute unbounded rows to the QP and `NaN` rows to
`DispatchResult::post_flow`, never fictitious finite redistributions.

`DispatchResult` returns generator output, bus angles and injections, base flows, optional
post-outage flows and validity, the exact polynomial objective, and the shared QP's
convergence diagnostics. The full construction remains on the Series reverse graph;
the permanent test checks the analytical marginal cost of demand in addition to primal
values. This is preventive DC security-constrained economic dispatch: the same generation
is used before and after each valid line outage. It does not yet model piecewise-linear
costs, binary commitment, ramping, reserves, generator outages, corrective redispatch,
losses, reactive limits, voltage constraints, or AC security.

Sources inspected for this tranche:

- MATPOWER 7.1 `opf_setup.m`, inspected 2026-10-21, for the separation of dispatchable
  injections, generator bounds and costs, linear network constraints, and DC model
  assembly;
- MATPOWER 7.0 `dcopf_solver.m`, inspected 2026-10-21, for the quadratic/linear objective,
  two-sided linear constraints, variable bounds, convergence reporting, and LP fallback
  when the quadratic block vanishes;
- MATPOWER 8.1 Developer Manual, section 8, inspected 2026-10-21, for the canonical DC
  OPF reduction to a QP with `l <= A x <= u` and explicit variable bounds;
- MATPOWER 8.1 User's Manual, DC optimal-power-flow sections, inspected 2026-10-21, for
  DC nodal balance, active-power generation bounds, branch-flow ratings, and polynomial
  generation costs; and
- V. H. Hinojosa and F. Gonzalez-Longatt, “Preventive Security-Constrained DCOPF
  Formulation Using Power Transmission Distribution Factors and Line Outage Distribution
  Factors,” *Energies* 11(6), 1497, 2018, DOI 10.3390/en11061497, for simultaneous base
  and post-contingency limits expressed with PTDF/LODF factors and generation decisions.

Permanent verification covers an exact two-generator quadratic optimum, a binding branch
rating, inactive generation, two batched demand scenarios, an analytical three-bus
preventive N-1 limit, all-outage result axes, bridge exclusion, objective reverse
sensitivity, infinite upper bounds, and failures for concave costs, `NaN` bounds, invalid
ratings, and an inactive fleet.

### E5d AC weighted state estimation

`power::estimate()` now owns static balanced-network AC weighted least-squares state
estimation. `Measurements` carries an explicitly ordered table of measurement kinds,
network-element indices, observed values, and positive standard deviations. The table may
mix active and reactive power at either line end, active and reactive bus injection,
voltage angle, and voltage magnitude in any order. Bus-power measurements mean net bus
injection, not generator terminal output. Values and standard deviations may be shared or
carry the Grid's exact leading scenario axes; the final axis is always the measurement
axis.

The state is ordered as every non-reference bus angle followed by every bus magnitude, so
its dimension is `2 * buses - 1`. The reference angle remains fixed. Each iteration forms
one canonical prediction containing all eight measurement families and one fully analytic
Jacobian, then gathers the user's requested rows. This centralizes the AC equations and
avoids one host dispatch per measurement. Bus derivatives use the compact complex
`dSbus/dV` identities; line-end derivatives use the corresponding incidence/admittance
forms. Scenario batches, buses, lines, states, and measurements remain Series axes.

The nonlinear solve uses weighted Gauss-Newton with
`G = (H / sigma)^T (H / sigma)`. It tests numerical observability from the gain spectrum,
solves the positive-definite system by Cholesky factorization rather than materializing an
inverse, wraps angle innovations through `atan2(sin(r), cos(r))`, and freezes converged
scenario lanes. A vectorized fraction-to-boundary limiter scales the coupled state step
before any voltage magnitude can cross zero. The final covariance is obtained by solving
`G covariance = I`; normalized residuals use the diagonal of
`R - H covariance H^T`. The returned objective is the exact weighted residual sum of
squares. Predictions, residuals, covariance, and the solved state remain on the Series
reverse graph.

The present estimator assumes independent Gaussian measurement errors represented by a
diagonal standard-deviation vector. It does not yet accept a dense correlated covariance,
hard zero-injection equality constraints, current-magnitude or synchronized phasor
measurements, topology-error hypotheses, or sparse gain factorization. It exposes the
weighted objective and normalized residual evidence but deliberately does not delete a
measurement or choose a chi-square/bad-data threshold on the user's behalf. The network
model remains the Grid's balanced positive-sequence AC model.

Sources inspected for this tranche:

- MATPOWER 6.0 `extras/se/doSE.m`, inspected 2026-10-21, for canonical measurement-family
  assembly, non-reference-angle state selection, analytic AC Jacobians, weighted gain and
  residual equations, and observability failure;
- MATPOWER 4.1 `extras/state_estimator/state_est.m` and `runse.m`, inspected 2026-10-21,
  for residual-covariance normalization and the separation between estimation and
  bad-data policy (the source labels this older estimator as under construction, so its
  case-specific behavior was not copied);
- pandapower `estimation/algorithm/base.py` on the `develop` branch, inspected
  2026-10-21, for the current `2n-k` minimum-observation contract, inverse-standard-
  deviation weighting, WLS gain construction, convergence reporting, and retained
  residual/Jacobian diagnostics;
- pandapower state-estimation documentation, inspected 2026-10-21, for measurement-type,
  uncertainty, redundancy, and bad-data workflow conventions; and
- A. Abur and A. G. Exposito, *Power System State Estimation: Theory and
  Implementation*, CRC Press, 2004, and A. Monticelli, *State Estimation in Electric
  Power Systems*, Springer, 1999, as the canonical texts identified by the implementation
  documentation. No accessible electric-power state-estimation implementation was found
  in the local MATLAB R2025b tree.

Permanent verification covers exact and deliberately rough starts, every measurement
family, arbitrary row ordering, angle wrap-around, positive-definite state covariance,
gross-error residual evidence, two leading scenarios, reverse sensitivity to observations,
and failures for insufficient or unobservable measurements, non-positive uncertainty, and
invalid element indices.

### E5e continuation power flow

`power::trace()` now follows the balanced AC solution branch from a compiled Grid toward a
`LoadPath`. The path supplies target net active and reactive bus injections at loading
factor one; intermediate schedules are
`base + factor * (target - base)`, so factors beyond one continue the same transfer rather
than changing its composition. `TraceStop::Target` corrects an explicitly requested
non-zero factor. `TraceStop::Nose` follows the branch through a singular ordinary power-
flow Jacobian and stops at the first loading fold.

The private `detail::AcKernel` is now the single owner of polar state packing, fixed slack
angle and PV/slack magnitude reconstruction, complex bus injection, the analytic
`dSbus/dV` derivatives, reduced mismatch/Jacobian assembly, and branch-end powers.
`power::ac()`, `power::estimate()`, and `power::trace()` all delegate to that kernel rather
than maintaining three versions of the AC equations. The kernel preserves Grid dtype and
platform; an explicit post-solve cast also prevents the shared dense LU implementation's
internal precision promotion from leaking into a float32 state.

The continuation state contains every non-reference angle, every PQ magnitude, and the
loading factor. The tangent solves the bordered system formed from the analytic AC
Jacobian, the exact transfer derivative, and the preceding normalized tangent. Each Euler
prediction is corrected by Newton iteration under a pseudo-arclength orthogonality
constraint. A fraction-to-boundary rule scales the complete coupled correction before a PQ
magnitude can cross zero. Corrector failure halves the step and retries; optional adaptive
control then uses the infinity norm between predicted and corrected states with the
MATPOWER damping formula and configured minimum/maximum bounds.

Target crossings are polished by a fixed-factor AC correction, so the final factor is
exact rather than the first step past the event. A tangent-factor sign reversal brackets a
fold; 44 vectorized bisection/corrector stages localize it while remaining nonsingular in
the bordered formulation. If a requested target lies beyond the first fold, that scenario
stops at the localized nose with `reached=false` instead of returning a fictitious target
solution. Batched scenarios advance through the same sequential continuation loop but all
network algebra and event refinement are Series-batched. Lanes that finish early are
frozen and marked false on later entries of `TraceResult::valid` while peer lanes continue.
All corrected voltages, injections, flows, factors, tangents, residuals, and validity masks
are retained along the point axis, and executed Series operations preserve reverse history.

The present contract keeps Grid bus classes fixed. It does not yet switch PV buses after
reactive limits, redistribute active generation at generator limits, terminate on voltage
or branch-flow limits, trace the complete lower branch back to zero loading, or provide
user-defined event callbacks. It uses dense bordered factorizations and the Grid's balanced
positive-sequence model. These are explicit future depth items rather than behavior hidden
behind generic options.

Sources inspected for this tranche:

- MATPOWER 7.1 `runcpf.m`, `cpf_predictor.m`, `cpf_corrector.m`, `cpf_tangent.m`,
  `cpf_p.m`, and `cpf_p_jac.m`, inspected 2026-10-21, for base/target transfer semantics,
  normalized tangent prediction, bordered correction, pseudo-arclength parameterization,
  event termination, and retry behavior;
- MATPOWER 7.1 `t_cpf.m`, inspected 2026-10-21, for target-factor, nose, full-curve,
  adaptive-step, limit-event, and unsuccessful-corrector verification structure;
- MATPOWER 8.1 Reference Manual and MP-Opt-Model continuation options, inspected
  2026-10-21, for step bounds and the damped predictor-error adaptation rule; and
- V. Ajjarapu and C. Christy, “The Continuation Power Flow: A Tool for Steady State
  Voltage Stability Analysis,” *IEEE Transactions on Power Systems* 7(1), 1992,
  DOI 10.1109/59.141737, for predictor-corrector continuation through the maximum-loading
  point. No electric-power CPF source was found in the local MATLAB R2025b toolbox tree.

Permanent verification covers an exact target against ordinary AC flow, a closed-form
two-bus saddle-node factor and voltage, a target beyond that fold, unequal batched noses,
batched target factors, PV-only systems with no magnitude state, float32 retention,
predictor/corrector axes and masks, reverse sensitivity to the target injection, and
failures for zero transfer, incompatible path axes, invalid damping, and inconsistent step
bounds.

### E5f constrained AC optimal power flow

`power::opf()` now owns the balanced positive-sequence AC optimal-power-flow contract.
The decision vector contains every non-reference voltage angle, every bus voltage
magnitude, and active and reactive output for every generator. The reference angle stays
fixed at the value compiled into `Grid`. Active/reactive generator starts, fixed active
injections, and fixed reactive injections are now retained separately by `Grid`; this
prevents optimization from reverse-engineering demand from a previously combined schedule.
Inactive generators are fixed to zero independently of user-supplied limits.

The equality system enforces both real and reactive nodal balance at every bus. The
private `detail::AcKernel` remains the single owner of voltage construction, bus
injection, branch-end power, and their polar derivatives. State estimation and OPF now
share the same branch derivative path as well as the same bus derivative path. Generator
incidence contributes exact constant Jacobian blocks, while the voltage blocks use the
canonical complex `dS/dVa` and `dS/dVm` identities. No finite-difference derivative is
used by the solver.

When `Limits::flow` is defined, each active branch is constrained at both terminals by
the squared apparent-power residuals `|S_from|^2 - limit^2 <= 0` and
`|S_to|^2 - limit^2 <= 0`. Leaving that field undefined removes branch-rating rows
entirely; an infinite placeholder row is not sent to the optimizer. Voltage magnitudes,
active generation, and reactive generation use explicit variable bounds. Positive finite
branch ratings and positive voltage lower bounds are required, lower bounds may not exceed
upper bounds, and inactive generation remains exactly zero.

`GenCost` retains its established active-power lower/upper fields and now accepts optional
constant, linear, and quadratic reactive-power costs in addition to the active polynomial.
Undefined optional terms mean zero. The scalar nonlinear-program objective is the sum over
all scenarios and generators, while `OpfResult::objective` preserves one value per
scenario. The result also returns active/reactive generation, complex voltages and
injections, both branch-end powers, active/reactive balance multipliers, separate
from/to rating multipliers, convergence, iteration count, maximum constraint violation,
first-order optimality, and the shared solver's stable public message.

Leading scenario axes are solved as one nonlinear program. Network equations, costs,
residuals, and per-scenario Jacobian blocks are evaluated with batched Series operations;
the independent blocks are embedded into one analytical block-diagonal Jacobian rather
than dispatched through a host loop. The current shared optimizer is dense, so this route
favors modest and medium scenario batches. A future sparse/block KKT owner can consume the
same callback contract without changing `power::opf()`. Solver-internal precision may be
higher, but returned primal values preserve the Grid's `Float32` or `Float64` dtype and
platform.

The canonical nonlinear owner is `pluss::subsp::math::optim`, configured through the
small power-specific `OpfOptions` boundary. `power.h` deliberately does not expose the
large optimizer option graph: `power.cpp` maps maximum iterations and feasibility/
optimality tolerances to the established scaled sequential-QP engine. This keeps compile
dependencies local while preserving one implementation of line search, restoration,
scaling, multiplier estimation, and convergence diagnostics.

The returned objective retains direct reverse sensitivity to polynomial cost
coefficients, matching the envelope derivative at a converged optimum. E5f does not yet
differentiate the optimizer solution implicitly with respect to loads, admittances, or
limits. It also does not yet implement piecewise-linear or higher-order costs, coupled PQ
capability curves, branch angle-difference limits, dispatchable-load power-factor rows,
generator ramp/reserve coupling, unit commitment, multi-period OPF, preventive AC N-1
security, or unbalanced multiphase OPF. Those contracts are deferred rather than silently
approximated.

Sources inspected for this tranche:

- MATPOWER 8.1 User's Manual, section 6.1, inspected 2026-10-22, for the standard
  `[Va, Vm, Pg, Qg]` AC OPF variable layout, full real/reactive balance equations,
  two-ended branch-flow inequalities, and variable bounds;
- MATPOWER `master` source `lib/+mp/mme_gen_opf_ac.m`, inspected 2026-10-22, for
  separate active/reactive polynomial costs and the distinction between box bounds and
  optional coupled generator capability constraints;
- MATPOWER 8.1 `mp.form_ac` reference/source contract, especially
  `port_apparent_power_lim_fcn`, inspected 2026-10-22, for squared apparent-power
  residuals and their first/second derivative conventions;
- R. D. Zimmerman, “AC Power Flows, Generalized OPF Costs and their Derivatives using
  Complex Matrix Notation,” MATPOWER Technical Note 2, 2010,
  DOI 10.5281/zenodo.3237866, for the complex polar injection and flow derivatives;
- MATPOWER MIPS 1.5.2 source/manual, inspected 2026-10-22, for the canonical nonlinear
  program separation into objective, equality, inequality, linear, and variable-bound
  callbacks and for public multiplier/convergence reporting; and
- the local PlussMa `math::optim` implementation and derivative audits, inspected
  2026-10-22, as the canonical solver owner reused by this tranche. No electric-power
  OPF implementation was found in the local MATLAB R2025b toolbox tree.

Permanent verification covers an analytical lossless active dispatch, reactive balance,
voltage limits, exact complex nodal balance, multiplier axes, a binding two-ended MVA
rating, two jointly solved load scenarios, inactive generation, `Float32` retention,
reverse sensitivity to quadratic cost coefficients, and failures for contradictory
bounds, non-positive ratings, concave quadratic costs, inactive fleets, and invalid solver
tolerances.

### E6a matrix-free linear truss statics

`structure::truss()` now compiles a validated two- or three-dimensional pin-jointed truss
from node positions, endpoint connectivity, cross-sectional area, and Young's modulus.
Connectivity remains immutable host metadata; positions and properties remain Series with
leading parameter/scenario axes. Geometry, material, and later load axes broadcast into a
single solve layout without iterating over elements or cases.

`structure::solve()` applies the global stiffness operator without constructing a dense
global matrix. It gathers displacement at every endpoint, evaluates all axial extensions
and `EA/L` forces in one Series batch, then scatter-adds equal and opposite forces to the
nodes. Prescribed displacement uses an explicit lifting decomposition. The reduced
operator is solved by the canonical `math::krylov::pcg` owner with a Series-assembled
Jacobi diagonal; constrained lanes use identity rows. The result returns nodal
displacement, constrained reactions, bar strain/stress/force, strain energy, residual and
convergence/breakdown diagnostics.

All numerical paths preserve dtype, platform, broadcasting, and reverse history. The
permanent suite verifies exact axial and imposed-motion solutions, batched loads and
sections, an oblique 3-D member, Float32, analytical load/area/geometry sensitivities,
and explicit mechanism and validation failures. The complete design, equations,
references, and deferred scope are recorded in `Eng/structure/IMPLEMENTATION.md`.

Sources inspected for this tranche:

- OpenSees `Truss.cpp` and `CorotTruss.cpp`, inspected 2026-10-22, for linear bar
  stiffness, force, and dimensional conventions;
- CALFEM for Python `core.py` (`bar2e`, `bar2s`) and `exs_bar2.py`, inspected
  2026-10-22, for an independent element/assembly/reaction workflow;
- DOLFINx elasticity and matrix assembly documentation, inspected 2026-10-22, for
  degree maps, Dirichlet lifting, null modes, and stress post-processing;
- PETSc 3.26 KSP documentation, inspected 2026-10-22, for matrix-free CG,
  preconditioning, and convergence policy; and
- local MATLAB R2025b `truss.m`, `assembleFEMatrices.m`, and `structuralBC.m`, inspected
  2026-10-22, as visible-source secondary contracts.

### E6b matrix-free planar frames and consistent mass

`structure::frame()` now compiles a two-dimensional rigid-jointed Euler-Bernoulli model
with `(ux, uy, rz)` nodal degrees of freedom. The frame stiffness operator gathers all
member endpoints, transforms them to local axes, evaluates the six local end actions,
rotates force resultants back to global axes, and scatter-adds both endpoints. It does
not assemble a dense global stiffness matrix. The result exposes local member-end force
lanes, reactions, energy, and the same convergence diagnostics as truss statics.

The truss-only support and PCG block was promoted to one source-private constrained
system engine. Both element families now share support validation, nonzero Dirichlet
lifting, free/fixed masking, matrix-free Jacobi PCG, mechanism reporting, and reaction
recovery. The canonical factories are `truss()` and `frame()`; an overloaded aggregate
`make()` was rejected because braced property records were ambiguous.

`structure::mass()` applies the complete consistent Euler-Bernoulli mass matrix through
the same transform/scatter architecture. `Sections::density` is volumetric and combines
with area to produce mass per length. The operator preserves batch axes, dtype, platform,
and reverse history and is ready to serve modal and transient iterations without exposing
or constructing a dense mass matrix.

The permanent verifier covers analytical horizontal and inclined cantilevers, two-member
assembly, batched properties and load cases, exact mass totals, Float32, reverse
sensitivities, and stable failure contracts. Full equations, force ordering, source
references, and deliberate limits are in `Eng/structure/IMPLEMENTATION.md`.

Sources inspected for this tranche:

- OpenSees `ElasticBeam2d.cpp` and Elastic Beam Column documentation, inspected
  2026-10-22, for stiffness/basic-force and consistent-mass contracts;
- CALFEM `core.py` functions `beam2e`, `beam2s`, and `beam2de`, inspected 2026-10-22,
  for independent transformation, end-force, and mass formulas;
- PETSc 3.26 KSP documentation, rechecked for shared matrix-free solve ownership; and
- visible local MATLAB R2025b PDE structural sources, inspected as a secondary API and
  result-organization reference.

### E6c matrix-free modal analysis and linearized buckling

The generalized Hermitian operator solver is now owned canonically by
`Series::LinearAlgAccessor::gen_eigh()`. It consumes a batched row-oriented trial block
and three callbacks: apply `A`, apply `B`, and solve with `A`. Zero-shift block inverse
iteration, `A`-orthonormalization, a small Rayleigh--Ritz extraction, mixed residual
convergence, ascending positive eigenvalues, real and complex Hermitian dtypes, batch
axes, and reverse history are implemented once in PlussQit rather than repeated in
structural mechanics. The permanent Series verifier independently covers diagonal,
dense, partial, batched, complex, Float32, differentiable, nonconverged, and invalid
operator paths; the complete public contract is documented in the Series linear-algebra
reference.

`structure::modes()` supplies frame stiffness, consistent mass, and matrix-free PCG
inverse actions to that engine. Supports are represented by identity stiffness lanes and
zero mass lanes, so constrained degrees cannot enter a physical mode. Returned angular
frequencies are square roots of the generalized eigenvalues; hertz are `omega/(2*pi)`;
mode shapes are stored as `[..., mode, node, 3]` and normalized to unit generalized mass.

`structure::buckling()` first obtains the reference elastic state through the shared
static solver. It derives each member's signed compressive force from local end actions,
applies the standard Euler--Bernoulli beam-column geometric stiffness matrix in the same
gather/scatter owner as elastic stiffness, then solves `K phi = factor K_g phi`. Factors
are positive multipliers on the supplied reference load and shapes are max-absolute
normalized. Scaling the complete reference load by `c > 0` therefore divides every
factor by `c`.

Both analyses preserve dtype, platform, leading parameter/load batches, and reverse
history. No dense global stiffness, mass, or geometric matrix is assembled, and there is
no host loop over cases, modes, nodes, members, or degrees of freedom. The only
sequential loops are the generalized-eigen outer iteration and the canonical PCG solve.
The deterministic sine trial block avoids random-state ownership and gives reproducible
ordering.

Permanent verification separates the generic solver from frame assembly, then covers all
three one-member cantilever modes against an exact reduced generalized eigensystem, unit
mass normalization, batched density, the discrete Euler-column factor, inverse load
scaling, Float32 retention, density and load sensitivities, absent density, excessive
mode count, nonhomogeneous supports, and tension-only reference states.

Sources inspected for this tranche:

- SLEPc 3.26.0 EPS and ST manuals, inspected 2026-10-23, for generalized operator
  problems, matrix-free actions, block subspaces, Rayleigh--Ritz extraction, residual
  convergence, and inverse spectral transformation;
- CALFEM MATLAB `fem/beam2gxe.m` on the `master` revision visible 2026-10-23, for an
  independent beam-column geometric stiffness convention and compression/tension sign
  handling;
- OpenSees `ElasticBeam2d.cpp` on the `master` revision visible 2026-10-23, rechecked for
  planar frame coordinate, stiffness, mass, and element-force ownership;
- local MATLAB R2025b `toolbox/pde/pde/pdeeig.m`, inspected 2026-10-23, for constrained
  generalized eigenproblems, explicit symmetry restoration, ordered eigenpairs, and
  reconstruction from the constrained basis; and
- the existing PlussMa matrix-free frame, consistent-mass, constrained-layout, and PCG
  machinery, audited before implementation and reused without a second assembly or
  solver path.

This tranche is linear modal analysis and linearized elastic buckling about a reference
state. It does not claim damped/gyroscopic modes, nonlinear post-buckling, follower-load
terms, material instability, mode tracking across parameter crossings, interval spectral
search, or a production sparse shift-invert factorization. Those require explicit later
contracts rather than being approximated behind these calls.

## Public Layout

The canonical implementation namespace remains `pluss::subsp::eng`, with `pluss::eng` as
the opt-in public namespace alias already established by PlussMa.

```text
Eng/
├── common/       shared engineering records, layouts, topology and result policy
├── lti/          linear time-invariant system representations and analysis
├── signal/       filtering, spectra, multirate transforms and denoising
├── control/      feedback, estimation, synthesis and optimal control
│   └── mpc/
├── circuit/      modified-nodal circuit assembly and simulation
├── power/        electric-power networks, flow, estimation and optimization
├── structure/    structural mechanics and finite-element assembly
├── thermal/      lumped thermal systems, exchangers and building loads
├── fluid/        engineered pipe, pump, valve and storage networks
├── robot/        rigid-body kinematics, dynamics and trajectory control
└── comms/        digital communications and RF network analysis
```

These modules are independent conveniences, not a mandatory vertical stack. A user can
enable `signal` without `power`, or `structure` without `comms`. CMake options, headers,
tests, examples, and documentation remain separately selectable.

The public API is functional first. Validated records such as `StateSpace`, `Circuit`,
`Grid`, `Frame`, or `Robot` may offer methods for convenient use, but those methods must
delegate to the same free-function engines. There will not be separate object and
functional algorithms.

## Canonical Ownership

Every implementation tranche begins with a repository-wide search. Existing machinery is
adapted or promoted before a new helper is introduced.

| Requirement | Canonical owner |
| --- | --- |
| Storage, broadcasting, dtype/platform dispatch, views and reverse sensitivity | `pluss::Series` |
| Dense solve, decomposition and matrix algebra | `Series::linearalg` |
| FFT, real FFT and inverse transforms | `Series` transform surface; establish this before Engineering DSP |
| STFT and inverse STFT | Existing `Series::stft()` and `Series::istft()` |
| ODE evolution and event handling | `pluss::subsp::math::ode` |
| Quadrature, interpolation, roots and nonlinear optimization | Existing PlussMa Math engines |
| Matrix-free iterative solves | Existing `pluss::subsp::math::krylov` CG/PCG/GMRES machinery |
| Generic covariance validation | Existing centralized PlussMa covariance machinery |
| Generic state estimation | Promote the reusable Kalman machinery now trapped in Math Finance; Finance and Engineering then delegate to one owner |
| Dense convex quadratic programming | `pluss::subsp::math::qp`; Engineering MPC only condenses and interprets control problems |
| Physical dimensions, units and quantities | Extract the reusable part of `natsci::common` to a neutral shared foundation; Engineering must not clone it |
| Constitutive laws and continuum physics | NatSci; Engineering supplies geometry, topology, boundary/use policy, loads and design workflows |
| Irregular topology metadata | Small host-side records are permitted; numerical values and assembled operations remain `Series`-native |
| Heterogeneous Pluss values | `pluss::Queue` only at genuine dynamic boundaries, never inside numerical kernels |

Two promotions are prerequisites, not optional cleanup:

1. A canonical `Series` FFT family must own `fft`, `ifft`, `rfft`, and `irfft`, including
   axes, lengths, normalization, complex dtype, batching, strides, platform dispatch, and
   reverse rules. Engineering must not repeat the host-vector FFT currently embedded in a
   finance pricing implementation.
2. Dimension, unit, quantity, and measurement contracts useful outside NatSci must move to
   a neutral shared namespace. NatSci and Engineering can temporarily forward to that one
   owner during migration; neither subspace should depend semantically on the other.

## Numerical Contract

Engineering payloads use `Series` throughout numerical paths.

- Trailing axes carry semantic dimensions such as state, port, node, degree of freedom,
  frequency, sample, or horizon.
- Leading axes are batch, scenario, parameter-sweep, load-case, or Monte Carlo axes.
- One implementation handles scalar, 1-D, 2-D, 3-D, 4-D, and higher supported batches
  through broadcasting and batched linear algebra.
- Numerical kernels do not iterate through `item()`, `host()`, raw storage, or
  `std::vector` lanes.
- Host loops are restricted to inherently irregular metadata construction: parsing a
  netlist, traversing a graph, enumerating elements, or advancing solver stages. Their
  arithmetic is submitted in batches through gather/scatter, matrix operations, or a
  matrix-free operator.
- Non-contiguous inputs remain valid unless a documented solver boundary requires a
  contiguous workspace. Such a boundary must restore the declared layout afterward.
- Dtype and platform are preserved unless the algorithm explicitly requires promotion.
- Differentiable compositions retain their graph. Iterative and implicit solvers receive
  explicit derivative tests rather than relying on accidental tape retention.

Dense materialization is not the default for large networks. Small and medium systems may
use batched dense Series kernels. Larger systems use a shared matrix-free operator over
the existing Krylov engines until a canonical sparse Series representation is justified.
No domain will invent its own sparse matrix type.

### Cache policy

Reusable immutable work is cached behind bounded, thread-safe caches. Suitable entries
include FFT plans, filter coefficients, state-space factorizations, topology layouts,
element maps, symbolic circuit stamps, power-network admittance structure, MPC condensing
layouts, and repeated linear-system factorizations.

Cache keys include all result-affecting properties: shape, axes, dtype, platform,
normalization, topology revision, model parameters where compiled into the artifact, and
algorithm version. Mutable borrowed buffers and graph-bearing result values are not cached
across calls. Every cached route retains an uncached correctness oracle and tests bounded
growth, invalidation, and concurrent access.

## Implementation Phases

### Phase E0: shared numerical foundation

1. Rename the empty `signal_proc` surface to `signal`.
2. Add independent CMake options and forward headers for every planned family.
3. Extract neutral dimension/unit/quantity contracts without breaking NatSci callers.
4. Add the canonical batched `Series` FFT family and replace private duplicate FFT code.
5. Audit `Series::linearalg` for Schur, generalized Schur/QZ, Sylvester, Lyapunov, CARE,
   and DARE support. Canonical matrix kernels belong there; Engineering adds system
   validation and interpretation.
6. Promote generic Kalman filtering out of Finance into a shared estimator engine.
7. Establish common axis, tolerance, convergence, diagnostic, and result records.

Verification covers complex and real transforms, normalization modes, arbitrary axes,
requested lengths, round trips, non-contiguous views, 2-D through 4-D batches, CPU/MPS
parity where a backend exists, gradients, ill-conditioned systems, and fail-closed
diagnostics.

### Phase E1: signals and LTI systems

`eng::signal` receives windows, convolution/correlation policy, FIR and SOS filtering,
zero-phase filtering, frequency response, filter design, detrending, Hilbert/envelope,
polyphase resampling, Welch spectra, cross spectra, coherence, spectrograms, and initial
denoising. Existing `Series::stft()` and `istft()` remain the transform owners.

`eng::lti` receives continuous/discrete state-space, transfer-function, zero-pole-gain,
and frequency-response records; validation; conversions; composition; continuous/discrete
simulation; poles/zeros; controllability/observability; Gramians; frequency and transient
responses; and exact/standard hold discretization.

The two families are implemented together because digital filters are LTI systems and
must not acquire a second transfer-function convention.

### Phase E2: control and estimation

Implement pole placement, PID forms, LQR/LQE, continuous and discrete Riccati equations,
Kalman filtering and smoothing, loop/sensitivity functions, gain and phase margins, and
basic robust diagnostics. State estimators call the promoted shared engine. Riccati and
Lyapunov work delegates to canonical linear-algebra kernels rather than explicit matrix
inversion.

Continuous and discrete conventions, sample times, feed-through behavior, detectability,
stabilizability, singular weighting, and covariance policy are validated explicitly.

### Phase E3: model-predictive and optimal control

Begin with linear MPC: horizon assembly, stage/terminal costs, input/state/output bounds,
soft constraints, reference tracking, warm starts, receding-horizon state, and batch
scenario evaluation. Repeated QPs reuse symbolic structure and factorizations.

Nonlinear MPC follows by composing existing ODE, autodiff, SQP, and constraint machinery.
Multiple shooting and collocation become shared optimal-control transcriptions, not MPC-
private implementations. Moving-horizon estimation reuses the same staged problem model.

### Phase E4: circuits and RF networks

Implement validated node/branch/device records and one modified-nodal assembly engine.
The first device set is R, L, C, independent/dependent sources, ideal operational
amplifier constraints, diode, and a deliberately limited transistor model. Analyses cover
operating point, DC sweep, small-signal AC, and transient response before noise, harmonic
balance, and advanced compact models.

Symbolic topology is assembled once; values are updated in batches for parameter sweeps.
Newton systems reuse factorization structure. Time integration delegates to PlussMa ODE/
DAE machinery where its contract fits rather than embedding a circuit-only integrator.

RF network work uses the same frequency-axis and complex-matrix policies for S, Z, Y,
ABCD, and transfer parameters; connection, cascade, de-embedding, renormalization,
stability, gain, and Touchstone parsing are built over those records.

### Phase E5: power and energy systems

Add per-unit conversion, bus/branch/generator records, admittance assembly, AC and DC power
flow, continuation power flow, state estimation, contingency evaluation, and AC/DC optimal
power flow. Power-flow equations and their Jacobians are batchable over contingencies and
load scenarios; OPF delegates to shared nonlinear optimization.

Thermal engineering begins with lumped resistance/capacitance networks, steady and
transient conduction, heat exchangers, moist-air state calculations, and HVAC/plant-loop
components. Thermophysical property calls use a centralized property provider with
explicit reference states and provenance.

### Phase E6: structures and finite elements

Establish mesh, node, degree-of-freedom, material, section, element, constraint, load-case,
and assembly contracts. Start with truss, beam, and frame elements; static response;
reaction recovery; modal analysis; buckling; and Newmark/generalized-alpha transient
response. Geometric and material nonlinearity follow only after the linear verification
suite is exhaustive.

Element calculations batch over elements and load cases. Connectivity uses gather/scatter
assembly. Large solves use matrix-free or future canonical sparse machinery, not a dense
global matrix disguised behind a convenient API.

### Phase E7: fluid networks and thermal-fluid systems

This family models engineered networks, not a second Navier-Stokes solver. It adds pipes,
junctions, reservoirs, tanks, pumps, valves, demand patterns, steady hydraulics,
extended-period simulation, water age/constituent transport, and network controls.

Constitutive relations and fluid properties delegate to NatSci physics and the shared
property provider. Topology, controls, demand policy, network solves, sizing, and operating
workflows belong to Engineering.

### Phase E8: robotics and mechatronics

Add SO(3)/SE(3) utilities only after auditing existing PlussMa algebra, then articulated
models, forward kinematics, geometric Jacobians, inverse kinematics, recursive Newton-
Euler inverse dynamics, articulated-body forward dynamics, mass matrices, contacts,
trajectory generation, and control adapters.

Robot topology is immutable and compiled once. Configuration and state values are batched
Series. Analytical derivatives or Series autodiff are exposed without converting robot
states into host vectors.

### Phase E9: communications

Implement symbols/constellations, pulse shaping, channel models, modulation/demodulation,
matched filtering, timing/carrier synchronization, equalization, packet framing, error
metrics, OFDM, and foundational channel coding. Streaming runtime concerns remain separate
from the batch numerical kernels; the first public surface operates on complete Series
blocks and batched frames.

## Breadth and Depth Schedule

Implementation will not attempt to finish one engineering discipline before another can
start. After E0, the breadth pass is E1, E2, one linear-MPC tranche, one linear-circuit
tranche, one AC/DC power-flow tranche, one linear-structure tranche, and one hydraulic-
network tranche. Each must be production-shaped, tested, and documented. Depth then grows
family by family behind those stable foundations.

Robotics and communications start after the shared transform, LTI, solver, and topology
contracts have survived the breadth pass. This avoids building attractive domain APIs on
unstable numerical ownership.

## Verification Standard

Each tranche records the exact revision or release inspected and supplies:

1. closed-form scalar and matrix examples where available;
2. published benchmark cases and reference-library output vectors;
3. valid, singular, ill-conditioned, empty, non-finite, and dimensionally invalid inputs;
4. float32, float64, complex, strided, batched, and broadcasting cases where meaningful;
5. conservation, passivity, stability, reciprocity, balance, or residual invariants;
6. reverse-sensitivity checks against analytic or high-accuracy finite-difference oracles;
7. CPU/backend parity where the underlying Series operation supports another backend;
8. deterministic permanent examples whose displayed output is executable-derived; and
9. allocation, throughput, scaling, cache-hit, and warm-start benchmarks.

MATLAB R2025b is a local behavioral oracle. Visible `.m` and coder source under
`/Applications/MATLAB_R2025b.app/toolbox` may be inspected for conventions and edge cases.
Protected `.p` files and compiled MEX code are not decompiled or described as inspected
source. MATLAB agreement alone is not proof: every family also uses an open source or
published independent reference.

## Reference Program

### Shared transforms, signals and control

- [SciPy signal source](https://github.com/scipy/scipy/tree/main/scipy/signal): filter
  design, convolution, SOS filtering, zero-phase filtering, spectral estimation,
  resampling, windows, validation, and edge policy.
- [FFTW 3](https://www.fftw.org/fftw3.pdf): multidimensional, real/complex, strided and
  batched transforms, planning, normalization implications, and cache design.
- [python-control](https://github.com/python-control/python-control): public LTI contracts,
  interconnection, response generation, control analysis, and synthesis conventions.
- [SLICOT Reference 5.9.1](https://github.com/SLICOT/SLICOT-Reference): robust Schur-based
  control algorithms, Lyapunov/Sylvester/Riccati equations, filtering, identification,
  and benchmark problems.
- A. J. Laub, “A Schur Method for Solving Algebraic Riccati Equations,” IEEE TAC 24(6),
  1979: invariant-subspace Riccati solution and conditioning.
- Local MATLAB Control and Signal sources, including `obsv.m`, `margin.m`, `lqr.m`,
  `lyap.m`, `place.m`, `filtfilt.m`, `resample.m`, `upfirdn.m`, `pwelch.m`, `cpsd.m`,
  `stft.m`, and `istft.m`.

### MPC and optimal control

- [acados](https://github.com/acados/acados) and its
  [problem formulation](https://docs.acados.org/problem_formulation/index.html): staged
  OCP structure, SQP/RTI, partial condensing, soft constraints, integration, and warm
  starts.
- [OSQP](https://osqp.org/docs/): reusable-factorization convex QP, scaling,
  infeasibility, warm starts, and deterministic status reporting.
- B. Stellato et al., “OSQP: an operator splitting solver for quadratic programs,”
  Mathematical Programming Computation 12, 2020.
- CasADi source and documentation for multiple shooting, collocation, sensitivities, and
  symbolic problem assembly; MATLAB MPC is a behavioral secondary reference.

### Circuits and RF

- [Xyce source](https://github.com/Xyce/Xyce), tagged releases and regression suite:
  device organization, nonlinear solves, transient/AC analyses, sensitivities, and
  scalable circuit assembly.
- [ngspice manuals and source](https://ngspice.sourceforge.io/docs.html): SPICE
  conventions, device stamps, DC/AC/transient contracts, and timestep policy.
- C. W. Ho, A. E. Ruehli, and P. A. Brennan, “The Modified Nodal Approach to Network
  Analysis,” IEEE TCAS 22(6), 1975: canonical MNA formulation.
- [scikit-rf](https://github.com/scikit-rf/scikit-rf): N-port records, network parameter
  conversion, connection/cascade, de-embedding, calibration, Touchstone, and vector fitting.

### Power, structures and continuum engineering

- [MATPOWER 8.1](https://github.com/MATPOWER/matpower) and its
  [manual](https://www.matpower.org/docs/manual.pdf): bus/branch data, admittance matrices,
  AC/DC power flow, derivatives, continuation, OPF, and published test cases.
- [OpenSees](https://github.com/OpenSees/OpenSees): structural element organization,
  constraints, nonlinear analysis, dynamic response, and regression examples.
- [MFEM](https://github.com/mfem/mfem): finite-element spaces, forms, assembly,
  matrix-free operators, higher-order methods, and scalable execution.
- N. M. Newmark, “A Method of Computation for Structural Dynamics,” ASCE EM 85(3), 1959;
  and the generalized-alpha literature for controlled high-frequency dissipation.
- [EPA EPANET 2.2](https://github.com/USEPA/EPANET2.2): hydraulic network equations,
  pumps/valves/tanks, extended-period simulation, water quality, and official QA cases.
- [EnergyPlus Engineering Reference](https://energyplus.readthedocs.io/en/latest/guides/engineering-reference/index.html):
  heat balance, HVAC, plant/air/water loops, psychrometrics, and building-load conventions.
- [CoolProp](https://github.com/CoolProp/CoolProp): fluid equations of state, transport
  properties, mixtures, humid air, reference states, and tabular acceleration.

### Robotics and communications

- [Pinocchio](https://github.com/stack-of-tasks/pinocchio): Featherstone-style articulated
  rigid-body algorithms, analytical derivatives, constraints, and efficient model/data
  separation.
- R. Featherstone, *Rigid Body Dynamics Algorithms*, 2008: spatial algebra, RNEA, CRBA,
  and ABA definitions.
- [GNU Radio](https://github.com/gnuradio/gnuradio): communications block contracts,
  channelizers, coding, equalizers, OFDM, synchronization, resampling, and streaming policy.
- [liquid-dsp](https://github.com/jgaeddert/liquid-dsp): lightweight modem, filter-bank,
  oscillator, synchronizer, and embedded DSP implementations.
- [scikit-rf](https://github.com/scikit-rf/scikit-rf): RF/microwave network analysis.
- Local MATLAB Communications sources are a behavioral and visible-source secondary
  reference for modulation, channels, synchronization, equalization, and coding.

## Per-Tranche Audit Record

Before code is written, the tranche note must identify:

1. the exact source revision, paper, standard, and local MATLAB files inspected;
2. the equations, conventions, tolerances, edge behavior, and test vectors adopted;
3. disagreements between references and the chosen Pluss contract;
4. existing Pluss machinery reused, promoted, or intentionally rejected;
5. the semantic axes and full batch/broadcast contract;
6. expected differentiation and platform behavior;
7. cacheable and non-cacheable artifacts;
8. performance measurements required before any fused kernel is justified; and
9. behavior deliberately deferred rather than approximated or silently accepted.

After every implementation pass, the completion report lists all source files, source
revisions, manuals, and papers actually read. A planned reference is not reported as read
until it was inspected for that pass.
