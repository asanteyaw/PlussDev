<!-- pluss-hand-authored -->

# Series API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::Series` — the numeric/boolean tensor object
underneath numeric `Timeline` columns — one method page at a time, with real,
compiled-and-run examples under every code block.

## Using This Reference

- Use the left sidebar to open one method page at a time. The method rail is
  intentionally alphabetized A-Z so it behaves like a fast lookup index rather
  than a tutorial sequence.
- Each method page contains its signature(s), parameter details, a **Gradient**
  section for anything differentiable, verified output under every example, and
  working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant
  scroll of every method. Use the left sidebar as the primary lookup index, and
  see [`Series_Methods`](../Series_Methods.md) for the broader method
  naming-and-organization note.

## Coverage

`Series` has roughly 350 distinct method names; this reference currently covers a
working subset, prioritized by how load-bearing each method is (indexing/shape,
factories, then the autograd-sensitive family) rather than alphabetically. Every
page here is real, hand-verified prose — nothing below is a mechanically generated
stub.

The current handwritten shape and slice/indexing cluster now covers:

- shape introspection: `size`, `sizes`
- view reshaping: `view`, `reshape`, `transpose`, `permute`, `unsqueeze`,
  `squeeze`, `flatten`
- combine/build primitives: `unite`, `pile`
- basic and dynamic selection: `pos`, `slice`, `traverse_slice`, `block`,
  `extract`, `index`
- selector writes: `traverse_put_`, `traverse_put`, `extract_put`,
  `extract_put_`, `index_put`, `index_put_`
- flat gather/scatter primitives: `take`, `put_`
- elementwise comparison/bounds helpers and a few load-bearing utility ops
- reduction and scan core: `sum`, `mean`, `prod`, `max`, `min`, `argmax`,
  `argmin`, `cumsum`, `cumprod`
- accessors: `stat`

The current handwritten `SeriesStatAccessor` subset covers:

- statistical dispersion: `var`, `std`, `nanvar`, `nanstd`
- statistical transforms and higher moments: `center`, `standardize`,
  `skewness`, `kurtosis`

`Series` also exposes `linearalg` and `traverse` accessors on the public
surface. Their dedicated handwritten accessor references should land with their
own batches rather than being stubbed prematurely here.

## Accessors

`Series` has namespace-like accessors for related method families:

- `stat` -> [`SeriesStatAccessor`](series_stat_accessor_api_reference.md)

The current handwritten accessor batch is `stat`: reduction statistics,
normalization transforms, and higher-moment summaries. Start with
[`Series::stat`](api/series/stat.md) if you want the entrypoint itself explained,
or jump straight to the dedicated accessor reference from the sidebar.

```{toctree}
:hidden:
:maxdepth: 1

api/series/pos
api/series/size
api/series/sizes
api/series/view
api/series/reshape
api/series/transpose
api/series/permute
api/series/unsqueeze
api/series/squeeze
api/series/flatten
api/series/unite
api/series/pile
api/series/slice
api/series/traverse_slice
api/series/traverse_put_
api/series/traverse_put
api/series/block
api/series/extract
api/series/extract_put
api/series/extract_put_
api/series/index
api/series/index_put
api/series/index_put_
api/series/take
api/series/put_
api/series/clamp_max
api/series/clamp_min
api/series/copy_
api/series/gather
api/series/index_select
api/series/maximum
api/series/minimum
api/series/sum
api/series/mean
api/series/prod
api/series/cumsum
api/series/cumprod
api/series/max
api/series/min
api/series/argmax
api/series/argmin
api/series/stat
series_stat_accessor_api_reference
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
</div>
