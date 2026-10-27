# PlussMa Natural Sciences Implementation Plan

## Purpose

`PlussMa/NatSci` is a collection of scientific domain modules built over the numerical
machinery already provided by PlussMa and the `Series` execution model. The subspace is
not intended to become a second numerical kernel library. Shared scientific contracts
belong in `common/`; domain folders should express models, invariants, and workflows in
terms of those contracts and reuse the existing integration, ODE, optimization, linear
algebra, and automatic-differentiation implementations. Scientific interpolation will
first receive one canonical PlussMa Math implementation because the audit found no general
engine to reuse today.

Correctness is ordered ahead of breadth. Public models must state their units, parameter
conventions, validity domains, and conservation assumptions. A model that cannot enforce
those contracts is not ready to be public.

## Engineering Rules

### Reuse before implementation

Every tranche begins with a repository-wide machinery audit across PlussQit, PlussMa,
PlussTils, PlussMic, and PlussAI. Its implementation note must name:

1. the existing public or internal machinery that can be reused;
2. the gap that existing machinery does not cover;
3. whether the gap belongs in NatSci or in a lower shared layer;
4. the one canonical implementation that all affected domains will call; and
5. the tests proving that no second behavioral path was introduced.

A new helper is not justified merely because adapting an existing abstraction takes more
design work. Common packing, validation, broadcasting, covariance, solver, and result
reconstruction behavior must be centralized and reused by mechanics, astronomy,
chemistry, and biology. Domain folders may supply semantic types and policies, but they
must not copy numerical machinery from another domain.

Reference implementations and papers are correctness guides. They may supply equations,
edge cases, conventions, and independent test vectors. Their source is not copied into
PlussMa; the implementation must fit Pluss's own type system, error model, backend model,
and automatic-differentiation contracts.

### Series-native computation

Scientific computational paths use `pluss::Series` operations exclusively:

- no element-by-element host loops;
- no `item()`, `host()`, or raw-storage reads inside a numerical kernel;
- no implicit CPU transfer or reconstruction through `std::vector`;
- no scalar loop over a batch when broadcasting, reductions, gather/scatter, or matrix
  operations can express the computation;
- no detached intermediate unless the algorithm explicitly defines a differentiation
  boundary; and
- no new backend kernel when a composition of existing differentiable `Series` operations
  has acceptable measured performance.

Metadata inspection for rank, shape, dtype, platform, and `numel()` is allowed. Stable
public diagnostics may reduce a predicate to one scalar only at a validation boundary;
the computation itself remains on its original platform.

Every vectorized operation must preserve or deliberately document:

- leading batch dimensions and broadcasting rules;
- dtype and platform;
- shape and lane correspondence;
- automatic-differentiation connectivity; and
- behavior for empty inputs and non-contiguous views.

An operation is not considered vectorized merely because it accepts a `Series`. Its
implementation must operate on complete arrays through Series kernels. Semantic axes are
defined explicitly, leading dimensions remain batch dimensions, and one implementation
must handle scalar, vector, matrix, 3-D, 4-D, and higher supported ranks through
broadcasting rather than rank-specific loops. Flattening is reserved for an established
solver boundary and must use a reversible layout object.

Focused tests for a vectorizable family cover, where meaningful:

- scalar and one-dimensional inputs;
- 2-D, 3-D, and 4-D batches;
- singleton and unequal leading-dimension broadcasting;
- non-contiguous views;
- empty dimensions;
- CPU/backend parity; and
- reverse sensitivity through the batched expression.

Optimization follows measurement, not intuition. A fused kernel is considered only after
the canonical Series composition is correct, covered by CPU/backend parity tests, and a
benchmark identifies a material bottleneck. The composed implementation remains the
correctness oracle for any later fused route.

### Cache policy

Caching is used where repeated construction or validation is measurable, but cache
correctness takes precedence over hit rate.

Good cache candidates include immutable dimension/unit metadata, validated parameter and
state layouts, solver tableaux, quadrature nodes and weights, interpolation coefficients,
and compiled reaction or network topology. Cache keys include every result-affecting
property, including shape, dtype, platform, unit revision, options, and algorithm version.

Mutable externally owned buffers, graph-bearing intermediate Series, and values whose
storage version cannot be observed are not cached across calls. Device Series caches must
be bounded, thread-safe, and platform-specific; they may not trigger an implicit CPU/device
transfer. A cache must expose a deterministic uncached path used as its correctness oracle
and must have tests for invalidation, contention, and bounded growth.

### Central policy ownership

Shared policy has one owner. NatSci common will define scientific dimension, unit,
parameter, state, trajectory, and domain-validation contracts. Existing PlussMa Math and
PlussQit layers continue to own numerical algorithms, tensor mechanics, backend dispatch,
and differentiation. If a behavior is useful outside natural sciences, it is promoted to
the appropriate lower layer rather than hidden in `NatSci/common/detail`.

The pre-implementation audit for the first phases identified this reuse map:

| Need | Canonical machinery to reuse or establish |
| --- | --- |
| Batched values, broadcasting, reductions, device execution, autodiff | Existing `Series` operations and factories |
| Named block packing, shape/dtype/platform layouts, flattening, reconstruction | Adapt `math::optim::ParameterPack`, `ParameterLayout`, `flatten_values`, and `unflatten_values`; do not create a second packing protocol |
| Deterministic ODE evolution and reverse sensitivities | Adapt `math::ode::fluxa` and `fluxa_adjoint` |
| Stochastic differential evolution | Adapt `math::sde::fluctua` and its adjoint path |
| Callable quadrature and sampled integration | Adapt `math::integ` and the `Series::antidiff()` surface |
| Parameter fitting and constrained optimization | Adapt `math::optim`/Septima and its derivative audit machinery |
| Pointwise linear blends | Reuse `Series::lerp` |
| Scientific interpolation over coordinates | No canonical engine currently exists. Establish one reusable PlussMa Math interpolation module before adding NatSci adapters |
| Covariance validation/factorization | Promote reusable covariance contracts from the current private statistics implementations before covariance-aware measurements; do not copy a model-local `stabilize_cov_` helper |
| Defined/shape/dtype/platform validation | Consolidate the repeated Math validation helpers into a shared internal contract layer before NatSci solver adapters depend on them |
| Fluent composition of setup and result transformations | `pluss::tils::suppose(...).then(...)` may assemble non-mutating orchestration when it improves clarity; it does not replace a fused or broadcasted Series expression |
| Immutable binding of cheap Pluss handles | Opt into `pluss::tils::let` where non-rebindable ownership is part of the contract; remember that it does not block a core type's const in-place methods |
| Reproducible seed allocation | Reuse `pluss::tils::rng_seed`, `next_rng_seed`, and `seed_or_next` rather than introducing a NatSci global RNG counter |
| Runtime-heterogeneous core values | Consider `pluss::Queue` when an API genuinely needs one of Series, Note, Timeline, or Duration at runtime; avoid dynamic dispatch in numerical inner loops and do not force Queue into statically typed quantity APIs |
| Host ranges and inherently irregular orchestration | `pluss::tils::transmit` and `map_axis` may coordinate non-numerical or irreducibly irregular work; they are not substitutes for Series batching because they iterate ranges/slices and materialize host containers |
| Scalar conversion at an explicit boundary | Reuse PlussTils scalar-cast helpers for reporting or validation boundaries only; never extract a batch into host scalars to perform a scientific computation |

This table is revised whenever the repository audit discovers a more canonical owner.

## Planned Layout

```text
NatSci/
├── common/
│   ├── compile-time SI dimensions and dimension algebra
│   ├── typed linear units and SI conversion
│   ├── scalar and Series-valued quantities
│   ├── versioned physical constants with provenance
│   ├── measurement uncertainty and propagation
│   ├── scientific parameter sets
│   ├── state vectors and named trajectories
│   ├── Cartesian spatial grids and differential operators
│   ├── adapters for PlussMa solvers
│   └── centralized dimensional and unit validation
├── physics/
│   ├── mechanics/
│   ├── astro/
│   ├── quantum/
│   ├── thermo/
│   ├── electromag/      (planned)
│   ├── fluids/
│   ├── relativity/      (planned)
│   └── continuum/       (planned)
├── chemistry/
│   ├── kinetics/
│   ├── quantum/
│   ├── thermo/
│   ├── equilibrium/
│   ├── electrochem/     (planned)
│   ├── transport/       (planned)
│   └── spectroscopy/    (planned)
└── biology/
    ├── population/
    ├── systems/
    ├── biophysics/
    ├── epidemiology/    (planned)
    ├── ecology/         (planned)
    ├── genetics/        (planned)
    └── neuroscience/    (planned)
```

## Common Foundation

### Phase C1: dimensional quantities and constants

Status: **implemented in the first common tranche**.

- Seven-base SI dimension vectors encoded in types.
- Compile-time multiplication, division, and integral powers of dimensions.
- Typed linear units whose labels are display metadata, never unit identity.
- Canonical SI storage and explicit conversion through a dimension-compatible `Unit`.
- The same `Quantity<Dimension, Rep>` contract for scalar `double` and `Series` values.
- Scalar/Series mixed arithmetic where broadcasting has a precise numerical meaning.
- CODATA 2022 constants carrying symbol, name, standard uncertainty, source, revision,
  and DOI metadata.
- First-order standard-uncertainty propagation for independent scalar measurements.
- Compile-time rejection of dimensionally invalid addition and conversion.

This phase intentionally supports **linear units only**. Celsius and Fahrenheit values are
affine quantity points, not scaled Kelvin quantities. They will not be admitted through a
linear-unit constructor because doing so would make multiplication and subtraction
ambiguous.

### Phase C2: scientific records

Status: **implemented in the second common tranche**. Domain-specific schemas and solver
adapters remain C3 work; the storage, dimensional, uncertainty, and trajectory contracts
they consume are now established.

- `ParameterSet` with named, dimension-checked parameters, immutable snapshots, bounds,
  source metadata, constant-time name lookup, and a cached solver layout.
- Typed state blocks that flatten to one `Series` for a solver and reconstruct without
  losing field names, shapes, dimensions, platform, or dtype.
- Named trajectories containing a monotonic independent coordinate plus shape-preserving
  state blocks.
- Covariance-aware measurements and Series-valued uncertainty. The independent-input
  formulas from C1 remain available but will not masquerade as correlation tracking.
- Affine temperature points and explicit differences between temperature and temperature
  interval.

The implementation reuses the extracted `math::optim::ParameterPack` and
`ParameterLayout`; no NatSci-specific packing protocol exists. Repacking moves block
storage out of the shared pack without another block copy. Record names are indexed with
transparent string lookup, while numerical flattening and reconstruction remain Series
operations and retain non-contiguous inputs, backend, dtype, and reverse-mode history.

Covariance shape, finiteness, symmetry, batched diagonal extraction, symmetrization, and
jitter now have one owner in `subsp::math::covariance`. The existing state-space model's
private symmetrize-plus-jitter path delegates to that owner. Batched diagonal extraction
uses an identity mask and reduction because the current routed `Series::diagonal` kernel
is only two-dimensional; it does not transfer data to the host. Positive-semidefinite
validation adds only a dtype-scaled numerical jitter before the existing batched Cholesky
kernel, so exact zero-variance directions remain valid while indefinite matrices fail.

The permanent C2 verifier covers bounded parameters, dimension mismatch, cached
flatten/repack, non-contiguous state blocks, real coordinates over complex states,
unordered trajectories, scalar and Series affine temperatures, four-dimensional
uncertainty, batched covariance propagation, singular covariance, invalid covariance,
and reverse sensitivities through both independent and correlated measurement paths.

### Phase C3: solver adapters

Status: **implemented**.

- ODE adapters preserving state names and quantity dimensions across flatten/unflatten.
- Quadrature adapters requiring the integrand and differential to produce the declared
  output dimension.
- Optimization adapters separating dimensioned parameters from dimensionless solver
  coordinates through explicit scaling.
- Interpolation adapters that preserve the dependent quantity dimension and validate the
  independent coordinate.
- Event and constraint adapters shared by mechanics, kinetics, and population models.

Adapters must call existing PlussMa solvers. They may add validation, packing, and result
reconstruction, but may not fork numerical algorithms into NatSci.

The C3 implementation follows that boundary directly:

- `common::solve::flow` and `flow_until` delegate all RK4 stepping to `math::ode`.
  Terminal-event detection was added once to the canonical ODE engine; NatSci only checks
  dimensions and reconstructs named states. Event residuals are scalar and dimensionless,
  support rising/falling/any crossing policy, and use RK4 bisection inside the detected
  step.
- `common::solve::quad` delegates batched callable integration to `math::integ::quadra`.
  The result dimension is inferred as coordinate dimension times integrand dimension.
- `common::solve::fit` converts physical parameters to explicit dimensionless coordinates
  using caller-supplied positive scales, then delegates to `math::optim::optima`. Partial
  field bounds become selected linear rows of the normalized identity matrix, so absent
  bounds remain absent and bound Jacobians are exact. User constraints are required to be
  dimensionless and receive fresh reverse Jacobians. Objective gradients are likewise
  regenerated from detached current coordinates at each evaluation, preventing stale tape
  state while retaining a typed Series-only user callback.
- Linear interpolation has one new canonical owner in `subsp::math::interp`; the NatSci
  adapter adds coordinate/value dimensions but no second interpolation formula. Interval
  location, gather, broadcasting, blending, and reverse sensitivities remain Series-native.
- Record schemas, layouts, name indexes, and parameter-bound descriptors are immutable and
  shared across solver repacks. Public `ParameterSet::repack` still validates bounds;
  solver-owned trusted rebinding skips repeated bound reductions without exposing an unsafe
  public route.

The permanent C3 verifier covers scalar and batched interpolation, clamp/linear/error edge
policies, reverse sensitivities, batched quadrature, parameterized multidimensional RK4,
derivative-dimension rejection, terminal-event refinement and direction filtering,
mixed-unit optimization, partial bounds, active nonlinear constraints, and invalid scales.

### Phase C4: dimension-aware differentiation

Status: **implemented**.

- First-order scalar gradients carry output-dimension/input-dimension units.
- Vector mappings expose an output-by-input Jacobian without losing physical dimension.
- Scalar objectives expose Hessians and Hessian-vector products through the existing
  centralized transform engine.
- `nth_diff<Order>` accepts scalar or array-valued mappings and returns the complete
  derivative tensor while deriving output/input-to-the-Order dimensions at compile time.
- `elasticity` computes the dimensionless normalized sensitivity
  `x / f(x) * df(x) / dx` and rejects a zero output denominator.

C4 does not introduce another tape or finite-difference implementation. Its callbacks
consume and return `SeriesQuantity` values, while `subsp::math::diff` remains the sole
owner of graph capture, Jacobian lane assembly, higher-order reverse propagation, HVP
policy, disconnected-input policy, and backend execution. `grad`, conventional
`hessian`, and `hvp` retain scalar-objective contracts; vector-valued first derivatives
use `jacobian`, while `nth_diff` is the universal higher-order route. Its tensor layout is
`output_shape` followed by `input_shape` once per derivative order, preserving mixed
partials rather than silently summing output lanes. The same engine is exposed through
`Series::calc.diff(fn, order)`; the accessor is a forwarding convenience, not another
tape. Quantity callbacks expose Series operations rather than `DualTrace` arithmetic, so
the typed transforms are reverse-mode; callers needing a forward first-order transform use
the lower `subsp::math::diff` surface. Full tensors grow exponentially and are bounded by
the 64-axis Series metadata contract; JVP, VJP, and HVP remain the matrix-free choices when
the complete tensor is unnecessary. The permanent verifier covers vectorized Jacobians
and Hessians, a directional HVP, scalar derivatives through order five, complete
vector-valued derivatives through order three, mixed partials, rank-eight output,
non-contiguous inputs, dimensional result types, empty tensors, and stable contract
failures.

## Physics Implementation

### Phase P1: particle mechanics foundation

Status: **implemented**.

P1 establishes the shared particle-state and Newtonian-mechanics layer used by later
orbital, continuum, and molecular families. The public surface is functional. `Bodies`
is a validated semantic record containing mass, position, and velocity; algorithms remain
free functions so later convenience objects cannot acquire a second numerical path.

The first tranche provides:

- batched body construction with position and velocity shaped `[..., bodies, axes]` and
  mass broadcast once to `[..., bodies]`;
- per-body and total linear momentum, center position, center velocity, kinetic energy,
  softened gravitational potential energy, total energy, and three-dimensional angular
  momentum;
- a center-frame transformation preserving leading batch dimensions;
- direct Newtonian acceleration with optional Plummer softening; and
- a reusable drift-kick-drift leapfrog step whose acceleration callback is constrained to
  return a dimensionally valid acceleration quantity.

#### Reuse audit

No particle or force-system implementation existed elsewhere in PlussQit, PlussMa,
PlussTils, PlussMic, or PlussAI. P1 therefore adds the missing domain contract but delegates
all numerical work to established owners:

| Concern | Canonical owner used by P1 |
| --- | --- |
| broadcasting, reductions, pair geometry, cross products, dtype/platform dispatch | `pluss::Series` operations |
| reverse sensitivities | the existing Series tape; mechanics creates no derivative registry or detached numerical path |
| dimensions and SI conversion | `NatSci/common` quantities and compile-time dimension algebra |
| gravitational constant and provenance | the CODATA 2022 constant record in `NatSci/common` |
| future adaptive integration and events | the C3 adapters over `math::ode`; P1 does not duplicate them inside mechanics |

Pair interactions are formed as one broadcasted tensor with shape
`[..., target body, source body, spatial axis]`. Gravity and potential energy use Series
reductions over the two semantic body axes; there is no body loop, host extraction, or CPU
round trip. Non-contiguous positions and arbitrary leading batches use the same expression.
The direct method has `O(N^2)` arithmetic and storage and is intentionally the correctness
baseline for small and moderate body counts. A blocked, fused, or tree approximation will
only be added after benchmarks identify the crossover, and it must retain parity with this
canonical composition.

#### Reference audit

REBOUND 5.2.2, revision `c341347`, was inspected on 2026-10-11. Its public simulation
contract and the visible `src/gravity.c`, `src/tools.c`, and
`src/integrator_leapfrog.c` implementations informed the direct-pair, conservation,
softening, and drift-kick-drift test cases. PlussMa does not reproduce REBOUND's particle
layout, callback system, compensated summation, tree solver, or specialized astronomical
integrators. The local MATLAB R2025b tree was also audited:
`toolbox/matlab/funfun/examples/orbitode.m` supplies an independent
restricted-three-body/event example for the later orbital solver phase; it is not treated
as a particle-mechanics implementation.

The permanent verifier covers scalar and batched masses, a non-contiguous batched position
view, center and momentum invariants, equal-and-opposite force balance, softened and
unsoftened gravity, potential and total energy, angular momentum, center-frame projection,
reverse sensitivity through pair potential, a harmonic-oscillator leapfrog step, invalid
mass, coincident-body, and spatial-axis failures. It runs with the C1-C4 suites so changes
to quantities, constants, records, solvers, or differentiation cannot silently invalidate
mechanics.

P1 deliberately defers collision response, constraints, rigid bodies, Lagrangian and
Hamiltonian systems, long-horizon conservation diagnostics, orbital elements, adaptive
integration, and scalable large-N approximations. Those require distinct contracts and
must not be smuggled into the direct-force primitive as flags.

### Phase P2: variational mechanics

Status: **implemented**.

P2 adds the variational layer without creating a second solver, derivative engine, or
untyped state container. Its public API remains functional. `TangentState<Q>` validates a
coordinate and generalized velocity, while `PhaseState<Q>` validates a coordinate and its
canonical momentum. The final axis is always the degree-of-freedom axis; every leading
axis is a preserved batch axis. Both state factories reject undefined, empty, non-finite,
non-floating, shape-mismatched, dtype-mismatched, and platform-mismatched inputs before a
field evaluation begins.

The tranche provides:

- compile-time dimensions for generalized velocity, acceleration, canonical momentum,
  canonical force, and generalized mass;
- `hamiltonian`, which evaluates a scalar or per-batch energy and constructs Hamilton's
  canonical field `dq/dt = dH/dp`, `dp/dt = -dH/dq`;
- `lagrange`, which returns the Lagrangian value, canonical momentum `dL/dv`, generalized
  mass matrix `d2L/dv2`, and the Euler-Lagrange acceleration field;
- `symplectic_step`, a separable kick-drift-kick map accepting positive, zero, or negative
  finite time steps; and
- `common::drift`, which reports absolute invariant drift and dimensionless drift against
  an explicit positive physical scale.

For an explicitly time-dependent Lagrangian, acceleration is solved from

```text
M(q, v, t) a = L_q - L_vq v - L_vt,
M(q, v, t)   = L_vv.
```

The `L_vt` term is retained. Omitting it would make apparently correct autonomous examples
fail for explicitly time-dependent systems. Singular generalized mass matrices fail through
the canonical batched linear solve rather than being regularized silently. The symplectic
map intentionally accepts only separable potential and kinetic callables; a generic
non-separable Hamiltonian requires an implicit symplectic method and is not approximated by
this explicit contract.

#### Reuse and performance audit

| Concern | Canonical owner used by P2 |
| --- | --- |
| first and higher derivatives | `subsp::math::diff`; mechanics adds no tape, finite-difference path, or derivative registry |
| dimensions and SI storage | `NatSci/common::Quantity` and compile-time dimension algebra |
| batch geometry and block extraction | `Series` reshape, broadcast masks, reductions, slicing, and matrix multiplication |
| generalized-mass solve | the existing batched `Series::linearalg.inverse()` and `matmul()` path |
| adaptive integration and events | C3 adapters over `math::ode`; P2 exposes fields and one symplectic step rather than duplicating trajectory orchestration |
| conservation normalization | the shared `common::drift` primitive with an explicit dimensioned scale |

Hamiltonian fields and kick-drift-kick stages evaluate all batches in one Series expression.
There is no degree-of-freedom or batch loop, host extraction, or CPU-only numerical branch.
Each first-derivative stage evaluates its callable once: the same traced result supplies the
displayed energy and the scalar reduction used for reverse propagation. Lagrangian assembly
uses the canonical full joint Hessian because `L_vv`, `L_vq`, and `L_vt` are all required;
batch-diagonal blocks are selected by identity masks and reductions, then solved together.
This is exact and backend-neutral, but full Hessian storage is quadratic in the flattened
state size. A future structured/matrix-free mass operator requires benchmarks and a distinct
contract; P2 does not conceal that cost behind a misleading option.

P2 also exposed a central transform defect: `value_and_grad` collected leaf sensitivities
correctly but reverse traversal could discard a non-leaf input's sensitivity before the
transform returned it. The canonical transform now retains each requested input only for
the scoped reverse pass and restores its previous retention policy afterward. A dedicated C4
regression verifies both the non-leaf gradient and an outer derivative through that result.
This repair is shared by mechanics, optimization, and PlussAI rather than patched locally.

#### Reference audit

The following sources were inspected for P2; they informed contracts and independent test
vectors, not implementation structure:

- REBOUND 5.2.2 revision `c341347`, `src/integrator_leapfrog.c`, `src/tools.c`, and
  `src/rebound.h`: drift/kick ordering, reversible signed stepping, and conservation
  diagnostics.
- MATLAB R2025b
  `toolbox/stats/stats/+stats/+mcmc/+impl/HamiltonianImpl.m`, SHA-256
  `c484de9d2e988188166b129bdf5d0e7b6df840788fdcd4b77de3b5addf1cef67`:
  explicit half-momentum/full-position/half-momentum leapfrog staging and mass scaling.
- MATLAB R2025b `toolbox/matlab/funfun/examples/batonode.m`, SHA-256
  `70c580a4f994e7c2022dd0cd9798ef495707ecfa3a0aa4c48b3ebdffff10e275`:
  state-dependent generalized mass matrices in a Lagrangian system.
- MATLAB R2025b `toolbox/matlab/funfun/ode45.m`, SHA-256
  `94f7eab52127b33776fda3c2f0f01fbaa9fbcf43745a7e9e4b39d8f228465898`, and
  `toolbox/matlab/funfun/private/odeevents.m`, SHA-256
  `cefbd86f715e70d9050ab9c936ca403208566217d09d3e20549426d233d803b1`:
  separation of field definition, mass handling, integration, requested output, and events.
- SciPy 1.16.2 `scipy/integrate/_ivp/ivp.py`: the same field-versus-trajectory ownership
  boundary and signed integration direction.
- Boost.Odeint `symplectic_rkn_sb3a_mclachlan.hpp`: separable canonical-system contract and
  coefficient-driven symplectic staging.
- Rein, Tamayo, and Brown, *High order symplectic integrators for planetary dynamics and
  their implementation in REBOUND*, arXiv:1907.11335: long-horizon conservation rationale
  and the distinction between symplectic maps and generic adaptive integration.

The permanent verifier covers batched Hamiltonian values and fields, one-step harmonic
motion, negative-step reversibility, zero-step identity, outer reverse sensitivities through
the composed map, batched Lagrangian momentum and mass matrices, explicitly time-dependent
mixed partials, absolute/scaled drift, and stable shape, energy-layout, and non-finite-step
failures. P1 and C4 verifiers run alongside it.

P2 deliberately defers implicit symplectic methods, constraints, rigid-body manifolds,
trajectory storage, adaptive symplectic control, Lyapunov spectra, and orbital elements.
Those are separate phases, not flags on the canonical field evaluators.

### Phase A1: osculating orbit foundation

Status: **implemented**.

A1 replaces the empty `physics/astro` placeholder with the first public astronomy contract.
`OrbitState` stores position and velocity in an inertial Cartesian frame with shape
`[..., 3]`; `Elements` stores six shape-matched fields: signed semimajor axis,
eccentricity, inclination, ascending-node longitude, periapsis argument, and true anomaly.
All angle quantities are dimensionless radians. Positive semimajor axes pair with
`0 <= e < 1`, negative axes pair with `e > 1`, and the parabolic `e = 1` singularity is
rejected rather than represented inaccurately in a semimajor-axis record.

The tranche provides `make_state`, `make_elem`, `to_elem`, and `from_elem`, plus specific
orbital energy, specific angular momentum, eccentricity vector, periapsis, apoapsis, and
elliptic period. Circular and equatorial singularities use deterministic conventions:
undefined node and periapsis angles become zero, while phase becomes argument of latitude
or true longitude so conversion back to Cartesian state remains meaningful. Hyperbolic
states round trip through true anomaly but deliberately have no apoapsis or period.

#### Reuse and performance audit

| Concern | Canonical owner used by A1 |
| --- | --- |
| vector products, norms, trigonometry, branch masks, broadcasting, dtype/platform dispatch | `pluss::Series` |
| reverse sensitivities through state construction and diagnostics | the existing Series tape |
| SI dimensions and representation | `NatSci/common::Quantity` and dimension algebra |
| many-body gravity and stepping | `physics::mechanics`; astro does not create another force solver |
| gravitational constants and provenance | `NatSci/common::constants` when callers choose a named body constant |

Every Cartesian or element field carries arbitrary leading batch axes. Conversion builds
one Series expression over all lanes; there is no orbit loop, component host extraction,
or CPU-only numerical branch. Host access is limited to whole-tensor contract reductions.
The permanent verifier covers the published Vallado state, circular-equatorial canonical
angles, elliptic and hyperbolic round trips, rank-two, rank-three, and rank-four Cartesian
outputs, expanded non-contiguous element storage, periapsis/apoapsis/period identities,
reverse history, and stable failures for radial, parabolic, and non-positive-GM inputs.
The permanent CPU benchmark converts 32,768 rank-four element lanes. On the 2026-10-14
development run, one batched call took 6.761 ms median versus 11.695 ms when the same work
was dispatched as 16 leading-axis pages, a 1.730x advantage with identical states. These
timings describe the current machine and are not hard pass thresholds; value parity is.

#### A1 reference audit

- REBOUND `src/tools.c`, current public source inspected 2026-10-14: Cartesian/classical
  conversion equations, signed semimajor-axis conventions, eccentricity and angular
  momentum vectors, angle normalization, and explicit treatment of nearly planar and
  nearly circular states. Its scalar particle layout and implementation were not copied;
  PlussMa derives the same relations as batched Series expressions.
- NASA NAIF CSPICE 1.0.3 `oscelt_c` documentation, inspected 2026-10-14: inertial-frame
  contract, positive gravitational parameter, degenerate angular-momentum failures,
  zero-node/zero-periapsis singular conventions, and the required state reconstruction
  sanity check.
- NASA NAIF CSPICE 1.1.2 `conics_c` documentation, inspected 2026-10-14: inverse conic
  state contract and validity requirements for eccentricity, periapsis, and attracting
  mass. CSPICE's epoch propagation and parabolic perifocal representation remain future
  work; A1 converts the state at the supplied true anomaly.
- Vallado, *Fundamentals of Astrodynamics and Applications*, standard state example
  `r = [6524.834, 6862.875, 6448.296] km`,
  `v = [4.901327, 5.533756, -1.976341] km/s`: independent six-element regression vector.
- Local MATLAB R2025b was searched for Aerospace Toolbox orbital conversion sources. The
  installed tree contains `toolbox/matlab/funfun/examples/orbitode.m` but no inspectable
  state/element conversion implementation, so MATLAB is not claimed as an A1 oracle.

A1 deliberately defers anomaly propagation, universal variables, Lambert transfer,
ephemerides, frame/time standards, perturbation models, and nonsingular element sets.
Those need their own contracts rather than flags on the instantaneous conversion.

### Phase A2: Kepler anomaly and two-body propagation

Status: **implemented**.

A2 adds the analytic two-body propagation layer over A1 without introducing an astronomy-
specific scalar solver. `to_mean` and `to_true` convert between true anomaly and the
elliptic eccentric or hyperbolic anomaly representation. `solve_mean` solves Kepler's
equation for mixed elliptic and hyperbolic lanes in one call and returns the anomaly,
residual, convergence mask, and iteration count. `mean_rate` evaluates
`sqrt(mu / abs(a)^3)`, while `advance` evolves mean anomaly by `n * dt`, solves the new
anomaly, and preserves the other five osculating elements.

Parabolic lanes remain deliberately unsupported because eccentric/hyperbolic anomaly is
singular at `e = 1`; that family requires a universal-variable state and Stumpff functions.
Elliptic mean anomaly is reduced for the solve and its whole-turn offset restored afterward,
so large positive and negative elapsed times retain revolution count without feeding large
arguments through the Newton kernel. Hyperbolic mean anomaly is not periodic and is never
wrapped. Strict mode fails if any lane misses its residual tolerance; diagnostic mode returns
the lane-wise mask instead.

#### A2 reuse and performance audit

| Concern | Canonical owner used by A2 |
| --- | --- |
| Newton iteration, active-lane convergence, iteration cadence | `subsp::math::roots::newton` |
| trigonometric/hyperbolic functions, masks, broadcasting, dtype/platform dispatch | `pluss::Series` |
| reverse sensitivities through time and orbital fields | the existing Series tape |
| elapsed-time and frequency dimensions | `NatSci/common::Quantity` |
| element validation and Cartesian reconstruction | the A1 `Elements` and `from_elem` contracts |

The implementation has no orbit or component loop. Elliptic and hyperbolic lanes share one
rank-preserving Series graph; inactive formulas receive masked finite inputs before eager
evaluation, preventing an unselected hyperbolic `cosh` from overflowing on a many-revolution
ellipse. Fixed integer cubes use multiplication chains instead of `pow(double)`, preserving
Float32 storage and avoiding an unnecessary CPU Float64 promotion.

The permanent verifier covers mixed conic types and eccentricities adjacent to one, positive
and negative mean anomalies, many revolutions, anomaly round trips, forward and backward
elapsed time, a circular quarter period, an elliptic full-period Cartesian return, hyperbolic
propagation, rank-three elapsed-time broadcasting, Float32 dtype preservation, reverse
sensitivity through an eccentric Kepler solve, diagnostic non-convergence, and strict failure.
On the final 2026-10-14 development run, propagating 32,768 rank-four lanes took 19.143 ms
median in one batched call versus 28.727 ms as 16 page dispatches, a 1.501x dispatch advantage
with identical phases. The same benchmark measured 6.766 ms versus 10.928 ms for A1 element
conversion. Timings are descriptive, not test thresholds; numerical parity is.

#### A2 reference audit

- REBOUND current `src/tools.c`, functions `reb_M_to_E`, `reb_E_to_f`, and `reb_M_to_f`,
  inspected 2026-10-14: elliptic and hyperbolic Kepler equations, mean-anomaly normalization,
  the signed `M + 0.71 e` elliptic initial estimate, and anomaly-to-phase conventions. PlussMa
  reuses its existing batched Newton owner, a scale-aware dtype tolerance, and an `asinh(M/e)`
  hyperbolic start rather than copying REBOUND's scalar loop or logarithmic estimate.
- NASA NAIF CSPICE 1.1.2 `conics_c` documentation, inspected 2026-10-14: osculating elements at
  an epoch, propagation to a requested epoch, support for elliptic/hyperbolic conics, positive-GM
  validation, and overflow as a reported failure rather than an accepted result.
- NASA NAIF CSPICE 1.0.3 `oscelt_c` documentation, inspected 2026-10-14: `conics_c` as the
  inverse epoch map, inertial-frame requirement, and warnings near singular conic subsets.
- The A1 Vallado state and Cartesian reconstruction provide the independent end-to-end check:
  a full elliptic period must reconstruct the initial position and velocity, not merely satisfy
  the same anomaly residual used by the solver.
- Local MATLAB R2025b was searched again for an inspectable general Kepler solver or conic
  propagator. The available `orbitode.m` example is a numerical restricted-three-body ODE, not
  an independent two-body anomaly implementation, so MATLAB is not claimed as an A2 oracle.

A2 deliberately defers universal-variable/parabolic propagation, Lambert transfer, perturbed
osculating-element evolution, ephemerides, and coordinate/time standards.

### Phase A3: universal-variable state propagation

Status: **implemented**.

A3 closes the two-body initial-value gap left intentionally by A2. The overload
`advance(OrbitState, elapsed, mu, options)` propagates Cartesian position and velocity
directly, without converting through classical elements. It therefore covers elliptic,
parabolic, and hyperbolic motion with one contract, including lanes on and immediately to
either side of `e = 1`. The element overload remains useful when callers already own
osculating elements; the state overload is the robust route near the parabolic singularity.

