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
  `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `asinh`, `acosh`,
  `atanh`
- elementwise transforms and activations: `expm1`, `log1p`, `reciprocal`,
  `sigmoid`, `relu`
- special functions: `erf`, `erfc`, `lgamma`
- log-space & NaN-aware reductions: `logsumexp`, `logcumsumexp`, `nansum`,
  `nanmean`
- running extrema: `cummax`, `cummin`
- activations II: `gelu`, `silu`, `softplus`, `leaky_relu`, `prelu`, `logit`,
  `hardshrink`
- broadcast & tiling: `broadcast_to`, `expand`, `expand_as`, `repeat`, `tile`,
  `repeat_interleave`
- windowed / gather / difference: `unfold`, `take_along_dim`, `narrow_copy`,
  `diff`, `ravel`
- pairwise (two-input) math: `hypot`, `atan2`, `logaddexp`, `logaddexp2`,
  `xlogy`, `copysign`, `fmod`, `remainder`, `lerp`, `nextafter`
- unary math II: `rsqrt`, `square`, `abs2`, `cbrt`, `exp2`, `log2`, `log10`,
  `frac`, `deg2rad`, `rad2deg`, `signbit`
- number theory core: `factor`, `is_prime`, `prime`, `factorial`
- grouped (segmented) reductions: `grouped_sum`, `grouped_count`,
  `grouped_mean`, `grouped_min`, `grouped_max`, `grouped_var`, `grouped_std`,
  `grouped_median`, `grouped_mode`
- transport and materialization core: `clone`, `contiguous`, `to`
- rounding and numeric-state helpers: `ceil`, `floor`, `round`, `trunc`,
  `isnan`, `isinf`, `isfinite`, `invalid`, `invalid_index`, `nan_to_num`
- elementwise arithmetic core: `add`, `sub`, `mul`, `div`, `pow`
- elementwise comparison core: `eq`, `ne`, `lt`, `le`, `gt`, `ge`
- selection/counting core: `select`, `count`, `nonzero`, `unique`, `isin`,
  `bincount`, `histc`
- ordering and rank selection: `argsort`, `sort`, `topk`, `kthvalue`
- order statistics: `median`, `nanmedian`, `mode`, `quantile`, `nanquantile`
- cyclic and planar rearrangement: `flip`, `flipud`, `fliplr`, `roll`, `rot90`
- matrix diagonal and triangle core: `trace`, `diag`, `diagonal`,
  `diag_embed`, `diagflat`, `diagonal_scatter`, `tril`, `tril_`, `triu`,
  `triu_`
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
- indexed write primitives: `scatter`, `scatter_`, `scatter_add`,
  `scatter_add_`, `scatter_reduce`, `scatter_reduce_`,
  `scatter_reduce_sum`, `scatter_reduce_amax`, `scatter_reduce_amin`,
  `masked_scatter`, `masked_scatter_`, `masked_select`, `where`, `where_`
- logical and bitwise masks: `logical_and`, `logical_or`, `logical_xor`,
  `logical_not`, `logical_not_`, `bitwise_and`, `bitwise_or`,
  `bitwise_xor`, `bitwise_not`
- elementwise comparison/bounds helpers and a few load-bearing utility ops
- reduction and scan core: `sum`, `mean`, `prod`, `max`, `min`, `argmax`,
  `argmin`, `amax`, `amin`, `norm`, `cumsum`, `cumprod`
- autograd and state controls: `tracks_slope`, `tracks_slope_`, `reverse`,
  `detach`, `detach_`, `retain_grad`, `retains_grad`, `has_sensitivity`,
  `sensitivity`, `clear_sensitivity`, `storage_version`
- accessors: `stat`
- linear-algebra accessor entrypoint: `linearalg`, with its complete product,
  determinant, transformation, factorization, norm, and solver surface
  documented in the dedicated accessor reference

The complete handwritten `SeriesStatAccessor` reference covers:

- descriptive reductions and higher moments
- normal density and cumulative-distribution helpers
- trailing-window summaries
- centering, standardization, and row/column normalization
- differentiable pairwise Euclidean distances

`Series` also exposes `linearalg` and `traverse` accessors on the public
surface. The `linearalg` accessor reference is complete; `traverse` should
receive its own verified batch rather than being stubbed prematurely here.

## Accessors

`Series` has namespace-like accessors for related method families:

- `stat` -> [`SeriesStatAccessor`](series_stat_accessor_api_reference.md)
- `linearalg` ->
  [`SeriesLinearAlgAccessor`](series_linearalg_accessor_api_reference.md)

The `stat` accessor reference is complete across its public surface. Start with
[`Series::stat`](api/series/stat.md) if you want the entrypoint itself explained,
or jump straight to the dedicated accessor reference from the sidebar.

```{toctree}
:hidden:
:maxdepth: 1

