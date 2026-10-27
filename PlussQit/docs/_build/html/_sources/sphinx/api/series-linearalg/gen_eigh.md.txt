<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::gen_eigh

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
template <class ApplyA, class ApplyB, class SolveA>
GenEigResult Series::LinearAlgAccessor::gen_eigh(
    ApplyA&& apply_a,
    ApplyB&& apply_b,
    SolveA&& solve_a,
    const GenEigOptions& options = {}) const

template <class ApplyA, class ApplyB, class SolveA>
GenEigResult SeriesLinearAlgProxy::gen_eigh(
    ApplyA&& apply_a,
    ApplyB&& apply_b,
    SolveA&& solve_a,
    const GenEigOptions& options = {}) const
```

## The Problem It Solves

`gen_eigh()` finds the lowest positive eigenpairs of the generalized Hermitian
problem

```text
A x = lambda B x
```

without requiring either operator to exist as a dense matrix. The receiver is
an initial block of trial vectors. Three callables describe how to apply `A`,
apply `B`, and solve a system with `A`. This division is useful when a matrix is
too large to assemble, when its action is naturally expressed as gathers,
stencils, transforms, or scatter-adds, or when a caller already owns a sparse
or preconditioned solve.

The method uses zero-shift block inverse iteration. Applying `solve_a` after
`apply_b` forms the transformed action `A^-1 B`; its largest transformed
eigenvalues are the reciprocals of the generalized eigenvalues nearest zero.
Each iteration performs an `A`-orthonormalization and a small Rayleigh--Ritz
solve inside the requested trial subspace. The returned values are therefore
ordered from the smallest positive `lambda` upward.

This is not a dense alternative spelling of [`eigh()`](eigh.md). `eigh()` owns
one materialized square matrix and returns its complete spectrum. `gen_eigh()`
owns no matrix, can request fewer modes than the state dimension, and delegates
the expensive operator and inverse actions to the caller.

## The Receiver Is A Trial Block

The receiver has shape

```text
[..., mode, state]
```

Each row along `mode` is one trial vector. The number of rows determines how
many eigenpairs are requested; there is no separate `count` option. Leading
dimensions are independent batch dimensions, and every callback must preserve
the receiver's complete shape, dtype, and platform.

The trial rows need not already be normalized or mutually orthogonal. They must
be linearly independent in the `A` inner product and must contain a component
in every eigendirection being requested. A zero row, duplicate row, or another
rank-deficient block is rejected. A deterministic, problem-informed seed often
converges faster than random noise and makes repeated runs reproducible.

`mode` must be positive and cannot exceed `state`. A full block with
`mode == state` spans the complete state space; with exact callbacks, its first
Rayleigh--Ritz projection can recover the full positive spectrum. A smaller
block iterates toward the lowest modes and ordinarily needs more than one
outer iteration.

## Callback Contracts

All three callables receive a `const Series&` shaped exactly like the trial
block and return a `Series` with that same shape, dtype, and platform. Strides
need not match; a callback may return a contiguous result or a valid view.

| callback | Required action | Important requirement |
| --- | --- | --- |
| `apply_a(x)` | Return `A x` for every trial row and batch lane | `A` must be Hermitian and positive definite on the active state space |
| `apply_b(x)` | Return `B x` for every trial row and batch lane | `B` must be Hermitian; returned modes must have positive `x^H B x` |
| `solve_a(rhs)` | Return the solution of `A y = rhs` | Accuracy controls outer convergence; shape, dtype, and platform cannot change |

For dense matrices, trial vectors are rows while ordinary matrix multiplication
acts on columns. Transpose the block before and after applying or solving the
matrix, as Example 2 shows. Matrix-free callables can usually operate on the
last `state` axis directly and avoid those transposes.

`solve_a` may use a direct factorization, an iterative Krylov solver, a cached
preconditioner, or another exact-enough inverse action. `gen_eigh()` does not
assume how the solve is implemented and does not repeat a solve-specific
residual check. An inaccurate inverse is visible through slower convergence or
nonconverged returned modes.

## Options

`GenEigOptions` controls the outer generalized-eigen iteration.

| Field | Meaning | Default |
| --- | --- | --- |
| `max_iterations` | Maximum block inverse/Rayleigh--Ritz updates | `64` |
| `relative_tolerance` | Relative residual multiplier for each eigenpair | `1e-8` |
| `absolute_tolerance` | Absolute floor added to the residual threshold | `1e-10` |

All tolerances must be finite and non-negative, and `max_iterations` must be
positive. A mode is marked converged when

```text
||A x - lambda B x||_2
  <= absolute_tolerance
     + relative_tolerance * max(||A x||_2, |lambda| ||B x||_2)
