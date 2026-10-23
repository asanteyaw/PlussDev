# Engineering Structures Implementation

This file records implemented behavior, ownership, verification, and deliberately deferred
scope for `pluss::eng::structure`. It is an implementation boundary, not user-facing API
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
