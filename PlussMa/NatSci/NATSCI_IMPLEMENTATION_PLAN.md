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

### Cross-domain quantum ownership

Quantum physics and quantum chemistry share state vectors, operators, normalization,
Hermiticity, eigensystems, density matrices, tensor products, partial traces, expectation
values, and unitary/open-system evolution. Those primitives belong to one reusable physics
quantum foundation. Chemistry will add molecular geometry, basis sets, electron integrals,
SCF, electronic energies, and molecular gradients while consuming that foundation; it
must not fork complex linear algebra, state validation, or propagation kernels.

### Phase P3: incompressible flow

Status: **complete. P3a-P3f implement and validate the periodic and bounded solver stack;
P3g supplies reproducible throughput measurements.**

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

The initial arm64 macOS CPU baseline uses `Float64`, a `64 x 64` grid, one warm-up, three
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

This tranche is not viscous compressible Navier--Stokes: heat conduction, species/source
terms, physical boundaries, entropy-stable/high-order Riemann solvers, low-Mach treatment,
and AMR remain separate work. Turbulence closure, non-Newtonian constitutive models,
interface-tracked multiphase flow, and unstructured meshes also remain separate families.

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