```

Exhausting the iteration budget is not an exception. The best Ritz estimates
are returned with the corresponding `.converged` lanes set to `false`, so a
caller can report, retry, or reject them under its own policy.

## Return Value

`GenEigResult` contains five public fields.

| Field | Shape | Meaning |
| --- | --- | --- |
| `.values` | `[..., mode]` | Positive generalized eigenvalues in ascending order |
| `.vectors` | `[..., mode, state]` | Eigenvectors stored by row and normalized so `x^H B x = 1` |
| `.residual` | `[..., mode]` | Euclidean norm of `A x - lambda B x` after final normalization |
| `.converged` | `[..., mode]` | Boolean acceptance result for each returned pair |
| `.iterations` | host `std::int64_t` | Number of outer updates executed |

Unlike `eigh()`, whose eigenvectors occupy columns, `gen_eigh()` preserves the
row-oriented trial-block layout. This keeps every mode as an independent batch
lane for matrix-free operators and iterative solves. Eigenvectors still have
the usual sign or complex-phase ambiguity; compare equations, projectors, or
magnitudes rather than exact vector entries.

## Dtype, Platform, And Batching

The trial block may be `Float32`, `Float64`, `Complex64`, or `Complex128`.
`.values` and `.residual` are real at the corresponding precision;
`.vectors` retain the trial dtype. Integer and Boolean trials are rejected.

The method itself does not move the large state block between platforms.
`apply_a`, `apply_b`, and `solve_a` decide where their work runs and must return
to the receiver's platform. The small `mode x mode` Hermitian decompositions
use the `eigh()` backend, including its documented CPU bridge when the current
platform needs one. This keeps the cost of any bridge proportional to the
number of requested modes rather than the full state dimension.

Leading batch dimensions are processed together. The implementation has no
host loop over a state, mode, or batch lane; only the outer convergence loop is
sequential. Operator callbacks should preserve that batching instead of
extracting lanes to host memory.

## Differentiation

`gen_eigh()` is composed from differentiable Series operations. Reverse history
flows through the executed operator actions, inverse actions,
orthonormalizations, and Rayleigh--Ritz solves. Consequently, parameters
captured by the callbacks receive sensitivities when those callbacks themselves
use differentiable operations.

There are three practical qualifications:

- A callback that detaches, copies through host scalars, or invokes a solver
  without a reverse rule breaks the graph at that callback.
- Eigenvector-dependent objectives require separated eigenvalues and must be
  invariant to sign or complex phase, just as for `eigh()`.
- The graph records the iterations that actually ran. Tight tolerances can
  increase both solve time and reverse-memory use.

Eigenvalue-only objectives are usually the most stable differentiable use.

## Example Output Helper

The examples use this small projection so meaningful digits and shapes remain
visible. It is the same helper used by the permanent reference executable.

```cpp
void values(const char* label, const pluss::Series& value) {
  std::cout << label << "=[";
  const auto lanes = value.host<double>();
  for (std::size_t index = 0; index < lanes.size(); ++index) {
    if (index) std::cout << ", ";
    std::cout << std::fixed << std::setprecision(10) << lanes[index];
  }
  std::cout << "] shape=[";
  for (std::int64_t axis = 0; axis < value.dim(); ++axis) {
    if (axis) std::cout << ", ";
    std::cout << value.size(axis);
  }
  std::cout << "]\n";
}
```

## Examples

### 1. A Complete Diagonal Generalized Spectrum

Here `A = diag(2, 8)` and `B = diag(1, 2)`, so the generalized
eigenvalues are `2/1` and `8/2`. Two independent trial rows span the complete
two-state space.

```cpp
auto seed = pluss::make_series({{1.0, 1.0}, {1.0, -1.0}});
auto a_diag = pluss::make_series({2.0, 8.0});
auto b_diag = pluss::make_series({1.0, 2.0});