The solve uses the dimensionless universal root `u = chi / sqrt(r0)`. Its normalized time,
energy, and radial-velocity terms keep the Newton equation independent of the caller's
physical scale while the `Quantity` boundary still enforces SI dimensions. `StatePropResult`
returns the propagated state, normalized root, normalized residual, lane-wise convergence
mask, and step count. Strict mode throws if any lane misses tolerance; diagnostic mode
returns the mask and intermediate result.

The Stumpff `C` and `S` functions use direct circular or hyperbolic forms away from zero and
fifth-order power series in the near-parabolic region. Branch inputs are masked before eager
evaluation, so an inactive square root, logarithm, or hyperbolic function cannot inject a
NaN or overflow into another conic family. Position and velocity are reconstructed with the
four Lagrange coefficients. Zero position, zero velocity, radial motion, non-positive GM,
invalid solver options, non-finite inputs, and strict non-convergence fail explicitly.

#### A3 reuse and performance audit

| Concern | Canonical owner used by A3 |
| --- | --- |
| Newton iteration, convergence cadence, and reverse-connected updates | `subsp::math::roots::newton` |
| vector norms/products, branch masks, broadcasting, dtype/platform dispatch | `pluss::Series` |
| state shape/layout validation and SI dimensions | A1 `OrbitState` plus `NatSci/common::Quantity` |
| solver policy and dtype-aware default tolerances | the existing A2 `KeplerOptions` machinery |
| reverse sensitivities through elapsed time and state expressions | the existing Series tape |

There is no orbit or component host loop. All leading state axes are batch axes, elapsed
time and GM broadcast across those axes, and every Newton step is one Series graph. Host
access is limited to whole-batch validation and convergence checks at the configured cadence.
The permanent verifier covers NASA's circular half-period state, zero-time identity, negative-
time reversal, elliptic and hyperbolic parity with A2, exact and near-parabolic energy
conservation, rank-four Float32 storage, multidimensional elapsed-time broadcasting,
non-contiguous state views, reverse sensitivity, diagnostic non-convergence, and strict
failures.

The permanent CPU benchmark propagates 32,768 rank-four Cartesian states. On the 2026-10-15
development run, one batched call took 55.495 ms median versus 72.533 ms when dispatched as
16 leading-axis pages, a 1.307x advantage with identical states. The same run measured
6.910 ms versus 11.197 ms for element conversion and 19.215 ms versus 27.175 ms for A2
element propagation. Timings are descriptive, not pass thresholds; value parity is enforced.

#### A3 reference audit

- NASA NAIF SPICELIB 2.2.0 `PROP2B`, inspected 2026-10-15: the direct state-to-state
  universal-variable contract, its advantage near `e = 1`, positive-GM and degenerate-state
  failures, unreasonable-time failure policy, and the independent 100,000,000 km circular
  half-period regression vector. The implementation cited there is based on Danby,
  *Fundamentals of Celestial Mechanics*, 2nd edition, pp. 168-180.
- poliastro current `core/propagation/vallado.py`, inspected 2026-10-15: the universal Kepler
  residual, elliptic/hyperbolic/parabolic initial estimates, Newton denominator, and Lagrange
  reconstruction coefficients. PlussMa normalizes the equation, evaluates whole Series
  batches, uses its shared root owner, and adds branch-safe Stumpff evaluation rather than copying the scalar loop.
- G. Bau and J. Roa, *Uniform formulation for orbit computation: the intermediate elements*,
  inspected 2026-10-15: the Sundman time transformation, Stumpff-series definition and its
  convergence for every argument, and the conic-independent motivation for universal
  variables.
- Local MATLAB R2025b's `matlabshared.orbit.internal.TwoBodyKeplerian` was located, but the
  installed implementation is a protected `.p` file. No inspectable MATLAB universal-variable
  source was available, so MATLAB is not claimed as an A3 implementation oracle.

A3 does not solve Lambert boundary-value transfers, perturbed motion, ephemeris/frame/time
standards, collision trajectories, or rectilinear conics. The public state constructor can
represent radial data for other diagnostics, but this propagator rejects it deliberately to
match its nondegenerate conic contract.

### Phase A4: zero-revolution Lambert transfer

Status: **implemented**.

A4 solves the two-point, fixed-time two-body boundary-value problem. `lambert(start, finish,
flight, mu, options)` returns the departure and arrival velocity fields required to travel
between two Cartesian position fields. A second overload accepts an explicit dimensionless
reference normal. The normal makes the transfer plane meaningful outside the XY plane;
`LambertOptions::sense` then selects the prograde or retrograde branch relative to that plane.
The default overload uses the positive Z direction.

The implementation uses Izzo's Lancaster-Blanchard `x` parameterization for the
zero-revolution family. Geometry is reduced to chord length, semiperimeter, and signed
`lambda`; the nondimensional time-of-flight equation uses Battin's hypergeometric series near
`x = 1`, Lagrange's form in the neighboring interval, and Lancaster's expression elsewhere.
One fourth-order Householder solve handles every batch lane, after which radial and transverse
terminal velocity components are reconstructed without converting through orbital elements.

`LambertResult` carries departure velocity, arrival velocity, the solved `x`, final residual,
lane-wise convergence mask, and iteration count. Strict mode rejects any unconverged lane;
diagnostic mode returns its best iterate. Non-positive flight time or GM, zero position,
collinear endpoints, an unusable reference normal, incompatible shapes/dtypes/platforms,
non-finite values, and invalid solver controls fail explicitly.

#### A4 reuse, differentiation, and performance audit

| Concern | Canonical owner used by A4 |
| --- | --- |
| fourth-order iteration and convergence cadence | `subsp::math::roots::householder` |
| converged-root reverse rule | the shared Householder implicit-function bridge |
| vector geometry, masks, broadcasting, and backend dispatch | `pluss::Series` |
| SI dimensions and state layout | `NatSci/common::Quantity` and A1 Cartesian contracts |
| endpoint verification | A3 universal-variable `advance(OrbitState, ...)` |

The Householder owner evaluates value and the first three `x` derivatives together so the
time-of-flight intermediates are reused. Once converged, it detaches the iteration history and
reattaches the root through the implicit equation. This gives
`dx/dp = -(partial F / partial p) / (partial F / partial x)` for captured Series parameters,
including flight time and transfer geometry, without retaining every solver iteration on the
reverse tape.

There is no transfer-lane or Cartesian-component host loop. Position fields may have arbitrary
leading batch axes, while flight time, GM, and the reference normal broadcast through Series
operations. The permanent verifier covers the standard Vallado transfer, propagation back to
the requested endpoint, prograde and retrograde motion, a non-XY transfer plane, rank-four
Float64 batches, strided Float32 input, reverse sensitivity, strict failures, and diagnostic
non-convergence.

The permanent CPU benchmark solves 32,768 rank-four transfers. On the 2026-10-15 development
run, one batched call took 86.631 ms median versus 110.067 ms for 16 leading-axis page calls, a
1.271x advantage with identical departure velocities. Timings are descriptive rather than test
thresholds; value parity is enforced before reporting them.

#### A4 reference audit

- D. Izzo, *Revisiting Lambert's Problem*, 2015: the signed `lambda` geometry, nondimensional
  time-of-flight variable, initial estimates, Householder derivatives, and terminal-velocity
  reconstruction used as the primary algorithmic specification.
- ESA PyKEP `src/lambert_problem.cpp`, inspected 2026-10-15: an executable reference for the
  Battin/Lagrange/Lancaster branches, Householder update, transfer-sense handling, and the
  radial/transverse reconstruction formulas. PlussMa independently expresses those formulas as
  batched Series operations and delegates iteration to its shared root machinery.
- R. H. Gooding and the NASA universal-variable Lambert literature, including NASA report
  `19700027262`: branch geometry, conic-independent formulation, and failure-domain checks.
- Vallado, *Fundamentals of Astrodynamics and Applications*: the 3600-second Earth-transfer
  regression case and its published terminal velocities.
- Local MATLAB R2025b was searched across the installed toolbox tree. No inspectable orbital
  Lambert implementation was present, so MATLAB is not claimed as an A4 source oracle.

A4 intentionally exposes only the zero-revolution solution family. Multi-revolution transfer
has distinct left/right roots for each revolution count; adding a scalar revolution option
without a result contract that preserves both families would silently discard valid solutions.
That extension remains a separate phase rather than being approximated inside this API.

### Cross-domain quantum ownership

Quantum physics and quantum chemistry share state vectors, operators, normalization,
Hermiticity, eigensystems, density matrices, tensor products, partial traces, expectation
values, and unitary/open-system evolution. Those primitives belong to one reusable physics
quantum foundation. Chemistry will add molecular geometry, basis sets, electron integrals,
SCF, electronic energies, and molecular gradients while consuming that foundation; it
must not fork complex linear algebra, state validation, or propagation kernels.

### Phase Q1: shared quantum state and operator algebra

Status: **implemented**.

The neutral numerical owner is `natsci::common::quantum`. Both
`physics::quantum` and `chemistry::quantum` are public facades over the same `Ket`, `Op`, and
functional kernels. Physics can therefore add Hamiltonian evolution and open-system dynamics,
while chemistry can add molecular bases, integrals, SCF, and electronic structure without
forking Hilbert-space representation or density-matrix semantics.

`Ket` stores amplitudes with shape `[..., N]`; `Op` stores operators with shape
`[..., N, N]`. Every leading axis is a batch axis. Each wrapper also retains an ordered vector
of subsystem sizes whose product must equal `N`. This metadata prevents operations between
incompatible Hilbert spaces and makes tensor products and partial traces unambiguous. The
wrappers are lightweight Series handles plus small dimension vectors; Series remains the sole
owner of numerical storage.

The first functional surface provides `ket`, `op`, `norm`, `normalize`, `inner`, `prob`, `dag`,
`trace`, `is_herm`, `density`, pure- and mixed-state `expect`, `purity`, state/operator `tensor`,
and `partial_trace`. Complex inner products conjugate the left operand. `density` constructs
`|psi><psi|`; expectation uses `<psi|A|psi>` or `Tr(rho A)` according to the state form.
Partial trace interprets its index list as the subsystems to keep and always returns an `Op`,
including the one-dimensional trivial space when every subsystem is traced out.

#### Q1 centralization and performance audit

| Concern | Canonical owner |
| --- | --- |
| amplitude/operator storage, broadcasting, matmul, conjugation, autodiff | `pluss::Series` |
| Hilbert factorization, validation, and quantum contractions | `common::quantum` |
| physics naming and future dynamics | `physics::quantum` facade |
| chemistry naming and future electronic structure | `chemistry::quantum` facade |

There are no host loops over states, amplitudes, matrix entries, or batch lanes. Norms,
probabilities, density matrices, expectation values, tensor products, and traces are Series
expressions. Partial trace loops only over the small subsystem-index metadata; each subsystem
is contracted across every batch and matrix lane at once with an identity mask and Series
reductions. This also avoids depending on the current 2-D-only `Series::diagonal`
implementation. Complex finiteness is checked through real and imaginary Series components
because the lower-level complex `isfinite` predicate is not yet implemented.

The permanent verifier covers real and complex normalization, conjugating inner products,
Born probabilities, Pauli-Y expectation, pure-state density and purity, Hermiticity, state and
operator tensor products, both Bell-state reductions, batched Bell reductions across two leading
axes, a non-adjacent GHZ reduction that traces the middle subsystem, tracing every subsystem,
batched observables, non-contiguous state views, reverse sensitivity, and stable validation
failures. A chemistry-side executable constructs and checks a projector through the chemistry
facade, proving both domains consume the same implementation.

The permanent CPU benchmark evaluates an eight-level observable for 32,768 states with shape
`[16,16,8,16,8]`. Three warmed 2026-10-15 process runs measured 3.417-3.699 ms median per
batched call versus 9.386-9.594 ms for 16 leading-axis page calls, a 2.557-2.768x advantage
with identical values. Timings are descriptive; numerical parity is enforced before reporting
them.

#### Q1 reference audit

- QuTiP 5 `Qobj`, `states.py`, tensor, expectation, and partial-trace implementations and
  tests, inspected 2026-10-15: state/operator distinctions, retained subsystem dimensions,
  ket-to-density projection, conjugating overlaps, tensor ordering, and the contract that
  partial trace returns a density operator.
- QuTiP 5 user guide, *Using Tensor Products and Partial Traces*: subsystem ordering, tensor
  composition, Bell-state reductions, and keep-index semantics.
- Nielsen and Chuang, *Quantum Computation and Quantum Information*: Hilbert tensor products,
  density operators, Born probabilities, expectation values, reduced states, and purity.
- Local MATLAB R2025b was searched for inspectable quantum-state and partial-trace sources. The
  installed core tree exposes documentation/support-package references but no suitable source
  oracle, so MATLAB is not claimed as a Q1 implementation reference.

### Phase Q2: spectra, metrics, channels, and closed evolution

Status: **implemented**.

The spectral primitive is owned by `Series::linearalg`, not NatSci. `eigh()` and `eigvalsh()`
now accept real symmetric or complex Hermitian input with shape `[..., N, N]`. On CPU, the
entire contiguous batch crosses the host boundary once and LAPACK solves each page in place;
eigenvalues have shape `[..., N]`, are ascending, and retain the input's real precision, while
eigenvectors have shape `[..., N, N]` and retain the input dtype. MPS and CUDA use their
existing explicitly enabled CPU solver bridges and return results to the originating platform.

`common::quantum` consumes that general primitive through `spectrum` and `ground`. It adds
Hermiticity and Hilbert-space validation but does not own another eigensolver. The same spectral
path supplies positive-semidefinite square roots, von Neumann `entropy`, and mixed-state
fidelity. Density validation checks Hermiticity, non-negative spectrum within tolerance, and
unit trace. Its decomposition is returned to the caller and reused, so entropy and matrix square
roots do not diagonalize the same state twice.

The fidelity convention is explicit. `root_fidelity` returns
`Tr(sqrt(sqrt(rho) sigma sqrt(rho)))`, reducing to the absolute overlap for two kets.
`fidelity` returns its square, reducing to the squared overlap for two kets. Pure/mixed overloads
use the expectation identity rather than converting both arguments and invoking the general
matrix formula.

`Channel` is a validated Kraus family. Construction checks one shared Hilbert space/platform and
`sum K_i^dag K_i = I` once. Complete positivity follows from the Kraus form; `apply` evaluates
`sum K_i rho K_i^dag` with Series matrix products and supports pure or mixed inputs. The only
host loop visits the short Kraus metadata list; no state or matrix lanes are extracted.

For a constant Hermitian Hamiltonian, `unitary` computes `exp(-i H t / hbar)` through the
existing differentiable, batched `Series::linearalg.matrix_exp()`. `schrod` evaluates an entire
strictly increasing time grid at once. The first requested time is the initial-state origin, so a
grid beginning at a nonzero physical time does not spuriously pre-evolve the supplied state.
Real Hamiltonians and complex states are promoted through the canonical Series dtype table before
matrix multiplication.

Three warmed 2026-10-15 CPU process runs measured 512 complex Hermitian 4x4 spectra at
0.880-0.896 ms through one batched call versus 6.344-6.481 ms through 512 page calls, a
7.207-7.230x advantage. A 256-point eight-state Schrodinger path measured 12.967-13.101 ms in
one time-grid call versus 18.373-18.413 ms through separate propagators, a 1.403-1.418x
advantage. Every benchmark checks full numerical parity before reporting timing.

### Phase Q3: matrix-form Lindblad evolution

Status: **implemented for constant and time-dependent Hamiltonians and collapse operators**.

`lindblad` reuses `pluss::subsp::math::ode::fluxa` instead of adding a private integrator. Its
right-hand side remains in matrix form:

```text
d rho / dt = -i [H, rho] / hbar
             + sum_i (C_i rho C_i^dag
                      - 1/2 (C_i^dag C_i rho + rho C_i^dag C_i)).
```

This avoids materializing a dense `N^2 x N^2` Liouvillian. Historical fixed-step RK4 remains the
default so existing trajectories do not silently change. `SolveMethod::RK45` selects the shared
Dormand-Prince 5(4) controller with weighted RMS error, accepted/rejected-step accounting, exact
endpoint landing, and bounded step sizes. `SolveMethod::ImplicitMidpoint` selects adaptive step
doubling around a matrix-free Crank-Nicolson solve; each implicit system delegates to the shared
batched complex GMRES engine. Host control flow visits requested spans, adaptive decisions, and
the short collapse-operator list only. Matrix entries and batch lanes remain Series operations.

`OpFn` and `CollapseFn` make the Hamiltonian and collapse family functions of time. Requested
grid points are validated for Hermiticity, shared Hilbert space, platform, and dtype promotion;
the numerical stages then evaluate the callbacks at their actual stage or midpoint times. This
separates physical time dependence from solver policy instead of encoding a sampled operator
sequence into an interpolation convention. The solver preserves the computed trajectory rather
than silently renormalizing or projecting each step, allowing callers and tests to observe
numerical drift.

### Phase Q4: sparse and matrix-free propagation

Status: **implemented**.

`math::linear_operator::CooMap` is the reusable sparse owner. Its row and column vectors are
immutable Int64 structure; numerical values remain a floating or complex Series. Application is
implemented with batched `gather`, multiplication, and `scatter_add`, so it does not allocate a
dense matrix, loop over nonzeros on the host, or sever gradients with respect to sparse values.
The same map applies to every column of a batched matrix, and its adjoint swaps structural indices
and conjugates values. Quantum's `SparseOp` adds Hilbert-space metadata and an explicit Hermitian
contract but delegates all numerical action to that common map.

`math::krylov::gmres` supplies restarted, complex-capable matrix-free linear solves. Arnoldi bases,
Hessenberg coefficients, and independent batch lanes remain Series values on the originating
platform. Only restart-boundary convergence decisions synchronize with the host. The small
Hessenberg least-squares problem uses the existing differentiable batched pseudoinverse, so
implicit solves retain reverse links through operator applications.

For a constant sparse Hamiltonian, `SolveMethod::Krylov` evaluates the exponential action through
an Arnoldi projection rather than integrating many small ODE steps. It materializes only the small
projected Hessenberg matrix, returns the final-subdiagonal residual estimate, and fails if the
requested tolerance is not met. RK4, RK45, and implicit midpoint remain available for sparse
Hamiltonians; time-dependent operators deliberately use those actual-time integrators rather than
pretending one exponential represents a varying generator. Sparse Lindblad evolution composes
left and right COO actions directly and still never forms the dense Liouvillian.

The permanent verifier now includes a commuting time-dependent Hamiltonian with a closed-form
rotation, a time-dependent damping rate with analytic survival probability, sparse and dense
Schrodinger parity, sparse amplitude damping, a stiff damping mode through implicit midpoint,
nonsymmetric/complex/batched GMRES, differentiability of GMRES and COO values, reverse-time RK45,
and oversized-step rejection. These augment the original constant amplitude-damping and trace
preservation checks.

The focused verifier also covers complex batched eigensystems and reconstruction, precision and
shape retention, pure and maximally mixed entropy, pure/pure and pure/mixed and mixed/mixed
fidelity, unitary closure, nonzero time origins, Kraus completeness and amplitude damping,
non-Hermitian spectra, invalid density operators, and repeated-time rejection. The chemistry
facade exports the identical engine.

#### Q2-Q3 reference audit

- LAPACK `DSYEV`/`ZHEEV` contracts and the local Accelerate headers: triangle selection,
  workspace query, column eigenvectors, convergence reporting, and ascending eigenvalues.
- QuTiP 5 `core/metrics.py` and `entropy.py`: pure-state shortcuts, the unsquared root-fidelity
  convention, PSD spectral truncation, and von Neumann entropy from density eigenvalues.
- QuTiP 5 `solver/sesolve.py`, `solver/mesolve.py`, and the dynamics guide: `-iH` as the
  Schrodinger generator, requested-time state trajectories, collapse-operator semantics, and the
  matrix-form Lindblad alternative to a superoperator.
- Gorini, Kossakowski, and Sudarshan (1976), and Lindblad (1976): the completely positive,
  trace-preserving Markovian generator used by the solver.
- Nielsen and Chuang, *Quantum Computation and Quantum Information*: unitary propagation,
  operator-sum channels, von Neumann entropy, and state fidelity.
- Local MATLAB R2025b was searched again for inspectable quantum solver code. No suitable source
  oracle is installed, so MATLAB is not claimed as a Q2-Q3 implementation reference.

#### Q4 and solver reference audit

- Local PyTorch source at `torch/csrc/autograd/FunctionsManual.cpp` and
  `tools/autograd/derivatives.yaml`, inspected 2026-10-15: Hermitian eigensystem reverse formula,
  conjugate phase-invariance requirement, and eigenvector-gradient failure at repeated spectra.
  Pluss uses an independent implementation and keeps eigenvalue-only gradients valid at repeated
  eigenvalues.
- Local SciPy `sparse/linalg/_isolve/iterative.py`, inspected 2026-10-15: restarted complex GMRES,
  conjugating inner products, modified Gram-Schmidt, true-residual convergence, and breakdown
  treatment. Pluss batches coefficients as Series and solves the projected least-squares problem
  with its existing pseudoinverse rather than reproducing SciPy's scalar Givens loop.
- Local SciPy `sparse/linalg/_expm_multiply.py`, inspected 2026-10-15: matrix-free exponential
  action contract, multiple requested times, shape behavior, and the Al-Mohy/Higham algorithm
  provenance. Pluss currently uses an Arnoldi projected exponential with a residual estimate, not
  SciPy's trace-shifted Taylor implementation.
- Local SciPy `integrate/_ivp/rk.py`, `radau.py`, and `bdf.py`, plus local MATLAB R2025b
  `explicitRungeKutta.m` and `ode15sImpl.m`, inspected 2026-10-15: embedded-error control,
  accepted/rejected steps, endpoint handling, and separation between explicit and stiff solver
  contracts. Pluss's stiff route is adaptive implicit midpoint with GMRES, not an unclaimed Radau,
  BDF, or `ode15s` clone.

The former limits on time dependence, fixed RK4, dense-only evolution, and first-order
eigensystem differentiation are closed. Remaining deliberate boundaries are full-spectrum dense
operations such as entropy, fixed Hilbert dimensions over one solve, no event/discontinuity
schedule for operator callbacks, and no preconditioner in quantum's implicit route. These are
stated future phases rather than permissive options with weaker behavior.

### Phase QC1: restricted Hartree-Fock over AO integrals

Status: **implemented**.

The first chemistry-specific electronic-structure layer is
`chemistry::quantum::rhf`. It accepts atomic-orbital overlap, core-Hamiltonian, and
electron-repulsion integrals plus nuclear repulsion energy. Integrals use chemists' notation,
`(mu nu | lambda sigma)`, with all leading dimensions treated as broadcast batch dimensions.
The solver is deliberately downstream of integral generation: molecular geometry, basis parsing,
and Gaussian integral evaluation can evolve independently without creating another SCF loop.

`rhf` implements closed-shell, even-electron Roothaan-Hall iteration. The overlap metric is
diagonalized once to construct the symmetric orthogonalizer `S^-1/2`; every Fock solve then uses
the existing batched `Series::linearalg.eigh`. Coulomb and exchange matrices are rank-six-capable
Series contractions over the two density axes. Occupied-orbital density construction, total and
electronic energy, RMS density change, and per-lane diagnostics are likewise whole-Series
expressions. No AO-index or batch-lane host loop exists. The sole host loop controls SCF
iterations, and convergence synchronizes only the aggregate boolean needed to stop that loop.

The public `AoIntegrals`, `HfOptions`, and `RhfResult` records preserve dtype, platform, batch
shape, final Fock and density matrices, metric-orthonormal coefficients, orbital energies,
per-lane convergence, energy/orbital-gradient residuals, and density-change diagnostics. A
second `rhf` overload accepts an initial density for restart or domain-specific guess policy. The
default core-Hamiltonian guess and
optional linear damping are policies of this functional solver; no stateful SCF object or second
numerical path exists.

Contract validation rejects undefined, non-floating, cross-platform, nonsquare, nonfinite, or
nonbroadcastable inputs; odd or overfilled electron counts; non-symmetric one-electron matrices;
non-positive overlap metrics; and, by default, ERIs that violate bra, ket, or pair permutation
symmetry. The symmetry audit can be disabled only when a producer already guarantees the same
contract and validation cost is material.

The permanent verifier covers the shared chemistry/physics quantum facade, an analytic
one-orbital system, reverse sensitivities with respect to overlap, core, ERI, and nuclear energy,
a non-orthogonal two-orbital oracle, metric orthonormality, density restart, unequal singleton
batch broadcasting through non-contiguous expanded views, lane-specific nuclear energies, and
strict spin/metric/ERI failures.

#### QC1 centralization audit

| Concern | Canonical owner |
| --- | --- |
| storage, broadcasting, reductions, contractions, platform, reverse history | `pluss::Series` |
| symmetric/Hermitian batched eigensystems and their reverse rule | `Series::linearalg.eigh` |
| reusable state/operator algebra and propagation | `natsci::common::quantum` |
| AO-integral convention, closed-shell density, J/K build, SCF policy | `natsci::chemistry::quantum` |
| molecular basis and integral production | deferred chemistry producer layer |

#### QC1 reference audit

- PySCF `pyscf/scf/hf.py` on the upstream master branch, inspected 2026-10-15: separation of
  `get_fock`, generalized eigensolve, occupation, density construction, effective potential,
  electronic energy, and dual energy/orbital-gradient convergence in the SCF kernel. PlussMa uses
  its own Series-native contracts and does not copy PySCF's object hierarchy or host-array loop.
- Psi4 Hartree-Fock documentation, inspected 2026-10-15: chemists'-notation ERIs, the
  Roothaan-Hall generalized eigenproblem, restricted closed-shell specialization, symmetric
  orthogonalization, core-Hamiltonian guess, energy/density convergence, damping/DIIS roles, and
  in-core versus direct/density-fitted J/K construction boundaries.
- The existing PlussQit batched `eigh` implementation and Q2 reverse tests were audited before
  construction. QC1 delegates to that owner rather than adding a chemistry eigensolver.
- The two-orbital regression vector was independently evaluated from the stated matrices with a
  separate scalar 2x2 generalized-eigen/SCF script. The one-orbital energy and all four reverse
  sensitivities have closed analytic forms, so neither test merely compares the implementation
  with itself.

QC1 intentionally starts from supplied dense AO integrals and restricted closed shells.
Unrestricted/open-shell references, direct and density-fitted J/K builders, basis/integral
generation, analytic nuclear gradients, and post-HF methods are later chemistry phases; they are
not represented as inert options on this solver.

### Phase QC2: Pulay CDIIS and orbital-gradient convergence

Status: **implemented**.

QC2 adds Pulay's commutator direct inversion in the iterative subspace to the single RHF
solver; it does not introduce a second SCF implementation. Each raw Fock matrix is paired with
the orthogonalized AO residual `X^T(FDS - SDF)X`. The bounded residual history forms one batched
Gram matrix per leading lane, and the augmented Pulay system is solved through the existing
batched `Series::linearalg.pinverse`. A relative singular-value cutoff handles linearly dependent
history vectors without diagonal bias. `diis_start`, `diis_space`, and `diis_rcond` expose only
policies that alter computation; setting `diis_space` to zero selects the unaccelerated fixed-
point path.

The CDIIS residual is not mislabeled as the physical stopping gradient. Convergence independently
evaluates the restricted occupied-virtual orbital gradient `2 C_vir^T F C_occ`, using the raw
Fock matrix generated by the candidate density. Its lane-wise RMS must satisfy
`gradient_tolerance` together with the total-energy change. A fully occupied finite basis has an
empty virtual block and therefore an exact zero orbital gradient. `density_rms` remains available
as a diagnostic but is not an alternate hidden stopping rule.

All AO, occupied, virtual, history-Gram, and batch-lane arithmetic remains in Series operations.
Host loops are bounded by the small DIIS history and assemble Series blocks; they never iterate
over AO entries or batch lanes. The permanent verifier covers accelerated and unaccelerated
agreement, restart, independent lane-wise Pulay solves, singleton batch broadcasting, strided
Float32 inputs, a full-occupancy zero-gradient case, reverse sensitivities through the solver,
bounded non-convergence, and invalid DIIS policies.

#### QC2 reference audit

- Pulay, *Convergence acceleration of iterative sequences. The case of SCF iteration*, Chemical
  Physics Letters 73 (1980), DOI `10.1016/0009-2614(80)80396-4`: residual-history extrapolation
  under the affine coefficient constraint.
- PySCF upstream `pyscf/scf/diis.py`, `pyscf/lib/diis.py`, and `pyscf/scf/hf.py`, inspected
  2026-10-15: the orthogonalized `SDF-FDS` CDIIS vector, bounded Fock/error history, augmented
  Pulay solve, separate raw-Fock orbital-gradient check, and dual energy/gradient convergence.
  PlussMa independently expresses these contracts as batched Series operations and uses its own
  pseudoinverse owner.
- Psi4 SCF `D_CONVERGENCE` documentation, inspected 2026-10-15: the density criterion is defined
  from the RMS or maximum orbital gradient, not from density-matrix change alone.

QC2 does not add EDIIS/ADIIS, level shifting, fractional occupation, or automatic retry policy.
Those strategies have distinct objective and state contracts and remain later SCF phases rather
than inert switches on CDIIS.

### Phase QC3: unrestricted Hartree-Fock and spin diagnostics

Status: **implemented**.

QC3 adds `chemistry::quantum::uhf` to the same AO-integral and SCF policy layer as `rhf`.
Callers provide alpha and beta electron counts explicitly. Each spin density uses unit orbital
occupation; the two Fock matrices share the Coulomb field of the total density while exchange
contracts only the same-spin density. The electronic energy is evaluated from both spin channels
without converting through an RHF density. `HfOptions` is now the one policy record shared by RHF
and UHF rather than two records that could drift.

UHF keeps one coupled Pulay problem. Each history entry contributes the sum of its alpha and beta
commutator inner products to the Gram matrix, and one affine coefficient vector extrapolates both
Fock histories. The stopping residual is the RMS of the concatenated alpha and beta occupied-
virtual Fock blocks. Empty occupied or virtual blocks contribute zero elements rather than a
fabricated gradient. Linear damping, bounded non-convergence, the overlap cutoff, and the
energy/gradient convergence pair retain the RHF meanings.

`UhfResult` exposes both spin-resolved orbital spectra, coefficients, densities, and Fock matrices,
along with total/electronic energy, density and gradient RMS values, energy change, convergence,
iteration/DIIS counts, and the determinant expectation value `<S^2>`. The spin diagnostic uses the
AO metric overlap between occupied alpha and beta orbitals, so it reports zero for a closed-shell
singlet, `3/4` for a one-electron doublet, and exposes spin contamination when opposite-spin
spatial subspaces separate. `UhfGuess` supplies both density matrices for restart without adding a
stateful solver object.

All AO contractions, metric transforms, spin-density construction, joint residual products,
energies, gradients, and spin diagnostics are Series expressions over complete batch lanes. The
only host loops remain the bounded SCF iteration and the small Pulay history assembly. UHF reuses
the RHF integral preparation, symmetric orthogonalizer, orbital solve, Coulomb/exchange kernels,
commutator residual, pseudoinverse-based Pulay solve, and final-orbital reporting.

The permanent verifier covers closed-shell RHF parity, a one-electron generalized-eigenvalue
oracle, empty-beta doublet spin, metric electron count, density restart, accelerated versus
unaccelerated agreement, broadcast batch lanes, joint DIIS, strided Float32 storage, analytic
reverse sensitivities, and invalid spin populations and restart densities.

#### QC3 reference audit

- PySCF upstream `pyscf/scf/uhf.py`, inspected 2026-10-16: separate unit-occupation spin density
  matrices; total-density Coulomb and same-spin exchange; spin-resolved occupied-virtual orbital
  gradients; UHF electronic-energy contraction; explicit `(n_alpha, n_beta)` occupations; and the
  occupied alpha/beta overlap formula for `<S^2>`. PlussMa independently expresses these contracts
  as batched Series operations and does not copy PySCF's object hierarchy or host-array kernels.
- PySCF upstream `pyscf/scf/diis.py` and the UHF `get_fock` route, inspected 2026-10-16: one CDIIS
  update over the coupled spin Fock/density state rather than unrelated alpha and beta solvers.
- Psi4 Hartree-Fock documentation, inspected 2026-10-16: UHF's independent real alpha/beta orbital
  spaces, orbital-gradient stationarity, DIIS as gradient-based convergence acceleration, and the
  distinction between a converged stationary point and a stability-certified minimum.
- The one-electron verifier uses the closed generalized characteristic polynomial
  `det(H - e S)` and analytic energy derivatives. Closed-shell UHF is separately required to match
  the existing RHF result, so neither numerical oracle is generated by the UHF implementation.

QC3 does not add ROHF, broken-symmetry initial-guess generation, stability Hessians, level shifting,
or direct/density-fitted J/K production. Those are separate physical or algorithmic contracts and
remain later phases rather than inactive fields on `HfOptions`.

### Phase P3: fluid dynamics

Status: **P3a-P3i implement and validate the periodic and bounded incompressible stack;
P3j-P3k provide periodic compressible Euler and Navier--Stokes--Fourier flow; P3l-a/b add
bounded inviscid and constant-transport thermal-wall gas flow; P3m-a/b add contact-resolving
HLLC fluxes and periodic WENO5 reconstruction; P3n-a/b add Sutherland transport and
conservative nonreacting species transport; P3n-c adds the composition-dependent ideal-gas
closure and conservative stiff homogeneous chemistry source; P3g supplies reproducible
throughput measurements.**

P3 is deliberately bounded to constant-density incompressible Newtonian flow on Cartesian
grids. It will provide a complete, verified core before NatSci moves to biology and
chemistry, but it will not disguise compressible shocks, turbulence closure, multiphase
flow, non-Newtonian constitutive laws, or unstructured meshes as options on one solver.

#### Phase P3a: reusable periodic spatial operators

`common::spatial` owns the discretization machinery shared by fluid flow, biological
diffusion, and chemical transport. The first tranche provides:

- validated one-, two-, and three-dimensional `PeriodicGrid` metadata with dimensioned SI
  spacing;
- centered first and second partial derivatives;
- scalar gradient and Laplacian;
- vector divergence and two-/three-dimensional curl;
- scalar transport `u dot grad(phi)` and vector self-advection `u dot grad(u)`; and
- dimension-aware quantity overloads deriving inverse-length, inverse-area, transport-rate,
  and acceleration dimensions at compile time.

