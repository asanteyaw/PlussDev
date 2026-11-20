<!-- generated from Plan.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::mpc::Plan`

A compiled predictive-control problem.

## Signature

```cpp
std::int64_t control::mpc::Plan::horizon() const noexcept;
std::int64_t control::mpc::Plan::states() const noexcept;
std::int64_t control::mpc::Plan::inputs() const noexcept;
pluss::ScalarType control::mpc::Plan::dtype() const noexcept;
pluss::Platform control::mpc::Plan::platform() const noexcept;
```

## Construction

Returned by `build`.

## Methods

| Method | What it does |
|---|---|
| [`horizon`](Plan/horizon.md) | The prediction horizon the plan was built with. |
| [`states`](Plan/states.md) | The number of states of the planned system. |
| [`inputs`](Plan/inputs.md) | The number of inputs of the planned system. |
| [`dtype`](Plan/dtype.md) | The floating dtype of the plan's solver. |
| [`platform`](Plan/platform.md) | The device the plan's solver lives on. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Plan/Introduction
Plan/horizon
Plan/states
Plan/inputs
Plan/dtype
Plan/platform
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Result.html">
    <span class="label">Previous</span>
    <span class="title">Result</span>
  </a>
  <a class="next-link" href="build.html">
    <span class="label">Next</span>
    <span class="title">build</span>
  </a>
</div>
<!-- apiref:nav:end -->