auto result = seed.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(a_diag); },
    [&](const pluss::Series& x) { return x.mul(b_diag); },
    [&](const pluss::Series& rhs) { return rhs.div(a_diag); });

values("values", result.values);
values("residual", result.residual);
std::cout << std::boolalpha
          << "converged=" << result.converged.all().item<bool>() << "\n"
          << "iterations=" << result.iterations << "\n";
```

### Output

```text
values=[2.0000000000, 4.0000000000] shape=[2]
residual=[0.0000000000, 0.0000000000] shape=[2]
converged=true
iterations=1
```

#### Common Operator Scaling Preserves The Spectrum

Multiplying both operators by the same positive constant does not change
`lambda`. Rank detection is relative to the `A`-Gram spectrum rather than an
absolute unit-scale threshold, so very small but well-scaled physical units do
not look singular.

```cpp
auto tiny_a = a_diag.mul(1e-20);
auto tiny_b = b_diag.mul(1e-20);
auto scaled = seed.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(tiny_a); },
    [&](const pluss::Series& x) { return x.mul(tiny_b); },
    [&](const pluss::Series& rhs) { return rhs.div(tiny_a); });

values("values", scaled.values);
std::cout << std::boolalpha
          << "converged=" << scaled.converged.all().item<bool>() << "\n";
```

##### Output

```text
values=[2.0000000000, 4.0000000000] shape=[2]
converged=true
```

### 2. Dense Matrices As Operator Callbacks

Dense storage is valid when the state is small. Because trial vectors are rows,
the callbacks transpose to columns for matrix multiplication and transpose the
result back. Calling through `linearalg()` uses the proxy signature and is
equivalent to the direct accessor style above.

```cpp
auto a = pluss::make_series({{4.0, 1.0}, {1.0, 3.0}});
auto result = seed.linearalg().gen_eigh(
    [&](const pluss::Series& x) {
      return a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
    },
    [&](const pluss::Series& x) { return x; },
    [&](const pluss::Series& rhs) {
      return a.linearalg.lu_solve(rhs.transpose(-2, -1))
          .transpose(-2, -1);
    });

values("values", result.values);
values("B_norm", result.vectors.square().sum(-1, false));
std::cout << std::boolalpha << "equation="
          << a.linearalg.mm(result.vectors.transpose(-2, -1))
                 .transpose(-2, -1)
                 .allclose(result.vectors.mul(result.values.unsqueeze(-1)),
                           1e-9, 1e-9)
          << "\n";
```

### Output

```text
values=[2.3819660113, 4.6180339887] shape=[2]
B_norm=[1.0000000000, 1.0000000000] shape=[2]
equation=true
```

With `B = I`, the `B` norm is the ordinary squared Euclidean norm.

### 3. Request Only The Lowest Mode

One trial row requests one eigenpair. The iteration repeatedly amplifies the
eigendirection associated with the smallest eigenvalue.

```cpp
auto result = pluss::make_series({{1.0, 1.0}}).linearalg.gen_eigh(
    [&](const pluss::Series& x) {
      return a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
    },
    [&](const pluss::Series& x) { return x; },
    [&](const pluss::Series& rhs) {
      return a.linearalg.lu_solve(rhs.transpose(-2, -1))
          .transpose(-2, -1);
    });

values("value", result.values);
values("residual", result.residual);
std::cout << "iterations=" << result.iterations << "\n";
```

### Output

```text
value=[2.3819660113] shape=[1]
residual=[0.0000000224] shape=[1]
iterations=30
```

The residual is not expected to print as exact zero for an iterative partial
solve; it satisfies the configured mixed tolerance.

### 4. Batch Independent Operator Families

The first leading lane uses `A = diag(2, 8)` and the second uses
`A = diag(8, 32)`. Both share `B = diag(1, 2)`.

```cpp
auto batch_a =
    pluss::make_series(std::vector<double>{2.0, 8.0, 8.0, 32.0}, {2, 2});