Scalar fields use `[..., spatial axes...]`; vector fields use
`[..., component, spatial axes...]`. Every leading axis is a batch axis. Operators validate
the trailing spatial extents and component count without scanning field values. Numerical
work is expressed through `Series::roll`, selection, broadcast arithmetic, reductions, and
strict-dtype stacking. The only host loop visits the one-to-three axis/component metadata;
it never extracts or iterates grid cells. Consequently, non-contiguous storage, backend
dispatch, complex fields, and reverse sensitivities retain the canonical Series behavior.

The centered periodic first derivative has discrete symbol `sin(k h) / h`, while the
second derivative has symbol `-2(1-cos(k h))/h^2`. Verification compares against those
discrete symbols rather than demanding the continuous derivative at finite spacing. The
permanent verifier covers 1D, 2D, and 3D grids, anisotropic spacing, exact orientation of
3D curl, scalar and vector advection, arbitrary leading batches, transposed non-contiguous
fields, dimension algebra, reverse sensitivity, and malformed grid/layout failures.

P3a intentionally supports periodic boundaries only. Reusing `roll` here gives a compact,
backend-native correctness baseline. No-slip, free-slip, inflow, and outflow boundaries
require explicit face contracts and ghost reconstruction; they will be added in P3c rather
than silently approximated by periodic wrapping or zero padding.

#### Phase P3b: periodic incompressible projection

P3b adds three deliberately separated functional layers.

`subsp::math::krylov::cg` is the reusable matrix-free real conjugate-gradient engine. The
operator is a callable from `Series` to `Series`; solve axes identify each independent
linear system while every remaining axis is retained as a batch axis. Inner products,
residual updates, active-lane masks, and convergence diagnostics remain Series operations,
so one invocation solves arbitrary leading batches without materializing a matrix or
iterating over lanes on the host. It accepts a layout-matched initial estimate and returns
the solution, per-batch residual norm, relative residual, convergence mask, breakdown mask,
and actual iteration count. Non-positive/non-finite active curvature is reported as a
breakdown rather than divided through.

The default convergence check examines the aggregate active mask every eight iterations.
This bounds wasted work without forcing a device synchronization for every recurrence;
`check_interval = 0` executes the entire fixed budget asynchronously, while another positive
interval lets callers choose their latency/synchronization tradeoff. Individual lanes stop
changing as soon as they meet their own absolute-or-relative tolerance, even while harder
lanes continue.

`common::spatial::solve_poisson` owns periodic scalar Poisson semantics shared by fluids,
chemical transport, and spatial biology. A periodic Laplacian has the constant field in its
nullspace, so every batch lane must have a zero-mean right-hand side. The solver rejects an
incompatible lane, subtracts admissible floating-point mean drift, applies CG to the
positive-semidefinite negative Laplacian, and fixes the non-unique result to zero spatial
mean. Dimension-aware input maps a source dimension `D` to solution dimension `D * Area`.
The wrapper can require convergence with one final aggregate check or return masks for
caller-controlled handling.

`physics::fluids` owns the physical interpretation:

- `IncompressibleState` validates a real finite velocity field shaped
  `[..., component, spatial...]` and a pressure field shaped `[..., spatial...]`, including
  exact dtype, platform, batch, component, and grid agreement;
- `mom_rate` evaluates `-u dot grad(u) - grad(p)/rho + nu laplacian(u)` and has a
  shape-checked body-acceleration overload;
- `div_report` returns the full centered divergence field plus signed mean, RMS, and
  maximum absolute divergence independently for every batch lane; and
- `project` solves `wide_laplacian(phi) = (rho / dt) div(u*)`, updates
  `u = u* - (dt / rho) grad(phi)` with the centered gradient, accumulates `p = p + phi`,
  and reports divergence before and after projection together with the linear-solver
  diagnostics.

The projection pairs the centered divergence and centered gradient with their exact
composition as the Poisson operator, `wide_laplacian = divergence(gradient(.))`, whose
stencil is `(u[i+2] - 2 u[i] + u[i-2]) / (4 h^2)` and whose symbol is `-sin^2(k h) / h^2`
per axis. On a cell-centered grid this is second-order consistent, the centered
divergence of the projected velocity is zero to solver tolerance, and the pressure
correction uses the same gradient the momentum equation does.

*Revision (2026-10-12).* P3b first shipped a matched forward gradient / backward
divergence pair (whose composition is the compact three-point Laplacian) to avoid the
wide stencil's checkerboard modes. P3e's convergence gate showed that choice was only
first-order accurate on cell-centered data: the backward difference of an analytically
divergence-free field is `O(h)`, so every projection injected an `O(h)` correction (a
Taylor-Green vortex at `n = 32` had error `7.4e-2` independent of the step, halving only
with `h`). The matched pair is a staggered-grid idea applied to collocated storage. It
remains available as `fwd_grad`/`back_div` for genuinely staggered use, but the projection
no longer uses it, and `DivergenceScheme` was removed because only one divergence is
meaningful now. The wide operator decouples the even and odd cells of an even-extent axis,
so its null space also holds the alternating mode; `solve_poisson` therefore removes the
parity-class mean from the right-hand side and the solution (the null space is a tensor
product of the per-axis spaces), and a right-hand side carrying a checkerboard component is
rejected as incompatible. A compatible right-hand side never excites those modes, and
without that handling rounding contamination stalled CG. Velocity checkerboard modes lie in
the kernel of the centered divergence and are not controlled by the projection; viscosity
damps them, and an inviscid long-time run would need a filter (not provided).

The permanent P3b verifier covers a batched callable CG solve, preserved per-batch
diagnostics, non-positive-curvature failure, one- and two-batch Poisson recovery up to the
zero-mean gauge, transposed non-contiguous storage, zero and incompatible right-hand sides,
pure-gradient velocity removal, centered versus projection divergence layouts, momentum
dimensions, body acceleration, malformed state/density failures, and reverse sensitivity
through the matrix-free Poisson recurrence. P3a additionally verifies the exact discrete
identities `back_div(fwd_grad(phi)) == laplacian(phi)` and
`wide_laplacian(phi) == divergence(gradient(phi))`, the wide operator's symbol, its
checkerboard null space, and the wide Poisson solve's null-space compatibility handling on
even and odd extents.

P3b deliberately did not claim a general nonsymmetric Krylov suite, preconditioner stack,
variable density, staggered-grid storage, bounded pressure conditions, or a complete
Navier-Stokes time integrator. Bounded pressure now belongs to P3c; time integration remains
P3d rather than being hidden behind permissive periodic-solver options.

#### Phase P3c: bounded faces and pressure conditions

P3c adds one reusable bounded Cartesian contract rather than embedding boundary arithmetic
inside fluid stepping. `common::spatial::BoxGrid` describes one to three trailing,
cell-centred spatial axes. `Bounds<D>` requires exactly one dimension-checked rule on every
low and high face: a prescribed value with dimension `D`, or an outward-normal flux with
dimension `D / Length`. Missing, duplicate, and out-of-range faces fail while the record is
built, before any numerical kernel starts.

`ghosts()` reconstructs only the low and high face planes needed by one stencil axis. It
does not allocate a fully padded field and therefore does not invent corner values that a
one-axis stencil never reads. For a face value `g`, the cell-centred reflection is
`u_ghost = 2 g - u_inner`. For an outward flux `q`, both opposing faces use
`u_ghost = u_inner + h q`; the outward-normal orientation is already part of `q`, so adding
an axis-dependent sign again would be incorrect. Bounded first derivatives, second
derivatives, gradients, and Laplacians compose these face planes with `slice`, `unite`, and
broadcast arithmetic. Scalar, batched face profiles and non-contiguous fields remain on
their original dtype, platform, and reverse graph.

The bounded Poisson overload separates a non-homogeneous problem into an affine boundary
contribution plus one homogeneous matrix-free operator. That keeps conjugate gradient on a
linear symmetric operator while retaining prescribed value or flux data in the right-hand
side. An all-flux problem checks compatibility independently in every batch lane and fixes
the constant nullspace to zero spatial mean. Any value face removes that gauge freedom, so
the solution is returned without mean subtraction.

`physics::fluids::FlowBounds` is a semantic translator over the common engine:

- no-slip and inlet faces reflect every component about a prescribed wall/entry velocity;
- free-slip faces reflect the normal component and assign zero normal derivative to each
  tangential component;
- outlet faces assign zero normal derivative to every velocity component; and
- `p_bounds()` maps walls and inlets to zero pressure flux, while an outlet fixes pressure
  to its supplied value or zero.

Velocity targets accept a constant component vector or a broadcastable face profile. The
implementation materializes each face profile once, then assembles component rules through
Series selection and stacking. Its only host loops visit at most three components and six
face records; grid cells and batch lanes are never iterated or transferred to the host.

The permanent P3c verifiers cover value and outward-flux sign conventions, scalar and
batched profiles, 1-D and 2-D bounded stencils, transposed storage, affine and all-flux
Poisson solves, compatibility rejection, 2-D no-slip/free-slip/inlet/outlet reconstruction,
moving wall planes, leading velocity batches, 3-D normal/tangential orientation, outlet
pressure recovery, malformed face sets/profiles, and reverse sensitivity through both
scalar and vector ghost reconstruction.

#### Phase P3d: projection time stepping

`physics::fluids::advance` takes one step of the constant-density, constant-viscosity
equations on a periodic Cartesian grid; `evolve` marches many steps through
`common::solve::march` (C3), so trajectories reuse the existing scheduler and layout
rather than a second one. Both accept an optional body acceleration `BodyFn(time, state)`.

- `StepRule::Euler/Rk2/Rk3` are the strong-stability-preserving Runge-Kutta rules of Shu
  and Osher, written as convex combinations of forward-Euler stages; each stage is followed
  by a projection, so every stage velocity is discretely divergence free. Because the
  projection is linear and the stage velocities are divergence free, this is exactly the
  Runge-Kutta method applied to `u' = P F(u)` (`P` the Leray projection), which is why the
  full order of the rule survives. The stage times are `t, t+dt` (RK2) and `t, t+dt,
  t+dt/2` (RK3), and the body force is sampled at them.
- `step_limit` reports per-batch-lane convective (`C / sum_i max|u_i|/h_i`) and viscous
  (`D / (nu sum_i 1/h_i^2)`) limits and their minimum. `StepGuard` supplies `C`, `D`
  (default one half, a conservative forward-Euler bound that the SSP combinations inherit)
  and whether `advance` rejects a step above the limit (`enforce`, default true).
- `StepResult` carries the next state, the limits, the post-step divergence report, and the
  rate-evaluation and projection counts.
- **Initial data must be discretely divergence free.** A smooth analytic velocity is
  divergence free only to `O(h^2)` in the centered discrete sense, so the first stage would
  project that residual away in a `dt`-independent jump. Call `project` once on the initial
  state (any step; it only scales the discarded pressure increment). P3e's forced and
  non-gradient cases do this; skipping it turned measured RK2/RK3 temporal order from 2/3
  into 1.
- Internal stage projections skip the per-stage divergence reports and state validation
  (device synchronizations). The finished step is validated and reported once.

P3d does not provide implicit or adaptive stepping, variable density, or bounded domains.
The convection term is the centered non-conservative `u . grad(u)`; kinetic energy is
conserved only to the order of the discretization.

The permanent P3d verifier covers lane-wise convective and viscous limits, all three step
rules, exact constant forcing, non-autonomous forcing at every stage clock, stage and
projection counts, guard rejection, C3 trajectory stacking, and reverse sensitivity
through SSPRK3 and the pressure projection.

#### Phase P3e: periodic validation gates

`validation_test` is the convergence and conservation gate for everything above. Every
case compares with an analytic solution or a same-grid fine-step reference, never with a
number produced by the code under test.

- **Taylor-Green vortex** (2-D, exact): velocity error at `t = 0.4`, `nu = 0.1` falls with
  measured spatial orders `1.8`, `1.95`, `1.99` for `n = 8, 16, 32` (error `2.9e-4` at 32);
  kinetic energy follows `exp(-4 nu t)`; mean momentum is preserved to `1e-13`; the
  centered divergence of the result is below `1e-11`; pressure matches up to its gauge.
- **Temporal order** (same grid, `T = 1`, against a `320`-step run of the same rule):
  measured `1.05`, `2.02`, `3.02` for Euler, RK2, RK3, both for the unforced vortex and
  for a manufactured non-gradient flow.
- **3-D Beltrami (ABC) flow** (`A = B = C = 1`, curl `u = u`): exact decay `e^{-nu t}`,
  second order in space between `n = 8` and `16`, divergence below `1e-10`.
- **Manufactured forced flow** (2-D): a two-mode stream-function velocity that is not a
  Navier-Stokes solution, made one by the analytic body force
  `f = u_t + u . grad(u) + grad(p) - nu laplacian(u)`; second order in space and
  the Euler/RK2/RK3 orders in time with a time-dependent force.
- **Reverse sensitivity:** the derivative of final kinetic energy with respect to the
  initial amplitude, through the stepper and the matrix-free pressure solves, agrees with a
  central finite difference.

The gate found the first-order projection defect described under P3b. Without it the
unit tests (which pinned exact discrete identities, not convergence) all passed.

#### Phase P3f-a: bounded projection and stepping

The bounded path now uses the same public `project`, `advance`, `step_limit`, and `evolve`
contracts as the periodic path, with `FlowBounds` selecting the spatial policy. There is
one SSP stage engine and one C3 trajectory scheduler; periodic and bounded overloads only
supply their rate, projection, and final-divergence operations. This prevents the RK stage
clocks, counters, stability checks, or trajectory layout from diverging between domains.

The bounded projection does not substitute the compact scalar Laplacian for the actual
cell-centred velocity correction. It constructs the matrix-free operator from the same
homogeneous ghost-reconstructed divergence and pressure gradient used by the update,
`D(G(phi))`, and solves `-D(G(phi)) = -rho div(u*) / dt`. Consequently the corrected
velocity satisfies the discretized divergence equation to the Krylov tolerance. Physical
velocity and pressure values enter the right-hand side, while pressure corrections and
velocity increments use homogeneous versions of the same face kinds. Closed/all-flux
domains check net-flux compatibility per batch lane and fix the pressure-correction gauge
to zero mean; a pressure outlet removes that nullspace.

Bounded momentum uses Series-native ghost planes for `u dot grad(u)` and `laplacian(u)`,
the existing dimensioned bounded pressure gradient, and the same optional body-force
callback as periodic stepping. Grid cells and batch lanes are never visited on the host.
The only loops traverse at most three semantic spatial axes. Stage validation remains at
the completed-step boundary, avoiding repeated finite/layout device synchronizations.

The permanent P3f-a verifier covers a deliberately divergent closed-box field, matched
projection convergence, rejection of incompatible inlet flux in a closed pressure
problem, two batched body-driven channels, exact preservation of the discrete steady
Poiseuille profile under SSPRK3, convergence of that profile toward the continuum
parabola, per-lane stability limits, bounded C3 trajectories, and reverse sensitivity
through the bounded RK/projection composition. When the MPS backend is available, the same
verifier runs a Float32 bounded projection on Metal, checks that no implicit host transfer
occurred, and applies a backend-appropriate divergence tolerance. The discrete equilibrium
is distinguished from the continuum profile because a reflected cell-centred wall ghost
changes the first cell's second-difference equation by an `O(h^2)` correction; the test
does not conceal that boundary truncation error behind a loose equality.

#### Phase P3f-b: bounded validation depth

`physics::fluids::settle` centralizes steady-state marching for periodic and bounded
domains. It reuses `advance` rather than owning another time integrator and tests the
dimensioned residual `max(abs(u[n+1] - u[n])) / dt` independently in every leading batch
lane. Absolute acceleration tolerance is combined with a relative tolerance based on the
first inspected residual. A lane must satisfy that threshold for `required_checks`
consecutive inspections; one fortuitously small update is not called convergence. The
result retains the final state, residual, convergence mask, physical time, step count, and
exact rate-evaluation and projection counts. Invalid budgets, inspection intervals, streak
lengths, tolerances, and time steps fail before marching. Exhaustion either returns
explicit unconverged lanes or throws, according to `require_convergence`.

The transient plane-Poiseuille gate starts from rest, drives two forcing lanes in one
Series-native solve, and compares the numerical streamwise velocity with the analytic odd
Fourier series at `t = 0.2`, `nu = 0.1`. With RK2 and `dt <= 0.5 h^2`, maximum errors at
`ny = 8, 16, 32` are respectively `1.2514e-2`, `4.05951e-3`, and `1.11962e-3`; refinement
reduces error by factors `3.08` and `3.63`. The gate also requires zero invented
cross-stream velocity and discrete divergence below `1e-11` at every resolution.

The lid-driven-cavity gate solves the unit square at `Re = 100` with a unit top-wall speed,
three stationary no-slip walls, RK2, and `dt = 0.32 / n`. Steady convergence requires two
successive residual checks below `2e-4 + 1e-4 r0`. The `n = 8` run converges in 350 steps
with residual `1.06680e-4`; `n = 16` converges in 650 steps with residual `1.17726e-4`.
Maximum centerline discrepancies from the Ghia tables fall from `0.318566` to `0.0453123`
for horizontal velocity and from `0.285518` to `0.042785` for vertical velocity. Samples
are linearly interpolated on cell centres. Near-wall Ghia coordinates outside the outermost
cell centre use explicit linear extrapolation rather than silently clamping to an interior
value. This is a two-grid accuracy gate, not a claim that a 16-square mesh reproduces the
benchmark's high-resolution solution pointwise.

*Performance record (arm64 macOS CPU, `Float64`, `n^2` cells, RK3, 8 CG iterations per
projection).* Profiling a `128^2` step found `roll` at about `13 ns` per element (a
gather through an index vector), the building block of every periodic difference. CodeM
`roll` now copies two contiguous blocks per outer slice on the CPU (reverse rule: roll by
the opposite shift; parity tests against an `index_select` reference across shapes, dims,
shifts, dtypes, and non-contiguous input). Effect on a `128^2` step: `mom_rate` `6.2 ms ->
1.5 ms`, one projection `30 ms -> 12 ms`, a full RK3 step `99 ms -> 41 ms`. Per-stage cost is
now dominated by the matrix-free CG solve (about one half), each iteration being roughly
twenty full-array Series operations at allocation-bound speed. The next measured options,
each requiring its own benchmark gate before adoption, are a native fused stencil/CG
kernel and a spectral periodic Poisson solve (the wide operator is diagonal in Fourier
space with symbol `-sin^2(k h) / h^2`); neither exists yet because CodeM has no FFT.
Stage-by-stage skipping of diagnostics measured no gain.

#### Phase P3g: reproducible throughput harness

The opt-in `BUILD_BENCHMARKS` target measures periodic and bounded `mom_rate`, `project`,
Euler, SSPRK2, and SSPRK3 paths without placing benchmark machinery in the production
API. Setup is Series-native: the periodic velocity is generated from a random stream
function and is therefore exactly divergence free under the same centered discrete
operators used by the solver. Timed results are synchronized through one scalar checksum,
so asynchronous backends cannot report enqueue time as completed work. The harness reports
median and minimum wall time, physical cells per second, and stage-cell updates per second;
CSV output supports durable regression records.

The initial arm64 macOS CPU baseline uses `Float32`, a `64 x 64` grid, one warm-up, three
measured rounds, and the production CG inspection interval of eight iterations:

| Path | Periodic physical cells/s | Bounded physical cells/s |
| --- | ---: | ---: |
| `mom_rate` | 10,779,000 | 2,603,000 |
| `project` | 536,129 | 64,553 |
| Euler | 428,594 | 61,944 |
| SSPRK2 | 216,019 | 29,858 |
| SSPRK3 | 146,389 | 20,387 |

SSPRK2 and SSPRK3 additionally report stage-normalized rates, approximately 432,037 and
439,166 stage-cell updates/s for the periodic path and 59,716 and 61,161 for the bounded
path. This separates integrator stage count from a regression in one stage. The harness
also runs on MPS, but tiny `64 x 64` grids are launch-bound there and are not a NatSci
acceptance gate. Physics remains CPU-correctness-first; backend acceleration is adopted
only after a measured workload justifies a specialized kernel.

The 2026-10-18 extension adds complete compressible residual and step paths. It first rejects
non-finite reference residuals, then times HLLC-WENO, HLLC-WENO plus Sutherland transport, and
SSPRK3 with Sutherland transport on the same `64 x 64` Float32 state. Five measured rounds after
two warm-ups produced:

| Path | Periodic physical cells/s | Bounded physical cells/s |
| --- | ---: | ---: |
| HLLC-WENO residual | 247,073 | 162,703 |
| HLLC-WENO + Sutherland residual | 231,257 | 145,918 |
| Sutherland SSPRK3 | 75,077 | 48,039 |

The corresponding SSPRK3 rates are 225,231 periodic and 144,117 bounded stage-cells/s. The
bounded case includes no-slip adiabatic walls, three-layer WENO ghost construction, harmonic
variable conductivity, wall viscosity, and all face validation; it therefore measures the
production boundary path rather than a periodic proxy. The benchmark exposed a Float32 dtype
promotion in the fractional Sutherland power. The constitutive constants and exponent now use
rank-zero dtype/platform-matched Series, and a permanent Float32 regression guards coefficient
and residual dtype preservation.

#### P3a reference audit

- Chorin, *Numerical Solution of the Navier-Stokes Equations*, Mathematics of
  Computation 22 (1968), pp. 745-762, DOI `10.1090/S0025-5718-1968-0242392-2`:
  primitive-variable incompressible equations, finite-difference projection structure,
  and applicability in two and three dimensions.
- PETSc `src/ts/tutorials/ex46.c` (main source inspected 2026-10-12): governing 2D/3D
  incompressible equations and manufactured divergence-free velocity/pressure fields.
- PETSc Stokes guide (main documentation inspected 2026-10-12): independent quadratic
  divergence-free manufactured fields and pressure normalization.
- DOLFINx 0.7.2 `demo_navier-stokes.py`: velocity/pressure field contracts, exact mass
  conservation objective, upwind advection distinction, and zero-mean pressure convention.
- Local MATLAB R2025b `toolbox/pde` and `toolbox/matlab` trees were searched for an
  inspectable Cartesian Navier-Stokes implementation. None was present; MATLAB is therefore
  not claimed as a P3a source oracle.
- PlussQit `Series::roll` implementation and its shape, reverse-manifest, and routing tests:
  signed shift semantics, strided operation, autodiff ownership, and backend routing.

These references guide equations, conventions, and independent checks. P3a remains an
original Series-native implementation and does not reproduce their internal structures.

#### P3b reference audit

- MATLAB R2025b `toolbox/matlab/sparfun/pcg.m`, SHA-256
  `49bf981ce40ce9f0a4a3a4bdbd8ddc2d2d5265639ecec3a8488bf324fc8d4648`:
  callable matrix-vector products, zero-right-hand-side handling, initial-estimate residual,
  relative residual convention, positive-curvature checks, and explicit non-convergence
  outcomes.
- PETSc `src/ksp/ksp/impls/cg/cg.c` (main source inspected 2026-10-12): CG recurrence,
  zero initial estimate, unpreconditioned residual norm, invalid-dot/indefinite-operator
  detection, and separation of solver state from the matrix implementation.
- PETSc `KSPCG` manual page (v3.26 inspected 2026-10-12): the symmetric positive
  (semi-)definite operator contract and communication-aware reduction policy.
- PETSc `MatNullSpaceCreate` manual page (v3.26 inspected 2026-10-12): explicit constant
  nullspace representation and projection of vectors out of an operator nullspace.
- Chorin, *Numerical Solution of the Navier-Stokes Equations*, Mathematics of Computation
  22 (1968), pp. 745-762, DOI `10.1090/S0025-5718-1968-0242392-2`: pressure projection and
  primitive-variable incompressible splitting.
- PlussMa optimization-private `solve_spd_system_cg_operator_` and
  `solve_spd_system_cg_series_`: audited for reusable machinery. They are intentionally not
  reused because they flatten to one system, extract host scalars every iteration, and are
  coupled to SQP status and regularization policy.
- PlussQit `Series` reduction, broadcast arithmetic, logical mask, `roll`, conversion, and
  reverse implementations/tests: backend routing and derivative ownership for every
  primitive used by P3b. A broadcast-`where` reverse limitation discovered by the verifier
  was avoided centrally in CG with typed mask arithmetic rather than hidden in fluid code.

These sources establish equations and failure contracts. The batched mask recurrence,
matched projection stencil, dimensioned state, and Series-native implementation are original
to PlussMa.

#### P3c reference audit

- Local MATLAB R2025b
  `toolbox/pde/pde/+pde/@PDEModel/applyBoundaryCondition.m`, SHA-256
  `75e1b9bda7876569aca0bb085516eb1c0ce3fe297f660388b285fecd54e4381b`:
  explicit Dirichlet, generalized Neumann, mixed, default-zero-Neumann, and vectorized
  boundary-record contracts. Pluss uses a smaller Cartesian value/outward-flux contract;
  it does not reproduce MATLAB's object or parser implementation.
- PETSc `DMBoundaryType` manual page (v3.26 inspected 2026-10-12): separation between
  physical boundary policy and ghost locations, including the rule that ghosted storage is
  not itself a boundary value prescription.
- PETSc DMSTAG manual (v3.26 development documentation inspected 2026-10-12): regular
  one-to-three-dimensional Cartesian cells, face locations, stencil-width ghost regions,
  and the distinction between global physical unknowns and regular local ghost storage.
- PETSc `src/dm/impls/stag/tutorials/ex2.c` (release source inspected 2026-10-12):
  prescribed normal flow, zero normal derivative for free slip, and the explicit choice
  between a fixed pressure and a constant-pressure nullspace.
- DOLFINx 0.7.2 `demo_navier-stokes.py`: prescribed boundary velocity, singular pressure
  solve support, and zero-mean pressure normalization when pressure is determined only up
  to a constant.
- PlussQit `Series::select`, `slice`, `unite`, broadcast arithmetic, reshape, and reverse
  implementations/tests: rank-preserving reconstruction, non-contiguous views, backend
  dispatch, and graph ownership for the P3c composition. The audit exposed the existing
  one-element scalar-wrapper behavior; P3c deliberately uses true rank-zero
  `make_scalar_like` operands at scalar face planes.

These sources establish boundary semantics, pressure gauges, and independent failure
contracts. The typed face records, affine Poisson split, profile broadcasting, and
Series-native component translation are original PlussMa implementations.

#### P3d and P3e reference audit

Sources establish equations, stage contracts, and independent validation cases. Their
implementations were used as correctness references only; no code was copied.

- Shu and Osher, *Efficient implementation of essentially non-oscillatory shock-capturing
  schemes*, J. Comput. Phys. 77 (1988) 439-471, and Gottlieb, Shu, and Tadmor, *Strong
  stability-preserving high-order time discretization methods*, SIAM Review 43 (2001)
  89-112: the SSPRK2/3 convex-combination forms, stage times, and the forward-Euler
  stability inheritance behind `StepGuard`.
- PETSc `src/ts/impls/explicit/ssp/ssp.c` (main source inspected 2026-10-13): explicit
  SSP stage dispatch, time-step ownership, reusable work-state policy, and the separation
  between a time integrator and its right-hand-side callback. PlussMa retains its own
  three-stage projection implementation because each intermediate fluid state must satisfy
  the incompressibility constraint.
- PETSc `src/ts/tutorials/ex46.c` (release source inspected 2026-10-13): time-dependent
  two- and three-dimensional incompressible equations, explicit physical-time input, and
  manufactured divergence-free velocity/pressure/forcing fields.
- Local MATLAB R2025b `toolbox/matlab/funfun/ode23.m`, SHA-256
  `f9a3ca1b80ba36fe8bd52974332eb00d682e56f5ac9fd4c5a543ecbda08ee34b`:
  staged callable evaluation, distinct stage coordinates, output-coordinate construction,
  and solver statistics. Its adaptive Bogacki-Shampine algorithm was not reproduced;
  P3d uses fixed SSP rules and projects every stage.
- Chorin (1968), see P3a: the projection splitting each stage applies.
- Almgren, Bell, and Szymczak, *A numerical method for the incompressible Navier-Stokes
  equations based on an approximate projection*, SIAM J. Sci. Comput. 17 (1996) 358-369,
  and Harlow and Welch, Phys. Fluids 8 (1965) 2182: the collocated-versus-staggered
  trade-off. PlussMa chose the exact centered projection (wide stencil) over the
  approximate compact-Laplacian projection so the centered divergence of the result is
  zero; Rhie and Chow, AIAA J. 21 (1983) 1525, describe the decoupling this leaves for
  velocity checkerboard modes.
- Taylor and Green, Proc. R. Soc. Lond. A 158 (1937) 499-521: the exact 2-D vortex and
  its pressure. Arnold (1965) and Dombre et al., J. Fluid Mech. 167 (1986) 353-391: the
  ABC/Beltrami family; for `A = B = C = 1` curl `u = u`, so convection is the gradient of
  `|u|^2 / 2` and the Navier-Stokes solution is `e^{-nu t} u_0`, `p = -|u|^2 / 2`.
- Roache, *Code verification by the method of manufactured solutions*, J. Fluids Eng. 124
  (2002) 4-10: the forced-flow construction.
- Local MATLAB R2025b: no inspectable Navier-Stokes solver exists in `toolbox/pde` or
  `toolbox/matlab`, so MATLAB is not claimed as an oracle for P3d/P3e.
- Brown, Cortez, and Minion, J. Comput. Phys. 168 (2001) 464-499, is the standard reference
  on the accuracy of projection methods (pressure converges more slowly than velocity). It
  was not inspected here; the measured velocity orders follow from the linear-projection
  argument under P3d, and pressure order is not gated.

#### P3f reference audit

- PETSc `src/snes/tutorials/ex19.c` (main source inspected 2026-10-13): unit-square
  lid-driven-cavity geometry, rigid no-slip walls, moving-lid Dirichlet data, Cartesian
  five-point differences, centered gradients, and the explicit distinction between
  centered and upwind convection. PlussMa does not reproduce PETSc's nonlinear SNES
  residual or process-grid implementation.
- PETSc `src/ksp/ksp/tutorials/ex87.c` (main source inspected 2026-10-13): the
  velocity-pressure saddle-point structure of the 2-D lid-driven cavity, Taylor-Hood
  discretization, and explicit zero-mean pressure treatment. It confirms that pressure
  gauge policy belongs to the operator contract rather than post-processing.
- DOLFINx main `demo_navier-stokes.py` (official documentation inspected 2026-10-13):
  no-slip/moving-wall boundary separation, incremental pressure correction, and the use of
  homogeneous correction data when physical boundary values are non-zero.
- Ghia, Ghia, and Shin, *High-Re solutions for incompressible flow using the
  Navier-Stokes equations and a multigrid method*, J. Comput. Phys. 48 (1982) 387-411,
  DOI `10.1016/0021-9991(82)90058-4` (paper inspected 2026-10-13): unit-square cavity,
  Reynolds-number convention, uniform meshes, steady convergence expectations, and the
  published `Re = 100` centreline coordinates and velocities used by P3f-b's independent
  cavity gate.
- Local MATLAB R2025b `toolbox/pde`, `toolbox/matlab`, and related installed toolbox trees
  were searched for an inspectable finite-difference Navier-Stokes or channel-flow solver.
  None suitable was found, so MATLAB is not claimed as a P3f source oracle.
- PlussMa P3c bounded ghost, pressure-boundary, and affine Poisson machinery and P3d SSP
  scheduler were audited before implementation. P3f-a extends those owners; it introduces
  no duplicate grid, face, Krylov, or trajectory engine.

These sources establish the equations, boundary/gauge contracts, and future benchmark
data. The matched ghost-based `D(G)` projection, dimensioned API, batched Series
composition, and discrete Poiseuille verifier are original PlussMa implementations.

#### Phase P3h: conservative forms, material fields, and diagnostics

Status: **complete for periodic variable-property flow and periodic/bounded
constant-property diagnostics.**

P3h closes the broader gaps that fit the existing incompressible Cartesian solver without
silently changing its numerical contract:

- `common::spatial::vector_flux`, `tensor_div`, and `flux_div` provide reusable tensor
  calculus for fields laid out as `[..., row, column, spatial...]`. The implementation uses
  broadcast outer products and Series reductions; host loops visit only the one-to-three
  component axes.
- `FlowForm::{Advect, Conserve, Skew}` selects `u_j D_j u_i`,
  `D_j(u_i u_j)`, or their half-sum. `StepOptions::form` carries that selection through
  every SSP stage. The compatibility default remains `Advect`; selecting a more
  conservative form is explicit rather than a behavioral change to established runs.
- On periodic centered grids, `Skew` is discretely energy neutral because the centered
  derivative is skew-adjoint: the verifier checks `sum(u * C_skew(u))` to roundoff for a
  non-solenoidal field, not only for a specially prepared divergence-free case.
- Bounded conservative transport reuses `vel_ghosts` and differentiates the pointwise
  momentum flux reconstructed on each side. It does not allocate padded fields or invent a
  second boundary policy.
- `strain`, `vort`, `stress`, and `flow_stats` expose the symmetric velocity gradient,
  vorticity, Newtonian viscous stress, kinetic-energy density, enstrophy, dissipation,
  maximum speed, and the established divergence report. Summaries preserve every leading
  batch axis and retain physical dimensions.
- Periodic variable-property overloads accept cellwise density and dynamic viscosity.
  Momentum uses `rho^-1 div(2 mu S)` rather than multiplying a constant-coefficient
  Laplacian by a varying scalar. Projection solves the weighted matrix-free equation
  `div(rho^-1 grad(phi)) = div(u*) / dt`, then applies
  `u = u* - dt rho^-1 grad(phi)`. The same batched CG owner and centered null-space policy
  are reused.
- `step_limit`, `advance`, and `evolve` accept those material fields. The explicit viscous
  guard uses the maximum cellwise kinematic viscosity `mu/rho` independently in each batch
  lane.

The permanent `flow_forms_test` verifies tensor layout and values, two leading batch axes,
the skew-energy identity, bounded ghost reuse, all three forms, dimensional stress and
diagnostic reductions, quadratic batch scaling, reverse sensitivity, malformed tensor
rejection, constant-property recovery in the inviscid limit, nonuniform stress divergence,
weighted-gradient removal by the variable-density projection, zero-density rejection,
variable-property stability limits, and an RK2 projected step.

