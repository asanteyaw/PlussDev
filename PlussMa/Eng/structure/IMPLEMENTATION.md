# Engineering Structures Implementation

This file records implemented behavior, ownership, verification, and deliberately deferred
scope for `pluss::subsp::eng::structure`. It is an implementation boundary, not user-facing API
prose. The generated public inventory in `PlussMa/public_calls.md` remains the source of
truth for which declarations may be documented publicly.

## E6a: Linear Truss Statics

### Public contract

The first structural tranche exposes two functional entry points:

- `structure::truss(const Mesh&, const Bars&) -> Truss` validates and compiles immutable
  node connectivity and differentiable element geometry/material values;
- `structure::solve(const Truss&, const Series& load, const Fix&,
  const SolveOptions&) -> StaticResult` solves one or more constrained static load cases.

`Mesh::position` has shape `[..., node, axis]`, where the final axis has size two or
three. `Bars::area` and `Bars::modulus` have shape `[..., bar]`. `load` has shape
`[..., node, axis]`, and optional prescribed values have shape
`[..., constrained_dof]`. All leading axes broadcast under ordinary Series rules, so
geometry sets, material sweeps, prescribed-motion cases, and load cases are solved in one
numerical path.

The result preserves those leading axes and returns nodal displacement and constrained
reaction, plus per-bar engineering strain, axial stress, signed axial force, total strain
energy, and Krylov convergence diagnostics. Positive axial force denotes tension.

### Canonical formulation

For a bar joining nodes `i` and `j`, the compiled geometry is

```text
delta = x_j - x_i
L     = ||delta||_2
n     = delta / L
k     = E A / L
```

Given nodal displacement `u`, the matrix-free element/global action is

```text
extension_e = n_e dot (u_j - u_i)
force_e     = k_e extension_e n_e
K u         = scatter(j, +force_e) + scatter(i, -force_e)
```

This is algebraically identical to the conventional element block
`(EA/L) [[nn^T, -nn^T], [-nn^T, nn^T]]`, but the implementation never forms a dense
global stiffness matrix. Endpoint displacement is gathered for every element in one
Series operation; all element forces are then scattered to nodes in one batched operation.
There is no numerical loop over elements or load cases.

Dirichlet values are lifted before solution. If `u_c` contains prescribed values and `F`
is the free-degree mask, the correction system is

```text
[F K F + (I - F)] delta = F (f - K u_c)
u = u_c + F delta
```

The canonical `math::krylov::pcg` engine solves this symmetric positive-definite system.
The preconditioner is the exactly assembled stiffness diagonal on free degrees and the
identity on constrained degrees. This keeps the element operator and preconditioner
matrix-free while preserving the established Krylov convergence and breakdown policy.

### Validation and failures

`truss()` rejects non-floating or non-finite coordinates, dimensions other than two or
three, invalid or self-referential connectivity, non-positive area/modulus, zero-length
bars, incompatible scenario axes, and mixed dtype/platform values.

`solve()` rejects malformed loads, duplicate or out-of-range constrained degrees,
non-finite prescribed values, dtype/platform drift, and free degrees with zero axial
diagonal. An excited coupled mechanism is reported by the Krylov breakdown/non-convergence
state. The default policy raises a stable runtime error; setting
`fail_on_nonconvergence=false` returns the diagnostic result for inspection. A singular
structure under a zero load can still return the valid zero equilibrium because no
operator application can infer an unexcited null mode from that right-hand side.

### Differentiation and performance

Coordinates, area, modulus, loads, and prescribed values remain Series throughout
compilation and solution. Gather, scatter-add, reductions, the Jacobi preconditioner, and
the iterative update all retain reverse history. Verification differentiates strain
energy with respect to load, area, and endpoint coordinates and checks the exact analytical
sensitivities.

Topology validation and degree-number construction are host-side because connectivity is
irregular immutable metadata. No physical lane is extracted to the host. `Float32` and
`Float64` are preserved. The same implementation is platform-neutral wherever the
underlying Series gather/scatter and Krylov kernels are available.

### Permanent verification

`Eng/structure/test/structure_test.cpp` covers:

- the exact two-node axial solution, reactions, strain, stress, force, and energy;
- nonzero prescribed displacement and reactions at both constrained ends;
- independent batched loads and independently batched section areas;
- an oblique three-dimensional bar with component reactions;
- `Float32` retention and tolerance-scaled agreement;
- reverse sensitivities to load, area, and geometry;
- zero transverse stiffness, an excited rotational mechanism, and the diagnostic
  non-throwing solve policy; and
- zero length, negative area, and duplicate-constraint failures.

### References inspected

- OpenSees `SRC/element/truss/Truss.cpp` and `CorotTruss.cpp`, inspected 2026-10-22,
  for the `EA/L` directional stiffness blocks, axial-force convention, dimensional node
  layout, and separation between linear and corotational geometry;
- CALFEM for Python `src/calfem/core.py` (`bar2e`, `bar2s`) and
  `examples/exs_bar2.py`, inspected 2026-10-22, for independent element stiffness,
  degree assembly, prescribed-displacement solve, reaction recovery, and member-force
  workflow;
- DOLFINx elasticity demo and `dolfinx::fem::assemble_matrix` contract, inspected
  2026-10-22, for immutable degree maps, Dirichlet lifting, rigid-body null modes,
  derived stress output, and separation of assembly from solver policy;