auto batch_seed = seed.unsqueeze(0).expand({2, 2, 2}, true);
auto result = batch_seed.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(batch_a.unsqueeze(-2)); },
    [&](const pluss::Series& x) { return x.mul(b_diag); },
    [&](const pluss::Series& rhs) {
      return rhs.div(batch_a.unsqueeze(-2));
    });

values("values", result.values);
values("residual", result.residual);
std::cout << std::boolalpha
          << "all_converged=" << result.converged.all().item<bool>() << "\n";
```

### Output

```text
values=[2.0000000000, 4.0000000000, 8.0000000000, 16.0000000000] shape=[2, 2]
residual=[0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000] shape=[2, 2]
all_converged=true
```

### 5. Complex Hermitian Operators

Complex trial vectors use conjugate inner products internally. This example's
off-diagonal entries are a conjugate pair, so the matrix is Hermitian but not
real; a plain transpose in the eigensolver would not satisfy this case.
Operator callbacks must preserve the complex dtype.

```cpp
using C = std::complex<double>;
auto complex_seed = pluss::make_series(
    std::vector<C>{{1, 0}, {1, 0}, {1, 0}, {0, 1}}, {2, 2});
auto complex_a = pluss::make_series(
    std::vector<C>{{3, 0}, {0, 1}, {0, -1}, {3, 0}}, {2, 2});
auto complex_b = pluss::make_series(
    std::vector<C>{{1, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2});

auto result = complex_seed.linearalg.gen_eigh(
    [&](const pluss::Series& x) {
      return complex_a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
    },
    [&](const pluss::Series& x) {
      return complex_b.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
    },
    [&](const pluss::Series& rhs) {
      return complex_a.linearalg.lu_solve(rhs.transpose(-2, -1))
          .transpose(-2, -1);
    });

values("values", result.values);
std::cout << std::boolalpha
          << "vectors_dtype=" << result.vectors.dtype().name << "\n"
          << "converged=" << result.converged.all().item<bool>() << "\n";
```

### Output

```text
values=[2.0000000000, 4.0000000000] shape=[2]
vectors_dtype=complex128
converged=true
```

### 6. Inspect A Deliberately Short Iteration Budget

Returning a nonconverged estimate lets an application decide whether to retry
with a better seed, tighter inverse solve, or larger budget.

```cpp
auto trial = pluss::make_series({{1.0, 1.0, 1.0}});
auto a_diag3 = pluss::make_series({2.0, 8.0, 32.0});
pluss::Series::LinearAlgAccessor::GenEigOptions options;
options.max_iterations = 1;
options.relative_tolerance = 1e-14;
options.absolute_tolerance = 0.0;

auto result = trial.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(a_diag3); },
    [&](const pluss::Series& x) { return x; },
    [&](const pluss::Series& rhs) { return rhs.div(a_diag3); }, options);

values("estimate", result.values);
values("residual", result.residual);
std::cout << std::boolalpha
          << "converged=" << result.converged.all().item<bool>() << "\n"
          << "iterations=" << result.iterations << "\n";
```

### Output

```text
estimate=[2.4615384615] shape=[1]
residual=[2.2789417841] shape=[1]
converged=false
iterations=1
```

### 7. Differentiate Through Captured Operator Parameters

The sum is `A[0]/B[0] + A[1]/B[1]`, so its sensitivity to the two diagonal
stiffness values is `[1, 1/2]`.

```cpp
auto tracked_a = pluss::make_series({2.0, 8.0});
tracked_a.tracks_slope_(true);
auto result = seed.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(tracked_a); },
    [&](const pluss::Series& x) { return x.mul(b_diag); },
    [&](const pluss::Series& rhs) { return rhs.div(tracked_a); });

result.values.sum().reverse();
values("values", result.values);
values("d_sum_d_A_diag", tracked_a.sensitivity());
std::cout << std::boolalpha
          << "has_sensitivity=" << tracked_a.has_sensitivity() << "\n";