The compact constant-viscosity diffusion and centered stress-divergence diffusion are not
claimed to be algebraically identical at finite grid spacing. The former uses the compact
three-point Laplacian and damps checkerboards; the latter is the physical symmetric-stress
form and composes centered first derivatives into the wide stencil. They converge to the
same incompressible continuum operator, but forcing an exact discrete equality would erase
an intentional stability distinction.

References read for P3h:

- Reiss and Sesterhenn, *A conservative, skew-symmetric Finite Difference Scheme for the
  compressible Navier--Stokes Equations*, arXiv:1308.6672 and Computers & Fluids 101
  (2014), 208-219: split convection, skew-adjoint kinetic-energy rationale, and composition
  from standard derivatives plus pointwise products.
- Almgren, Bell, Colella, Howell, and Welcome, *A Conservative Adaptive Projection Method
  for the Variable Density Incompressible Navier-Stokes Equations*, JCP 142 (1998), LBNL
  publication abstract: variable-density projection structure and conservation scope.
- Guermond and Quartapelle, *A Projection FEM for Variable Density Incompressible Flows*,
  JCP 165 (2000): the density-weighted pressure projection contract.
- PETSc, official *Guide to the Stokes Equations using Finite Elements*: Newtonian
  symmetric stress `mu (grad u + grad u^T)`, pressure coupling, and incompressibility.
- DOLFINx, official `demo_stokes.py` documentation: mixed momentum/divergence signs,
  pressure null space, and boundary formulation.
- Local PlussMa `common/field.h`, `common/boundary.h`, `common/poisson.h`, and
  `subsp/math/krylov.h`: reusable Series-native stencils, ghost planes, gauge handling, and
  batched matrix-free solve machinery. These were adapted in place rather than copied into
  a fluid-only stack.

#### Phase P3i: bounded materials and stiff time scales

Status: **complete for bounded variable-density/variable-viscosity flow, adaptive
constraint marching, and scalar-viscosity first-order IMEX diffusion.**

P3i extends the existing owners instead of creating a second bounded solver:

- Bounded variable-density projection solves the matched matrix-free equation
  `D(rho^-1 G(phi)) = D(u*) / dt`. The correction uses the same density-weighted gradient,
  homogeneous linearized flow bounds, and ghost-based divergence as the operator. The
  permanent verifier constructs an exact weighted gradient and requires both velocity
  removal and pressure-gauge recovery.
- Bounded variable viscosity uses conservative face stresses. Harmonic means carry
  dynamic viscosity to faces, normal gradients come from the established velocity ghost
  rules, and whole-field Series expressions assemble `div(2 mu S)`. Host loops visit only
  the one-to-three geometric axes and components; no cell or batch lane is scalarized.
- Material-field overloads now remain intact through `mom_rate`, `stress`, `project`,
  `step_limit`, `advance`, `evolve`, and `settle` on a `BoxGrid`.
- `common::solve::march_to` is the reusable variable-width scheduler for sequential
  constrained systems. It validates every proposed width, clamps the last accepted width
  to the exact stop coordinate, detects floating-point non-progress, preserves trajectory
  schema, and fails if the step budget is exhausted.
- `fluids::evolve_to` obtains a single shared step from the minimum lane-wise CFL/diffusion
  limit, safety factor, and user bounds. A final remainder may be smaller than `min_step`;
  an unstable interior step may not.
- `StepRule::Imex1` keeps transport, pressure, and forcing explicit, solves diffusion with
  a matrix-free backward-Euler Helmholtz operator, and then applies the established
  projection. Periodic and bounded scalar-viscosity paths are supported. Material-field
  viscosity fails explicitly rather than silently using the wrong implicit operator.
  Because diffusion is implicit, enforcement uses the convective limit only. The result
  reports Helmholtz Krylov iterations separately from rate evaluations and projections.
- `math::krylov::pcg` is the single preconditioned-CG owner; `cg` delegates to it with the
  identity preconditioner. `PoissonOptions::pc` selects no scaling or diagonal scaling.
  Variable-density projection forms the exact vectorized interior diagonal of the periodic
  `-D(rho^-1 G)` operator and a positive local bounded approximation. The batched verifier
  requires an exact diagonal system to collapse from multiple iterations to one and checks
  that pressure scaling preserves the variable-density correction.

The verifier covers a bounded weighted-gradient projection, nonuniform face stress,
material-field stepping and trajectories, exact-stop adaptive coordinates, explicit
viscous-limit rejection, and the analytic backward-Euler amplification of a periodic
shear Fourier mode.

References read for P3i:

- Almgren, Bell, Howell, and Colella, *An Adaptive Projection Method for the
  Incompressible Navier-Stokes Equations* (LLNL/LBNL report): variable-density momentum,
  density-weighted projection, and the separation of prediction from projection.
- Hou and Wetton, *Second-Order Convergence of a Projection Scheme for the Incompressible
  Navier-Stokes Equations with Boundaries*, SIAM J. Numer. Anal. 30 (1993), DOI
  `10.1137/0730030`: matched near-boundary divergence and pressure treatment.
- PETSc, official *Guide to the Stokes Equations using Finite Elements* and DMStag
  variable-viscosity examples: the Newtonian `div(mu (grad u + grad u^T))` operator,
  free-slip traction interpretation, and sharply varying viscosity tests.
- Minion, *Higher-Order Semi-Implicit Projection Methods* (2001), and the Sandia report
  SAND2004-3742, section 4.1: explicit convection, implicit diffusion, and projection as
  distinct stages, including the removal of the explicit viscous stability restriction.
- PETSc `TSARKIMEX` official manual: stiff/non-stiff operator separation and solver work
  accounting. P3i deliberately implements only the auditable backward-Euler IMEX member;
  it does not claim the higher-order PETSc ARK catalogue.
- Local PlussMa `common::solve::march`, `math::krylov::cg`, bounded ghost reconstruction,
  and projection kernels: reused owners for trajectories, batched linear solves, and
  boundary semantics.
- PETSc `KSPCG` and `PCJACOBI` official manual pages and implementation links: SPD
  requirements for left-preconditioned CG, residual/scaled-residual recurrences, and
  diagonal scaling semantics. The Pluss implementation retains its lane masks and
  convergence diagnostics rather than copying PETSc code.

#### Phase P3j: periodic compressible ideal-gas dynamics

Status: **complete for inviscid, calorically perfect gas flow on periodic Cartesian grids.**

`physics::fluids::gas` is a separate conservative solver rather than a mode on the
incompressible state. Its dimensioned state stores density `rho`, momentum density `rho u`,
and total-energy density `E`; pressure is recovered from
`p = (gamma - 1) (E - |rho u|^2 / (2 rho))`. Construction and every accepted Runge--Kutta
stage reject non-finite values, non-positive density, and non-positive pressure.

The spatial operator is a finite-volume flux divergence over one to three trailing grid
axes. Piecewise-constant or minmod-limited linear reconstruction supplies left and right
face states. A face whose reconstructed state is inadmissible falls back as a whole to its
cell-average pair instead of clipping individual conserved variables. The Rusanov local
Lax--Friedrichs flux uses the maximum left/right acoustic speed. All cells, components, and
leading batch axes remain in `Series`; the only host loop visits the at-most-three geometric
axes (and the at-most-three momentum components when assembling a physical flux).

Forward Euler, SSPRK2, and SSPRK3 update the same conservative rate. The multidimensional
acoustic limit is `CFL / sum_d ((|u_d| + c) / dx_d)`, reduced only over spatial axes so every
batch lane retains its own limit. `step` may enforce that limit; `evolve` builds a fixed-step
trajectory, while `evolve_to` uses the shared `common::solve::march_to` scheduler, takes the
minimum safe lane width, and lands exactly on the requested terminal time.

The permanent verifier covers an analytic two-dimensional batched CFL, exact preservation
of uniform states, periodic conservation of mass/momentum/total energy, fixed trajectory
accounting, invalid models/states/rules/steps, and reverse sensitivity through a conservative
Euler step. The Sod problem uses 200 cells, gamma 1.4, `(rho,p)_L=(1,1)`,
`(rho,p)_R=(0.125,0.1)`, and zero velocity. At `t=0.1`, the numerical left-star average is
checked against the analytic `rho=0.426319`, `u=0.927453`, `p=0.303130`; positivity and exact
adaptive stop time are checked independently.

References read for P3j:

- Sod, *A Survey of Several Finite Difference Methods for Systems of Nonlinear Hyperbolic
  Conservation Laws*, JCP 27 (1978), DOI `10.1016/0021-9991(78)90023-2`: Euler shock-tube
  initial data, exact wave pattern, and reference state used by the verifier.
- van Leer, *Towards the Ultimate Conservative Difference Scheme V. A Second-Order Sequel
  to Godunov's Method*, JCP 32 (1979): conservative piecewise-linear reconstruction and
  limiting at discontinuities.
- Rusanov, *The Calculation of the Interaction of Non-Stationary Shock Waves with
  Barriers*, USSR Comput. Math. Math. Phys. 1 (1962), DOI
  `10.1016/0041-5553(62)90062-9`: local maximum-wave-speed dissipative flux.
- Shu and Osher, *Efficient Implementation of Essentially Non-Oscillatory Shock-Capturing
  Schemes*, JCP 77 (1988), DOI `10.1016/0021-9991(88)90177-5`: the two- and three-stage
  strong-stability-preserving Runge--Kutta compositions.
- FLASH 2.1 official user guide, section 16.1.1: independent multidimensional Sod setup and
  analytic-comparison practice for gamma 1.4.
- Local PlussMa `common::field`, `common::solve`, quantity/record/trajectory owners, and
  PlussQit `Series` operations: reused for grids, exact-stop marching, dimension safety,
  trajectory packing, batching, platform retention, and reverse graphs.

P3j by itself is not viscous compressible Navier--Stokes. P3k adds the Newtonian/Fourier
transport terms while retaining this Euler path as its exact zero-transport specialization.
Species/source terms, physical gas boundaries, entropy-stable/high-order Riemann solvers,
low-Mach treatment, and AMR remain separate work. Turbulence closure, non-Newtonian
constitutive models, interface-tracked multiphase flow, and unstructured meshes also remain
separate families.

#### Phase P3k: periodic compressible Navier--Stokes--Fourier flow

Status: **complete for constant Newtonian viscosity, physical bulk viscosity, and constant
Fourier conductivity on periodic one-, two-, and three-dimensional Cartesian grids.**

P3k extends the P3j conservative owner; it does not introduce a second gas state, time
integrator, trajectory type, or reconstruction path. `gas::Transport` contains dimensioned
SI shear viscosity `mu`, physical bulk viscosity `zeta`, and thermal conductivity `k`.
`gas::Model` now carries the dimensioned specific gas constant used to recover
`T = p / (rho R)`. The defaults preserve the former inviscid behavior: every transport
coefficient is exactly zero, and the rate kernel returns the existing Euler result without
performing diffusive arithmetic.

The viscous stress is
`tau = mu (grad(u) + grad(u)^T - 2/3 I div(u)) + zeta I div(u)`.
The public input is `zeta >= 0`, not the potentially negative second-viscosity coefficient;
this keeps validation aligned with physical dissipation. The conservative diffusive flux is
`(0, tau, tau.u + k grad(T))`. `gas::heat_flux` separately reports the physical Fourier
flux `q = -k grad(T)`, making the sign difference between heat flow and the conservative
right-hand flux explicit.

Cell-centred gradients and stresses are Series expressions. Face fluxes are centred
averages of adjacent cell values and their face differences telescope exactly on a periodic
grid, so mass, momentum, and total energy retain finite-volume conservation. All cells and
leading batch lanes remain inside Series kernels; host loops visit only the one-to-three
geometric axes and stress components. `common::spatial::sym_grad` is the single reusable
symmetric-gradient owner used by both compressible and incompressible constitutive paths.

`gas::limits` reports the acoustic, diffusive, and selected limits independently. The
diffusive guard uses the larger of `(4 mu / 3 + zeta) / rho` and
`k / (rho cv)`, with `cv = R / (gamma - 1)`, and the Cartesian explicit bound
`diffusion / (alpha_max sum(1 / dx_d^2))`. Every SSP stage receives the same combined
operator and is validated for positive density and pressure; adaptive evolution recomputes
the combined limit from each accepted state. Temperature, stress, heat flux, and the full
conservative rate are public diagnostics, retain dtype/platform/batch shape, and remain on
the Series reverse graph.

The permanent `compressible_viscous_test` requires exact zero-transport Euler recovery,
the independent discrete one-dimensional identities
`d(rho u)/dt = (4 mu / 3 + zeta) D(Du)` and
`dE/dt = k D(DT)`, correct Fourier-flux sign, two-lane temperature recovery, unmixed
three-dimensional shear diffusion, periodic conservation, the analytic explicit diffusion
limit, invalid-coefficient and unstable-step rejection, and reverse sensitivity through a
viscous step. The original Sod and Euler conservation verifier remains green independently.

References read for P3k:

- SU2 official *Governing Equations in SU2*, compressible Navier--Stokes section: the
  conservative convective/viscous flux split, Newtonian Stokes stress, and
  `tau.u + k grad(T)` energy flux sign.
- NASA Reference Publication 1232, *The Compressible Navier--Stokes Equations and
  Algorithms for Their Numerical Solution*, section 2.1: continuity, momentum, total
  energy, deviatoric stress, and heat-conducting-fluid scope.
- NASA AIAA 2015-0294, equations 1--5: an independent conservative-state, viscous-flux,
  ideal-gas pressure, temperature, and bulk-correction formulation.
- Local PlussMa `common::spatial`, dimensioned quantities, P3j finite-volume fluxes, and
  `common::solve::march_to`: audited and extended as the canonical grid, constitutive,
  state, and scheduler owners instead of being duplicated.
- Local MATLAB R2025b `toolbox/pde` and installed examples were searched for an inspectable
  compressible Navier--Stokes implementation. Only generic heat-conduction coefficient
  machinery was found, so MATLAB is not claimed as a P3k numerical oracle.

The next closure phases are deliberately separate: bounded compressible wall/inlet/outlet
fluxes and temperature-dependent transport; higher-resolution shock fluxes; scalable
multigrid; non-Newtonian rheology; LES/RANS closure; multiphase interfaces; AMR;
unstructured/curvilinear meshes; and MPI decomposition. This ordering keeps each new
physical or mesh contract verifiable instead of hiding unfinished behavior behind options.

#### Phase P3l-a: bounded inviscid compressible flow

Status: **complete for conservative Cartesian slip, prescribed-state, transmissive, and
prescribed-static-pressure faces; characteristic inlets/far fields and viscous thermal walls
remain P3l-b.**

P3l-a extends the same `gas::State`, finite-volume rate, SSP stages, CFL policy, trajectory,
and reconstruction owner from P3j. A `spatial::BoxGrid` overload changes only face topology:
interior faces remain one vectorized Series flux field, while each geometric end face is
replaced by the numerical flux between its adjacent cell and a boundary-generated exterior
state. Interior differences therefore telescope exactly, and changes in total conserved
quantities are attributable to physical boundary fluxes rather than periodic wrap.

`gas::Bounds` requires exactly one rule for every low/high face. `slip` reflects only normal
velocity, preserving density, pressure, and every tangential component. `fixed` accepts a
dimensioned primitive state and broadcasts scalar, component-vector, face-profile, or batched
face-profile data only when dtype, platform, and the resulting face layout match the interior.
`outlet(face)` is zero-normal-gradient transmission. `outlet(face, pressure)` mirrors static
pressure through the face while extrapolating density and velocity; it rejects a non-positive
exterior pressure instead of clipping it.

The API deliberately does not expose `far`, `inlet`, or `wall` labels yet. A prescribed
primitive state is not a characteristic subsonic inlet, a fixed exterior state is not a
far-field invariant boundary, and tangential velocity reflection in an inviscid Rusanov flux
is not physical no-slip shear. Likewise, nonzero transport is rejected for `BoxGrid` rather
than silently using inviscid ghost states for viscous stress or heat flux. P3l-b must add total
condition/mass-flow inlet policies, subsonic/supersonic characteristic selection, no-slip
adiabatic/isothermal/heat-flux walls, corners, and their independent nozzle/Couette/flat-plate
gates before P3l is complete.

Reconstruction does not wrap across the domain: bounded minmod uses edge-replicated neighbors,
and physical face fluxes replace terminal interior placeholders. Slopes for density, momentum,
and energy are each computed once per axis and reused by both reconstructed sides. Cells,
components, batches, and face profiles remain Series expressions; host loops visit only the
one-to-three spatial axes and momentum components. The same route retains reverse sensitivity
through fixed and adaptive SSP integration.

The permanent `compressible_boundary_test` covers batched prescribed free streams, matching
pressure outlets, global mass/energy conservation at closed slip faces, two-dimensional
tangential free streams, minmod integration, exact terminal time and stage accounting, reverse
sensitivity, incomplete face sets, malformed profile broadcasting, and explicit rejection of
bounded viscous transport.

References read for P3l-a:

- SU2 official *Markers and Boundary Conditions* documentation: separate Euler/symmetry,
  far-field, inlet, outlet, heat-flux, and isothermal-wall policy families rather than one
  generic ghost-value flag.
- SU2 official inviscid bump tutorial: subsonic inlet/outlet data are characteristic contracts,
  with stagnation or mass-flow inlet data and static-pressure outlet data. This is why P3l-a
  names direct primitive prescription `fixed`, not `inlet`.
- SU2 AIAA SciTech 2014 formulation paper: inviscid wall, no-slip adiabatic/isothermal wall,
  far-field, and characteristic inlet/outlet are physically distinct boundary operators.
- NASA subsonic-boundary and entropy-stable-wall literature inspected for the acceptance plan:
  incoming/outgoing characteristic count and wall energy consistency are required before the
  deferred names become public.
- Local PlussMa `common::spatial::BoxGrid`, face ordering, Series reconstruction, P3j fluxes,
  and `common::solve::march_to`: reused as canonical topology, arithmetic, and scheduling
  owners. No duplicate cell or batch loop was introduced.

#### Phase P3l-b: characteristic and viscous thermal boundaries

Status: **complete for ideal-gas characteristic far fields, subsonic/supersonic pressure
outlets, stationary or tangentially moving no-slip walls, and adiabatic, fixed-temperature,
or prescribed-outward-heat-flux thermal data with constant or Sutherland transport. Inlet
characteristics are completed in P3l-c; published validation cases remain release gates.**

`far(face, primitive)` now classifies every face lane independently. Supersonic outflow copies
the complete interior state, supersonic inflow uses the complete ambient state, and subsonic
flow combines the outgoing interior and incoming ambient acoustic invariants. Entropy and
tangential velocity follow the upwind direction. A pressure outlet retains the outgoing
Riemann invariant while imposing static pressure; backflow and a target pressure that reverses
the reconstructed normal velocity fail explicitly instead of producing an inlet by accident.

`wall(face)` is a stationary adiabatic no-slip wall. Overloads accept tangential wall speed,
while `wall_temp` and `wall_heat` select fixed temperature or outward heat flux. The inviscid
face remains impermeable and does not acquire tangential Rusanov wall friction. A separate
diffusive face assembly constructs velocity ghosts, computes one batched Newtonian stress
field, and inserts traction, `tau dot u_wall` work, and Fourier heat transfer into the same
finite-volume balance. Fixed-temperature data is imposed weakly through the half-cell heat
flux; cell energy is never overwritten. Prescribed positive heat flux means energy leaves the
fluid. Normal wall motion is rejected because it requires ALE geometry and grid-work terms.

The bounded explicit step limit now combines acoustic and viscous/thermal restrictions, and
every SSP stage recomputes the boundary traction and heat flux from its own state. Cell, face,
component, and batch arithmetic remains in Series expressions; host loops visit only the one
to three coordinate axes and stress components.

Independent gates cover the acoustic invariants and entropy choice, supersonic inflow/outflow,
pressure-outlet backflow, exact linear Couette shear, moving-wall power `mu U^2/H`, linear
isothermal conduction, prescribed-flux global energy accounting, adiabatic conservation,
combined stability limits, reverse sensitivity, and malformed wall data. HLLC is used for the
Couette oracle so Rusanov's intentional diffusion of a stationary shear contact is not mistaken
for physical viscosity.

References read for P3l-b:

- Poinsot and Lele, *Boundary Conditions for Direct Simulations of Compressible Viscous
  Flows*, Journal of Computational Physics 101 (1992): incoming/outgoing characteristic
  counting and non-reflecting boundary construction.
- NASA/TM-2014-218282: independent no-slip velocity conditions plus adiabatic normal
  temperature gradient, prescribed wall temperature, or mixed thermal data for compressible
  Navier--Stokes walls.
- NASA Wind-US official boundary-condition guide: viscous walls impose no slip, zero pressure
  gradient, and a separate adiabatic or constant-temperature condition; moving walls prescribe
  tangential speed.
- SU2 official marker documentation and compressible wall implementation audit: heat-flux and
  isothermal no-slip markers are distinct, and isothermal energy transfer is imposed weakly as
  a conservative wall flux.
- Local `spatial::BoxGrid`, face ordering, conservative gas flux, SSP stage, and quantity
  machinery: reused as owners; the wall implementation adds no cell-wise host loop.

#### Phase P3l-c: subsonic inlet characteristics

Status: **complete for ideal-gas total-condition and density-speed subsonic inlets on every
Cartesian face. Published nozzle, flat-plate, and corner-interaction datasets remain release
verification gates for the complete P3l claim.**

`inlet(face, total_temperature, total_pressure, direction)` takes one outgoing acoustic
invariant from the adjacent interior state. A fixed 48-iteration batched bisection solves the
subsonic Mach number from the stagnation relations and unit inflow direction; all face and batch
lanes advance together as Series expressions. Static temperature, pressure, density, speed, and
energy then follow from the ideal-gas isentropic relations. The solver rejects a non-unit or
outward direction and total data for which the outgoing characteristic has no inward subsonic
root.

The density-speed overload specifies mass flux per unit area through `rho |u|` and uses the same
outgoing invariant to determine sound speed and static pressure. It rejects a zero/reversed or
supersonic result; complete supersonic inflow remains the existing prescribed primitive-state
contract because every characteristic enters the domain there. Viscous ghost velocity and
temperature use the reconstructed inlet state, while WENO computes one characteristic exterior
state per boundary face and broadcasts it through the three ghost averages.

Independent checks recover a known Mach-0.2 state generated from stagnation relations, recover
the same state from density and speed, preserve a uniform WENO5/HLLC duct between matching total
inlet and static-pressure outlet, and reject outward direction and incompatible supersonic data.

References read for P3l-c:

- NASA/AIAA 2018-0334, *Formulation and Implementation of Inflow/Outflow Boundary Conditions
  to Simulate Propulsive Effects*: stagnation-property subsonic inflow retains one outgoing
  interior Riemann invariant and determines static state from total pressure and temperature.
- SU2 official inlet documentation and inviscid-bump tutorial: total temperature, total
  pressure, and unit flow direction define total-condition inlet data; density, speed, and
  direction define the mass-flow form; static-pressure outlet supplies the complementary
  subsonic downstream condition.
- NASA compressor/inlet characteristic formulations: subsonic inflow receives one datum from
  the interior, while supersonic inflow prescribes the complete state.

#### Phase P3m-a: shared HLLC contact flux

Status: **complete for ideal-gas HLLC on periodic and bounded Cartesian grids, with guarded
per-face Rusanov fallback; Roe, WENO, low-Mach, and carbuncle controls remain later P3m work.**

`gas::Options::flux` selects `FluxScheme::Rusanov` or `FluxScheme::Hllc`. Both mesh families
call one canonical numerical-face function; boundary code no longer owns a second physical or
Rusanov flux implementation. HLLC uses left/right acoustic wave bounds, the Rankine--Hugoniot
contact speed, and conservative star states. Tangential momentum is carried in each side's star
state, so the same kernel applies along every Cartesian axis and in one, two, or three
dimensions.

Degenerate star denominators are masked before division. A face falls back to Rusanov when its
contact denominator is scale-small, a star gap collapses, contact/star energy is non-finite, or
either star density/pressure violates the configured floors. This fallback is lane-local: one
bad face does not downgrade another cell or batch lane. It is a robustness guard, not a claim of
formal positivity preservation; a complete positivity limiter remains in P3m.

The defining regression is a stationary contact with equal velocity and pressure but a density
jump. HLLC preserves all conserved rates to roundoff while Rusanov produces the expected density
diffusion. Additional gates require batched two-dimensional free-stream preservation, bounded
fixed/outlet free streams, invalid-policy rejection, and finite reverse sensitivity. Existing
Sod, conservation, viscous, and bounded tests remain independent.

References read for P3m-a:

- Toro, Spruce, and Speares, *Restoration of the Contact Surface in the HLL-Riemann Solver*,
  Shock Waves 4 (1994), DOI `10.1007/BF01414629`: three-wave structure, contact speed, star
  states, and the contact-restoration purpose of HLLC.
- NASA/AIAA FUN3D HLLC formulation (AIAA J. 2008-3090): piecewise left, left-star,
  right-star, and right flux selection from the Rankine--Hugoniot states.
- SU2 official convective-scheme documentation and HLLC wedge regression configuration:
  HLLC as a production-selectable compressible convective flux used with MUSCL reconstruction.
- Local P3j Rusanov, admissibility fallback, minmod reconstruction, and Series reverse kernels:
  centralized and extended rather than copied into a parallel HLLC solver.

#### Phase P3m-b: shared limiters and WENO5

Status: **complete for shared minmod, monotonized-central, van Leer, and periodic or bounded
scalar-component WENO5 reconstruction. Roe fluxes, low-Mach preconditioning, and a formal
positivity limiter remain later P3m gates.**

One limiter kernel now serves periodic and bounded MUSCL reconstruction. Periodic WENO5 uses
the Jiang--Shu three-stencil candidate polynomials, scale-aware smoothness regularization, and
nonlinear weights, then supplies the canonical HLLC/Rusanov face engine. It reconstructs
complete density, momentum, and energy Series without extracting cells or batch lanes. Bounded
WENO5 obtains three face-normal ghost averages from the actual boundary policy: wall states are
mirrored, prescribed states are extended, and far-field/outlet states are reconstructed through
their characteristic contracts. Axis-wise finite-volume reconstruction does not require corner
ghost values, so multidimensional behavior is deterministic and independent of face traversal.

Verification requires exact quadratic cell-average face recovery, periodic and bounded
free-stream preservation, bounded Sod positivity/finiteness with HLLC, rejection of axes shorter
than the formal stencil, reverse sensitivity, and unchanged MUSCL limiter behavior.

References read for P3m-b:

- Jiang and Shu, *Efficient Implementation of Weighted ENO Schemes*, Journal of Computational
  Physics 126 (1996): fifth-order candidate stencils, smoothness indicators, and nonlinear
  weights.
- Zhang and Shu, *Positivity-preserving high order finite difference WENO schemes for
  compressible Euler equations*, Journal of Computational Physics 231 (2012): positivity is a
  separate limiting contract, which is why admissibility fallback is not labelled a formal
  positivity-preserving WENO method.
- SU2 official convective-scheme documentation: MUSCL limiters and higher-order reconstruction
  remain policies around one numerical-flux owner rather than separate solvers.

#### Phase P3m-c: Roe, entropy repair, and low-Mach dissipation

Status: **complete for ideal-gas Roe fluxes on periodic and bounded Cartesian grids, acoustic
Harten--Hyman entropy smoothing, an optional scale-relative entropy floor, Rieper low-Mach
dissipation scaling, lane-local Rusanov face fallback, admissible reconstruction scaling,
batch-conservative stage limiting, and optional multidimensional H-correction. A face-local
Hu--Adams--Shu cut-off limiter would improve locality further but is not required for positivity
of an accepted stage.**

`FluxScheme::Roe` is implemented by the same canonical face owner as Rusanov and HLLC. Roe
density, velocity, total enthalpy, sound speed, wave strengths, eigenvalues, and conservative
eigenvectors are assembled with `Series` operations over every face and batch lane. Only the
small physical component count is traversed in C++; no cell or batch value is extracted to the
host. The linearly degenerate contact and shear waves retain their exact Roe eigenvalues, while
the two acoustic waves receive the transonic entropy repair. This distinction preserves an exact
stationary contact instead of diffusing it through an indiscriminate eigenvalue floor.

`RoeMach::Rieper` scales the normal-velocity jump by the clamped local face Mach number. It
therefore changes only artificial upwind dissipation: physical Euler fluxes, equations of state,
signal speeds, and acoustic time-step limits remain unchanged. The policy returns continuously to
classic Roe at Mach one. Invalid Roe averages or non-finite flux lanes fall back to Rusanov at the
same face. Each reconstructed face state is first tested against the density and pressure floors.
An invalid candidate is scaled toward its own donor-cell average by a vectorized bisection over
the convex admissible set, retaining the largest representable admissible fraction instead of
discarding the entire reconstruction. If a complete high-order Euler/SSP stage is still
inadmissible, the stage is recomputed with cell reconstruction and Rusanov flux. A second
vectorized bisection finds the largest admissible convex fraction of the high-order stage for
every cell, then the spatial minimum is applied once per independent batch lane. Both endpoint
updates are conservative, so the shared lane coefficient preserves mass, momentum, and energy;
valid lanes remain exactly on their high-order endpoint. An inadmissible first-order fallback is
a hard error.

`RoeShock::HFix` implements the Sanders--Morano--Druguet multidimensional dissipation
used by Athena. Each reconstructed face receives
`eta = 0.5 * abs((u_R + c_R) - (u_L - c_L))`; a target face takes the maximum of its own value
and the four surrounding acoustic jumps for every transverse direction. That value floors every
Roe characteristic magnitude, including linearly degenerate fields. The policy is therefore
explicit and off by default: ordinary Roe continues to preserve stationary contacts and shear,
while long-lived grid-aligned shocks can request the deliberate transverse damping. Periodic and
bounded grids share the Roe kernel; bounded H stencils replicate the nearest physical-edge value
and boundary fluxes receive the corresponding edge plane of the same floor field.

The permanent verifier reconstructs a nontrivial one-dimensional Roe flux independently, checks
the exact stationary-contact and multidimensional shear-wave properties and transonic eigenvalue
limit, measures the analytical Rieper dissipation ratio, preserves periodic and bounded batched
Float32 free streams, crosses a strong pressure jump with WENO5/SSPRK3, retains reverse
sensitivity, proves conservative fractional fallback and lane isolation, reconstructs the exact
periodic H stencil independently, damps an odd-even grid mode, preserves periodic and bounded
free streams, and rejects invalid policy values. The benchmark executes classic, low-Mach, and
H-corrected Roe through the real periodic and bounded WENO paths rather than timing an isolated
algebra surrogate. On the focused `32 x 32` CPU run, H correction cost less than 5% over both
periodic and bounded Roe; it remains an opt-in cost.

References read for P3m-c:

- Roe, *Approximate Riemann Solvers, Parameter Vectors, and Difference Schemes*, Journal of
  Computational Physics 43 (1981), DOI `10.1016/0021-9991(81)90128-5`: Roe averages, Property U,
  wave decomposition, and exact resolution of isolated linearly degenerate waves.
- Harten and Hyman, *Self Adjusting Grid Methods for One-Dimensional Hyperbolic Conservation
  Laws*, Journal of Computational Physics 50 (1983): transonic entropy repair for Roe-type
  linearizations.
- Rieper, *A Low-Mach Number Fix for Roe's Approximate Riemann Solver*, Journal of Computational
  Physics 230 (2011), DOI `10.1016/j.jcp.2011.03.025`: local-Mach scaling of the normal velocity
  jump without changing the physical Euler flux.
- Miczek, Roeppke, and Edelmann, *New Numerical Solver for Flows at Various Mach Numbers*,
  Astronomy & Astrophysics 576 (2015): low-Mach asymptotics, kinetic-energy dissipation, and the
  requirement that flux preconditioning leave the physical equations unchanged.
- SU2 `roe.cpp` and official convective-scheme documentation: production Roe eigenstructure,
  entropy-floor policy, L2Roe/LMRoe separation, and low-Mach regression practice. The formulas
  were independently expressed with Pluss `Series`; no source was copied.
- Zhang and Shu, *On Positivity-Preserving High Order Discontinuous Galerkin Schemes for
  Compressible Euler Equations on Rectangular Meshes*, Journal of Computational Physics 229
  (2010): convexity of the positive-density/positive-pressure admissible set and linear scaling
  toward a valid cell average.
- Hu, Adams, and Shu, *Positivity-preserving method for high-order conservative schemes solving
  compressible Euler equations*, Journal of Computational Physics 242 (2013): limiting a
  high-order update toward a positivity-preserving first-order Lax--Friedrichs endpoint, density
  before pressure, and application at every SSP stage. The present limiter uses one conservative
  coefficient per batch lane rather than their more local face cut-off.
- Sanders, Morano, and Druguet, *Multidimensional Dissipation for Upwind Schemes: Stability and
  Applications to Gas Dynamics*, Journal of Computational Physics 145 (1998): transverse
  multidimensional dissipation and the H stencil for grid-aligned shock instability.
- Stone et al., *Athena: A New Code for Astrophysical MHD*, Astrophysical Journal Supplement 178
  (2008), Appendix C, and the official Athena `integrate_2d_vl.c`/`roe.c` sources: the acoustic
  `eta` measure, four transverse neighbours, and `max(abs(lambda), eta_H)` characteristic floor.
  The implementation here generalizes the same stencil over arbitrary Cartesian rank with
  `Series` operations; no source was copied.

#### Phase P3m-d: canonical compressible-flow acceptance suite

Status: **complete for Lax, Shu--Osher, translated isentropic-vortex convergence, a Cartesian
Quirk carbuncle stress case, and the Woodward--Colella double-Mach reflection at its canonical
`t = 0.2`. The suite exercises bounded and periodic Cartesian flow, WENO5, HLLC, Roe with
H-correction, positivity limiting, mixed face policies, time-dependent boundaries, and SSP-RK
stage timing through production entry points.**

`compressible_benchmarks_test` is intentionally separate from local kernel tests. Lax density,
velocity, and pressure are compared in mean absolute error with an independently implemented
scalar exact ideal-gas Riemann solution: pressure is found from the two shock/rarefaction
functions and every self-similar region is sampled at cell centres. The oracle neither calls a
Pluss flux nor evolves a Series. Shu--Osher has no elementary closed final solution, so its
acceptance contract does not invent point values: the WENO5/HLLC result must remain admissible,
the shock must traverse the expected domain region, and the transmitted density-wave standard
deviation must exceed both a physical floor and the same mesh evolved by the deliberately
diffusive cell/Rusanov path. The smooth vortex is compared directly with its analytic periodic
translation at two resolutions; the measured density error must converge above the stated order
and the fine-grid error must remain below an absolute envelope.