api/series/abs
api/series/abs2
api/series/acos
api/series/acosh
api/series/add
api/series/all
api/series/amax
api/series/amin
api/series/any
api/series/argmax
api/series/argmin
api/series/argsort
api/series/asin
api/series/asinh
api/series/atan
api/series/atan2
api/series/atanh
api/series/bincount
api/series/bitwise_and
api/series/bitwise_not
api/series/bitwise_or
api/series/bitwise_xor
api/series/block
api/series/broadcast_to
api/series/cartprod
api/series/cbrt
api/series/ceil
api/series/center
api/series/clamp_max
api/series/clamp_min
api/series/clear_sensitivity
api/series/clone
api/series/comp
api/series/contiguous
api/series/copy_
api/series/copysign
api/series/count
api/series/cos
api/series/cosh
api/series/cummax
api/series/cummin
api/series/cumprod
api/series/cumsum
api/series/deg2rad
api/series/detach
api/series/detach_
api/series/diag
api/series/diag_embed
api/series/diagflat
api/series/diagonal
api/series/diagonal_scatter
api/series/diff
api/series/div
api/series/eq
api/series/erf
api/series/erfc
api/series/exp
api/series/exp2
api/series/expand
api/series/expand_as
api/series/expm1
api/series/extract
api/series/extract_put
api/series/extract_put_
api/series/factor
api/series/factorial
api/series/flatten
api/series/flip
api/series/fliplr
api/series/flipud
api/series/floor
api/series/fmod
api/series/frac
api/series/gather
api/series/ge
api/series/gelu
api/series/grouped_count
api/series/grouped_max
api/series/grouped_mean
api/series/grouped_median
api/series/grouped_min
api/series/grouped_mode
api/series/grouped_std
api/series/grouped_sum
api/series/grouped_var
api/series/gt
api/series/hardshrink
api/series/has_sensitivity
api/series/histc
api/series/hypot
api/series/index
api/series/index_put
api/series/index_put_
api/series/index_select
api/series/intersect1d
api/series/intersection
api/series/intersection_along
api/series/invalid
api/series/invalid_index
api/series/is_prime
api/series/isfinite
api/series/isin
api/series/isinf
api/series/isnan
api/series/kthvalue
api/series/kurtosis
api/series/le
api/series/leaky_relu
api/series/lerp
api/series/lgamma
api/series/linearalg
api/series/log
api/series/log10
api/series/log1p
api/series/log2
api/series/logaddexp
api/series/logaddexp2
api/series/logcumsumexp
api/series/logical_and
api/series/logical_not
api/series/logical_not_
api/series/logical_or
api/series/logical_xor
api/series/logit
api/series/logsumexp
api/series/lt
api/series/masked_scatter
api/series/masked_scatter_
api/series/masked_select
api/series/max
api/series/maximum
api/series/mean
api/series/median
api/series/min
api/series/minimum
api/series/mode
api/series/mul
api/series/nan_to_num
api/series/nanmean
api/series/nanmedian
api/series/nanquantile
api/series/nanstd
api/series/nansum
api/series/nanvar
api/series/narrow_copy
api/series/ne
api/series/neg
api/series/nextafter
api/series/nonzero
api/series/norm
api/series/permute
api/series/pile
api/series/pos
api/series/pow
api/series/prelu
api/series/prime
api/series/prod
api/series/put_
api/series/quantile
api/series/rad2deg
api/series/ravel
api/series/reciprocal
api/series/relu
api/series/remainder
api/series/repeat
api/series/repeat_interleave
api/series/reshape
api/series/retain_grad
api/series/retains_grad
api/series/reverse
api/series/roll
api/series/round
api/series/rot90
api/series/rsqrt
api/series/scatter
api/series/scatter_
api/series/scatter_add
api/series/scatter_add_
api/series/scatter_reduce
api/series/scatter_reduce_
api/series/scatter_reduce_amax
api/series/scatter_reduce_amin
api/series/scatter_reduce_sum
api/series/select
api/series/sensitivity
api/series/setdiff1d
api/series/setdiff_along
api/series/setxor1d
api/series/setxor_along
api/series/sigmoid
api/series/sign
api/series/signbit
api/series/silu
api/series/sin
api/series/sinh
api/series/size
api/series/sizes
api/series/skewness
api/series/slice
api/series/softplus
api/series/sort
api/series/sqrt
api/series/square
api/series/squeeze
api/series/standardize
api/series/stat
api/series/std
api/series/storage_version
api/series/sub
api/series/sum
api/series/take
api/series/take_along_dim
api/series/tan
api/series/tanh
api/series/tile
api/series/to
api/series/topk
api/series/trace
api/series/tracks_slope
api/series/tracks_slope_
api/series/transpose
api/series/traverse_put
api/series/traverse_put_
api/series/traverse_slice
api/series/tril
api/series/tril_
api/series/triu
api/series/triu_
api/series/trunc
api/series/unfold
api/series/unique
api/series/union1d
api/series/unionize
api/series/unionize_along
api/series/unite
api/series/unsqueeze
api/series/var
api/series/view
api/series/where
api/series/where_
api/series/xlogy
series_stat_accessor_api_reference
series_linearalg_accessor_api_reference
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
</div>