```

### Output

```text
values=[2.0000000000, 4.0000000000] shape=[2]
d_sum_d_A_diag=[1.0000000000, 0.5000000000] shape=[2]
has_sensitivity=true
```

### 8. Precision Is Preserved

```cpp
auto seed32 = seed.to(pluss::ScalarType::Float32);
auto a32 = a_diag.to(pluss::ScalarType::Float32);
auto b32 = b_diag.to(pluss::ScalarType::Float32);
auto result = seed32.linearalg.gen_eigh(
    [&](const pluss::Series& x) { return x.mul(a32); },
    [&](const pluss::Series& x) { return x.mul(b32); },
    [&](const pluss::Series& rhs) { return rhs.div(a32); });

std::cout << "values_dtype=" << result.values.dtype().name << "\n"
          << "vectors_dtype=" << result.vectors.dtype().name << "\n";
```

### Output

```text
values_dtype=float32
vectors_dtype=float32
```

### 9. Rank, Layout, And Positive-Spectrum Failures

```cpp
try {
  (void)pluss::make_series({{1.0, 0.0}, {2.0, 0.0}})
      .linearalg.gen_eigh(
          [&](const pluss::Series& x) { return x.mul(a_diag); },
          [&](const pluss::Series& x) { return x; },
          [&](const pluss::Series& rhs) { return rhs.div(a_diag); });
} catch (const std::exception& e) {
  std::cout << std::string(e.what()).substr(0, std::string(e.what()).find('\n')) << "\n";
}

try {
  (void)seed.linearalg.gen_eigh(
      [&](const pluss::Series& x) { return x.select(-1, 0); },
      [&](const pluss::Series& x) { return x; },
      [&](const pluss::Series& rhs) { return rhs.div(a_diag); });
} catch (const std::exception& e) {
  std::cout << std::string(e.what()).substr(0, std::string(e.what()).find('\n')) << "\n";
}

try {
  auto negative_b = pluss::make_series({-1.0, -1.0});
  (void)seed.linearalg.gen_eigh(
      [&](const pluss::Series& x) { return x.mul(a_diag); },
      [&](const pluss::Series& x) { return x.mul(negative_b); },
      [&](const pluss::Series& rhs) { return rhs.div(a_diag); });
} catch (const std::exception& e) {
  std::cout << std::string(e.what()).substr(0, std::string(e.what()).find('\n')) << "\n";
}
```

### Output

```text
linearalg::gen_eigh: the trial subspace is rank deficient
linearalg::gen_eigh: apply_a must preserve shape, dtype, and platform
linearalg::gen_eigh: requested modes are not in the positive generalized eigenspace
```

## Failure Modes

The call rejects an undefined, empty, integral, Boolean, rank-zero, or rank-one
trial; `mode > state`; non-finite trial or callback output; invalid options;
rank loss under `A`; callback shape/dtype/platform drift; a non-positive
requested Ritz value; or a non-positive final `B` norm.

The method cannot prove that opaque callbacks are linear, Hermitian, mutually
consistent, or that `solve_a` actually inverts `apply_a`. Violating those
mathematical contracts can produce nonconvergence or meaningless Ritz pairs
without a structural callback error. Verify returned residuals and convergence
instead of accepting values solely because the call returned.

## Performance Guidance

- Keep state operations matrix-free when possible; only `mode x mode` projected
  matrices are materialized.
- Request only the modes the algorithm consumes. The projected decomposition
  and block storage grow with `mode`.
- Batch independent parameter cases in leading axes rather than calling the
  method in a host loop.
- Reuse factorizations or preconditioners inside `solve_a`; it is called once
  per outer iteration.
- Better trial vectors and inverse solves reduce outer iterations. Always read
  `.residual` and `.converged` when using an approximate inverse.

## See Also

- [`eigh`](eigh.md): complete dense symmetric or Hermitian decomposition.
- [`eigvalsh`](eigvalsh.md): dense eigenvalues without returning vectors.
- [`cholesky_solve`](cholesky_solve.md): reusable dense SPD inverse action.
- [`lu_solve`](lu_solve.md): dense general inverse action used in Example 2.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eigvalsh.html"><span class="label">Previous</span><span class="title">eigvalsh</span></a>
  <a class="next-link" href="inverse.html"><span class="label">Next</span><span class="title">inverse</span></a>
</div>