- PETSc 3.26 KSP manual, inspected 2026-10-22, for matrix-free operator ownership,
  explicit convergence/breakdown reporting, reusable preconditioners, and Jacobi/CG
  composition; and
- local MATLAB R2025b `toolbox/matlab/demos/truss.m`,
  `toolbox/pde/pde/assembleFEMatrices.m`, and
  `toolbox/pde/pde/+pde/@StructuralModel/structuralBC.m`, inspected 2026-10-22, for
  visible structural assembly, prescribed displacement, result, and boundary-condition
  contracts. Protected or compiled MATLAB internals were not inspected.

### Deliberate limits and next depth

E6a is a linear, pin-jointed, small-displacement truss solver. It does not silently treat
bars as beams or frames. Distributed loads, section rotation, bending/shear/torsion,
consistent or lumped mass, thermal strain, tension-only behavior, geometric stiffness,
material plasticity, contact, and nonlinear equilibrium belong to later explicit
tranches. The next E6 work adds beam/frame elements and reusable mass/constraint assembly;
modal, buckling, and transient response will then consume those same operators.

## E6b: Planar Frames And Consistent Mass

### Public contract

E6b adds three functional entry points without exposing element matrices or assembly
helpers:

- `structure::frame(const Mesh&, const Sections&) -> Frame` compiles a planar,
  rigid-jointed Euler-Bernoulli frame;
- `structure::solve(const Frame&, const Series& load, const Fix&,
  const SolveOptions&) -> FrameResult` performs constrained linear statics; and
- `structure::mass(const Frame&, const Series& acceleration) -> Series` applies the
  assembled consistent mass operator without materializing a global mass matrix.

A frame has exactly two coordinate lanes and three nodal degrees of freedom in
`(ux, uy, rz)` order. `Sections::area`, `inertia`, and `modulus` end in the member count.
`density` is optional for static response and mandatory for `mass()`; it is volumetric
density, so the mass per unit length is `density * area`. Leading geometry, section,
load, prescribed-value, and acceleration axes broadcast. The result retains the common
batch shape and reports displacement, constrained reactions, strain energy, Krylov
diagnostics, and local member-end actions in
`(Ni, Vi, Mi, Nj, Vj, Mj)` order.

The dedicated `truss()` and `frame()` names are intentional. Overloading a generic
`make(mesh, {...})` made aggregate property lists ambiguous because a short braced list
could initialize either record. No compatibility aliases preserve that defective call
surface.

### Frame operator

For direction cosines `c = dx/L` and `s = dy/L`, each member gathers global endpoint
motion and transforms it to local lanes:

```text
u =  c ux + s uy
v = -s ux + c uy
q_local = (ui, vi, rz_i, uj, vj, rz_j)
```

The local stiffness coefficients are

```text
ka = EA/L       kb = 12EI/L^3
kc = 6EI/L^2    kd = 4EI/L       ke = 2EI/L
```

and the local end actions are the standard Euler-Bernoulli `K_local q_local` product.
The implementation evaluates those six equations directly over all members and all
leading scenarios. Axial/shear resultants are rotated back to global coordinates,
moments are invariant under the planar rotation, and endpoint contributions are
scatter-added to nodes. No dense element batch or global stiffness matrix is stored.

`FrameResult::end_force` is the force exerted by the member in its positive local degree
directions. For a horizontal cantilever under a positive tip-y load, the fixed-end lanes
therefore contain negative shear and negative moment, while the free-end shear is
positive. Reactions are `K u - load` only on constrained degrees and zero elsewhere.

### Shared constrained solver

E6b removes the support/solver implementation from the truss-specific path. Trusses and
frames now use one source-private constrained-system engine for:

- support range and uniqueness checks;
- scenario broadcasting and prescribed-value construction;
- Dirichlet lifting;
- free/fixed masking;
- Jacobi-preconditioned `math::krylov::pcg`;
- convergence and mechanism policy; and
- reaction recovery.

Each element family supplies only matrix-free `apply()` and `diagonal()` operations.
This is the canonical foundation for later modal, buckling, and transient solvers; those
tranches do not need to invent another degree-numbering or boundary-condition path.

### Consistent mass action

`mass(frame, acceleration)` applies the complete six-degree Euler-Bernoulli consistent
mass matrix. In local coordinates the scale is `rho A L / 420`, with translational
coefficients `140`, `70`, `156`, and `54`, translation-rotation couplings `22L` and
`13L`, and rotary coefficients `4L^2` and `-3L^2`. Local generalized inertia is rotated
and scatter-added with the same topology owner used by stiffness.

The operator form is deliberate: modal iteration and time integration require `M x`, not
a dense `M`. It also preserves gradients through density, area, coordinates, and the
input acceleration. A uniformly accelerating horizontal member produces equal axial
nodal inertia whose sum is exactly `rho A L`.

### Validation, differentiation, and performance

`frame()` rejects non-planar coordinates, invalid or self-connected topology,
zero-length members, non-positive area/inertia/modulus/density, mixed dtype/platform,
non-finite values, and incompatible scenario axes. `solve()` shares the truss load and
support failures and reports an unstiff free degree or excited mechanism explicitly.
`mass()` rejects missing density, malformed acceleration, non-finite values, and
dtype/platform drift.