The Quirk case is the Cartesian perturbation variant used by fixed-grid solvers: a Mach-6 planar
shock advances through deterministic grid-scale pre-shock noise. Because the exact solution is
one-dimensional, transverse density variance is the defect measure. The H-corrected Roe result
must suppress that defect relative to classic Roe rather than merely remain finite. The
double-Mach case uses the published `[0,4] x [0,1]`, gamma-1.4, Mach-10 states and the canonical
end time. Automated invariants check positive finite density and pressure, exact traversal of the
moving top-shock rule through the final SSP stage, agreement of the top-row shock classification
with its analytic motion, downstream travel of the reflected Mach stem, and compression above
the incident post-shock density.

Two reusable boundary contracts were added rather than embedding benchmark logic in the
integrator. `blend(mask, a, b)` composes two rules on one physical face using a Boolean Series;
mask broadcasting is dtype- and platform-strict, and both ordinary exterior states and WENO
ghost bands preserve the reflective depth of the selected branch. Viscous velocity, stress-work,
temperature, heat-flux, and Sutherland-viscosity paths use the same composition. `BoundFn`
provides boundaries as a function of physical time. The shared step core evaluates it at the
actual SSP abscissae: `t_n` for Euler, `t_n` and `t_n + dt` for RK2, and `t_n`, `t_n + dt`, and
`t_n + dt/2` for RK3. Positivity fallback reuses the boundary object from that stage. Constant
`Bounds`, fixed-step evolution, and adaptive evolution all delegate to this one stepping core.

References read for P3m-d:

- Toro, *Riemann Solvers and Numerical Methods for Fluid Dynamics*, third edition, chapters 4
  and 6: exact ideal-gas pressure functions, Newton solve, star state, and self-similar sampling
  used by the independent Lax oracle.
- Shu and Osher, *Efficient Implementation of Essentially Non-oscillatory Shock-Capturing
  Schemes, II*, Journal of Computational Physics 83 (1989), DOI
  `10.1016/0021-9991(89)90222-2`, and the NASA High-Fidelity CFD Workshop Shu--Osher
  specification: Mach-3 shock/density-wave initial data, domain, and final-time contract.
- Yee, Vinokur, and Djomehri, *Low-Dissipative High-Order Shock-Capturing Methods Using
  Characteristic-Based Filters*, Journal of Computational Physics 150 (1999), and the official
  FLASH isentropic-vortex verification page: periodic minimum-image construction, analytic
  thermodynamic perturbation, and translation by the free-stream velocity.
- Quirk, *A Contribution to the Great Riemann Solver Debate*, International Journal for
  Numerical Methods in Fluids 18 (1994), DOI `10.1002/fld.1650180603`: the moving Mach-6
  odd-even-decoupling stress test and its one-dimensional exact character.
- Stone et al., *Athena: A New Code for Astrophysical MHD*, Astrophysical Journal Supplement
  178 (2008), Appendix C: growth of transverse perturbations at grid-aligned shocks and the
  H-correction acceptance rationale.
- Woodward and Colella, *The Numerical Simulation of Two-Dimensional Fluid Flow with Strong
  Shocks*, Journal of Computational Physics 54 (1984), DOI
  `10.1016/0021-9991(84)90142-6`, together with the official FLASH double-Mach setup: incident
  states, split lower wall, moving exact top boundary, supersonic inflow, outflow, and `t = 0.2`
  comparison time.

#### Phase P3n-a: temperature-dependent single-gas transport

Status: **complete for periodic and bounded calorically perfect gas flow with Sutherland
viscosity, configurable bulk-viscosity ratio, fixed-Prandtl conductivity, harmonic interior and
prescribed-temperature wall conductivity, and wall-temperature viscosity. Tabulated transport,
passive scalars, and multispecies diffusion remain P3n-b.**

`SutherlandLaw` computes viscosity from the local static-temperature Series, bulk viscosity as
a configured non-negative ratio, and conductivity from `mu cp / Pr`. The resulting
`TransportField` preserves every batch/spatial lane, dtype, platform, and reverse graph.
Harmonic face conductivity is used for variable-coefficient Fourier transfer. SSP stages and
adaptive limits recompute coefficients from their own state; constant and Sutherland transport
cannot be enabled together. Public field consumers validate shape, platform, finiteness, and
sign once at their boundary, while the constitutive hot path avoids duplicate reductions.

The permanent verifier compares four temperatures with independently evaluated reference
values, checks the exact reference viscosity and fixed-Prandtl relation, batch-lane isolation,
constant-temperature stress equivalence, zero uniform-temperature heat flux, explicit
two-stage SSP reconstruction, diffusive stability selection, reverse sensitivity, and invalid
parameters/layouts.

References read for P3n-a:

- NASA Wind-US official `VISCOSITY` and chemistry-data documentation: reference-temperature
  Sutherland formula, species parameterization, and the distinction between Sutherland and
  multicomponent Wilke transport.
- NASA Wind-US utilities guide: equivalent `T^(3/2)/(T+S)` parameterization and the limits of
  reducing higher-order NASA transport fits to a Sutherland curve.
- Cantera transport source and PlussMa's existing Cantera-validated chemistry transport layer:
  audited so later species mixing reuses the chemistry owner rather than duplicating it in
  fluids.

#### Phase P3n-b: conservative nonreacting species transport

Status: **complete for periodic calorically perfect gas flow with conservative species
advection, mixture-averaged molecular diffusion, synchronized SSP stepping, fixed/adaptive
trajectories, batched multidimensional fields, and reverse sensitivity. Finite-rate chemistry,
species-enthalpy energy flux, and bounded species conditions remain subsequent P3n work.**

`physics/fluids/compressible_species.h` composes a gas `State` with species partial densities
stored as `[..., species, spatial...]`. Its factory accepts normalized mass fractions and
materializes `rho Y`; the partial densities, rather than fractions, are the evolved conserved
variables. Gas and species updates consume one `gas::detail::axis_fluxes` result per physical
axis. The species advective flux is the canonical Riemann mass flux multiplied by the
mass-flux-upwind face composition. Face fractions are reconstructed with the selected gas
reconstruction, clipped to the physical simplex, and renormalized, so their flux sum remains
the gas mass flux even when nonlinear WENO weights differ by species.

Molecular properties remain owned by `chemistry::transport::GasTransport`. A diffusion-only
binary-coefficient entry point avoids evaluating unused viscosity and conductivity. The fluid
adapter moves only the species axis, computes mole fractions and CHEMKIN mixture diffusivities
in the chemistry layout, and moves the result back. Face fluxes use a direct two-point mole-
fraction gradient, harmonic diffusivity, face density, and Cantera's mass-averaged correction
`j_k <- j_k - Y_k sum_j j_j`; therefore the net diffusive mass flux is zero at every face.
No cell or species host loop appears in the hot path. C++ loops cover only the small number of
spatial directions and SSP stages.

One shared batch-wise positivity fraction blends high-order and cell/Rusanov fallback updates
across density, momentum, energy, and every partial density. This retains conservation while
preventing independent species clipping after a step. The species diffusion limit is combined
with the gas acoustic/viscous limit. `evolve` and `evolve_to` use the common `StateVector`
marcher and retain `species_density` in every trajectory state.

The permanent `compressible_species_test` verifies exact uniform-composition transport,
per-species periodic conservation, `sum rhoY = rho`, zero-sum corrected diffusion flux,
variance decay, combined stability limits, fixed and terminal-time-exact adaptive paths,
rank-four `[batch, species, x, y]` layouts, reverse sensitivity, and fail-closed composition,
layout, and transport-cardinality diagnostics. The complete fluid suite passes 14/14 tests.

References read for P3n-b:

- Cantera `MixTransport.cpp` and `MixTransport.h` (main branch, read 2026-10-19):
  mixture-averaged diffusive mass flux relative to the mass-averaged velocity and the explicit
  `-Y_k sum_j j_j` correction that enforces zero total species diffusion flux.
- Kee, Coltrin, and Glarborg, *Chemically Reacting Flow: Theory and Practice* (2003), as cited
  by Cantera's mixture-averaged transport owner: CHEMKIN mixture-diffusion convention and
  species/mole/mass relations.
- Toro, *Riemann Solvers and Numerical Methods for Fluid Dynamics*, third edition: passive
  conservative quantities must share the hydrodynamic face mass flux rather than solve an
  unrelated scalar Riemann problem.
- Strang, *On the Construction and Comparison of Difference Schemes*, SIAM Journal on
  Numerical Analysis 5 (1968), DOI `10.1137/0705041`: reviewed for the subsequent symmetric
  flow/reaction source-splitting phase; no reaction split is claimed in P3n-b.

#### Phase P3n-c: reacting-mixture EOS and homogeneous chemistry

Status: **complete for a conservative composition-dependent ideal-gas state, vectorized NASA-7
thermodynamic closure, safeguarded energy-to-temperature recovery, instantaneous finite-rate
mass sources, and stiff adiabatic constant-volume chemistry updates. Spatial reacting Riemann
fluxes, species-enthalpy diffusion, and symmetric flow/reaction splitting remain P3n-d.**

`chemistry::thermo::IdealGas` is the single thermodynamic owner shared by chemistry and fluids.
The kinetics mechanism now retains optional species molar masses in its canonical species order;
the ideal-gas model requires complete positive mass data and caches their reciprocals once. Its
public operations preserve every leading batch or spatial axis while the trailing axis denotes
species. Concentrations, mean molar mass, density, pressure, species-weighted internal energy and
enthalpy, `cp`, `cv`, `gamma`, and sound speed are assembled entirely from Series operations.
Internal energy includes NASA-7 formation energy, so reaction heat release is represented by the
conserved energy and changing composition rather than an artificial source added to a
calorically-perfect gas equation.

The inverse closure `T(e,Y)` uses lane-wise bracketed Newton iteration over the common NASA
validity interval. Endpoint energies first prove that every requested lane is recoverable; each
Newton proposal is accepted only inside its live bracket and otherwise falls back to bisection.
The implementation uses one batched Series graph for all cells and species, retains reverse
sensitivity, rejects non-monotone heat capacity or non-convergence, and never extracts a cell or
species to a host loop.

`physics/fluids/compressible_reacting.h` introduces a thermochemical `State` containing `rho`,
`rho*u`, total energy density, and `rho*Y`. `reacting::Model` owns one mechanism/EOS pair and
validates reaction mass balance against the supplied molar masses once. Primitive and conserved
constructors, velocity, temperature, pressure, sound speed, and mass fractions all route through
the same checked thermodynamic snapshot, avoiding duplicate temperature inversions in public hot
paths. The instantaneous source is `d(rhoY_k)/dt = M_k omega_k`; density, momentum, and total
energy rates are exactly zero for the closed adiabatic source problem.

`react()` delegates the stiff `[T,C]` solve to the existing vectorized Rosenbrock chemistry
reactor. After integration, one uniform lane-wise mass projection removes solver-tolerance drift
without changing species ratios; `sum(rhoY)` therefore equals `rho` exactly. The operator leaves
`rho`, momentum, and total chemical energy bitwise unchanged, then validates the resulting state
by recovering temperature from that conserved energy. No claim of a coupled reacting-flow step is
made until the spatial thermochemical flux and Strang composition are implemented.

The permanent `reacting_ideal_gas_test` checks mixture identities, independent scalar pressure,
density/pressure and energy/temperature round trips on both NASA intervals, multidimensional
lanes, agreement with the established reactor energy convention, mechanism mass balance, reverse
sensitivity through inversion, and range/layout/composition failures. The permanent
`compressible_reacting_test` checks primitive recovery, positive pressure and sound speed,
cell-wise zero-sum mass sources, exact conservation of `rho`, momentum, and total energy through a
stiff exothermic step, exact species-density closure, expected reactant/product movement,
temperature rise, reverse sensitivity, and fail-closed invalid mechanisms and widths. Chemistry
tests pass 2/2 and the complete fluids suite passes 14/14.

References read for P3n-c:

- Cantera `src/thermo/IdealGasPhase.cpp` and `src/thermo/ThermoPhase.cpp` (main branch, read
  2026-10-19): mass-specific mixture properties, `u_k = R T (h_k/RT - 1)`, ideal-gas pressure and
  sound speed, and bounded Newton recovery of temperature from internal energy with `cv` as the
  derivative.
- Cantera `src/zeroD/IdealGasReactor.cpp` (main branch, read 2026-10-19): species mass sources
  `M_k omega_k` and the adiabatic internal-energy temperature balance used by the shared reactor.
- AMReX-Combustion PelePhysics `Source/Eos/Fuego.H` (development branch, read 2026-10-19):
  `EY2T`, `RTY2P`, `RTY2Cs`, species internal energies, mixture heat capacities, and the
  conserved-variable EOS boundary used by reacting-flow solvers.
- Shampine and Reichelt, *The MATLAB ODE Suite*, SIAM Journal on Scientific Computing 18 (1997),
  together with MATLAB R2025b `ode23s.m`: the already-centralized modified Rosenbrock stiff
  integrator reused by the homogeneous source operator.
- Strang, *On the Construction and Comparison of Difference Schemes* (1968): symmetric
  source/transport composition remains the specified P3n-d coupling contract, not an implicit
  claim of this homogeneous-source tranche.

#### Industrial CFD acceptance programme

The target beyond P3k is an industrial CFD system, not merely a larger catalogue of flow
functions. No release may describe the solver as industrial-grade until every gate below is
implemented, independently verified, regression-tested, benchmarked, and exposed through a
stable public contract. Partially implemented policies are not added as inert option fields.

1. **P3l, bounded compressible flow.** Conservative finite-volume boundary faces must
   support symmetry/slip walls, no-slip adiabatic walls, prescribed heat flux and wall
   temperature, total/static inlet data, pressure outlets, far fields, and correct
   subsonic/supersonic characteristic selection. Corner and edge ghost construction must be
   deterministic. Verification includes inviscid wall reflection, Couette heat transfer,
   laminar flat-plate skin friction, subsonic nozzle mass flow, and conservation accounting
   that separates physical boundary flux from numerical drift.
2. **P3m, production convective fluxes.** The canonical face engine must support HLLC and
   Roe-class contact-resolving fluxes alongside Rusanov, MUSCL limiters and fifth-order WENO
   reconstruction, entropy/positivity safeguards, and low-Mach preconditioning. Required
   tests include Sod, Lax, Shu--Osher, double Mach reflection, isentropic vortex convergence,
   carbuncle stress cases, and exact uniform/free-stream preservation on every mesh family.
3. **P3n, transport and species.** Constant transport expands to Sutherland and tabulated
   viscosity/conductivity, fixed-Prandtl and modelled-Prandtl closure, passive scalars,
   multispecies diffusion, finite-rate reactions, source splitting, and chemistry coupling.
   Shared thermo/chemistry owners remain in NatSci common/chemistry; fluids performs only
   conservative assembly and transport-specific checks.
4. **P3o, scalable elliptic and implicit solves.** Geometric multigrid is required for
   structured pressure/Helmholtz systems, with algebraic-multigrid integration for general
   meshes. Newton--Krylov and higher-order IMEX/BDF paths must expose convergence reason,
   nonlinear/linear iterations, residual histories, preconditioner work, and fail-closed
   termination. Single-grid CG remains a small-problem fallback, not the production claim.
5. **P3p, turbulence.** DNS remains an explicit resolution choice; LES begins with
   Smagorinsky/WALE and dynamic filtering, while RANS begins with verified SA and SST
   variants plus wall-distance and wall-treatment machinery. Every model is compared with
   NASA Turbulence Modeling Resource flat-plate, channel, airfoil, and separated-flow cases;
   model constants and exact variant names are serialized into results.
6. **P3q, rheology and phases.** Generalized-Newtonian power-law, Carreau--Yasuda, and
   Bingham regularizations reuse one constitutive-rate interface. Multiphase work adds
   conservative VOF, bounded interface compression, surface tension, contact angles, and
   phase-aware material interpolation before any broader multiphase claim. Dam-break,
   static-drop pressure jump, capillary-wave, and rising-bubble suites are release gates.
7. **P3r, mesh hierarchy.** Curvilinear metrics must satisfy a discrete geometric
   conservation law. Block-structured AMR requires conservative prolongation/restriction,
   refluxing, subcycling, tagging, load balancing, and coarse/fine ghost ownership.
   Unstructured finite-volume support requires explicit cell/face/connectivity storage,
   quality metrics, partition-independent residuals, and boundary-region metadata. Existing
   Series Cartesian operators remain optimized specializations behind shared face/reduction
   concepts rather than being discarded.
8. **P3s, parallel execution.** A partition owner must centralize interior/halo Series,
   deterministic local/global indexing, nonblocking halo exchange, collective reductions,
   and restart redistribution. MPI+threads is the CPU baseline. Strong/weak scaling,
   communication overlap, bitwise restart equivalence where feasible, deterministic
   reductions where requested, and sanitizer/fault diagnostics are mandatory.
9. **P3t, operational reliability.** Versioned checkpoints, mesh/field provenance,
   residual and conservation telemetry, configurable output cadence, restart across process
   counts, resource limits, NaN localization, and stable diagnostics are part of the solver,
   not application-specific afterthoughts. Every benchmark stores equations, models,
   discretization, tolerances, compiler/build identity, and timing counters.

Cross-cutting implementation rules:

- Series expressions own cell, face, component, batch, and local-partition arithmetic;
  host loops may visit only bounded topology metadata, mesh blocks, or partitions.
- One face-flux contract serves Euler, viscous gas, species, turbulence, and AMR refluxing.
  One transport contract serves incompressible, compressible, non-Newtonian, and chemistry
  consumers. One Krylov/multigrid report schema serves every implicit subsystem.
- Every new model has a scalar-independent batched verifier, manufactured/convergence test,
  conservation test, malformed-input suite, reverse-sensitivity decision, and throughput
  baseline before it enters the public aggregate headers.
- Reference solutions are generated independently or taken from published benchmark data;
  an implementation is never used to manufacture its own expected outputs.
- Performance acceptance records memory traffic, allocations, cells/s, stage-cells/s,
  parallel efficiency, and solver work. Optimization may change layout or fusion, but not
  weaken residual, positivity, conservation, or reproducibility gates.

Architecture references for the industrial programme:

- SU2 official governing-equation, boundary-marker, turbulence, and verification material:
  conservative compressible/incompressible contracts and production boundary semantics.
- PETSc official KSP/SNES/TS/PCMG/GAMG manuals: composable Krylov, nonlinear,
  implicit-time, geometric-multigrid, and algebraic-multigrid contracts with explicit
  convergence reasons.
- AMReX official `AmrCore`, `MultiFab`, ghost-cell, refluxing, checkpoint, load-balancing,
  MPI+X, GPU, and regression documentation: block-structured AMR ownership and scalable
  execution patterns. Pluss will adapt the architecture to Series rather than copy code.
- NASA Turbulence Modeling Resource: exact SA/SST variants, canonical grids, boundary data,
  and verification/validation cases for turbulence release gates.
- hypre and p4est official designs will be reviewed before the AMG and forest-mesh phases;
  OpenFOAM and MFEM will be secondary source audits for finite-volume boundary, mesh, and
  field-ownership edge cases.

### Phase P4: thermodynamics

Status: **P4a ideal-gas and calorically perfect-gas foundation, P4b closed-system
process balances, and P4c temperature-dependent species properties implemented.**

Thermodynamics is CPU-first. Its data-parallel kernels are ordinary `Series` expressions,
so they retain the input dtype, shape, platform, broadcasting rules, and reverse graph,
but no GPU-specific implementation is a prerequisite for this physics layer. The public
surface remains functional: state quantities enter as dimensioned values and each
operation returns a dimensioned result. A stateful phase object will only be introduced
when composition-dependent models have enough shared cached data to justify one; it will
delegate its calculations to the same functional kernels.

#### Phase P4a: ideal and calorically perfect gases

`physics::thermo` establishes the reusable low-pressure gas relations before species,
mixture, or real-gas behavior is attempted. Molar state functions implement `pV = nRT`
and all three inverse queries. Mass-specific state functions derive the gas constant from
molar mass and implement `p = rho R_s T`. The calorically perfect subset supplies
`cv = R_s / (gamma - 1)`, `cp = cv + R_s`, zero-reference `u = cv T` and `h = cp T`,
the acoustic speed `sqrt(gamma R_s T)`, ideal isothermal compressibility `1 / p`, and
thermal expansion `1 / T`.

Absolute temperature is represented by `common::TemperaturePoint`, not by a temperature
interval. Celsius and Fahrenheit inputs therefore apply their affine offset at the point
boundary and all equations consume kelvin internally. Amount, pressure, volume, density,
molar mass, gas constants, heat capacities, and heat ratio must be finite and physically
positive; `gamma` must exceed one and `cp` must exceed `cv`. A malformed lane invalidates
the call rather than allowing one batch member to seed infinities or NaNs downstream.

Every equation is one broadcasted Series expression. Leading batch dimensions and
parameter sweeps therefore compose without loops or host extraction, and reverse
sensitivity remains attached. The permanent verifier covers CODATA metadata, standard
molar volume at 273.15 K and 100 kPa, all inverse molar relations, the dry-air mass basis,
Celsius conversion, multidimensional amount/temperature and gamma/temperature
broadcasting, compile-time output dimensions, reverse sensitivity, and invalid-state
diagnostics.

The scope is intentionally narrow. `energy` and `enthalpy` use zero as the reference state;
they are not formation properties. Temperature-dependent heat capacity, species reference
enthalpy/entropy, mixtures, phase equilibrium, and non-ideal equations of state are not
approximated by extra flags on these functions.

Planned thermodynamics sequence:

1. **P4d:** mixtures and real-gas equations of state after a shared nonlinear root and
   phase-stability contract is available.

#### P4a reference audit

- Cantera stable `IdealGasPhase` C++ reference (inspected 2026-10-13): ideal-gas state
  contract, molar volume `RT / p`, pressure-independent ideal-gas enthalpy and heat
  capacity, entropy pressure correction, and internal energy `h - RT`.
- Cantera 3.0 `src/thermo/IdealGasPhase.cpp` (inspected 2026-10-13): the caloric identity
  `cv = cp - R` and the ideal-gas acoustic-speed implementation. PlussMa does not copy
  Cantera's phase classes, species storage, or caches.
- Cantera 3.2 MATLAB thermo documentation (inspected 2026-10-13): public state/property
  roles and the ideal-gas sound-speed relation.
- Mohr, Newell, Taylor, and Tiesinga, *CODATA recommended values of the fundamental
  physical constants: 2022*, J. Phys. Chem. Ref. Data 54, 033105 (2025), DOI
  `10.1063/5.0279860`: exact molar gas constant `8.31446261815324 J mol^-1 K^-1` and
  standard molar volume `22.71095464...e-3 m^3 mol^-1` at 273.15 K and 100 kPa.
- Local MATLAB R2025b toolbox sources were searched for an inspectable ideal-gas or
  thermodynamic-property implementation. No suitable source was present, so MATLAB is not
  claimed as a P4a implementation oracle.
- Existing PlussMa `Quantity`, affine `Point`, dimensional algebra, physical-constant
  records, and PlussQit Series arithmetic were audited and reused. P4a introduces no
  duplicate unit conversion, storage, broadcasting, or differentiation machinery.

#### Phase P4b: closed-system process balances

P4b fixes one sign convention at the API boundary: heat transferred into a closed system
is positive and work performed by that system is positive. The first law is therefore
`delta_e = heat - work`. `delta_e`, `heat_in`, and `work_out` expose all three algebraic
forms with dimension-checked energy inputs; signed cooling, compression work, and stored
energy loss remain valid, while undefined, non-real, empty, or non-finite Series fail
before arithmetic.

`IdealPath` records the final absolute temperature together with boundary work, heat, and
internal-energy change. Its four named constructors cover distinct calorically perfect,
quasi-static ideal-gas paths rather than accepting a permissive mode flag:

| Function | Fixed condition | Returned balance |
| --- | --- | --- |
| `iso_v` | Volume | `W = 0`, `Delta U = Q = m cv Delta T` |
| `iso_p` | Pressure | `Q = m cp Delta T`, `Delta U = m cv Delta T`, `W = Q - Delta U` |
| `iso_t` | Temperature | `Delta U = 0`, `Q = W = nRT ln(V2 / V1)` |
| `iso_s` | Entropy, reversible adiabatic | `T2 = T1 (V1 / V2)^(gamma - 1)`, `Q = 0`, `W = -Delta U` |

These functions accept scalar or Series representations for every naturally batchable
quantity. Their formulas use centralized representation-level `log`, `exp`, `pow`, and
zero construction, preserving dtype, platform, broadcasting, and reverse sensitivity.
There are no loops over states or parameter lanes. A temperature endpoint remains an
affine `TemperaturePoint`; only the difference between endpoints is a temperature
quantity.

`pv_work` handles a general quasi-static pressure law by integrating `p(V) dV` through
`common::solve::quad`. That owner already supplies dimension propagation, adaptive-rule
policy, batched callable evaluation, and autodiff. A batched pressure callback places a
singleton node axis after its batch dimensions, for example amount shape `[batch, 1]`
against volume nodes `[nodes]`, so one callback evaluates every state and node together.
Reversed volume bounds naturally return negative work. P4b does not call every adiabatic
process isentropic: `iso_s` specifically models a reversible adiabatic path.

The verifier covers each first-law rearrangement, heating and expansion for constant
volume and pressure, expansion/compression sign reversal for the isothermal path,
reversible-adiabatic cooling, two-dimensional gamma/temperature/volume broadcasting,
analytic-versus-Gauss-Legendre `p dV` work, reversed integration bounds, reverse
sensitivity with respect to batched amount, and invalid volume/heat-ratio failures.

#### P4b reference audit

- NASA Glenn, *Ideal Gases under Constant Volume, Constant Pressure, Constant
  Temperature, & Adiabatic Conditions* (inspected 2026-10-13): `dQ = dU + p dV`,
  constant-volume, constant-pressure, isothermal, and reversible adiabatic ideal-gas
  derivations, including `cp = cv + R`.
- NASA Glenn, *Isentropic Flow Equation Derivation* (inspected 2026-10-13): entropy
  differential, `p / rho^gamma = constant`, temperature-density relation, and the
  calorically perfect identities used by `iso_s`.
- MIT Unified Engineering, Thermodynamics Chapter 6 (inspected 2026-10-13): first-law
  sign convention, separation of boundary/flow/shaft work, enthalpy in steady-flow energy
  accounting, and `dh = cp dT` for an ideal gas. P4b implements the closed-system subset;
  a steady-flow control-volume result is not implied by these functions.
- Existing PlussMa `common::solve::quad` and `math::integ::quadra` implementations/tests:
  dimensioned callable integration, node-axis batching, orientation, convergence records,
  and reverse sensitivity. P4b forwards to that owner rather than maintaining a private
  quadrature rule.
- Local MATLAB R2025b's PDE thermal material sources were inspected for positive specific
  heat and density contracts, but no suitable ideal-gas process implementation was found;
  MATLAB is not claimed as a P4b numerical oracle.

#### Phase P4c: temperature-dependent species properties

P4c does not introduce a second thermodynamic-polynomial implementation. The repository
already contained a chemistry-owned NASA-7 evaluator, so its neutral numerical machinery
was promoted to `common/thermo_poly.h`. Chemistry keeps source-compatible names through a
thin namespace facade, while physics exposes the same model as typed molar and mass-basis
properties in `physics/thermo/species.h`. Coefficient records, midpoint selection,
validation, polynomial powers, and coefficient caches therefore have one canonical owner.

`Nasa7::eval` consumes a temperature Series of any rank and appends one species axis. A
rank-two `[a,b]` field produces `[a,b,S]`; rank-three and rank-four fields similarly produce
rank-four and rank-five results. Low/high selection, `T` through `T^4`, and all species are
evaluated by broadcasted Series operations and reductions. There is no host loop over
temperatures, batches, or species during evaluation. The only species loop occurs once in
the constructor while validating metadata and packing immutable coefficient tables.

The model caches low/high coefficients, midpoint thresholds, exponents, and enthalpy and
entropy integration weights on the exemplar's dtype and platform. Evaluation rejects a
different dtype or platform rather than silently transferring every call. Computed values
are deliberately not memoized: a Series may alias mutable external storage, so a cache
keyed by its handle could return stale thermodynamic state. One `eval` returns `cp/R`,
`h/(RT)`, and `s/R` together so downstream kinetics can reuse a single fused result.

NASA polynomial evaluation remains defined as extrapolation outside a record's declared
range, matching Cantera's property-update path and preserving the existing 298.15 K
chemistry references for species whose metadata begins at 300 K. `check` is the explicit
hard-range contract. Both paths reject undefined,
empty, non-real, non-finite, non-positive, dtype-mismatched, and platform-mismatched
temperatures. Exactly `Tmid` selects the low interval, matching Cantera `NasaPoly2`.

The physics `props` wrapper returns dimensioned molar `cp`, `h`, `s`, and `g`; `mass_props`
divides the fused molar result by scalar or species-shaped molar mass to return specific
heat and specific energy. The permanent verifier covers 2-D, 3-D, and 4-D temperature
fields, appended species axes, mixed per-species midpoint branches, exact-midpoint behavior,
non-contiguous transposed storage, molar and mass dimensions, `g = h - Ts`, reverse
`dH/dT = Cp`, model metadata failures, explicit range rejection, and input dtype failures.
The established kinetics, equilibrium, electrochemistry, spectroscopy, and transport suites
all pass against the centralized engine.

The permanent CPU benchmark evaluates `[16,16,8,4]` temperatures for 12 species. On the
2026-10-14 development run, one rank-four call took 6.957 ms median; dispatching the same
work one leading-axis page at a time took 16.125 ms, a 2.318x advantage, with identical
results. Timings are descriptive rather than hard test thresholds; parity is checked before
measurement.

#### P4c reference audit

- Cantera development `include/cantera/thermo/NasaPoly2.h` (inspected 2026-10-14): two
  seven-coefficient regions, coefficient ordering, and the exact `T <= Tmid` low-region
  selection rule. Cantera's update routine evaluates the selected polynomial directly; it
  does not make range enforcement part of every property call.
- B. J. McBride, M. J. Zehe, and S. Gordon, NASA/TP-2002-211556, *NASA Glenn
  Coefficients for Calculating Thermodynamic Properties of Individual Species* (PDF
  inspected 2026-10-14): definitions and analytic integrations for `Cp/R`, `H/(RT)`, and
  `S/R`, multi-range fitting, and thermodynamic reference constants.
- NASA Glenn, *Chemical Equilibrium with Applications* official overview (inspected
  2026-10-14): provenance and role of the species thermodynamic database in equilibrium
  calculations.
- The existing chemistry `Nasa7`, mechanism, reactor, equilibrium, electrochemistry,
  spectroscopy, and transport implementations and tests were audited before promotion.
  Their verified NIST/Cantera fixtures were reused rather than transcribed into a second
  physics database.
- Local MATLAB R2025b was searched for an inspectable NASA-polynomial species evaluator;
  no suitable implementation was found, so MATLAB is not claimed as a P4c oracle.

The sources above guided equations and boundary conventions. PlussMa's common ownership,
typed quantity wrappers, high-rank Series graph, cache policy, and tests are independent
implementations.

## Domain Sequence

### Physics

1. `mechanics/`: particles and force models, Lagrangian and Hamiltonian systems,
   conservation diagnostics, rigid-body state, symplectic stepping, events, and Lyapunov
   analysis.
2. `astro/`: orbital elements and Cartesian states, two-body and multi-body systems,
   Lambert transfer, ephemeris interpolation, coordinate transforms, black-body radiation,
   basic cosmology, stellar relations, and compact-object utilities.
3. `quantum/`: normalized states, operators, expectation values, density matrices, tensor
   products, partial traces, time evolution, and open-system dynamics.
4. Thermodynamics, electromagnetism, fluids, relativity, and continuum mechanics follow
   only after their common dimensional and solver requirements are stable.

### Chemistry

1. `kinetics/`: reactions, stoichiometric systems, mass action, Arrhenius rates, batch and
   flow reactors, stiff integration, conservation checks, sensitivities, fitting, and model
   reduction.
2. `quantum/`: atomic and molecular structures, basis data, one- and two-electron
   integrals, Hartree-Fock, density matrices, energies, gradients, and deliberately scoped
   post-HF/DFT extensions.
3. Thermodynamics, equilibrium, electrochemistry, transport, and spectroscopy follow.

### Biology

1. `population/`: growth, competition, predator-prey systems, structured populations,
   migration, stochastic birth-death processes, selection, harvesting, and equilibria.
2. `systems/`: reaction, gene-regulatory, and metabolic networks; Hill and
   Michaelis-Menten kinetics; steady states; bifurcation; sensitivity; identifiability; and
   stochastic motifs.
3. `biophysics/`: diffusion, membranes, electrophysiology, polymers, binding, population
   balance, biomechanics, and spatial/compartment models.
4. Epidemiology, ecology, genetics, and neuroscience follow.

#### B1: population (implemented 2026-10-13)

Layout: `include/pluss/subsp/natsci/biology/dynamics.h` is the shared biology layer
(`Coeff` double-or-Series parameters, validation, batched `matvec`/`solve`, RK4 `simulate`
and `trajectory` over `math::ode::fluxa`); `population/{growth,interaction,selection,
structured,stochastic}.h` only state equations. Reused, not rebuilt: `math::ode::fluxa`,
`linearalg::inverse`, `pluss::tils::seed_or_next`, `codem::make_scalar_like`. States carry
species on the last axis; leading axes are replicate/sweep lanes; the only host loops are
the sequential time/event loops. Power iteration on `M + I` replaces a general eigensolver
(none exists in linalg).

Verification (`population/test/population_test.cpp`): logistic and Gompertz closed forms,
Allee equilibria/sign pattern, Lotka-Volterra vs SciPy DOP853 plus conserved invariant,
generalised LV and replicator flow vs SciPy with simplex preservation, Leslie
lambda/stable distribution/reproductive value/elasticity/R0/generation time vs NumPy
`eig`, batched lanes, and Gillespie moments (Poisson immigration-death, binomial death)
with seeded reproducibility.

