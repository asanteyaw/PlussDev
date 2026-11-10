<!-- generated from multigrid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Multigrid

Geometric multigrid for the cell-centred, conservative, second-order operator on a structured grid of one to three dimensions,

`(A u)ᵢ = − Σₐ hₐ⁻² [ b⁺ₐ,ᵢ (u_{i+eₐ} − uᵢ) − b⁻ₐ,ᵢ (uᵢ − u_{i−eₐ}) ]`,

the discrete form of `−∇·(b ∇u)`, with periodic, Dirichlet or Neumann ends on each axis. A [`Hierarchy`](multigrid/Hierarchy.md) builds the coarse grids once for a problem; its V-cycle then acts as an approximate inverse of `A`. Used as a preconditioner for conjugate gradients ([`pcg`](krylov/pcg.md)) it makes the iteration count nearly independent of the grid size, which plain conjugate gradients cannot.

| You want | Use |
|---|---|
| To describe the grid, its edges and coefficients | [`Axis`](multigrid/Axis.md), [`Edge`](multigrid/Edge.md), [`Problem`](multigrid/Problem.md) |
| Tuning of smoothing and coarsening | [`Options`](multigrid/Options.md) |
| `A u`, or an approximate `A⁻¹ r` | [`Hierarchy::apply`](multigrid/Hierarchy/apply.md), [`Hierarchy::vcycle`](multigrid/Hierarchy/vcycle.md) |

## The Surface

| Symbol | What it is |
|---|---|
| [`Axis`](multigrid/Axis.md) | One spatial axis of the grid: its number of cells, spacing and the conditions at its two ends. |
| [`Edge`](multigrid/Edge.md) | The condition at one end of an axis: periodic, Dirichlet or Neumann. |
| [`Hierarchy`](multigrid/Hierarchy.md) | The multigrid levels of one problem, with the operator, the V-cycle and the grid transfers. |
| [`Options`](multigrid/Options.md) | Tuning of the smoother, the coarsening and the execution path of the V-cycle. |
| [`Problem`](multigrid/Problem.md) | Everything that defines the operator: the axes, the face coefficients and an optional mass term. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

multigrid/Axis
multigrid/Edge
multigrid/Hierarchy
multigrid/Options
multigrid/Problem
```
<!-- apiref:toctree:end -->