Topology and support lists are immutable host metadata. Every physical lane remains in
Series operations. Member transforms, stiffness, diagonal assembly, mass action, and
load cases are batched; there is no host loop over a member, node, or scenario. Float32
is normalized at the frame operator boundary rather than widened by scalar coefficient
expressions. Reverse verification covers load, second moment of area, and density.

### Permanent verification

`Eng/structure/test/frame_test.cpp` covers:

- exact cantilever tip displacement, rotation, support reactions, local end actions,
  and strain energy;
- a 45-degree member proving local/global transform invariance;
- a two-member cantilever proving shared-node scatter assembly;
- batched inertia values and separately batched loads;
- exact consistent-mass action under rigid axial acceleration;
- Float32 retention;
- reverse sensitivities to tip load, inertia, and density; and
- negative inertia, missing density, non-planar geometry, and underconstraint failures.

The original truss verifier remains a separate regression for the centralized solver.

### References inspected

- OpenSees `SRC/element/elasticBeamColumn/ElasticBeam2d.cpp` at the repository `master`
  revision visible on 2026-10-22, especially `getTangentStiff()` and `getMass()`, for
  the two-dimensional three-DOF node contract, `EA/L` and `EI/L` basic stiffness,
  coordinate-transformation ownership, and the full consistent mass coefficients;
- OpenSees Elastic Beam Column documentation, inspected 2026-10-22, for required
  `A`, `E`, and `Iz`, geometric transformation, mass-per-length semantics, consistent
  versus lumped mass, and local-force queries;
- CALFEM for Python `src/calfem/core.py` at `master`, inspected 2026-10-22,
  specifically `beam2e`, `beam2s`, and `beam2de`, for an independent six-lane local
  stiffness, transformation matrix, section-force convention, and consistent mass
  matrix;
- PETSc 3.26 KSP documentation, already inspected in E6a and rechecked for E6b, for
  retaining one matrix-free CG/preconditioner owner across element families; and
- local MATLAB R2025b PDE structural sources and visible modal result classes, inspected
  2026-10-22. They confirm structural boundary/result organization but do not expose a
  standalone planar frame element implementation, so no MATLAB formula was claimed.

### Deliberate limits and next depth

E6b is small-displacement, linear-elastic, planar Euler-Bernoulli framing with rigid
joints and nodal loads. Distributed/member loads, releases, Timoshenko shear, 3-D frame
torsion, thermal strain, lumped mass, damping, geometric stiffness, plastic hinges, and
nonlinear equilibrium remain explicit later tranches. The next structural step can now
build modal and buckling operators from the shared stiffness/mass actions before adding
time integration.

## E6c: Matrix-Free Modes And Linearized Buckling

### Public contract

E6c adds two functional analyses over the existing immutable `Frame`:

- `structure::modes(frame, fix, options) -> ModeResult` returns the lowest elastic
  vibration modes; and
- `structure::buckling(frame, reference_load, fix, options) -> BuckResult` returns the
  lowest positive linearized load multipliers about the reference elastic state.

`EigOptions::count` is the requested block size. `max_iterations`,
`relative_tolerance`, and `absolute_tolerance` govern the outer generalized-eigen
residual. `solve` is the canonical PCG policy used every time the stiffness inverse is
applied. Counts and iteration limits must be positive; tolerances must be finite and
non-negative; and `count` cannot exceed the number of free degrees of freedom.

`ModeResult::omega` and `hertz` have shape `[..., mode]`.
`ModeResult::shape` has shape `[..., mode, node, 3]` and is normalized so
`phi^T M phi = 1`. `BuckResult::factor` has shape `[..., mode]`, while its shape field
uses the same mode/node layout and is normalized to unit maximum absolute component.
Both results include an equation residual for each mode, a Boolean convergence lane, and
the host outer-iteration count. Mode signs are not canonical; physical comparisons use
equations, subspaces, or sign-invariant quantities.

### Canonical generalized-eigen owner

The domain-independent iteration lives on
`Series::LinearAlgAccessor::gen_eigh()`, not in PlussMa. For a row-oriented trial block
`[..., mode, state]`, it solves

```text
A x = lambda B x
```

from callbacks that apply `A`, apply `B`, and solve `A y = rhs`. The engine performs
zero-shift block inverse iteration on `A^-1 B`, `A`-orthonormalizes the trial rows,
extracts Ritz pairs from the projected Hermitian problem, sorts the smallest positive
generalized eigenvalues first, and accepts each lane under

```text
||A x - lambda B x||_2
  <= atol + rtol max(||A x||_2, |lambda| ||B x||_2).
```

It returns best estimates with `converged=false` when the budget is exhausted. Callback
shape, dtype, platform, and finiteness are checked at the operation boundary. Real and
complex Hermitian systems, leading batches, Float32/Float64 precision, and reverse
history share this one owner. Only the small `mode x mode` Rayleigh--Ritz matrices are
materialized.

### Modal operator

For free structural degrees, modal analysis supplies

```text
A(phi) = K phi
B(phi) = M phi
solve_a(r) = K^-1 r
```

using the frame's matrix-free elastic stiffness and consistent mass actions. Fixed
degrees receive identity stiffness and zero mass, which excludes constrained motion while
keeping the inverse system nonsingular. Every modal support must be homogeneous; a
nonzero prescribed displacement is a static state, not a vibration boundary condition.
Density is mandatory because `M` cannot be inferred from elastic properties.

