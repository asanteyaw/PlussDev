<!-- generated from TrussModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::Truss`

An immutable compiled truss of pin-jointed bars.

## Signature

```cpp
std::int64_t structure::Truss::nodes() const noexcept;
std::int64_t structure::Truss::bars() const noexcept;
std::int64_t structure::Truss::dim() const noexcept;
```

## Construction

Accessors return counts.

## Methods

| Method | What it does |
|---|---|
| [`nodes`](TrussModel/nodes.md) | The number of nodes. |
| [`bars`](TrussModel/bars.md) | The number of bars. |
| [`dim`](TrussModel/dim.md) | The number of spatial axes: 2 for a plane truss, 3 for a space truss. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TrussModel/Introduction
TrussModel/nodes
TrussModel/bars
TrussModel/dim
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Fix.html">
    <span class="label">Previous</span>
    <span class="title">Fix</span>
  </a>
  <a class="next-link" href="FrameModel.html">
    <span class="label">Next</span>
    <span class="title">Frame</span>
  </a>
</div>
<!-- apiref:nav:end -->