Open for later passes: harvesting, migration, harvesting/stability via Jacobian (needs a
general eigensolver or Routh-Hurwitz), tau-leaping, SUNDIALS cross-check.

References read for B1 (pages fetched 2026-10-12/13): Wikipedia pages Lotka-Volterra
equations, Competitive Lotka-Volterra equations, Replicator equation, Gillespie
algorithm, Matrix population models, Allee effect, Gompertz function, Functional
response; popbio `eigen.analysis.R`; local MATLAB R2025b `lotka.m` and
`odeSensitivity.m` (behavioural reference only). Numerical oracle: SciPy `solve_ivp`
(DOP853, rtol/atol 1e-13) and NumPy `linalg.eig` from the `cpp_env` Python.

#### B2: systems biology (implemented 2026-10-13)

`biology/systems/{kinetics,network,gene_networks}.h`, sharing `biology/dynamics.h` and
the B1 Gillespie engine. `network.h` is the central piece: a mass-action `Network`
(net stoichiometry `[R,S]`, reactant orders `[R,S]`) with batched `flux`, `rate`,
analytic zero-safe `flux_jacobian`/`jacobian`, combinatorial SSA `propensity` (feeds
`population::stochastic::gillespie` unchanged), and `steady_state` (damped Newton on
the rate equations closed by linear conservation laws `L x = T`, lane-wise
fraction-to-boundary positivity). `kinetics.h`: Michaelis-Menten, Briggs-Haldane Km,
competitive inhibition, Hill activation/repression. `gene_networks.h`: repressilator,
toggle switch, Goodwin oscillator.

Verification (`systems/test/systems_test.cpp`): full enzyme mechanism vs SciPy DOP853 with
both conservation laws; analytic Jacobian vs central differences over 3 batched lanes;
dimer steady state vs closed form (single and batched totals); repressilator, toggle and
Goodwin vs SciPy; toggle bistability in two batched lanes; SSA moments vs the exact
binomial law of A<->B plus mass conservation; combinatorial propensity C(5,2)=10;
validation errors.

Not done: moiety conservation discovery (left null space needs an SVD/QR null-space
routine; `L` is currently supplied by the caller), bifurcation continuation, parameter
sensitivity/identifiability (route through `math::ode::fluxa` adjoint), tau-leaping, SBML
import, SUNDIALS/libRoadRunner cross-run (not installed).

References for B2: first drafted from Wikipedia pages (Michaelis-Menten, Hill, Repressilator, Goodwin,
CRNT; see the source policy below), then re-verified where possible. The toggle switch (Gardner
et al. 2000, eq. 1) and the repressilator (Elowitz & Leibler 2000, Box 1) equations were read
from the papers themselves; the repressilator beta convention and test parameters were
CORRECTED from that reading. Michaelis-Menten, Hill and Goodwin (Griffith n > 8, via Gonze &
Abou-Jaoude 2013) remain secondary-sourced; libRoadRunner docs were consulted for the
conservation-closed steady-state approach. The local MATLAB R2025b has no SimBiology
toolbox. Numerical oracle: SciPy DOP853 from `cpp_env`.

#### B3: biophysics (implemented 2026-10-13)

`biology/biophysics/{diffusion,membrane}.h`. Diffusion reuses `common::spatial::laplacian`
on `PeriodicGrid` (fields `[..., spatial...]`, leading axes are batch lanes; `Coeff` may be a
per-lane Series), so reaction-diffusion is `D lap(u) + f(u)` with any Series reaction.
Membrane: Nernst, GHK voltage (cation/anion roles by sign vector), Hodgkin-Huxley
(`m^3 h`, `n^4`, 1952 rates in the depolarisation convention with the removable
singularities of the alpha functions filled), and a passive cable
(`tau V_t = lambda^2 V_xx - V + drive`) on the same grid.

Verification (`biophysics/test/biophysics_test.cpp`): discrete Fourier-mode decay to 1e-10,
mass conservation, batched per-lane D; heat-kernel semigroup property; Fisher-KPP
Ablowitz-Zeppetella exact front residual with measured second-order spatial convergence
(ratio 3.5-4.6 between h and h/2), minimum-speed formula, front-speed ordering across batched
`r`; Nernst and GHK closed forms (and GHK -> Nernst limit); Hodgkin-Huxley rest gates and
three batched current lanes (0, 2, 10 uA/cm^2) vs SciPy DOP853 to 2e-6; cable mode decay and
steady state; validation errors.

Correction (same day, after reading the original paper): the Hodgkin-Huxley rate functions were sign-inverted; see the primary-source audit. Not done: membranes with spatial HH (cable + gating on the grid, trivial next step),
polymers, binding equilibria, population balance, biomechanics, compartment models; bounded
(non-periodic) domains wait for the P3f domain abstraction owned by the physics track.

References read for B3 (fetched 2026-10-13): Wikipedia *Hodgkin-Huxley model* (equations and
1952 rate functions; no parameter values on the page), *Goldman equation* (GHK voltage, Nernst),
*Fisher's equation* (minimum speed 2 sqrt(rD), Ablowitz-Zeppetella solution), *Cable theory*
(cable equation, lambda, tau); a web search for the HH 1952 parameter set (gNa=120, gK=36,
gL=0.3, ENa=115, EK=-12, EL=10.6, Cm=1) returned only secondary sources (an arXiv paper, the
Case neurowiki table, the Physiome CellML entry) and no copy of the original paper
(Hodgkin & Huxley 1952, J. Physiol. 117:500; PMC1392413), which I did NOT read. Oracle: SciPy
DOP853 from `cpp_env`; exact-solution and discrete-spectrum checks elsewhere.

#### Source policy (adopted 2026-10-13)

Wikipedia is NOT an acceptable authority for equations or parameters in NatSci. B1-B3
were first drafted from it; the equations that matter were then re-checked against the
primary papers where an open copy could be obtained (see "Primary-source audit" below), and
every later tranche is written from papers, reference implementations and the local MATLAB
install first. Each tranche reports which primary documents were actually read, with the
SHA-256 of the local copy, and which could not be obtained (paywalled or blocked).

#### Primary-source audit of B1-B3 (2026-10-13)

| Item | Primary source read | Result |
| --- | --- | --- |
| Toggle switch | Gardner, Cantor & Collins, Nature 403:339 (2000), eq. (1a,b); local PDF sha256 `936bb96f7ca8de9efdfa4e27628e4f7db67374a740025df035cba5c873d2c9a9` | Equations match the implementation exactly (du = a1/(1+v^b) - u, dv = a2/(1+u^g) - v). |
| Repressilator | Elowitz & Leibler, Nature 403:335 (2000), Box 1; local PDF sha256 `66f9cd860be4fe45c9355a7ec1bf8823046743908e66a25885a7a301a194fa7c` | Equations match. Correction: beta in the equation is the protein/mRNA decay-rate ratio (0.2 for 10 vs 2 min half-lives); Fig. 1b plots the inverse (5). The B2 test used beta = 5 (wrong convention) and now uses the paper's alpha=216, alpha0/alpha=1e-3, beta=0.2, n=2, whose steady state is linearly unstable (eigenvalues) and oscillates (limit cycle, p1 in about 1.25-59.2, SciPy). The paper's printed stability criterion could not be recovered from the garbled OCR (a reconstruction disagreed with the true eigenvalues) and is not implemented. |
| Hodgkin-Huxley 1952 | READ 2026-10-13: J. Physiol. 117:500-544, 45-page scan from the Caltech nb250c course site (sha256 `4e8ad32bc53f5572f8e6c64a84762ab7164527500be5bd838ee02f78e7ca30ce`), eqs. 7, 12, 13, 15, 16, 20, 21, 23, 24, 26 and Table 3, Fig. 12 | **Real bug found and fixed.** The paper's V has depolarisation NEGATIVE (V_Na = -115, V_K = +12, V_l = -10.613 mV). The implementation had been written from a Wikipedia-derived form that put the paper's rate functions into a depolarisation-POSITIVE variable with reversal potentials +115 / -12 / +10.613, so every gating rate ran in the wrong direction; the NumPy mirror shared the transcription, so the SciPy check could not catch it. Now every rate is the paper's with V -> -V. Checked against an independent implementation written directly in the paper's own sign convention (`biophysics/test/hh_paper_ref.py`, DOP853) to 2e-6, and against Fig. 12: 6 mV initial depolarisation is subthreshold, 7 and 15 mV fire ~100 mV action potentials, threshold bisection gives 6.502 mV. |
| Kimura 1962 fixation formula | NOT read: paywalled/403 | Used only as a diffusion approximation, validated against an exact Wright-Fisher chain (0.4% at N=40, s=0.01). |
| Goodwin oscillator, Griffith n > 8 | NOT read (secondary: Gonze & Abou-Jaoude 2013) | Equation form unverified against Goodwin 1965 / Griffith 1968. |
| Kermack-McKendrick 1927 | READ 2026-10-13: Proc. R. Soc. A 115:700-721, JSTOR scan from the UNM mathcamp site (sha256 `5a72dcf9dbf11706e3f1e9345c17e1a1222ce726aadc05898905934e3ed37609`), pages 702-711 | The general final-size relation, their eq. (20), `-log[(1-p)/(1-y0/N)] = A N p` with `p = (N - x_inf)/N` and `A` = infectivity x mean duration, is exactly the relation used for `final_susceptible` (and re-derived earlier); the special constant-rate SIR section (pp. 714ff) was not read. |
| Izhikevich 2003 | NOT obtained: the open copy redirects to an identity-provider login | (not yet implemented) |
| Fisher-KPP, cable, GHK, Nernst | NOT read (secondary) | Exact-solution / discrete-spectrum tests verify the numerics, not the provenance. |

Note on oracles: SciPy checks in B1-B3 confirm the integrator and the implementation
against an independent transcription of the same equations; only the primary-source rows
above verify that the equations themselves are the published ones.

#### Epidemiology (implemented 2026-10-13)