The raw generalized eigenvalue is `omega^2`. The result takes its positive square root,
forms `hertz = omega/(2*pi)`, and explicitly mass-normalizes each returned row after the
outer solve. A parameter batch in geometry, section properties, or density remains a
leading result batch; all modes in every case are evaluated together.

### Geometric stiffness and load factors

Buckling first solves the complete reference nodal load with the existing constrained
static path. Local member end actions provide a signed compressive resultant. For each
member of length `L` and compression `N`, the transverse/rotational geometric action is
the standard beam-column matrix scaled by `N/(30L)` with coefficient pattern

```text
[ 36,  3L, -36,  3L ]
[ 3L, 4L2, -3L, -L2 ]
[-36, -3L,  36, -3L ]
[ 3L, -L2, -3L, 4L2 ]
```

Axial lanes are zero. Local contributions rotate and scatter through the same canonical
frame assembly machinery as elastic stiffness. The generalized problem is

```text
K phi = factor K_g(reference_load) phi.
```

Therefore a factor multiplies the complete supplied load pattern. Because the static
state and geometric stiffness are linear in that pattern, multiplying the reference load
by `c > 0` divides the factor by `c`. A purely tensile state has no positive geometric
eigenspace and is rejected rather than reported as a compressive capacity.

### Differentiation and performance

Geometry, area, inertia, modulus, density, and the reference load stay in Series
operations. The stiffness/mass/geometric actions, PCG inverse iterations,
orthonormalization, Ritz extraction, normalization, and result transforms retain reverse
history. Verification includes the analytical inverse-square-root sensitivity of modal
frequency to density and inverse scaling of the buckling factor with load.

Connectivity and support indices remain immutable host metadata. All physical lanes are
gathered, transformed, evaluated, and scatter-added in batches. No global `K`, `M`, or
`K_g` matrix is assembled; no host loop traverses members, nodes, degrees, modes, or
scenarios. A deterministic sine block supplies reproducible full-rank trial rows. The
only sequential work is mathematically iterative: outer eigensolver updates and inner
PCG updates. Callers should request only consumed modes because projected work and block
storage grow with `count`.

### Permanent verification

`Eng/structure/test/spectral_test.cpp` first tests the canonical Series solver on a
diagonal generalized problem, independently of frame assembly. Structural checks then
cover:

- all three free modes of a one-member cantilever against its exact reduced generalized
  eigensystem;
- unit generalized mass for every returned mode;
- a batched density sweep and the expected inverse-square-root frequency scaling;
- the discrete Euler-column buckling factor and exact inverse reference-load scaling;
- Float32 retention;
- reverse sensitivities with respect to density and reference load; and
- stable rejection of absent density, excessive mode count, moving modal supports, and a
  tension-only reference state.

The existing frame and truss suites remain separate regressions for the operators and
constraints consumed here.

### References inspected

- SLEPc 3.26.0 EPS and ST manuals, inspected 2026-10-23, for generalized Hermitian
  operator problems, matrix-free actions, block projection, Rayleigh--Ritz extraction,
  residual convergence, and inverse spectral transformation;
- CALFEM MATLAB `fem/beam2gxe.m` at the `master` revision visible 2026-10-23, for an
  independent geometric beam stiffness formulation and axial-force sign branches;
- OpenSees `SRC/element/elasticBeamColumn/ElasticBeam2d.cpp` at the `master` revision
  visible 2026-10-23, rechecked for planar beam coordinate, stiffness, mass, and
  end-action conventions;
- local MATLAB R2025b `toolbox/pde/pde/pdeeig.m`, inspected 2026-10-23, for the
  constrained generalized problem `Kc UI = lambda B' M B UI`, symmetry handling,
  ordered results, and constrained-basis reconstruction; and
- the E6a/E6b Pluss matrix-free assembly and PCG owners, audited and reused instead of
  introducing dense global matrices or a second constrained solve.

### Deliberate limits

E6c is undamped linear modal analysis and linearized elastic buckling of planar
Euler--Bernoulli frames. It does not yet implement damped or gyroscopic modes, interval
spectral searches, mode tracking through crossings, follower-load geometric terms,
material instability, nonlinear equilibrium paths, or post-buckling. The generic Series
solver accepts user-supplied inverse actions, but PlussMa currently supplies Jacobi-PCG
rather than a production sparse direct or multilevel shift-invert solver.

## E6d: Transient Frame Motion

### Public contract

`structure::motion(frame, load, time, fix, initial, options)` integrates the linear
second-order frame equation

```text
M a(t) + C v(t) + K u(t) = load(t)
C = mass_damping M + stiffness_damping K.
```

`load` has shape `[..., time, node, 3]`; `time` is a strictly increasing vector with at
least two samples. Frame properties, load scenarios, initial displacement/velocity, and
zero-valued support batches broadcast to one leading scenario shape. Results store
displacement, velocity, acceleration, and support reaction as
`[..., time, node, 3]`; kinetic/strain energy and Krylov diagnostics are
`[..., time]`. The returned time vector is the supplied Series, and iteration counts are
integer metadata over the time axis.

Only homogeneous supports are accepted. A constant nonzero support displacement is not
a complete transient boundary condition because its velocity and acceleration histories
are unspecified; accepting it as stationary would silently inject an impulse at the
initial sample. Moving support motion needs a later explicit history contract.

### Generalized-alpha step

The public control is the high-frequency spectral radius `rho_inf` in `[0, 1]`, stored as
`MotionOptions::spectral_radius`. Parameters follow the second-order Chung--Hulbert
convention used independently by MFEM:

```text
alpha_m = (2 - rho_inf) / (1 + rho_inf)
alpha_f = 1 / (1 + rho_inf)
gamma   = 1/2 + alpha_m - alpha_f
beta    = (1 + alpha_m - alpha_f)^2 / 4.
```

`rho_inf = 1` is the average-acceleration Newmark rule. Lower values damp unresolved
high-frequency response without adding physical Rayleigh damping. For each possibly
nonuniform step `dt`, the implementation predicts the intermediate alpha state and
solves directly for `a_alpha`:

```text
u_pred = u_n + alpha_f dt v_n
       + alpha_f dt^2 (1/2 - beta/alpha_m) a_n
v_pred = v_n + alpha_f dt (1 - gamma/alpha_m) a_n

c_u = beta  alpha_f dt^2 / alpha_m
c_v = gamma alpha_f dt   / alpha_m

[(1 + c_v mass_damping) M
 + (c_u + c_v stiffness_damping) K] a_alpha
  = load_alpha - C v_pred - K u_pred.
```

The endpoint state is extrapolated from
`u_alpha = u_pred + c_u a_alpha`,
`v_alpha = v_pred + c_v a_alpha`, and
`a_alpha = (1-alpha_m) a_n + alpha_m a_(n+1)`. Loads are linearly sampled at the same
alpha point. This keeps force, inertia, damping, and stiffness synchronized; it does not
mix endpoint loading with an intermediate equilibrium equation.

### Matrix-free ownership and initialization

Every effective solve reuses the E6b frame stiffness and consistent-mass actions plus the
canonical batched PCG engine. Rayleigh damping is never assembled: its two terms are
folded into the mass and stiffness coefficients above. A Jacobi diagonal is assembled
once from the exact stiffness diagonal and the consistent-mass diagonal, then rescaled
per step. No global `M`, `C`, or `K` is materialized.

Initial acceleration is recovered from equilibrium rather than assumed zero:

```text
M a_0 = load_0 - C v_0 - K u_0.
```

An exact zero right-hand side normally lets an iterative method return in zero steps.
That primal shortcut would make algorithmic reverse mode report a zero Jacobian even
though the mathematical derivative is `M^-1`. For tracked zero-RHS scenario lanes,
initialization therefore uses the linear identity

```text
M(a + p) = r + p
M p0      = p
a         = (a + p) - p0,
```

with a deterministic free-degree probe `p`. The two primal solutions cancel exactly at
`r = 0`, while reverse history retains the inverse-mass action. Untracked zero states keep
the zero-iteration fast path. This policy is local to consistent initialization and does
not perturb ordinary time steps.

Endpoint reactions are recovered as `M a + C v + K u - load` and masked to constrained
degrees. Kinetic and strain energies are respectively `v^T M v / 2` and `u^T K u / 2`.
The average-acceleration undamped rule preserves the verified linear oscillator energy;
generalized-alpha filtering deliberately dissipates its high-frequency component.

### Performance and differentiation

The time loop is necessarily sequential because every state depends on its predecessor.
Within each sample, all scenarios, nodes, members, and degrees are Series lanes. Member
gather/transform/scatter, mass and stiffness actions, damping, energies, reactions, and
PCG iterations remain batched and platform-neutral. Time increments remain scalar Series
views, so no host conversion severs a differentiable time schedule. Histories are joined
in one allocation per result field rather than repeatedly reallocated.

The complete trajectory retains reverse history through initial equilibrium, every
effective solve, Rayleigh damping, nonuniform `dt`, and output assembly. The permanent
test checks the full load-history Jacobian, including the exact-zero first sample that
exercises the shifted inverse-mass rule.

### Permanent verification

`Eng/structure/test/motion_test.cpp` covers:

- undamped axial free vibration against an independently coded scalar
  average-acceleration recurrence and exact energy conservation;
- nonuniform, forced, mass-and-stiffness-damped generalized-alpha motion against a
  separate scalar recurrence;
- batched density and output shape, Float32 retention, reactions only on supports, and
  generalized-alpha high-frequency dissipation;
- reverse sensitivities to initial displacement and every force sample, including a
  zero initial force whose consistent-acceleration derivative is nonzero;
- diagnostic return when the iteration budget is exhausted; and
- stable failures for absent density, repeated time, moving supports, invalid spectral
  radius, negative damping, and inconsistent supported initial motion.

The pre-existing static, mass, modal, and buckling tests remain independent regressions
for every operator consumed by the integrator.

### References inspected

- N. M. Newmark, “A Method of Computation for Structural Dynamics,” ASCE Journal of the
  Engineering Mechanics Division 85(3), 1959, as identified and independently encoded
  by the MFEM second-order solver;
- J. Chung and G. M. Hulbert, “A Time Integration Algorithm for Structural Dynamics With
  Improved Numerical Dissipation: The Generalized-alpha Method,” Journal of Applied
  Mechanics 60(2), 371--375, 1993, DOI `10.1115/1.2900803`, for the spectral-radius
  parameterization and intermediate-state equilibrium;
- MFEM `linalg/ode.hpp` and `linalg/ode.cpp` on the `master` revision visible
  2026-10-23, specifically `NewmarkSolver` and `GeneralizedAlpha2Solver`, for an
  independent second-order predictor/corrector implementation, stability checks, and
  `rho_inf` conversion;
