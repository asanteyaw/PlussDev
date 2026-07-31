<!-- pluss-hand-authored -->

# Series API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::Series` — the numeric/boolean tensor object underneath
every numeric `Thread` column — one method page at a time, with real, compiled-and-run
examples under every code block.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its signature(s), parameter details, a **Gradient**
  section for anything differentiable, verified output under every example, and
  working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant
  scroll of every method — see [`Series_Methods`](../Series_Methods.md) for the
  full method-naming-philosophy note and [`coden_method_index`](../coden_method_index.md)
  for a flat cross-referenced index of the entire surface, documented and not yet
  documented alike.

## Coverage

`Series` has roughly 350 distinct method names; this reference currently covers a
working subset, prioritized by how load-bearing each method is (indexing/shape,
factories, then the autograd-sensitive family) rather than alphabetically. Every
page here is real, hand-verified prose — nothing below is a mechanically generated
stub.

```{toctree}
:hidden:
:maxdepth: 1

api/series/pos
api/series/size
api/series/sizes
api/series/view
api/series/reshape
api/series/slice
api/series/block
api/series/extract
api/series/extract_put
api/series/extract_put_
api/series/index
api/series/index_put_
api/series/clamp_max
api/series/clamp_min
api/series/copy_
api/series/gather
api/series/index_select
api/series/maximum
api/series/minimum
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
</div>
