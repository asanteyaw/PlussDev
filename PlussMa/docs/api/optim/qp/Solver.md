<!-- generated from Solver.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `qp::Solver`

A quadratic program whose matrices have been checked and factorised once, ready to be solved for many right-hand sides.

## Signature

```cpp
qp::Solver qp::prepare(const pluss::Series &p, const pluss::Series &a, qp::Options options = {});
```

## Construction

A `Solver` is returned by [`qp::prepare`](prepare.md). It is cheap to copy (it shares nothing mutable) and does not change when it solves.

## Methods

| Method | What it does |
|---|---|
| [`constraints`](Solver/constraints.md) | The number of constraint rows `m` of the prepared problem: the length of `lower`, `upper` and of `A x`. |
| [`dtype`](Solver/dtype.md) | The floating-point type the prepared problem is solved in. |
| [`platform`](Solver/platform.md) | The device the prepared problem lives on: CPU or an accelerator. |
| [`solve`](Solver/solve.md) | Solves the prepared problem for a new linear term and bounds. |
| [`variables`](Solver/variables.md) | The number of variables `n` of the prepared problem: the length of `x` and of `q`. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Solver/Introduction
Solver/constraints
Solver/dtype
Solver/platform
Solver/solve
Solver/variables
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Result.html">
    <span class="label">Previous</span>
    <span class="title">Result</span>
  </a>
  <a class="next-link" href="WarmStart.html">
    <span class="label">Next</span>
    <span class="title">WarmStart</span>
  </a>
</div>
<!-- apiref:nav:end -->