- OpenSees `SRC/analysis/integrator/Newmark.cpp` and `GeneralizedAlpha.cpp` on the
  `master` revision visible 2026-10-23, for displacement-form effective coefficients,
  alpha-level load synchronization, and mass/damping/stiffness tangent composition;
- PETSc 3.26 `TSALPHA2` documentation and its Chung--Hulbert reference, inspected
  2026-10-23, for the scalable implicit second-order solver boundary; and
- local MATLAB R2025b
  `toolbox/pde/pde/+pde/+internal/@FESolverUtilities/solveTimeDependent.m`, inspected
  2026-10-23, for second-order initial-condition constraint reduction, proportional
  damping ownership, nonuniform requested output times, and full-state reconstruction.

### Deliberate limits

E6d is linear small-displacement motion of the current planar Euler--Bernoulli frame.
It does not claim adaptive time stepping, nonlinear iteration, plasticity, contact,
follower loads, base-acceleration input, moving supports, distributed member loads,
modal superposition, or frequency-response analysis. Those require distinct load and
state contracts rather than hidden reinterpretation of nodal force histories.

## E6e: Matrix-Free Harmonic Response

### Public contract

`structure::harmonic(frame, load, omega, fix, options)` solves the steady-state phasor
equation

```text
[K + i omega C - omega^2 M] u(omega) = load(omega)
C = mass_damping M + stiffness_damping K.
```

`omega` is a non-empty, finite, non-negative real vector in radians per unit time.
`load` has shape `[..., frequency, node, 3]` and may be real or complex. A real force
amplitude is promoted through differentiable dtype conversion to the complex precision
paired with the frame: Float32 becomes Complex64 and Float64 becomes Complex128. The
result returns displacement, velocity, acceleration, and constrained reaction phasors;
velocity is `i omega u` and acceleration is `-omega^2 u`.

Frame parameter batches and load-scenario batches broadcast before the explicit
frequency axis. A material batch `[case, member]` and a force sweep
`[frequency, node, 3]` therefore produce `[case, frequency, node, 3]`; the two extents
are never accidentally aligned as competing broadcast axes. Complex phase is retained
through forcing, response, reactions, and reverse mode.

Only homogeneous support amplitudes are accepted. A prescribed complex support motion
requires its own kinematic forcing terms and cannot be represented correctly by merely
inserting a nonzero constrained displacement into the reduced right-hand side.

### Matrix-free dynamic stiffness

The implementation reuses the E6b `FrameKernel` actions for stiffness and consistent
mass. Rayleigh damping is evaluated as a linear combination of those same actions, so
neither `K`, `M`, `C`, nor the frequency-dependent dynamic stiffness is assembled. One
operator call evaluates every model batch, force scenario, and frequency lane:

```text
x -> Kx + i omega (mass_damping Mx + stiffness_damping Kx) - omega^2 Mx.
```

Constrained degrees use identity rows inside the Krylov operator and zero right-hand
sides. Physical reactions are recovered from complete dynamic equilibrium and masked to
constrained degrees. Free degrees are checked against the canonical stiffness diagonal
before iteration so an unsupported mechanism fails with a structural diagnostic rather
than an opaque Krylov result.

The complex, generally indefinite system delegates to the canonical restarted
`math::krylov::gmres` implementation. PCG is deliberately not reused: positive
definiteness is lost near and above resonance even when `K` and `M` are individually
positive definite. `HarmonicResult` carries physical and relative residuals, convergence
lanes, iteration count, operator-evaluation count, and restart count. By default, any
unconverged frequency is an error; disabling `fail_on_nonconvergence` returns diagnostics
for application-owned retry policy.

### Performance and differentiation

Frequency is a Series batch axis, not a host loop. Element gathers, coordinate
transforms, stiffness/mass actions, Arnoldi vectors, small Hessenberg solves, reactions,
and phasor kinematics all remain vectorized over frequency and scenario lanes. GMRES
allocates a restarted basis rather than a dense global structural matrix, and its
iteration budget is shared across the batch.

The complete solve remains on the Series reverse graph. Sensitivities can flow through
real-to-complex force promotion, complex forcing, material properties, both damping
coefficients, frequency, the Krylov projection, and final result projections. As with
any iterative differentiable solve, the graph represents the iterations that executed.

### Permanent verification

`Eng/structure/test/harmonic_test.cpp` covers:

- independently evaluated complex axial responses at zero and nonzero frequencies;
- complex force phase and exact velocity/acceleration phasor identities;
- support reactions confined to constrained degrees;
- independent material and frequency batch axes;
- Float32-to-Complex64 retention;
- reverse sensitivity from a real force amplitude through complex promotion and GMRES;
- explicit zero-budget nonconvergence reporting; and
- stable failures for negative frequency, missing density, mismatched force/frequency
  extents, nonzero support amplitudes, and required convergence.

The complete E6a--E6d suite remains an independent regression for every frame operator
used by the harmonic solver.

### References inspected

- local MATLAB R2025b
  `toolbox/pde/pde/+pde/+internal/@FESolverUtilities/solveDirectFrequencyResponse.m`,
  inspected 2026-10-24, for the constrained direct equation
  `K + i omega C - omega^2 M`, synchronized phasor kinematics, damping ownership, and
  frequency-wise forcing;