`biology/epidemiology/{compartmental,models}.h`. `compartmental.h` is the central piece:
`Split{F, V, m, infected_index}` and the next-generation matrix `F V^-1` / `R0 = rho(F V^-1)`
of van den Driessche & Watmough (2002), from batched finite-difference Jacobians
(`dynamics::jacobian`, second-order one-sided at the disease-free state) and the existing
`structured::dominant` spectral-radius routine; it checks the M-matrix sign pattern of V
(the paper's assumptions), so an epidemiologically inconsistent split is rejected.
`models.h`: SIR, SIS, SEIR with demography, Hethcote final size and peak, and the paper's
treatment (eq. 5), staged progression (eq. 9), multistrain (eq. 14) and vector-host (eq. 19)
models with their F/V splits. Stochastic epidemics reuse the mass-action `Network` + Gillespie
engine unchanged.

Verification (`epidemiology/test/epidemiology_test.cpp`): numerical R0 vs the paper's
closed forms (treatment eq. 6 and the untreated SEI limit, vector-host eq. 21, multistrain
eq. 17, staged progression eq. 13); batched R0 lanes; final-size root vs SciPy brentq and vs a
400-time-unit ODE integration (1e-9); peak infective vs trajectory; SEIR numerical
next-generation R0 and endemic equilibrium s* = 1/R0 (Hethcote section 3.2) vs a 600-time-unit
SciPy integration; population conservation; stochastic SIR attack rate (300 SSA replicates,
N = 1000) vs the deterministic final size within 0.015; split-consistency and input
validation errors.

Primary documents read: van den Driessche & Watmough, Math. Biosci. 180 (2002) 29-48,
author copy (sha256 `50a48e5b1de2a16cb0f1afefad6635060a4b92b085b3d86a369fb5825627dfb0`), sections 2-5 read in full for definitions, assumptions
(A1)-(A5), Lemma 1, Theorem 2 and sections 4.1-4.5; Hethcote, SIAM Review 42(4):599-653
(2000), JSTOR scan (sha256 `23ef51ffeda15f009a4eee81534c3c6021d5e9e67cec6aab8e438e669e85eab6`), theorem 2.1 text only because the displayed equations
were lost in the scan (final size relation re-derived from the quoted quotient ODE
di/ds = -1 + 1/(sigma s) and then checked independently against an ODE run). Not
obtained: Kermack & McKendrick 1927 (403). Reference implementations looked at: EoN
(Miller), `simulation.py` and `analytic.py` (network/percolation oriented; no well-mixed
compartmental formulas to compare), SciPy `integrate/_ivp/rk.py` (step-size control only).

#### Genetics (implemented 2026-10-13)

`biology/genetics/{markov,wright_fisher,moran}.h`. Shared `markov.h`: fundamental-matrix
absorption probabilities and times for any absorbing chain. `wright_fisher.h`: selection and
mutation maps, Hardy-Weinberg, an exact batched binomial sampler (inverse CDF over a
lgamma pmf), batched stochastic generations, the exact (2N+1)-state transition matrix, and
the Kimura diffusion fixation probability. `moran.h`: exact birth-death chain and its
closed form.

Verification (`genetics/test/genetics_test.cpp`): overdominance equilibrium t/(s+t);
mutation-selection balance q* = sqrt(mu/s); binomial pmf/moments; exact fixation probability
and absorption time vs an independent NumPy/SciPy chain (1e-10), Kimura within 0.5% of the
exact chain, neutral fixation = p0, neutral time vs the diffusion value to 2%; Moran chain =
closed form for all start states; simulated fixation probability (6000 replicates) vs the exact
chain within 4 sigma, selection raising fixation across batched lanes, seeded reproducibility,
mutation-drift stationarity.

Primary documents: Kimura 1962 could not be obtained (paywalled/403), so the diffusion
formula is validated rather than sourced. A Khatri bioRxiv preprint (sha256 prefix
`c08f2301e449b7f8`) and the graph-Moran arXiv literature were consulted for context, and the Moran
result is proved here by solving the chain exactly.

#### Engine fixes found while writing NatSci (2026-10-13)

Cross-checked against the local PyTorch checkout (`aten/src/ATen/ScalarOps.h`, `native/LinearAlgebra.cpp`).
1. **Scalar operands were rank 1.** `Series` arithmetic with a scalar (`x.add(1.0)`, `x.mul(2.0)`,
   `x.pow(2.0)`, `x.gt(0.5)`, and the `alpha` multiply inside `add`/`sub`) built a shape-`[1]` constant, so every
   0-d operand came back rank 1. Torch wraps a scalar as a 0-dim "wrapped number". Fixed in `OpsInternal.cpp`
   (`scalar_like_`, `scalar_to_series_`, scalar `pow`) and `compat/Invoke.cpp` (`scalar_like`).
   `test_patch67_autodiff_transforms` had encoded the bug (`nth_diff` of a scalar returning `[1,3,3]`).
2. **Fractional scalars truncated on integer tensors.** `int64_tensor.add(0.5)` stayed int64 with the constant
   truncated to 0 (`[0,1,2,3] + 0.5 = [0,1,2,3]`), and `eq(0.5)` matched 0. Torch promotes to a floating result.
   A shared rule `ops_internal::scalar_operand_dtype` (OpsInternal.h) now builds a float32 constant when the
   tensor is integer/bool and the value is fractional or non-finite (an integral value keeps the integer dtype,
   since a C++ double cannot be told from a Python int); used by all three scalar-constant helpers.
3. **Batched matmul looped over batch pages on the host** (one Series per page, then `pile`). Torch folds
   `[..., n, k] @ [k, p]` into one GEMM (`should_fold`). `native/cpu/Matmul.cpp` now folds `[..., n, k] @ [k, p]`
   and `@ [k]`, folds `[n, k] @ [..., k, p]` through a permute, and calls the GEMM on buffer offsets for equal-batch
   f32/f64 inputs; broadcast batches keep the old path. This was the main cost behind the 4.4x speedup found when
   `dynamics::matvec` was changed to fold lanes into one GEMM (that workaround is kept).
Regression tests: `PlussQit/tests/CodeM/test_scalar_operand_semantics.cpp`, `test_matmul_batched_fold.cpp`
(every batch layout against per-page reference products, plus a many-lane timing check). Full CodeM suite:
684 test cases / 523,381 assertions pass.

Open engine observation (not changed, risky to alter silently): integer true-division promotes to Float64 here
while torch gives the default dtype (float32).

#### Chemistry kinetics and thermodynamics (implemented 2026-10-13)

Shared machinery was promoted from biology into `common/` so chemistry and biology use one
copy (biology keeps its old spellings as aliases): `common/dynamics.h` (Coeff, validation,
batched matvec/solve, finite-difference Jacobians, RK4 simulate/trajectory),
`common/reaction.h` (mass-action `Network`: flux, rate, analytic Jacobian, combinatorial
propensity, conservation-closed Newton steady state), `common/ssa.h` (batched Gillespie), and a
new `common/stiff.h`: an adaptive, lane-batched port of the Shampine-Reichelt modified
Rosenbrock (2,3) pair (`ode23s`) for autonomous stiff systems.

`chemistry/{thermo,kinetics,reactor}.h`:
 * `thermo.h`: NASA-7 polynomials batched over temperature lanes (cp/R, h/RT, s/R, g/RT),
   validity-range check, shared CODATA gas constant.
 * `kinetics.h`: `Mechanism` assembled from species and `ReactionSpec`s (elementary,
   three-body with efficiencies, Lindemann/Troe falloff, reversible or not), CGS/cal input
   units converted to SI, forward rate constants, equilibrium constants
   `Kc = exp(-dG0/RT) (p0/RT)^dn`, reverse rates `kr = kf/Kc`, rates of progress and net
   production rates, all batched over lanes of temperature and concentration.
 * `reactor.h`: closed constant-volume reactor (adiabatic with the Cantera `IdealGasReactor`
   energy equation, or isothermal), pressure, internal energy, element totals, ignition delay
   by maximum dT/dt.

Verification: `chemistry/kinetics/test/kinetics_test.cpp` against an independent NumPy
implementation written natively in CGS/cal units (so it also checks the unit conversions;
data dumped from the same mechanism fixture): all 29 forward rate constants and equilibrium
constants at 3 temperatures (1e-10 / 1e-9), net production rates of all species, a hand
Arrhenius value, detailed balance at Kc, irreversible reactions, ignition delays at 1000 /
1100 / 1200 K within 1.5% of a Radau reference, final state after 5 ms, conservation of
elements and of internal energy along the path, non-negativity. Thermodynamic data checked
against NIST WebBook / NIST-JANAF (H2O formation enthalpy -241.826 kJ/mol and S = 188.835,
H2, O2, OH entropies), polynomial continuity at 1000 K to 1e-9 for every species (this also
validates the transcription of all 120 coefficients), and dh/dT = cp, ds/dT = cp/T.
`common/test/stiff_test.cpp`: exact decay at lambda = 1e6 (explicit RK4 would need > 3.5e5
steps), Robertson vs SciPy Radau, Van der Pol (mu = 100) vs Radau, analytic vs
finite-difference Jacobian, multi-lane agreement, output grids.

Primary documents read (GitHub `main`, 2026-10-13): Cantera `include/cantera/kinetics/
Arrhenius.h`, `Falloff.h`, `src/kinetics/Falloff.cpp`, `BulkKinetics.cpp`,
`include/cantera/thermo/NasaPoly1.h`, `src/thermo/SpeciesThermoFactory.cpp`,
`IdealGasPhase.cpp`, `src/zeroD/IdealGasReactor.cpp`, `data/h2o2.yaml` (mechanism and NASA data;
fetched through a text-extraction tool, so transcription was cross-checked as above, rate
parameters are NOT independently verified against the file). Local MATLAB R2025b
`toolbox/matlab/funfun/ode23s.m` (sha256 prefix `117b8a2fe1e29aed`) and
`+matlab/+ode/+internal/DESolver.m` for the Rosenbrock coefficients, error estimate, step control
and threshold = AbsTol/RelTol. NIST WebBook pages for H2O, H2, O2, OH. Not read: the original
papers (Shampine & Reichelt 1997; Gilbert, Luther & Troe 1983 for Troe falloff; the CHEMKIN
manual).
Convention note: the NASA coefficients reproduce NIST's 1-bar standard-state entropies, while
Cantera's default reference pressure for these data is 1 atm; `Kc` uses p0 = 1 atm to match the
reference implementation.

#### Chemistry equilibrium (implemented 2026-10-13)

`chemistry/equilibrium.h`: gas-phase chemical equilibrium for the four standard problems (assigned
T,P / H,P / T,V / U,V) by the element-potential Newton method of Gordon & McBride, NASA RP-1311
(1994): reduced iteration systems in (pi_i, dln n, dln T), species corrections, the lambda
step control, and log-variable updates, all batched over lanes (T, P, V, energy and element
totals may each carry leading axes). Elements that are absent in a lane are dropped together with
every species containing them (impossible species carry exactly zero moles). Condensed phases and
ions are out of scope.

Verification (`chemistry/equilibrium/test/equilibrium_test.cpp`): three (T,P) states (2000 K and
3000 K at 1 and 10 atm) against an independent dual formulation (SciPy `fsolve` on the
element-potential equations, started from SLSQP Gibbs minimisation and continued in T and P) to
1e-8 relative; the final element potentials satisfy mu_j/RT = sum_i pi_i a_ij and the element
balance to 1e-8 / 1e-10; batched lanes equal the scalar runs; H,P adiabatic flame temperatures
(2 H2 + O2 + 7 Ar from 298.15 K: 2266.47 K; stoichiometric H2/air, no NOx: 2386.68 K, inside the
usual 2380-2390 K range) with the enthalpy balance; U,V equilibrium of the reactor test state
reproduces the Radau-integrated reactor's temperature at 5 ms (2936.1636 K) to 1e-4 K and its
H2O, H2, O2, OH concentrations to 0.2% -- an end-to-end consistency check of the thermodynamic
data, the reverse rates from Kc and the equilibrium solver; T,V equals T,P at the induced
pressure.

Primary document read: Gordon & McBride, NASA RP-1311, Part I (NTRS 19950013764, scanned PDF
viewed page by page; sha256 `6241ca2eece86c36edf82bc400039bc83f137223ebf1db7e3eb0186e6b23d097`):
sections 2.2-2.4 (equations 2.5-2.47), 3.1-3.3 (initial estimates, step control, convergence
criteria 3.1-3.6). Not used: condensed-phase tests (section 3.4), derivatives (2.5), the transport
chapter, rocket/shock/detonation chapters. Cantera's own equilibrium solvers were not read.

Cantera run (added 2026-10-13, after the user approved installing it into an isolated venv; Cantera
3.2.0, `data/h2o2.yaml` shipped with the wheel): `chemistry/verification/compare_cantera.py`. Results:
NASA-7 coefficients identical (max difference 0.0); all 29 reactions match (net stoichiometry, Arrhenius,
third-body efficiencies, Troe coefficients), which independently verifies the transcription of the
mechanism; net production rates agree to 1.7e-14 and equilibrium constants to 2.1e-14 relative at 1000,
1500 and 2500 K; closed adiabatic constant-volume ignition delays at 1000/1100/1200 K agree to 0.025%
(sampling resolution) and final temperatures/concentrations after 5 ms to all printed digits; TP
equilibrium compositions agree to 2e-14 relative at three states and HP / UV temperatures to 1e-6 K
(2266.469903, 2386.678566, 2936.163625 K). One real difference found and fixed: at exactly the NASA mid
temperature Cantera uses the low-range polynomial (`T <= Tmid`), this code used the high range, a 1.7e-7
relative difference only at T = 1000 K; the convention now matches Cantera. Cantera's reference pressure
is confirmed as 1 atm (`Solution.reference_pressure` = 101325 Pa).

## Reference Baseline

References are consulted and pinned by publication/revision; their code is not vendored.
PlussMa owns its implementation and test vectors.

The cross-domain lookup policy and source selection table live in
[`../REFERENCE_IMPLEMENTATION_MAP.md`](../REFERENCE_IMPLEMENTATION_MAP.md). This section
records NatSci-specific revisions and artifacts.

| Area | Authoritative source | Use in PlussMa |
| --- | --- | --- |
| Quantity type safety | mp-units, C++20 reference implementation, commit `e228418b9075ca747d57c65065960034f206b5d4` inspected 2026-10-10 | Dimension algebra, quantity/unit separation, and rejection of invalid operations |
| SI definitions | BIPM, *The International System of Units (SI Brochure)*, 9th edition | Base dimensions, coherent derived units, and exact SI defining constants |
| Physical constants | Mohr, Newell, Taylor, and Tiesinga, *CODATA recommended values of the fundamental physical constants: 2022*, J. Phys. Chem. Ref. Data 54, 033105 (2025), DOI `10.1063/5.0279860` | Values, absolute standard uncertainties, exactness, symbols, and revision metadata |
| Measurement uncertainty | JCGM 100:2008, *Evaluation of measurement data - Guide to the expression of uncertainty in measurement* | Standard uncertainty terminology and first-order propagation law |
| Constants metadata | Astropy `Constant` public contract | Provenance fields: name, symbol, value, unit context, uncertainty, reference, and system |
| ODE methods | Hairer, Norsett, and Wanner, *Solving Ordinary Differential Equations I* | Solver selection and validation; implementations remain in PlussMa Math |
| Stiff kinetics | Hairer and Wanner, *Solving Ordinary Differential Equations II* | Stiffness policy and stiff-reactor verification |
| Higher-order automatic differentiation | Local PyTorch revision `470132c6a10d1279ee66a43778edb91f1032512f`, `torch/autograd/functional.py` and `torch/_functorch/eager_transforms.py`, inspected 2026-10-11 | Jacobian output-plus-input shape convention, differentiable Jacobian construction, strict disconnected-input policy, and transform composition; Pluss extends composition to arbitrary order |
| Numerical optimization | Local MATLAB R2025b Optimization Toolbox at `/Applications/MATLAB_R2025b.app/toolbox/optim` | Public contracts, option semantics, visible `.m` and coder-source algorithms, diagnostics, and independent executable parity cases; protected `.p` and MEX internals are treated only as black-box behavior |
| Orbital mechanics | Vallado, *Fundamentals of Astrodynamics and Applications* | Coordinate and orbital-element conventions |
| Quantum mechanics | Nielsen and Chuang; QuTiP reference implementation | State/operator contracts, tensor composition, partial traces, and open systems |

Primary links used by the first tranche:

- <https://github.com/mpusz/mp-units>
- <https://www.bipm.org/en/publications/si-brochure>
- <https://physics.nist.gov/cuu/pdf/JPCRD2022CODATA.pdf>
- <https://www.bipm.org/en/committees/jc/jcgm/publications>
- <https://docs.astropy.org/en/stable/api/astropy.constants.Constant.html>

Reference artifact hashes used for the first tranche:

- NIST CODATA 2022 PDF: `4d7e7f34b98ab2fc4df68b38247f818f6fc8bdf7f25f91abcdfbc329e22d2f32`
- JCGM 100:2008 PDF: `41bbf068fbc0d7986c98691b2d1af6680cb3044f6a1a89b3560933ed9ef9626c`

MATLAB is a correctness oracle, not an implementation donor. For a relevant family, the
audit records the inspected public entry points and visible sources, constructs independent
inputs that exercise options and failures, captures MATLAB outputs where execution is
available, and compares those results with PlussMa. Protected `.p` files and compiled MEX
modules are neither decompiled nor presented as inspected source.

## Acceptance Rules

Every public NatSci family must include:

1. A recorded full-stack reuse audit and an identified canonical owner for each shared
   behavior.
2. Compile-time contract tests for dimensional validity where applicable.
3. Independent numerical references or published benchmark values.
4. Scalar and multidimensional batched `Series` paths when the operation is naturally
   vectorizable.
5. CPU correctness before backend-specific acceleration.
6. Conservation, normalization, or positivity checks dictated by the model.
7. Stable diagnostics for invalid dimensions, units, shapes, domains, and parameters.
8. Autodiff checks for every continuous Series-native operation.
9. Benchmarks before introducing a fused kernel or cache, plus parity against the canonical
   uncached Series composition.
10. Exact executable-derived examples before documentation claims an output.
11. Source/version metadata for empirical constants and parameterizations.


## Chemistry: transport, electrochemistry, spectroscopy (breadth pass, 2026-10-14)

**Transport** (`chemistry/transport.h`): Chapman–Enskog pure-species viscosity/conductivity/binary diffusion
(Cantera `GasTransport.cpp`, `MixTransport.cpp`), mixture rules (Wilke/Mathur–Saxena/Blanc-type, CHEMKIN and
mole-fraction diffusion forms), collision integrals from the Neufeld analytic fits. Verified against Cantera
3.2.0 (pure species within ~0.4%, difference = Neufeld fits vs Cantera's Monchick–Mason tables, which could not be
extracted). Gap: polar species (water) unsupported. Read: Cantera transport sources, `MMCollisionInt.cpp`.

**Electrochemistry** (`common/electrochem.h`): Nernst, Faraday, Butler–Volmer (batched overpotential inversion via
`math/roots.h` Illinois), Tafel slope, Arrhenius exchange current, Debye–Hückel limiting/Davies activities, ionic
strength, polarisation curve. Shared with biology membranes. Tests in `chemistry/electrochemistry/test`.

**Spectroscopy** (`chemistry/spectroscopy.h`, test `chemistry/spectroscopy/test`): Beer–Lambert (matmul-style
batched absorbance), Gaussian/Lorentzian/Voigt lineshapes, Doppler width, Olivero–Longbothum FWHM, anharmonic
diatomic rotational/vibrational ladders (partition function, cv/R from level variance, exact for the model).
Voigt uses the Weideman (1994) N=32 rational Faddeeva approximation in real pair arithmetic, verified against
SciPy `wofz`/`voigt_profile` (<1e-10). cp/R of N2 from statistical mechanics (NIST WebBook constants) agrees with
NASA-7 within 1%; rotational classical limit checked with the Mulholland expansion. Limitations: diatomics only,
no electronic excitation, no line-list (HITRAN) ingestion.

## Biology breadth pass 2: pharmacokinetics, binding/speciation, sensitivity (2026-10-14)

All three are built from ND Series ops (batched `matrix_exp`, batched Newton, one batched Jacobian call), no host loops over lanes.

- **Pharmacokinetics** (`biology/pharmacokinetics/compartments.h`): `rate_matrix` (mass-balance K from transfer and elimination rates), `propagator`/`response` (K `[B,n,n]` x times `[T]` -> one `[B,T,n,n]` batched expm), `infusion_response` (augmented exponential), `auc_infinity` (-K^-1 x0), `steady_state_trough`, `bateman` (with ka=ke limit), `two_compartment_macro`. Oracles: SciPy `expm`/`solve` on random 3-compartment lanes (<1e-11), closed-form Bateman, tr/det of K, reverse-mode gradient through `matrix_exp` vs central differences.
- **Speciation engine** (`common/speciation.h`): stoichiometric mass-action solve, Newton on ln x with the SPD Jacobian diag(x)+N diag(c) N^T and a step cap; shared by biology and chemistry (complexation). Positive totals only (acid-base with OH- needs a separate proton-balance formulation; not done).
- **Binding** (`biology/systems/binding.h`): ligand-depletion 1:1 binding (cancellation-free quadratic), MWC allostery, multi-ligand competition, Adair n-site binding with depletion. Oracles: SciPy `brentq` on the mass-balance scalar equations (<=1e-8 rel), explicit binomial-state enumeration for MWC, MWC(c=1)=Langmuir, a 20^4 lane grid satisfying mass action. MWC (Monod, Wyman, Changeux 1965) and Adair primary papers were NOT obtained; formulas checked only by independent enumeration, not against the papers.
- **Sensitivity / identifiability** (`common/sensitivity.h`): forward sensitivities integrated as one augmented system (J_y and J_p each from one batched central-difference call), Fisher information, spectrum, numerical rank, Cramer-Rao std, normalised sensitivities. Oracles: closed-form decay and logistic sensitivities, finite difference of whole Michaelis-Menten simulations, a non-identifiable pair (rank 1) vs identifiable cascade (rank 2). Limitation: finite-difference Jacobians (autodiff Jacobian would be exact; RK4 fixed step; no bifurcation continuation yet).

## Biology breadth pass 3: constraint-based metabolism on a new batched LP solver (2026-10-14)

- **LP solver** (`include/pluss/subsp/math/linprog.h`, verification in `Math/Numerical/optimization/verification/linprog/`): PlussMa had no LP solver
  (Optima matched HiGHS on 0/12 (interior point), 5/12 (SQP), 8/12 (working set) small LPs; the repo's "simplex" is Nelder-Mead). Built a batched
  homogeneous self-dual interior-point method with implicit bounds, infeasibility / unboundedness certificates and lanes that share the matrices.
  Verified: ~33,000 differential problems vs HiGHS (certificates checked in numpy), NETLIB (afiro ... israel) vs published optima, a C++ test that certifies
  every answer against the original data (373 problems), and a mutation check.
- **FBA** (`biology/metabolism/fba.h`): `flux_balance`, `flux_variability` (all 2r LPs in ONE batched call), `knockouts`/`single_knockouts`, `sweep_flux`.
  e_coli_core (COBRApy 'textbook'): optimum 0.873922 (equal to COBRApy), FVA worst |diff| 1.1e-8 (fraction 1) / 2.6e-7 (fraction 0.9) over 190 bounds,
  95 single knock-outs worst |diff| 3.3e-10, glucose-uptake sweep exact; plus a hand-solvable toy network (shadow prices, FVA, knock-outs, sweep).
  Limitations: reaction (not gene) knock-outs; no loopless FVA; dense normal equations (genome-scale iJO1366 not yet run); NearOptimal lanes are accurate to ~5e-8.

## Biology breadth pass 4: gene knockouts, loopless FVA, MILP, genome scale (2026-10-14)

- **MILP** (`include/pluss/subsp/math/milp.h`, namespace `pluss::subsp::math::milp`): branch-and-bound whose open nodes are lanes of one batched LP call (interior point) or
  warm-started dual-simplex solves (exact). Verified against HiGHS `milp` (gap 0) and brute force: 2,440 campaign solves + a 52-problem permanent test, both backends.
- **LP solver upgrades** (`linprog.h`, namespace `pluss::subsp::math::linprog`): exact simplex backend with warm start, Ruiz scaling, sparse assembly, lane chunking, NearOptimal status.
  Public and general (any `min c^T x` with rows and bounds, any lane shape); see `Math/Numerical/optimization/verification/linprog/README.md`.
- **Gene knockouts** (`biology/metabolism/genes.h`): GPR rule parser/compiler -> `[K, r]` activity matrix with elementwise min/max, then ONE batched/warm-started LP sweep.
  e_coli_core 137 single and 40 double deletions match COBRApy to 9e-13.
- **Loopless FVA** (`metabolism/loopless.h`): CycleFreeFlux cleaning + COBRApy's second stage as batched simplex stages; matches COBRApy's `loopless="cycleFreeFlux"` to 1.2e-11
  over 190 bounds. `loopless_range_exact` is the exact loop-law MILP (Schellenberger et al. 2011) restricted to the support of the cycle space; it confirmed the disputed
  reactions (FRD7 max 0, SUCDi min 5.064) and agrees with the analytic toy loop network.
- **Genome scale** (iJO1366, 1805 x 2583, `metabolism/test/genome_scale_test.cpp`, CTest label `slow`, ~5 min): exact FBA 0.982372 (= COBRApy) in ~170 s (cold simplex; the interior point returned NearOptimal with a
  0.46% error); 16 sampled FVA LPs warm-started in 5.4 s (<= 16 pivots each, worst |diff| 1.9e-9); 12 sampled gene deletions through the GPR rules by warm dual simplex in 118 s (worst |diff| 2.7e-11).
  Hidden bug found here: the optimal basis contains artificial columns for dependent rows (102 of them), and the first warm-start validator rejected such bases, silently cold-starting every lane (~200 s each);
  `Result::warm_started` now makes engagement observable and the tests assert it. Limits: dense basis inverse (~30 ms/pivot at m = 1805, no LU / sparse updates), so a full 2r-lane genome-scale FVA is hours.
- Lessons: pinned objective floors have almost no interior (use the simplex); `Series::where` needs a Bool receiver; every new solver path found a bug in an older one (empty-column scaling, unsound node pruning).

## Industry-scale LP / MILP (2026-10-14)

- New sparse stack `include/pluss/subsp/math/lp/` (use as `namespace lp = pluss::subsp::math::lp;`, no root alias): sparse LU + dual/primal revised simplex, presolve/postsolve, sparse interior point with crossover,
  branch-and-cut (Gomory, c-MIR, covers, strong branching, dives, RENS/RINS), MPS reader, `series_bridge.h` (`lp::solve_sparse` for `linprog::Problem`/`milp::Problem`). Included from `pluss/ma/math/optimization.h`.
- Verification: `Math/Numerical/optimization/test/lp_sparse_test.cpp` (CTest) + `verification/sparse/README.md`. NETLIB 88/88 vs HiGHS; random MILP campaigns (30 seeds) vs HiGHS clean.
- MIPLIB subset, 60 s limit, single thread: 14 of 26 proven optimal and equal to HiGHS (blend2, cap6000, dcmulti, fiber, gt2, khb05250, mas76, misc07, mod010, nw04, p0201, pk1, qiu, qnet1, rentacar);
  the rest stop at the time limit with valid bounds and no false claims (10teams, air04, air05, arki001, bienst1, danoint, markshare2, mas74, neos-5013590-toitoi, noswot, rout).
- Soundness bugs found by this verification and fixed: presolve crossing-bound overwrite (infeasible model reported solved), coefficient-tightening shift/implied-bound errors (rentacar), IPM "unbounded" on infeasible models.
- Open (performance, not correctness): hypersparse solves and partial pricing, supernodal Cholesky, clique cuts / node propagation / probing / conflict analysis, multithreading.

## Bifurcation continuation (2026-10-14)

- `natsci/common/continuation.h` (namespace `common::continuation`): pseudo-arclength continuation of `G(u, p) = 0` (Keller 1977), derivatives by central differences with all 2n+3 evaluation lanes in ONE batched residual call
  (`G(u [K,n], p [K]) -> [K,n]`); bordered (n+1) linear algebra on the host (pivoting + determinant signs). Detects and refines (bisection with the Newton corrector) folds (tangent p-component sign),
  branch points (sign of det [G_u G_p; t^T], which does not vanish at folds) and Hopf points (unstable-eigenvalue count from `stability.h` changes by two with det G_u fixed); Hopf frequency from the characteristic polynomial on the
  imaginary axis. `branch_directions` returns the tangents of all branches through a branch point from the algebraic bifurcation equation (second directional derivatives), enabling branch switching.
- Verified (`NatSci/common/test/continuation_test.cpp`, CTest): cubic fold at `p = +-2/(3 sqrt 3)`; pitchfork `p u - u^3` (branch point at 0, switching onto `u^2 = p`); transcritical; Brusselator Hopf at `B = 1 + A^2`, `omega = A`;
  Lorenz: pitchfork at `r = 1`, switching onto the convective branch, Hopf at `r = sigma (sigma+b+3)/(sigma-b-1)` with `omega^2 = b (sigma + r_H)` (from the characteristic polynomial).
- Not yet: periodic-orbit continuation, codimension-two curves (fold/Hopf in two parameters), first Lyapunov coefficient.

- Update (same day): hypersparse LU solves (reach-based FTRAN/BTRAN), index-list vector updates in the dual simplex (no O(m) loops left except cheap scans), incremental CHUZR, node propagation, root probing, clique cuts.
  LP: NETLIB 90/90 and Kennington 16/16 equal to HiGHS (ken-18, 105k rows: unfinished after 227 s before, 58 s now; pds-10 12 s -> 3 s; ken-11 10 s -> 1.4 s). MIPLIB 60 s: 14/26 proven optimal and equal to HiGHS
  (qiu now ends at the limit with the optimal incumbent; nw04 9 s vs 2.7 s before because probing costs time on 87k columns). Still open: conflict analysis, supernodal Cholesky, multithreading, closing the gap to HiGHS on large degenerate LPs (cre-b/osa-60 ~20 s vs ~1-14 s).

- Update (2026-10-15), after reading the HiGHS simplex source: (1) postsolve now rebuilds the basis for removed singleton rows (the column becomes basic, the row non-basic), so the warm start on the original model needs ~0 pivots
  (ken-18 with duals 49 s -> 17 s, pds-20 56 -> 30 s, pds-10 3.3 -> 1.7 s); (2) implied-free column singletons of equality rows are eliminated with their row (primal postsolve from the original row, verified by mutation);
  (3) `lp::solve` with `LpMethod::Auto` races presolve+simplex against the interior point with crossover on the original model for large models (two threads, cooperative cancellation); (4) refactor interval 300;
  (5) the primal shortcut after postsolve is guarded by an objective + feasibility check (it returned wrong "optimal" values without the guard). NETLIB 90/90 and Kennington 16/16 equal HiGHS with dual certificates; MIPLIB 14/26 optimal.
  Remaining gap vs HiGHS: presolve strength (doubleton-equation aggregation, dominated columns need a dynamic matrix) - pds-20 30 s vs 1.2 s, dfl001 25 s vs 4.9 s, ken-18 17 s vs 3.8 s.

- Update (2026-10-15, later): `lp/aggregate.h` is the layered LP-only second presolve stage the user agreed to (dynamic row/column lists, doubleton-equation aggregation, postsolve of x and of the basis including the case where a column sits at a bound
  inherited from an eliminated column); `presolve.h` is untouched and still the only presolve MILP uses. `lp::solve` chains presolve -> aggregate -> presolve ... until nothing changes. Verified: 300 random doubleton-rich LPs (primal-only and dual
  certificate paths, mutation-checked), NETLIB 90 + Kennington 16 vs HiGHS with dual certificates. Times (with duals, race on): ken-18 17 s -> 5.9 s (HiGHS 3.8 s), pds-20 30 s -> 14 s (10.5 s simplex only; HiGHS 1.2 s), cre-b 3.3 s -> 1.7 s;
  Kennington total 84 s -> 52 s. Remaining: dfl001 27 s vs 4.9 s (our LU has no Markowitz ordering, so fill is higher), osa-60, dominated-column presolve.

## Chemistry: polymers (2026-10-15)

- `chemistry/polymers.h` (namespace `chemistry::polymers`, test `NatSci/chemistry/polymers/test/polymers_test.cpp`, reference generator `NatSci/chemistry/verification/gen_polymers_ref.py`), all batched over lanes:
  worm-like / freely-jointed / freely-rotating chain statistics and force-extension (Langevin, Marko-Siggia with batched bisection inversion); Flory-Schulz, Carothers and Poisson distributions with Mn/Mw/Mz;
  free-radical polymerization by the closed method of moments (initiator, propagation, combination + disproportionation termination, transfer to monomer) with the quasi-steady-state results;
  Mayo-Lewis, azeotrope, Skeist drift and Meyer-Lowry; Flory-Huggins free energy, chemical potentials, spinodal, critical point and the binodal.
- Verification (every oracle independent of the formula under test): worm-like chain <R^2> and <Rg^2> against quadrature of their definitions (rel. 1e-9); freely-rotating chain against the brute-force double sum, and its continuum limit against the worm-like chain;
  Langevin against mpmath (50 digits, including the small-force series branch); distributions against scipy.stats; **radical moments against chain-length-resolved kinetics** (every live and dead length an explicit species, combination by convolution, scipy Radau;
  worst relative error 2e-5 over 5 time points, monomer mass balance 1e-7); Skeist ODE against DOP853 and the Meyer-Lowry closed form against both (1e-9); Flory-Huggins symmetric binodal against the exact relation ln((1+m)/(1-m)) = chi N m
  and asymmetric binodals against scipy fsolve (down to a dilute phase of 8e-6), plus the coexistence residuals (1e-11).
- Bugs caught while building it: the chain-resolved oracle needs N >> the chain length (a 700-species truncation hid a 5e-4 error that was the oracle's, not the model's; N = 1200); the binodal Newton collapsed onto the trivial root a = b until the
  equations were divided by the gap (centre/half-gap variables, Landau-expansion start, quadratic chi continuation); `max_rel` helper ignored NaN (std::max) and falsely passed - it now fails on non-finite values.
- Sources: NOT read in this session (paywalled or not fetched): Kratky & Porod 1949, Benoit & Doty 1953, Marko & Siggia 1995, Flory 1936/1940/1942, Mayo & Lewis 1944, Meyer & Lowry 1965, Odian 4th ed., Kiparissides 1996. The formulas are standard and every one is
  checked numerically against its own definition or a first-principles simulation as listed above, but the literature itself is unread here; a primary-source pass would be the next step if wanted.
- Not yet: step-growth with full distribution kinetics, gelation (Flory-Stockmayer), polymer dynamics (Rouse/Zimm), copolymer sequence statistics beyond the terminal model, residual-monomer/diffusion-controlled (gel effect) termination.

## LP / MILP: deferred TODO list (recorded 2026-10-15, to resume later)

1. Forrest-Tomlin basis update (replace the product-form eta file) - the main cause of the remaining gap on dense bases: pilot87 ~8 s vs 2.9 s, dfl001 ~9 s vs 4.9 s, osa-60 ~15 s vs 13 s.
2. MILP conflict analysis (HiGHS-style) - node counts are still above HiGHS on several MIPLIB instances (air04/air05/10teams/bienst1/danoint/markshare2/mas74/noswot/qiu/rout unsolved in 60 s).
3. Supernodal sparse Cholesky for the interior point (pds-class problems and anything where the interior point should win).
4. Multithreading (parallel interior point, parallel branch-and-bound / dives); today everything is single-threaded except the opt-in `LpMethod::Race`.
5. Smaller items: dominated-column presolve, `dual implied free` rows for inequalities, partial pricing for very wide LPs, tighter race scheduling (the race currently costs more than it gains, so `Auto` = simplex).

## Population balance (2026-10-15)

- `common/population_balance.h` (namespace `common::population_balance`, test `NatSci/common/test/population_balance_test.cpp`), shared by chemistry (crystallization, aggregation, breakage) and biology (size-structured populations), batched over lanes:
  uniform / geometric / integer grids; exact cell moments; SSP-RK3 (Shu-Osher); growth + nucleation + outflow by cell-centred finite volumes with van Leer MUSCL reconstruction (second order on smooth profiles, monotone at fronts,
  nucleation as an inflow flux with a linear-extrapolation ghost cell); aggregation and breakage on a fixed pivot grid (Kumar & Ramkrishna 1996) as dense assignment tensors, so a right-hand side is a few matrix products over all lanes.
- Verified against closed forms (no second implementation of the scheme): pure translation (second-order convergence, ratio > 3 on 100/200/400 cells; number conserved to 1e-12); size-dependent growth G = a(1 + bL) against the characteristics;
  nucleation with growth (mu_0 = flux * t exactly, mu_1 = G B t^2 / 2); MSMPR crystallizer steady state n = (B/G) exp(-L/(G tau)) with total number B tau and second moment; discrete Smoluchowski with the constant kernel against the exact
  N_k = a^{k-1}/(1+a)^{k+1}, additive kernel M0 = exp(-M1 t), product kernel before gelation (M0 = 1 - t/2, M2 = 1/(1-t)); fixed-pivot aggregation on a coarse geometric grid reproduces the exact N(t) = N0/(1 + K N0 t/2) and conserves mass to 1e-9;
  breakage with constant selection rate and uniform daughters conserves mass to 1e-12 and M0 grows like exp(t).
- Findings while building it: domains must exceed the tail (the MSMPR second moment is 3% off with a 6-sigma-short domain); fragments below the lowest bin edge must be included in the breakage operator or mass leaks (1e-11 per event);
  the initial first-order boundary cell was fixed by a linear-extrapolation ghost through the inflow state. Time: the discrete aggregation tests dominate (~30 s) - the pair tensor is dense B^2 x B.
- Not yet: quadrature method of moments (QMOM / DQMOM), size-structured cell division with a Malthusian eigenvalue, non-uniform-grid limiter, coupled crystallizer with supersaturation balance.

- Polymers, second tranche (2026-10-15): `chemistry/step_growth.h` and `chemistry/polymer_dynamics.h` (both in the `polymers_test`).
  Step growth: second-order, third-order (self-catalysed) and stoichiometrically-imbalanced conversion kinetics (closed forms checked against DOP853 integration of the rate laws to 1e-10); closed moment equations M0..M3 reproducing the Flory
  distribution averages; AB_{f-1} branching X_n, X_w and gel point checked against the Smoluchowski kinetics of the reactive group pairs (kernel (f-2)(i+j)+2, LSODA, N = 900; agreement ~1e-6 for f = 3 and 4, X_w -> 17 at a = 0.4);
  A_f + B_g gelation: critical conversion 1/sqrt(r(f-1)(g-1)) and the sol fraction from the Stockmayer / Macosko-Miller branching generating functions (batched bisection for the smallest root) checked against Monte Carlo random bonding of 10^5 molecules
  (union-find components, agreement < 8e-3, six cases incl. f = g = 3).
  Rouse / Zimm: exact Rouse modes (eigenvalues vs eigvalsh, Doi-Edwards large-N limits tau_p, viscosity c zeta N^2 b^2 / 36, D = kT/(N zeta), G(t)); Zimm with the preaveraged Oseen interaction (spectrum of L^T A L from the Cholesky factor of H and `linalg::eigh`, vs numpy eigvals(H A) to 1e-9;
  D = kT/(N^2 zeta) sum H, fitted exponent between 0.45 and 0.75 approaching 1/2, h* = 0 recovers Rouse).
- Still open in polymers: reversible / cyclising step growth, diffusion-controlled (gel-effect) termination, entangled dynamics (reptation), copolymer sequence statistics beyond the terminal model. Sources again unread (Flory 1936/1953, Stockmayer 1943-44, Macosko & Miller 1976, Rouse 1953, Zimm 1956, Doi & Edwards 1986);
  every formula is checked against an independent numerical oracle as listed.

## Biochemistry: enzyme mechanisms and biochemical thermodynamics (2026-10-15)

- `biology/systems/enzymes.h` (+ `math/lambert_w.h`, test `NatSci/biology/systems/test/enzymes_test.cpp`, oracles `NatSci/biology/verification/gen_enzymes_ref.py`):
  steady-state rate laws (uncompetitive, mixed / non-competitive, substrate inhibition (Haldane), reversible Michaelis-Menten with the Haldane relationship, rapid-equilibrium ordered and random bi-bi, ping-pong bi-bi, Morrison tight binding);
  a general MECHANISM ENGINE (King-Altman by linear algebra: states + first-/pseudo-first-order steps -> rate matrix -> batched steady-state null vector -> net product flux), so any ordered / ping-pong / random / branched mechanism with dead-end complexes is a list of steps;
  integrated Michaelis-Menten via the Lambert W function (Schnell-Mendoza; new batched `lambert_w0` working from log x so huge arguments do not overflow) and the total quasi-steady-state approximation, plus the full mass-action rates.
- `biology/metabolism/thermodynamics.h`: Alberty-style transformed Gibbs energies (Legendre transform at fixed pH, pseudoisomer groups through the proton binding polynomial), transformed reaction energies, apparent equilibrium constants and an extended Debye-Hueckel activity coefficient
  with the constants supplied by the caller (the literature values are not embedded because they were not verified here).
- Verification: Lambert W against mpmath (60 digits, z up to 700); the engine against SymPy EXACT rational steady states for Michaelis-Menten, ordered bi-bi (5 states, 10 steps, with products) and ping-pong (4 states) over lanes with and without products (1e-11);
  the engine reproduces the closed forms it must contain - the uncompetitive law and Haldane substrate inhibition follow exactly from dead-end complexes (1e-12), Cleland's ping-pong form equals the engine's initial rate, mixed -> competitive / uncompetitive limits;
  Morrison against the numerical binding equilibrium (1e-9); progress curves against full mass-action integration (Radau, 1e-12): Schnell-Mendoza 1.7e-3 in the QSSA regime but 0.43 when E0 ~ S0, where the total QSSA keeps 2e-2 (7.6e-5 in the QSSA regime) - the reason both are provided;
  transformed Gibbs energies / apparent K' against an explicit species-level equilibrium with stepwise Henderson-Hasselbalch fractions (ATP, ADP, Pi with 1, 1, 3 protons, six pH values, two temperatures; 1e-9 in ln K').
- Not yet: allosteric KNF / Monod-Wyman-Changeux enzyme kinetics with substrate (binding.h has MWC for ligands only), enzyme parameter fitting and identifiability, protein folding and denaturation, nucleic-acid melting, pH / metal (Mg2+) binding of ATP with literature constants, thermodynamic constraints in FBA (loop law uses none).

## Source-verified pass: reversible step growth, enzymes, thermodynamics, folding, nucleic acids, estimation, MDF (2026-10-15)

Requested rule: correctness first, authoritative sources read, implementations from the local MATLAB install and the GitHub projects named earlier used as oracles. Documents READ (SHA-256 prefixes) and what each verified:

| Source | What it settled |
| --- | --- |
| eQuilibrator `equilibrator-cache` (GitLab) `thermodynamic_constants.py` 40b1f3e3, `models/compound.py` ccbfb160, `models/compound_microspecies.py` 350c5f37 | Alberty's Legendre transform with extended Debye-Hueckel (a1, a2, a3, B = 1.6), Mg2+ term, microspecies logsumexp. `metabolism/thermodynamics.h` now follows it exactly (charge, N_H, N_Mg, pH, pMg, I, T). Finding: eQuilibrator uses R = 8.31e-3; CODATA is used here. alpha(298.15)/RT = 1.1758, matching Alberty's 1.17582 |
| Cantera `DebyeHuckel.cpp` c6315f8d, `WaterProps.cpp` 07d2749a | Independent cross-check of the Debye-Hueckel A: Cantera default 1.172576 (0.3% below Alberty's value, a different dielectric-constant source) |
| Biopython `Bio/SeqUtils/MeltingTemp.py` 4f3cbc58 (run as the oracle) | nearest-neighbour tables (DNA_NN1-4, RNA_NN1-3, R_DNA_NN1 generated into `nucleic_acid_tables.h`), `Tm_NN` formula, salt corrections 1-7; `biophysics/nucleic_acids.h` reproduces 105 Biopython Tm values to 1e-9 deg C |
| PyFolding `models.py` 8b277549 (imported and run as the oracle), `constants.py` 06987b30 | equilibrium denaturation (two-state, sloping baselines, three-state, dimer) and chevron models in `biophysics/protein_folding.h`. Findings: PyFolding's sloping model writes the signal as native baseline + (difference baseline) F, not Y_N(1-F) + Y_U F; its dimer formula loses digits for K >> Pt (rationalised form used here, checked against 60-digit arithmetic) |
| arXiv 1701.06122, Einav & Phillips, J. Phys. Chem. B 121 (2017) 3813, 631b627a | MWC active-state probability, leakiness, dynamic range, EC50, effective Hill coefficient (eq. 1-12) for `enzymes.h` allosteric kinetics; verified against explicit enumeration of all 2^(m+1) microstates and against the mass-action mechanism in the rapid-exchange limit |
| MIT OpenCourseWare 10.569 Synthesis of Polymers, lecture 3 (P. Hammond), 6f8c4879 | closed-system K = p^2/(1-p)^2, X_n = 1 + sqrt(K); open-system K = X_n (X_n - 1) [W]/[M]0; verified by integrating the rate law (DOP853) and by a population balance with random coupling + random scission relaxing to the most probable distribution |
| MATLAB R2025b Statistics Toolbox `nlinfit.m` f4cd502b (`LMfit`), `nlparci.m` 00fc70f0, `private/statjacobian.m` feccfec3 (also RUN in `matlab -batch` as the oracle) | Levenberg-Marquardt, forward-difference Jacobian, intervals; `common/estimation.h` matches nlinfit/nlparci (beta 1e-6, intervals 1e-5) and even reproduces nlinfit's failure from a bad start (SSE 11.746) |
| Data2Dynamics `PLE/arPLEInit.m` d7be7e20 | profile-likelihood thresholds chi2inv(1 - alpha, 1) pointwise / chi2inv(1 - alpha, p) simultaneous |
| Noor et al. 2014 PLoS Comput. Biol. 10:e1003483 (open access, read), equilibrator-pathway `thermo_models.py` fbaa046b | MDF linear program and its shadow prices; `metabolism/pathway_thermodynamics.h` (MDF + thermodynamics-based flux balance as a MILP on the sparse stack), verified against scipy/HiGHS and by exhaustive enumeration of direction patterns |

NOT obtainable (paywalled / blocked): Monod, Wyman & Changeux 1965; Koshland, Nemethy & Filmer 1966 (KNF is therefore not implemented; the Adair form is); Flory 1936/1953; Stockmayer 1943-44; Rouse 1953; Zimm 1956; Doi & Edwards; Schnell & Mendoza 1997; Tzafriri 2003; Marko & Siggia 1995; Becktel & Schellman 1987; SantaLucia 1998 (the Biopython tables cite it); Alberty's book; Odian; Cleland 1963; King & Altman 1956; Henry et al. 2007 (TFA formulation follows the standard big-M statement, checked by enumeration, not against the paper). The MIT lecture is course material, not a primary paper.
Constants / conventions found by reading: eQuilibrator R = 8.31 kJ/(mol K); Biopython uses R = 1.987 cal/(mol K) for Tm; PyFolding RT at 298.15 K with R = 1.987204118e-3.

## Spatial models tranche (2026-10-15)

`subsp/natsci/biology/spatial/reaction_diffusion.h` (Series-native cell-centred finite-difference Laplacian, 1-3 D, Periodic/Neumann/Dirichlet via ghost cells in the py-pde convention, batched `[..., S, spatial...]`, RK4 `integrate`) and `spatial/turing.h` (2x2 Turing conditions, band edges and k_c^2, growth rate, n-species `unstable_modes`, Brusselator / Schnakenberg / Gray-Scott kinetics with steady states and Jacobians). Test `NatSci/biology/spatial/test/spatial_test.cpp` (CTest `spatial_test`):

* Laplacian: second-order convergence for each boundary type; Neumann heat eigenmode decays at the exact discrete rate 4D/h^2 sin^2(n pi / 2N).
* Turing: 600 random 2x2 systems (42 unstable) against numpy eigenvalue scans (flag, band edges, growth samples); analytic Jacobians against central differences; Gray-Scott steady states against fsolve.
* PDE: small-perturbation growth of a Brusselator Turing mode equals Re lambda(k_d^2) of the dispersion relation; morphogen gradient vs cosh solution; Fisher-KPP front speed vs 2 sqrt(rD) with the asymptotic -3/(2 lambda t) shift; 1-D Gray-Scott against MATLAB `pdepe` (gen_spatial_ref.m).

Sources read: py-pde boundary-discretization documentation and Brusselator example (ghost-cell conventions). NOT obtained: Turing 1952, Pearson 1993, Prigogine-Lefever 1968, Gray-Scott 1984, Bramson 1983, Fisher/KPP originals (the -3/2 log t shift is as quoted in arXiv:1510.03329 (sha 7c7734df), checked by the simulation). Re-sourced 2026-10-19: Ablowitz-Zeppetella speed/profile via Roques arXiv:2511.10104 (sha 93b084a1) plus a SymPy proof that the closed form solves the PDE exactly; the 1979 paper itself is paywalled.

## Metabolic control analysis (2026-10-19)

`biology/metabolism/control.h`: elasticities (central differences, or analytic via `common::reaction::flux_jacobian`), unscaled/scaled concentration and flux control coefficients with moiety conservation handled through Gamma (normal equations + one refinement; no independent-row choice), response coefficients `C dv/dp`, summation and connectivity defect audits, conservation laws from the SVD of N, and a general-rate-law Newton steady state with conservation closure. Source read: COPASI `copasi/steadystate/CMCAMethod.cpp` (sha afeb4f85cb57): C^S = -L (N_R E L)^-1 N_R, C^J = I + E C^S, scaling by v/x. Not obtained: Kacser & Burns 1973, Heinrich & Rapoport 1974 (the theorems are verified numerically instead). Test `control_test` on an ATP/ADP-conserving 5-reaction network with Michaelis-Menten rates, 4 lanes: matches COPASI's link-matrix formula, finite differences of re-solved steady states under enzyme scaling (the definition), both theorems, and parameter responses vs re-solved steady states. Note: connectivity holds on the free directions ker Gamma (`C^J E B = 0`, `C^S E B = -B`), not on the full species space when moieties are conserved.

## Ionization (2026-10-19)

`biophysics/ionization.h`: Henderson-Hasselbalch net charge, Van Slyke buffer capacity and a batched bisection isoelectric point for any set of ionizable groups; `protein_groups` encodes the Bjellqvist pK table as implemented in Biopython `Bio/SeqUtils/IsoelectricPoint.py` (read, sha 93a0d329ba63; Bjellqvist 1993/1994 papers not obtained). Test `ionization_test`: 200 sequences x 6 pH values vs Biopython `charge_at_pH` to 1e-10; pI vs Biopython within its own 1e-4 bisection (and its [4.05, 12] clamp checked); Q(pI)=0; beta = -dQ/dpH; beta_max = ln10/4 at pH = pK. Generator `verification/gen_ionization_ref.py` (loads the Biopython file directly, no Bio package).

## Population engine v2 (2026-10-19)

Assessment before: `population/` was 740 lines of closed-form rates (growth, interaction, selection, ecology, one-matrix structured) plus a 10-line alias of the Gillespie engine: no discrete-time dynamics, no stochastic demography, no integral projection, no delays, no community or evolutionary theory. Added (each header with an oracle-verified CTest, all lane-batched Series code, no per-element loops except time stepping):

| Header | Content | Oracles |
| --- | --- | --- |
| `maps.h` | logistic, Ricker, Beverton-Holt, Hassell, Maynard Smith-Slatkin maps with analytic derivatives; attractors, Lyapunov exponents, period detection, periodic points and multipliers, superstable cycles (Feigenbaum cascade) | exact 2-cycle formula, ln 2 at r = 4, numpy long-run exponents over parameter sweeps, mpmath 50-digit superstable parameters (R_n, delta -> 4.669) |
| `demography.h` | fundamental matrix, R0, generation time, damping ratio (Hotelling deflation + Gelfand radius), fixed-design LTRE, chain-rule sensitivities to lower-level parameters, transient amplification (2-norm), life tables (Euler-Lotka by Newton, Fisher reproductive value, quadrature weights), Tuljapurkar approximation, stochastic growth rate and quasi-extinction by lane-parallel simulation (i.i.d. and Markov environments) | R package popbio's own functions sourced from the CRAN mirror (read: fundamental.matrix.R, net.reproductive.rate.R, generation.time.R, damping.ratio.R, LTRE.R, stoch.growth.rate.R, eigen.analysis.R); popbio rounds the moduli in damping.ratio to 5 decimals, so the exact |lambda| ratio is the primary check; exact reflection-principle recursion for quasi-extinction; scalar environments with closed forms; Leslie-matrix identity for the life table; continuous Euler-Lotka r = m0 - c - mu |
| `birth_death.h` | birth-death generators, transient law (matrix exponential), exact mean time to extinction, quasi-stationary distribution, linear birth-death closed forms, Galton-Watson extinction, Brownian first-passage viability and the Dennis regression estimator | scipy expm / solve / eig, Gillespie cross-check, brentq roots of the offspring pgf, Monte Carlo with Brownian-bridge crossing correction, numpy lstsq |
| `ipm.h` | midpoint-rule integral projection models: lambda, stable density, reproductive value, sensitivity/elasticity kernels, life expectancy, R0, spreading speed of integrodifference equations | exact Gaussian-growth kernel (lambda = c, AR(1) stationary density, v = 1), numpy full eig, first-order perturbation check, scipy minimisation for the Laplace kernel, direct front simulation with the pulled-front correction |
| `delay.h` | method-of-steps RK4 DDE integrator with Hermite history, Hutchinson and Nicholson laws, complex Lambert W0 (path-following Newton) and the exact rightmost-root theory, Hopf onset | mpmath complex Lambert W, characteristic-equation residual, measured envelope rate and frequency of the linear regime, independent scipy DOP853 method-of-steps solutions to 2e-6 |
| `community.h` | spectral abscissa without an eigensolver, random community matrices with pair correlation tau (elliptic law), GLV invasion growth rates, chemostat/Monod resource competition with R*, adaptive dynamics (selection gradient, second derivatives, convergence/evolutionary stability, branching, singular strategies, canonical equation) | numpy eigvals, May and Allesina-Tang criteria, sympy second derivatives of Gaussian logistic competition (f_yy = 1/sa^2 - 1/sk^2, f_xy = -1/sa^2), fsolve, simulated exclusion, RK4 canonical-equation flow vs closed form |

Sources read this tranche: popbio R sources (CRAN mirror, sha prefixes d4a2fad5, c95fc302, 6c893bb8, 23b415a2, bcd38ea2), Allesina & Tang arXiv:1105.2071 (via ar5iv: criteria quoted above), arXiv:1005.3862 (adaptive-dynamics conditions (1)-(4) and the Gaussian example, sha 6a3206d9), Biopython not involved. NOT obtained: Caswell (2001), Tuljapurkar (1990), Easterling-Ellner-Dixon (2000), Ellner-Rees (2006), Kot-Lewis-van den Driessche (1996), May (1972/1976), Hutchinson (1948), Gurney-Blythe-Nisbet (1980), Dennis-Munholland-Scott (1991), Dieckmann-Law (1996), Geritz et al. (1998), Tilman (1982); the Brownian first-passage formula and the IDE/Hopf results are therefore checked by simulation and independent solvers rather than against those papers.

Population v2 TODO (shelved 2026-10-19): host-parasitoid / coupled-map lattices; McKendrick-von Foerster PDE solver; environmental-noise SDE growth (Lande); maturation-delay stage models; fitting layer (IPM vital-rate regression, ML PVA) on common/estimation.h.

## Proton linkage, elementary flux modes, flux sampling (2026-10-19)

* `biophysics/linkage.h`: Wyman proton linkage. Independent-site and polyprotic proton partition functions (stable log-sum-exp), mean bound protons, apparent constants K_app = K_ref Z_b / Z_f, linked binding with depletion (reuses `binding::receptor_ligand`), buffer-linked observed enthalpy. Test `linkage_test`: ln Z and <H> vs brute-force enumeration of all 2^5 protonation microstates (mpmath), `d log K / dpH = <H>_f - <H>_b` vs finite differences, low/high-pH limits, the textbook bell-shaped enzyme profile 1/(1 + 10^(pK1-pH) + 10^(pH-pK2)) from the polyprotic fractions, equivalence of two equivalent sites with statistical-factor polyprotic constants. Wyman 1964 and Baker-Murphy 1996 were not obtained.
* `metabolism/modes.h`: elementary flux modes by the nullspace / double-description algorithm (reversible reactions split, 2-cycles dropped, sign-canonical output, combinatorial adjacency test plus an exact rank test on the final columns: the adjacency test alone let a non-elementary mode through on the first random network, caught by the oracle). Test `modes_test`: 8 random networks (4 metabolites, 9 reactions, mixed reversibility) match brute-force minimal-support enumeration exactly, plus a dead-end hand case. Host-side combinatorics by nature (Schuster-Hilgetag 1994, Urbanczik-Wagner 2005, Terzer-Stelling 2008 not obtained).
* `metabolism/modes.h` `sample_fluxes`: hit-and-run over `S v = 0, l <= v <= u` with chains as Series lanes, Chebyshev-centre start (LP; infinite bounds must be passed as real infinities, 1e300 silently gave radius 0), fixed fluxes kept exact. Test: exact moments of a triangle (mean 1/3, var 1/18, cov -1/36) and of a box in the nullspace (means, variance 1/3, fourth moment 1/5, fixed flux preserved). Fixed 2026-10-19 (user flagged both): bounds with |b| >= 1e20 (inf, 1e300, ...) are treated as unbounded and every unbounded side is truncated at +-`bound_cap` (default 1000, COBRA convention; samples are uniform on the truncated polytope, `Sample::truncated_bounds` reports how many sides were cut); reactions pinned by the network (blocked, implied equalities) are found by one batched flux-variability solve and held exactly (`Sample::fixed_reactions`); a polytope with no interior even after that raises instead of returning degenerate samples. Tests: uniform on a truncated [0, 10] given as inf / 1e300 / 1e25, a blocked reaction stays exactly 0 with the rest uniform, a single-point flux space raises.

### linprog sentinel-infinity fix at the source (2026-10-19)
The sampler's "1e300 gives radius 0" was a solver bug, not a sampler quirk. Reproduced: bounds of +-1e30 made the dense `linprog` report Infeasible (objective 2e30), +-1e300 gave NaN, and the sparse pipeline returned a wrong optimum (-3 instead of -2.5). Fix: any bound or row side with |x| >= 1e20 (HiGHS convention, `lp::kInfThreshold`) is a real infinity at every entry point: dense `linprog::standardize`, `lp::solve` / `solve_simplex_pipeline` (via `Model::has_huge_values` / `canonicalize_infinities`), `solve_ipm` and `InteriorPoint`, `MipSolver`, dense `milp::solve` (so an integer variable with a sentinel bound now raises the clear "must have finite bounds" error instead of exploding). A vacuous dense row written with a sentinel right-hand side (`b_ub`/`b_eq` beyond 1e20 or non-finite) now raises, because rows are shared by all lanes and cannot be dropped per lane. Regression: `linprog_infinity_test` (inf, 1e20, 1e30, 1e300 through dense LP, sparse simplex, interior point, a hand-built Model with sentinel row sides, dense MILP, sparse branch-and-cut). Still documented, not changed: interior-point accuracy with pinned objective floors (use the simplex, `exact_options()`).

### Interior point with pinned objective floors (2026-10-20)
User asked for the "pinned objective floor" accuracy limit to be fixed rather than documented. Reproduced on random degenerate networks (flux variability at fraction 1, m = 40..80): the batched interior point left lanes at IterationLimit and `flux_variability` threw "a variability LP did not reach optimality". Fix in `linprog::solve_unchunked`: new `Options::polish` (default true) re-solves every lane that ends IterationLimit or NearOptimal with the exact simplex and merges x, objective, duals, status and certificates back (a lane the simplex also cannot decide keeps the interior-point answer and status). Verdicts PrimalInfeasible / Unbounded are untouched. Regression `linprog_polish_test`: polished interior point equals the simplex to 1e-7 on 2 networks x 2r FVA lanes, `polish=false` still fails on the first (documents the need), infeasible/unbounded verdicts unchanged. Found on the way: the dense simplex needed 25,259 pivots on a degenerate 100 x 320 LP and hit its iteration cap (its Bland fallback after 30 degenerate pivots crawls). FIXED 2026-10-23: after a run of degenerate pivots the entering column is now drawn at random among the improving candidates (deterministic LCG stream) instead of Bland's first-index rule; the same LP now takes 1,571 pivots, and the regression is in `linprog_polish_test`. The genome-scale iJO1366 check (FBA, FVA, gene deletions vs COBRApy) still passes (407 s). Also fixed: PlussQit `linearalg.h` (edited 2026-10-23) used `pluss::series::full_like/zeros_like` without including `series_factory.h`, which broke every TU that reached it first (added the include).

## Biomechanics (2026-10-20)

New `biology/biomechanics/` (umbrella `biomechanics.h`, included from `biology.h`); every header Series-native and lane-batched, every test against an independent oracle, generators in `NatSci/biology/verification/gen_{muscle,chain,tissue,hemo,locomotion}_ref.py`:

| Header | Content | Oracle |
| --- | --- | --- |
| `muscle.h` | Hill-type muscle-tendon unit in the De Groote-Fregly (2016) form: active/passive force-length, force-velocity and its inverse, tendon force-length, activation dynamics, pennation geometry, isometric tendon-fiber equilibrium (vectorised bisection), explicit tendon-dynamics state rates and an RK4 simulator with time-varying excitation / length | Curve constants PARSED from OpenSim `DeGrooteFregly2016Muscle.h` (sha prefix c430bfc2; `.cpp` 704304cd) into an independent numpy model; scipy brentq equilibria; scipy DOP853 trajectories; the model's defining key points f_act(1) = 1, f_v(0) = 1, f_v(-1) = 0, tendon force 1 at strain 0.049, passive force 1 at strain 0.6 |
| `chain.h` | planar serial multibody (limb) dynamics: recursive Newton-Euler inverse dynamics with end-point force, mass matrix, Coriolis/gravity bias, forward dynamics, kinematics, energy, RK4 simulation | sympy Lagrangian derivation of M, h and J^T F for a 3-link chain, DOP853 PD-controlled trajectory, energy conservation, hanging/horizontal static checks |
| `tissue.h` | incompressible hyperelasticity: neo-Hookean, Mooney-Rivlin, Yeoh, Gent, Fung-type, Ogden; uniaxial / equibiaxial / planar / simple-shear stresses; fibre-reinforced artery (HGO / GOH dispersion form) | sympy dW/dF with the pressure eliminated per boundary condition; the oracle caught a wrong simple-shear normal-stress difference (missing W2 term) in the first draft |
| `hemodynamics.h` | 2- and 3-element Windkessel (impedance, exact exponential integrator), Poiseuille resistance, Womersley pulsatile flow in a rigid tube (flow, velocity, wall shear, impedance) with a complex Bessel J0/J1 by Miller's recurrence (normalised by cos z for large Im z), Moens-Korteweg and Bramwell-Hill wave speeds | mpmath complex Bessel (|z| up to 64), Womersley by substitution into the momentum equation and the axial force balance (this exposed a wrong sign of i that mpmath alone, sharing the formula, could not), Poiseuille limit, scipy ODE for the Windkessel steady cycle, BH = MK identity |
| `locomotion.h` | SLIP running/hopping apex map (batched event-driven stance with secant-refined liftoff), linear inverted pendulum closed forms, orbital energy, capture point, time to position | DOP853 with touchdown/liftoff events, energy conservation over consecutive steps, LIPM ODE |

Sources read: OpenSim `DeGrooteFregly2016Muscle.{h,cpp}` (GitHub opensim-core), Abaqus Analysis User's Guide page "Anisotropic hyperelastic behavior" for the exact HGO / GOH energy and the Macaulay bracket (the original Gasser-Ogden-Holzapfel paper was behind a bot check). NOT obtained: De Groote et al. 2016, Hill 1938, Zajac 1989, Thelen 2003, Millard 2013, Holzapfel-Gasser-Ogden 2000, Gasser-Ogden-Holzapfel 2006, Womersley 1955, Frank 1899, Westerhof 2009, Blickhan 1989, Geyer et al. 2005, Kajita et al. 2001, Hof 2008, Pratt 2006; the results are checked against independent computation (sympy, mpmath, scipy), exact limits and conservation laws instead.
Not built (candidates): Millard 2012 muscle curves, muscle-driven multibody coupling (moment arms), full 3-D multibody, QLV viscoelasticity (needs E1), 1-D arterial network wave propagation, bone remodelling, contact mechanics.

### Muscle-driven limb (2026-10-23)
`biomechanics/musculoskeletal.h`: moment-arm polynomials per muscle and joint, muscle-tendon length by virtual work (dL/dq = -r, integrated in closed form), MTU velocity, joint torques R^T F, isometric starting state, and the fully coupled RK4 simulation of chain (chain.h) + Hill muscles (muscle.h) with M-vector muscle parameters. Test `musculoskeletal_test`: -dL/dq = moment arm by finite differences, power balance tau . qdot = -sum F Ldot, initial equilibrium vs brentq, and a 5-muscle 2-link arm (shoulder/elbow/biarticular, one posture-dependent moment arm) over 0.5 s against a DOP853 solution built from the sympy chain dynamics and the OpenSim-parsed muscle equations (joint angles 2e-5, muscle states 2e-4).

### Parasitoid maps and surface chemistry (2026-10-23)
* `population/parasitoid.h`: Nicholson-Bailey, density-dependent host (Beddington) and negative-binomial aggregation (May) maps, analytic interior equilibria, Jacobians, spectral radius of 2x2 matrices (real and complex pairs), ring coupled-map lattice with a synchrony measure. Test `parasitoid_test`: fixed points, Jacobians (numerical), stability vs numpy eigenvalues, May's stabilisation for k < 1 (k = 0.2, 0.5, 0.9 stable; 1.2, 2, 5 unstable) and the k -> infinity limit, attraction/repulsion of the orbit, lattice vs numpy and conservation of total abundance.
* `chemistry/surface.h`: Langmuir (plain, dissociative, competitive), Freundlich, Sips, Temkin, BET (finite and infinite layers) with the BET-plot regression and specific surface area, isosteric heat, Langmuir-Hinshelwood and Eley-Rideal rates. Test `surface_test`: BET closed form vs direct summation of the multilayer model (c = 5..100, n = 3..40), regression vs numpy polyfit, Sips/Langmuir limits, isosteric heat = Q for K = K0 exp(Q/RT), the LH optimum K_A P_A = 1 + K_B P_B and maximal rate vs a grid search, and a CO-oxidation microkinetic model through `common::reaction` (RK4 trajectory vs DOP853 to 1e-9, site-conserving Newton steady state and turnover frequency vs scipy fsolve). The 1938 BET paper was not obtained.

### Crystallization (2026-10-23)
`chemistry/crystallization.h`: classical nucleation theory (critical radius, barrier, rate), JMAK kinetics with the Avrami-plot regression and the constant-nucleation 3-D constant k = pi I G^3 / 3 (n = 4), and the MSMPR crystallizer with size-independent growth: closed-form steady state for magma order 1 (explicit supersaturation (6 k_b k_g^3 tau^4)^(-1/(b + 3g)), B = (c_in - c) / (6 rho_c k_v G^3 tau^4), the first draft had G^2 and the ODE oracle caught it), the moment equations (c, mu_0..mu_3) and the full population-balance right-hand side on `common::population_balance` (MUSCL + SSP-RK3). Test `crystallization_test`: CNT closed forms and Gibbs' dG* = (4 pi / 3) gamma r*^2; Avrami fit recovers (n, k); JMAK n = 4 vs a Monte Carlo of nucleation and interface-controlled growth in a periodic box (60,000 probe points, within 0.03); steady state vs fsolve of the moment ODEs; moment-ODE trajectory (RK4) vs DOP853 to 1e-6; the finite-volume PBE reaches the analytic exponential CSD (supersaturation 2e-3, moments within 1%, density within 2%).

### Population v2 follow-ups (2026-10-23): age structure, environmental noise, vital-rate fitting
Layer survey before writing (user rule: reuse first, decide Series vs PlussMa, accuracy then speed, C++20): *Series* already provides `antidiff().trapezoid / cumulative_trapezoid` (used for every age integral), `roll`, `pile` (= stack; `unsqueeze + unite` helpers elsewhere in my code are redundant), `masked_select`; *PlussMa* provides `math::sde::fluctua` (Euler-Maruyama / Milstein / Stratonovich-Heun, lane-batched, own Wiener stream), `stats::regression` (OLS and GLM with Binomial/Poisson), `common::population_balance` (MUSCL transport, kept for non-constant velocities), `population::ipm`, `demography::life_table` (quadrature weights). No Series addition was needed; everything new is domain code and lives in PlussMa.
* `population/age_structured.h`: McKendrick-von Foerster / Gurtin-MacCamy by characteristics on the diagonal grid dt = da (exact shift transport, exact mortality integral along the characteristic, implicit newborn node, predictor-corrector for density dependence, plus-group). No CFL limit, no numerical diffusion, second order. Test: exact transport to 1e-14, growth-rate error ratios > 3.5 per halving to r = beta0 - c - mu, trapezoid Euler-Lotka agreement, Gurtin-MacCamy equilibrium (N* = (beta0 - c - mu0)/mu1, B* = (beta0 - c) N*) to 0.2% and the e^{-(beta0-c)a} age profile, two lanes in one call.
* `population/stochastic_growth.h`: density-independent lognormal growth, stochastic logistic in log space with its gamma stationary law (shape 2r/s^2 - 1, rate 2r/(K s^2)), block-wise path generation on `sde::fluctua`, a Brownian-bridge Rao-Blackwellised first-passage estimator (`crossing_probability`) and Feller's demographic extinction probability. Test: ln N_T mean/variance, E[N_T], stationary mean and variance of 30,000 lanes, first-passage estimate vs the exact Brownian CDF (error < 0.01 with 6,000 lanes; the naive crossing count is biased low), Feller vs the exact linear birth-death master solution converging as b + d grows.
* `population/vital_rates.h`: survival (logistic GLM), growth (OLS), fecundity (Poisson GLM) and recruit-size fits wired to `ipm` (kernel, lambda, sensitivities). Test against R `glm` / `lm` / `eigen` on a simulated 1,800-individual data set (coefficients to 1e-6..1e-9, standard errors, lambda 1e-6) plus recovery of the generating parameters.
Not done: ML count-based PVA with observation error (Kalman), bootstrap confidence intervals for lambda, Millard curves (next).

### Millard muscle curves and math::bezier (2026-10-23)
Layer decision: a piecewise quintic Bezier evaluator is a general numerical primitive, so it lives in PlussMa `math/bezier.h` (Series-native: power-basis Horner evaluation of x(u), y(u) and derivatives; u(x) and its inverse by a 32-interval table bracket plus safeguarded Newton with an all-lanes convergence exit; segment skipping; linear extension with the end slopes; `corner()` control-point builder), not in biology and not in Series. `biomechanics/millard.h` builds the four Millard (2012) curves exactly as OpenSim's SmoothSegmentedFunctionFactory does (factory, toolkit and curve-class defaults read from opensim-core). `muscle.h` was refactored once, centrally: `Params::curves` (a `muscle::Curves` bundle) switches the whole muscle-tendon machinery (geometry, fibre force, isometric equilibrium, state rates, musculoskeletal coupling) between the De Groote closed forms and the Millard curves with no change to callers. Test `millard_test`: an independent numpy/scipy re-implementation of the construction (control points, brentq u(x), analytic Bezier derivatives) matches value, slope and curvature on grids spanning all segments and both extensions (1e-12 / 1e-10 / 1e-8), the defining key points (f_act(1) = 1, f = 0.1 at both ends, f_v(-1) = 0, f_v(0) = 1 with slope 5, f_v(1) = 1.4, passive 1 at strain 0.7, tendon ~1 at strain 0.049), C^2 joins, inverse round trips, brentq muscle-tendon equilibria with the Millard family, and zero state rates at equilibrium. Lesson: the first Newton-only inversion gave 1e-3 errors on control polygons with repeated points (dx/du ~ 0 mid-segment); the table bracket + safeguard fixed it. `public_calls.md` now has a "Shared math owners" section for `math::bezier`.
Not done: Millard fibre damping inside the dynamics inversion (curves support it; `state_rate` assumes zero damping), Millard acceleration muscle, curve integrals (potential energy).

### Enzyme progress-curve fitting (2026-10-23)
`systems/enzyme_fitting.h` composes existing layers only: `enzymes::integrated_michaelis_menten` (Lambert W closed form) and `dynamics::simulate` for the model, `common::estimation` (nlinfit-compatible LM, nlparci intervals) for the fit; global fits over several initial substrate levels, many starting points in one call (LM lanes), natural-scale parameters with magnitudes inside the model. Test against scipy `least_squares(method="lm")` on noisy synthetic curves: Vmax, Km to 1e-6, standard errors 1e-4, 95% intervals 1e-5, sum of squares; competitive product inhibition by ODE (Vmax, Km, Kp to 2e-4); no-inhibition data drives Kp large and recovers the plain fit.

### Statistical rate theory (2026-10-23)
`chemistry/rate_theory.h`: translational / rotational / vibrational partition functions, Eyring and partition-function TST with the Wigner correction and Eyring analysis (regression of ln(k/T) on 1/T through `stats::regression::ols_fit`), hard-sphere collision theory, exact harmonic densities of states by Beyer-Swinehart, RRKM microcanonical rates and the strong-collision Lindemann-Hinshelwood fall-off. Layer decisions: the stride-cumulative Beyer-Swinehart recursion rho_new(E) = sum_k rho(E - k nu) is a `reshape + cumsum + reshape` on the Series backend (no loop over energy bins, lane-batchable), regression and constants are reused, nothing was added to Series. Test `rate_theory_test`: partition functions to 1e-13; Eyring/TST identities; Eyring analysis vs numpy polyfit; Beyer-Swinehart counts equal to brute-force enumeration of the harmonic states of 5 and 4 oscillators (exact integers at every sampled energy); with a 1 cm^-1 mode filling the grid, k(E), Q and k_inf agree with a direct summation to 1e-11, and k_inf / k_TST = (bin/kT) / (1 - e^{-bin/kT}) to 1e-8 (the exact discrete link to TST; without a quasi-continuum the exact-count RRKM rate fluctuates and the identity does not hold, which is physical); fall-off at five collision frequencies, monotone, bounded by k_inf, with the omega -> 0 limit omega * P(E >= E0). Not done: Eckart tunnelling, hindered rotors, anharmonic counts, full master-equation (weak collision) fall-off, variational TST.

### Follow-ups completed / open (2026-10-23, end of session)
Done and tested: Millard-compatible fibre damping in `muscle::state_rate` (Newton-in-bracket velocity solve, DOP853 oracle); `rate_theory.h` hindered rotor (plane-wave levels vs Fourier collocation, scipy Mathieu for N = 1, A&S large-q asymptotics) and the weak-collision master equation (symmetric Sinkhorn+Newton scaling to relative accuracy, eigenvalue vs scipy, propagator decay check, detailed balance, limits).
OPEN: `vital_rates::bootstrap_lambda` / `resample` are written but their replicates differ from the R reference by up to 0.04 in lambda (test block disabled with `#if 0`); suspect index base or recruit resampling convention between R (`sample.int`, exported minus one) and `Series::index`. Not run: ctest of the full tree after the last edits, regeneration of `public_calls.md` and `public_intent.yaml` front doors for `hindered_rotor_*`, `master_equation`, `weak_collision`, `bootstrap_lambda`, `resample`. Not built: Eckart tunnelling, Kalman count-based PVA.

Resolved (2026-10-24): the bootstrap mismatch was the R export (`as.numeric` of a matrix is column-major; index matrices are now exported row-major with `t()`); `bootstrap_lambda` reproduces all 100 R replicate lambdas, the standard error and the percentile interval. Front doors for the hindered rotor, master equation and bootstrap recorded in `public_intent.yaml`.

## Eckart tunnelling and Kalman count-based PVA (2026-10-24)

Layer survey: Eckart reuses only Series ops (batched over T, V1/V2/nu); the thermal integral is plain Simpson in s = sqrt((E-E0)/RT) (T(E) opens like a square
root, which made the first plain-energy Simpson version 1e-3 off). Kalman PVA is canonical on `math::state::filter/smooth` (NaN = missing year, batching, input
path) and `math::optimization::nelder_mead_minimizer`; the extinction step reuses `birth_death::quasi_extinction_cdf/ultimate_quasi_extinction` and
`dennis_estimate` (starting values). Nothing new went into Series or PlussMa/Math.

* `chemistry/rate_theory.h`: `eckart_transmission(E, V1, V2, nu*)` (closed-form cosh/cos expression, Johnston-Heicklen form, zero below the energetic threshold)
  and `eckart(V1, V2, nu*, T)` = kappa(T). Oracle: direct DOP853 integration of the 1-D Schroedinger equation through V(x)=A y/(1+y)+B y/(1+y)^2 with L fixed
  from the barrier-top curvature (mass 1 amu; T(E) is mass-independent) — four barriers incl. endothermic and the cos branch: T(E) to 2e-8, kappa(T) to 1e-6;
  kappa -> 1 at high T and -> Wigner for a broad barrier verified. Generator `chemistry/verification/gen_rate_theory_ref.py` (eckart_reference.h).
* `biology/population/count_pva.h`: `to_grid`, `loglik`, `fit` (ML of mu, ln sigma^2, ln eta^2; SEs from the central-difference information matrix; smoothed
  log abundance), `extinction_risk` (horizons, Simpson over the filtered N(m,P) of the current ln N), `ultimate_extinction_risk`. Oracle: independent scalar
  Kalman/RTS in numpy, scipy L-BFGS-B + Nelder-Mead, FD Hessian, scipy quad (`biology/verification/gen_count_pva_ref.py`): log-likelihood to 1e-9, risk to 1e-9;
  closed Gaussian integral for the ultimate risk; eta^2 -> 0 reduces to the Dennis estimator.
* Public calls: `rate_theory::eckart_transmission`, `rate_theory::eckart`; `count_pva::{to_grid, loglik, fit, extinction_risk, ultimate_extinction_risk}` with types `Parameters`, `Grid`, `Fit`.
* Not done: parameter uncertainty folded into the risk (parametric bootstrap), Gompertz density dependence (theta != 1), time-varying census spacing off the grid.
* Primary sources not obtained: Johnston & Heicklen (1962), Eckart (1930), Dennis et al. (2006) — formulas are verified numerically, not transcribed.

## Fluids P3o-a: spectral periodic Poisson solve (2026-10-24)

Layer survey: the canonical Series transforms (`rfft` over the last spatial axis, `fft` over the others, mirrored back with `irfft(n)`) are used unchanged; the new
`common/spectral.h` only composes them and builds the exact symbols of the existing operators (compact `-(4/h^2) sin^2(pi j/n)`, wide `-sin^2(2 pi j/n)/h^2`), so
the solve inverts *the same discrete operator* the CG path did (not a continuous-k approximation). Null modes (constant; alternating mode of an even axis for the wide
stencil) come from integer frequencies, set to zero. `PoissonOptions::method` (default `Fft`) falls back to CG for non-CPU or non-Float32/64 fields; bounded and
variable-coefficient solves are untouched (multigrid next). Diagnostics keep the batch layout; the true residual is reported, iterations = 0.

* Verification (`fluids/test/spectral_poisson_test.cpp`): FFT vs tight CG for both stencils in 1-3 axes with odd and even extents and batches (1e-7), operator equation
  1e-8, exact eigenfunction to 1e-13, null-component rejection, Float32; all existing incompressible/validation/time-step/bounded tests pass unchanged on the new default.
* Performance (periodic Taylor-Green bench, Float32, CPU): projection 36.8 -> 3.3 ms at 128^2 and 119.6 -> 11.2 ms at 256^2 (~11x); RK3 step 181 -> 13.4 ms (128^2),
  1149 -> 50.7 ms (256^2) (~14x, 22x). `natsci_fluids_bench --poisson=cg` reproduces the old path.
* Public calls: `spatial::spectral::{forward, inverse, symbol, solve}` (+ `Symbol`), `spatial::PoissonMethod`, `PoissonOptions::method`.
* Not done: a Series-level `rfftn/irfftn` (the 1-D composition lives in spectral.h; promote to Series if another consumer appears), MPS (no complex dtype).

## Fluids P3o-b: geometric multigrid (2026-10-24)

Layer survey: nothing existed (only CG/PCG in `math::krylov`, FFT now canonical on Series). Decision: the engine is generic structured-grid numerics, so it lives in
PlussMa `math/multigrid.h` (next to krylov), built purely from Series expressions (no cell loops); natsci adapters live in `common/poisson.h` and `physics/fluids.h`.

* Engine: cell-centred conservative operator `-sum_a h^-2 [b+ (u+ - u) - b- (u - u-)]`, periodic / Dirichlet / Neumann ends per axis, optional per-lane face
  coefficients and a mass term; Chebyshev smoother (no inner products); linear interpolation with *series-resistance weights* and series-conductance coarse faces
  (exact in 1-D; a plain linear transfer needed 46+ iterations on a planar 1000:1 jump, this needs 7-9); restriction = adjoint/2^d (tested at every level);
  coarsest level (<= 64 cells) inverted densely in double precision. V-cycle is symmetric positive, so it is the PCG preconditioner (`multigrid::solve`).
* Wiring: `PoissonMethod::{Auto, ConjugateGradient, Fft, Multigrid}`. Auto = FFT (periodic constant coefficient), multigrid for bounded / variable-density
  problems once the largest extent >= 64, PCG otherwise. Explicit requests that the problem cannot honour throw. Bounded and periodic variable-density
  projections use the *exact* D(beta G) operator with a parity-split preconditioner (the centred operator couples cells two apart, so it is 2^d compact problems;
  periodic variable-density is then exact, bounded couples classes only through wall rows, handled by a wall-sized mass term beta/(n H^2)).
* Verification: operator vs explicit scalar loops (all edge combinations); symmetry; adjointness; mesh independence (9 iterations from 32^2 to 256^2); jump
  coefficients; singular/batched lanes (`linear_algebra/test/multigrid_test.cpp`); bounded/periodic Poisson vs CG and vs the spectral solve
  (`common/test/poisson_multigrid_test.cpp`); projections vs CG in Float64 at 1e-12 (`fluids/test/multigrid_projection_test.cpp`). Found and fixed on the way: per-class mean
  projection stalled the bounded preconditioner (class constants are weakly coupled, not null); independent low/high face coefficients are not a symmetric operator.
* Performance, honest: iteration counts are mesh independent (constant-density Poisson 9; 1000:1 bounded projections 3-4x fewer than PCG) but a V-cycle costs ~4-5 CG
  iterations because the Series CPU path is op-count bound (each elementwise pass ~2 ns/cell, broadcast ops 3x, strided cats 6x; small levels are overhead dominated).
  Measured (Float32, 2e-4): bounded projection 256^2 1.36 s vs 1.92 s PCG; periodic variable-density 256^2 0.16 s vs 0.32 s (the exact parity-split path), 512^2 0.39 vs 0.60 s.
  A hybrid wall smoother halved iterations but cost more time. **Next lever = a native fused stencil/Chebyshev/transfer kernel in CodeM** (Track-B style), not more Series tuning.
* Public calls: `math::multigrid::{Hierarchy, solve, chebyshev}` (+ `Problem`, `Axis`, `Edge`, `Options`), `spatial::PoissonMethod::{Auto, ConjugateGradient, Fft, Multigrid}`.
* Not done: semi-coarsening for anisotropic spacing, odd extents (fall back to the compact lattice / PCG), 3-D bounded wall hybrid, GPU.

## Fluids P3p-a: large-eddy simulation closures (2026-10-24)

Layer survey: the variable-viscosity stress divergence (`var_rate`, periodic and bounded) already conserves momentum with a cell-wise dynamic viscosity, so a
closure only has to produce nu_t from the resolved gradient; `advance_stages` takes a rate callable, so the closure is re-evaluated at every RK stage with no new
stepping code. Everything is Series arithmetic over the component loops (d <= 3); nothing is added to Series or PlussMa/Math.

* `physics/fluids/les.h`: `Closure{model, constant, width, clip, local_average, test_ratio}`; Smagorinsky (C_s 0.17), WALE (C_w 0.325, S^d with the 3-D 1/3 kept in 2-D),
  dynamic Smagorinsky (Germano-Lilly, periodic only; (1/4,1/2,1/4) test filter of width sqrt(6) Delta; global per-lane or locally filtered average; clipped).
  `eddy_viscosity` (periodic, bounded), `sgs_dissipation`, `les::advance` (periodic, bounded; Imex1 rejected). Delta = geometric mean of spacings.
* Verification (`fluids/test/les_test.cpp`): formulas vs explicit 3x3 loops (1e-14); pure shear (WALE = 0, Smagorinsky = (C Delta)^2 gamma), rotation, 2-D strain;
  Taylor-Green |S| converging second order; **discrete energy budget** d<KE>/dt(closure) - d<KE>/dt(no closure) = -<nu_t |S|^2> to 2e-3 for all three models through the
  full projected RK3 step; dynamic model vs an independent scalar-loop implementation (anisotropic spacing, global and local) to 1e-12; bounded cavity; rejections.
* Source note: the WALE S^d definition and pure-shear property were confirmed in the Ducros-Nicoud-Poinsot note (sha256 de775465b861dc71); C_w = 0.325 and
  C_w^2 ~ 10.6 C_s^2 come from OpenFOAM documentation and a course slide; Nicoud & Ducros (1999), Smagorinsky (1963), Germano (1991), Lilly (1992) were not obtained.
* Public calls: `les::{eddy_viscosity, sgs_dissipation, advance}`, `les::Closure`, `les::Model`.
* Not done: van Driest / wall-distance damping for Smagorinsky, a-posteriori turbulence validation (decaying isotropic turbulence spectra, channel flow vs DNS), SGS for the
  compressible solver, SA / SST RANS (P3p-b).

## Public calls by module (snapshot 2026-10-24)

Generated from `PlussMa/public_calls.md` (authority; regenerate with `ruby tools/generate_public_calls.rb`, intent in `tools/public_intent.yaml`). Document only these names; every other symbol in the headers is internal. Types and overloads are listed in `public_calls.md`.

- `pluss::subsp::natsci::biology::biomechanics::chain`: `make`, `inverse_dynamics`, `forward_dynamics`, `mass_matrix`, `bias_forces`, `forward_kinematics`, `energy`, `simulate`
- `pluss::subsp::natsci::biology::biomechanics::hemodynamics`: `windkessel2_impedance`, `windkessel3_impedance`, `windkessel_capacitor_step`, `poiseuille_resistance`, `womersley_number`, `womersley_flow`, `womersley_velocity`, `womersley_wall_shear`, `bessel_j01`, `moens_korteweg`, `bramwell_hill`
- `pluss::subsp::natsci::biology::biomechanics::locomotion`: `slip_apex_map`, `lipm_state`, `lipm_capture_point`, `lipm_time_to_position`
- `pluss::subsp::natsci::biology::biomechanics::millard`: `active_force_length`, `force_velocity`, `passive_force_length`, `tendon_force_length`, `curves`
- `pluss::subsp::natsci::biology::biomechanics::muscle`: `active_force_length`, `passive_force_length`, `force_velocity`, `tendon_force_length`, `activation_rate`, `isometric_equilibrium`, `simulate`
- `pluss::subsp::natsci::biology::biomechanics::musculoskeletal`: `constant_arms`, `moment_arms`, `muscle_tendon_length`, `muscle_tendon_velocity`, `joint_torques`, `equilibrium_state`, `rate`, `simulate`
- `pluss::subsp::natsci::biology::biomechanics::tissue`: `neo_hookean`, `mooney_rivlin`, `yeoh`, `gent`, `fung`, `uniaxial_stress`, `equibiaxial_stress`, `simple_shear`, `ogden_uniaxial`, `artery_stresses`
- `pluss::subsp::natsci::biology::biophysics::ionization`: `charge`, `isoelectric_point`, `buffer_capacity`, `protein_groups`
- `pluss::subsp::natsci::biology::biophysics::linkage`: `ln_partition_independent`, `ln_partition_polyprotic`, `log10_apparent_constant`, `bound_complex`
- `pluss::subsp::natsci::biology::metabolism`: `flux_balance`, `flux_variability`, `single_knockouts`, `elementary_modes`, `sample_fluxes`
- `pluss::subsp::natsci::biology::metabolism::control`: `coefficients`, `elasticities`, `steady_state`, `response`, `conservation_laws`
- `pluss::subsp::natsci::biology::metabolism::pathway_thermodynamics`: `max_min_driving_force`, `thermodynamic_flux_balance`
- `pluss::subsp::natsci::biology::population::age_structured`: `step`, `simulate`, `trajectory`, `total`, `births`, `survivorship`, `euler_lotka`
- `pluss::subsp::natsci::biology::population::birth_death`: `generator`, `transient`, `mean_time_to_extinction`, `quasi_stationary`, `linear_extinction_probability`, `ultimate_extinction`, `quasi_extinction_cdf`, `dennis_estimate`
- `pluss::subsp::natsci::biology::population::community`: `spectral_abscissa`, `random_community`, `invasion_growth_rate`, `chemostat_rate`, `r_star`, `selection_gradient`, `classify`, `singular_strategy`
- `pluss::subsp::natsci::biology::population::count_pva`: `to_grid`, `loglik`, `fit`, `extinction_risk`, `ultimate_extinction_risk`
- `pluss::subsp::natsci::biology::population::delay`: `integrate`, `hutchinson_rate`, `nicholson_rate`, `rightmost_root`
- `pluss::subsp::natsci::biology::population::demography`: `fundamental_matrix`, `net_reproductive_rate`, `damping_ratio`, `life_table`, `ltre_fixed`, `tuljapurkar`, `simulate_growth`, `quasi_extinction_probability`, `amplification`, `lambda_gradient`
- `pluss::subsp::natsci::biology::population::growth`: `logistic`, `gompertz`, `exponential`, `theta_logistic_rate`
- `pluss::subsp::natsci::biology::population::ipm`: `mesh`, `discretize`, `analyze`, `life_expectancy`, `net_reproductive_rate`, `spreading_speed`
- `pluss::subsp::natsci::biology::population::maps`: `iterate`, `attractor`, `lyapunov`, `detect_period`, `periodic_point`, `superstable_parameter`
- `pluss::subsp::natsci::biology::population::parasitoid`: `nicholson_bailey`, `negative_binomial`, `beddington`, `nicholson_bailey_equilibrium`, `negative_binomial_equilibrium`, `jacobian`, `spectral_radius`, `ring_step`, `synchrony`
- `pluss::subsp::natsci::biology::population::stochastic_growth`: `simulate`, `crossing_probability`, `stochastic_growth_rate`, `lognormal_mean`, `lognormal_variance`, `feller_extinction_probability`
- `pluss::subsp::natsci::biology::population::structured`: `leslie`, `project`, `dominant`, `sensitivity`, `elasticity`
- `pluss::subsp::natsci::biology::population::vital_rates`: `fit`, `kernel`, `analyze`, `resample`, `bootstrap_lambda`
- `pluss::subsp::natsci::biology::spatial`: `laplacian`, `integrate`, `box`
- `pluss::subsp::natsci::biology::spatial::turing`: `turing_conditions`, `growth_rate`, `unstable_modes`
- `pluss::subsp::natsci::biology::systems::enzyme_fitting`: `initial_guess`, `fit_integrated`, `fit_product_inhibition`
- `pluss::subsp::natsci::chemistry::crystallization`: `critical_radius`, `nucleation_barrier`, `nucleation_rate`, `jmak_fraction`, `avrami_fit`, `moment_rate`, `steady_state`, `steady_moment`, `exponential_density`, `pbe_rate`
- `pluss::subsp::natsci::chemistry::polymers`: `molecular_weights`, `fh_binodal`, `mayo_lewis`, `wlc_force`
- `pluss::subsp::natsci::chemistry::rate_theory`: `translational_per_volume`, `rotational_linear`, `rotational_nonlinear`, `vibrational`, `eyring_rate`, `tst_rate`, `wigner`, `eckart_transmission`, `eckart`, `eyring_fit`, `collision_rate`, `beyer_swinehart`, `rrkm`, `high_pressure_limit`, `strong_collision`, `hindered_rotor_levels`, `hindered_rotor_partition`, `master_equation`, `weak_collision`
- `pluss::subsp::natsci::chemistry::step_growth`: `conversion_second_order`, `sol_gel`, `reversible_rates`
- `pluss::subsp::natsci::chemistry::surface`: `langmuir`, `langmuir_competitive`, `sips`, `freundlich`, `temkin`, `bet`, `bet_fit`, `specific_surface_area`, `isosteric_heat`, `langmuir_hinshelwood_rate`, `eley_rideal_rate`
