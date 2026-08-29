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

`Series` exposes a large public method surface, and this reference currently
covers a working subset, prioritized by how load-bearing each family is
(indexing/shape, factories, then the autograd-sensitive family) rather than by
code generation order. Every page here is real, hand-verified prose — nothing
below is a mechanically generated stub.

The current handwritten shape and slice/indexing cluster now covers:

- unary math core: `abs`, `neg`, `sign`, `exp`, `log`, `sqrt`, `sin`, `cos`,
  `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`
- elementwise arithmetic core: `add`, `sub`, `mul`, `div`, `pow`
- elementwise comparison core: `eq`, `ne`, `lt`, `le`, `gt`, `ge`
- selection/counting core: `select`, `nonzero`, `unique`, `isin`, `bincount`,
  `histc`
- number theory core: `factor`, `is_prime`, `prime`
- flat set logic core: `union1d`, `unionize`, `intersect1d`, `intersection`,
  `setdiff1d`, `setxor1d`, `comp`, `cartprod`
- axis-wise set logic core: `unionize_along`, `intersection_along`,
  `setdiff_along`, `setxor_along`
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
- autograd root controls: `tracks_slope`, `tracks_slope_`, `reverse`,
  `has_sensitivity`, `sensitivity`, `clear_sensitivity`
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

api/series/abs
api/series/acos
api/series/add
api/series/all
api/series/any
api/series/argmax
api/series/argmin
api/series/asin
api/series/atan
api/series/bincount
api/series/block
api/series/cartprod
api/series/clamp_max
api/series/clamp_min
api/series/clear_sensitivity
api/series/comp
api/series/copy_
api/series/cos
api/series/cosh
api/series/cumprod
api/series/cumsum
api/series/div
api/series/eq
api/series/exp
api/series/factor
api/series/extract
api/series/extract_put
api/series/extract_put_
api/series/flatten
api/series/gather
api/series/ge
api/series/gt
api/series/has_sensitivity
api/series/histc
api/series/index
api/series/index_put
api/series/index_put_
api/series/index_select
api/series/intersect1d
api/series/intersection
api/series/intersection_along
api/series/is_prime
api/series/isin
api/series/le
api/series/log
api/series/lt
api/series/max
api/series/maximum
api/series/mean
api/series/min
api/series/minimum
api/series/mul
api/series/ne
api/series/neg
api/series/nonzero
api/series/permute
api/series/pile
api/series/pos
api/series/pow
api/series/prime
api/series/prod
api/series/put_
api/series/reshape
api/series/reverse
api/series/select
api/series/sensitivity
api/series/setdiff1d
api/series/setdiff_along
api/series/setxor1d
api/series/setxor_along
api/series/sign
api/series/sin
api/series/sinh
api/series/size
api/series/sizes
api/series/slice
api/series/sqrt
api/series/squeeze
api/series/stat
api/series/sub
api/series/sum
api/series/take
api/series/tan
api/series/tanh
api/series/tracks_slope
api/series/tracks_slope_
api/series/transpose
api/series/traverse_put
api/series/traverse_put_
api/series/traverse_slice
api/series/unique
api/series/union1d
api/series/unionize
api/series/unionize_along
api/series/unite
api/series/unsqueeze
api/series/view
api/series/where
series_stat_accessor_api_reference
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
</div>