- local MATLAB R2025b
  `toolbox/pde/pde/+pde/+internal/@FESolverUtilities/solveModalFrequencyResponse.m`,
  inspected 2026-10-24, for an independent modal projection of the same dynamic
  stiffness and the distinction between direct and reduced response;
- Y. Saad and M. H. Schultz, “GMRES: A Generalized Minimal Residual Algorithm for
  Solving Nonsymmetric Linear Systems,” SIAM Journal on Scientific and Statistical
  Computing 7(3), 856--869, 1986, DOI `10.1137/0907058`, for Arnoldi residual
  minimization and restarted basis ownership; and
- PETSc 3.26 `KSPGMRES` documentation and `src/ksp/ksp/impls/gmres/gmres.c`, inspected
  2026-10-24, for complex-system support, restart controls, orthogonalization, residual
  diagnostics, and scalable operator/preconditioner separation.

### Deliberate limits and next depth

E6e is direct linear frequency response with proportional viscous damping. It does not
yet provide modal truncation, hysteretic material damping, frequency-dependent
impedance, prescribed base motion, response spectra, random vibration, or nonlinear
harmonic balance. Near an undamped eigenfrequency the dynamic stiffness is singular by
physics; the method reports nonconvergence instead of silently regularizing the model.

Plasticity, follower loads, and contact remain separate E6 tranches because each
introduces path state or nonsmooth constraints that should not be hidden inside the
elastic geometric solver.

## E6f: Corotational Nonlinear Equilibrium

### Public contract

`structure::solve(frame, load, fix, nonlinear_options)` is the geometric-nonlinear
overload of the canonical frame solve. It keeps the E6b planar Euler--Bernoulli section
law elastic while permitting large nodal translations and rotations under the
small-strain corotational assumption. The result returns displacement, constrained
reaction, current-local member end actions, strain energy, final free residual,
per-scenario convergence, attained load factor, Newton iterations, accepted steps, and
cutbacks.

The overload is selected by `NonlinearOptions`; the original `SolveOptions` overload
remains the small-displacement linear solve. Loads and prescribed support values retain
the established `[..., node, 3]` / `[..., constrained dof]` shape contract. Model,
loading, and support scenario axes broadcast once before continuation. The current
implementation uses proportional loading, so both external load and nonzero prescribed
support displacement advance with the same scalar load factor.

### Objective element state

One private corotational owner serves static and transient analysis. For each member it
forms the current chord from original coordinates plus nodal translation, then removes
that chord's rigid rotation from both nodal rotations. The three deformational measures
are current extension and two end rotations:

```text
e      = current_length - reference_length
theta1 = nodal_rotation_1 - chord_rotation
theta2 = nodal_rotation_2 - chord_rotation.
```

The basic elastic forces are `N = EA e / L0` and
`[M1, M2] = EI/L0 [[4,2],[2,4]] [theta1, theta2]`. Their exact transformation produces
global internal force, current-local end actions, and an analytical consistent tangent
action. Axial-force and end-moment geometric terms are included in that action; a global
stiffness matrix is never assembled. Summed member energy is
`(N e + M1 theta1 + M2 theta2)/2`.

This separation is objective: translating or finitely rotating an undeformed member
does not create strain, force, or energy. A current chord that collapses to zero length
is a hard error because its frame and tangent are undefined.

### Continuation and safeguarded Newton solves

`load_steps` defines the initial proportional increment. Every target load is solved by
Newton iterations on the free residual `load - internal(displacement)`. The tangent is
applied matrix-free through member gather, batched local algebra, and scatter assembly.
Backtracking accepts only a residual-decreasing correction. A failed inner solve or
line search restores the last accepted equilibrium and halves the load increment;
`max_cutbacks` and `min_load_step` bound that retry policy.

The elastic tangent is symmetric on the stable branch, so corrections use canonical
batched CG. Loss of positive curvature is reported as nonconvergence rather than hidden
behind an arbitrary regularization. Load control therefore does not claim to pass a
limit point; arc-length continuation belongs to a later contract. Setting
`fail_on_nonconvergence=false` returns the last accepted state and attained load factor
for application-owned retry or diagnosis.

For reverse mode, residual construction remains recorded while each converged Newton
Jacobian is treated as the local linearization. This is the implicit-solve derivative:
it retains dependence on load, geometry, and section properties without differentiating
the Jacobian construction itself. Tracked exact-zero right-hand sides use two cancelling
shifted solves, preserving the inverse-tangent derivative while leaving the primal state
exactly zero.

### Performance and permanent verification

Continuation and Newton iterations are sequential by mathematical dependency. All
scenario, member, node, and degree axes remain Series lanes; current frames, basic
deformations, force transformation, tangent actions, residuals, and energies are
vectorized and platform-neutral. Connectivity is gathered and scattered without dense
global assembly.

`Eng/structure/test/nonlinear_test.cpp` covers the exact axial oracle, a
large-displacement cantilever, member end-force signs, strain energy, finite rigid-body
rotation, batched areas, Float32 retention,
nonzero and exactly-zero reverse load sensitivities, bounded cutback reporting, and
invalid continuation controls. The complete linear structure suite remains an
independent small-displacement regression.

### References inspected

- OpenSees `SRC/coordTransformation/CorotCrdTransf2d.cpp` on `master`, inspected
  2026-10-24, for current chord length/direction, exact basic-to-global force
  transformation, geometric tangent composition, and zero-length rejection;
- OpenSees `SRC/analysis/algorithm/equiSolnAlgo/NewtonRaphson.cpp` on `master`,
  inspected 2026-10-24, for current-tangent reformation, unbalance ownership, linear
  solve failure propagation, and convergence-test separation;
- Roman Vetter, *Growth, Interaction and Packing of Thin Objects*, Section 2.3,
  inspected 2026-10-24, for the corotated-frame construction, removal of rigid motion,
  consistent tangent requirement, and Euler--Bernoulli strain energy; and
- local MATLAB R2025b
  `toolbox/pde/pde/+pde/+internal/@FESolverUtilities/solveStationaryNonlinear.m`,
  inspected 2026-10-24, for residual-scaled Newton iteration, residual-decreasing step
  bisection, finite-state checks, and explicit failure on singular or exhausted solves.

## E6g: Nonlinear Generalized-Alpha Motion

### Public contract

`structure::motion(frame, load, time, fix, initial, nonlinear_options)` integrates the
same elastic corotational frame under a nodal force history. `load` is
`[..., time, node, 3]`; `time` is a finite, strictly increasing vector in the frame
precision. Initial displacement and velocity broadcast over model and load scenarios.
Transient supports are homogeneous because moving-base acceleration and support work
need a separate kinematic-input contract.

The result carries time, displacement, velocity, acceleration, support reaction,
kinetic energy, nonlinear strain energy, alpha-level residual norm, per-sample
convergence, and Newton iteration count. The existing `MotionOptions` overload remains
the linear E6d integrator.

### Coupled nonlinear step

E6g retains the Chung--Hulbert generalized-alpha parameterization already verified in
E6d, but equilibrium is re-formed at every Newton iterate:

```text
R(a_alpha) = load_alpha - M a_alpha - C v_alpha - fint(u_alpha)
J          = M + c_v C + c_u Kt(u_alpha)
C          = mass_damping M + stiffness_damping K0.
```

`K0` is the initial elastic stiffness, making Rayleigh damping a stable linear operator;
`Kt` is the current corotational tangent. Force, inertia, damping, and internal force are
all evaluated at their synchronized alpha states. Each Newton correction uses the same
residual-decreasing backtracking policy as E6f. The consistent mass and initial
stiffness actions are reused from E6b, and the nonlinear force/tangent owner is reused
unchanged from E6f.

Initial acceleration is recovered from full nonlinear equilibrium. Its SPD mass solve
uses canonical preconditioned CG and the exact-zero shifted-inverse policy from E6d.
Effective step tangents use CG on the stable symmetric branch; curvature loss or an
exhausted iteration budget becomes a failed sample. With
`fail_on_nonconvergence=false`, the approximate history and diagnostics are returned.

Reactions are reconstructed at output endpoints as
`M a + C v + fint(u) - load` and masked to supported degrees. Kinetic energy is
`v^T M v/2`; strain energy comes from current corotational element measures, not from
the invalid nonlinear expression `u^T K0 u/2`.

### Performance, differentiation, and verification

Time and Newton loops are sequential. All scenarios, members, nodes, and degrees remain
batched Series lanes inside each iteration. No host lane loop or global matrix assembly
is introduced. Histories are accumulated by sample and joined once per returned field.

The recorded residual and detached local Jacobian implement first-order implicit
sensitivity of converged states while avoiding derivative-of-Jacobian paths that would
request unsupported higher-order angle rules. The permanent verifier compares the full
force-history Jacobian with an independently coded scalar recurrence in the axial limit,
including a zero first force sample.

`Eng/structure/test/nonlinear_motion_test.cpp` also covers forced nonuniform time steps,
mass and stiffness damping, large-displacement lateral free motion, finite energy,
batched density, Float32 retention, zero-budget diagnostic return, homogeneous-support
enforcement, and invalid line-search controls.

### References inspected

- J. Chung and G. M. Hulbert, “A Time Integration Algorithm for Structural Dynamics
  With Improved Numerical Dissipation: The Generalized-alpha Method,” *Journal of
  Applied Mechanics* 60(2), 371--375, 1993, DOI `10.1115/1.2900803`, for second-order
  accuracy, alpha-state equilibrium, and high-frequency spectral damping;
- OpenSees `SRC/analysis/integrator/GeneralizedAlpha.cpp` on `master`, inspected
  2026-10-24, for `beta`/`gamma`, synchronized alpha states, and effective current
  tangent composition `alphaF Kt + alphaF c2 C + alphaM c3 M`;
- OpenSees `SRC/coordTransformation/CorotCrdTransf2d.cpp` on `master`, inspected
  2026-10-24, for current basic velocity/acceleration transforms and corotational
  resisting-force/tangent ownership; and
- local MATLAB R2025b
  `toolbox/pde/pde/+pde/+internal/@FESolverUtilities/solveTimeDependent.m`, inspected
  2026-10-24, for constrained second-order initial states, proportional damping,
  nonuniform requested times, nonlinear coefficient reevaluation, and full-state
  recovery.

### Deliberate limits and next depth

E6f/E6g provide planar elastic Euler--Bernoulli frames with large geometry on the stable
load-controlled branch. They do not yet provide 3-D rotations, Timoshenko shear,
distributed/follower member loads, plastic hinges or fiber sections, contact, arc-length
continuation, adaptive time-step rejection, moving supports, or nonlinear damping.
Those are separate state and solver contracts, not flags to attach to this one.
