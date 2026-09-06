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
- special functions II: `digamma`, `polygamma`, `mvlgamma`, `erfinv`,
  `erfcinv`, `i0`, `sinc`, `igamma`, `igammac`, `zeta`, `beta` (with `betaln`)
- log-space & NaN-aware reductions: `logsumexp`, `logcumsumexp`, `nansum`,
  `nanmean`
- running extrema: `cummax`, `cummin`
- activations II: `gelu`, `silu`, `softplus`, `leaky_relu`, `prelu`, `logit`,
  `hardshrink`
- probability normalization: `softmax`, `log_softmax`
- broadcast & tiling: `broadcast_to`, `expand`, `expand_as`, `repeat`, `tile`,
  `repeat_interleave`
- windowed / gather / difference: `unfold`, `take_along_dim`, `narrow_copy`,
  `diff`, `ravel`
- pairwise (two-input) math: `hypot`, `atan2`, `logaddexp`, `logaddexp2`,
  `xlogy`, `copysign`, `fmod`, `remainder`, `lerp`, `nextafter`
- elementwise II: `fmax`, `fmin`, `heaviside`, `gcd`, `lcm`, `ldexp`, `frexp`,
  `float_power`, `floor_divide`
- distribution summaries: `aminmax`, `count_nonzero`, `histogram`
- multivariate spread: `cov`, `corrcoef`
- norm distance and constraint: `dist`, `renorm`
- unary math II: `rsqrt`, `square`, `abs2`, `cbrt`, `exp2`, `log2`, `log10`,
  `frac`, `deg2rad`, `rad2deg`, `signbit`
- number theory core: `factor`, `is_prime`, `prime`, `factorial`
- grouped (segmented) reductions: `grouped_sum`, `grouped_count`,
  `grouped_mean`, `grouped_min`, `grouped_max`, `grouped_var`, `grouped_std`,
  `grouped_median`, `grouped_mode`
- axis reordering: `t`, `mT` / `adjoint`, `moveaxis` / `movedim`,
  `swapaxes` / `swapdims`
- complex numbers: `real`, `imag`, `angle`, `conj`, `sgn`
- torch interop: `tensor` (the reverse of
  [`pluss::series::from_torch`](api/series_factory/from_torch.md))
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
- shape, layout, dtype, and state introspection: `size`, `sizes`, `dim`,
  `numel`, `stride`, `dtype`, `is_contiguous`, platform predicates, dtype
  predicates, `is_leaf`, `is_nonzero`, `is_same_size`, and `is_set_to`
- whole-Series and elementwise comparison: `equal`, `allclose`, and `isclose`,
  with exact shape-aware equality, broadcasted tolerance masks, and native C++
  summary results
- random sampling and stochastic fills: `bernoulli`, `bernoulli_`,
  `multinomial`, `normal_`, `uniform_`, `random_`, `log_normal_`,
  `exponential_`, `geometric_`, and `cauchy_`
- view reshaping: `view`, `reshape`, `transpose`, `permute`, `unsqueeze`,
  `squeeze`, `flatten`
- combine/build primitives: `unite`, `pile`
- axis partitioning: `chunk`, `split` / `split_with_sizes`, `tensor_split`,
  `unbind`, `hsplit` / `vsplit` / `dsplit`
- basic and dynamic selection: `pos`, `slice`, `traverse_slice`, `block`,
  `extract`, `index`
- selector writes: `traverse_put_`, `traverse_put`, `extract_put`,
  `extract_put_`, `index_put`, `index_put_`
- axis-slice indexed writes: `index_add`, `index_add_`, `index_copy`,
  `index_copy_`, `index_fill`, `index_fill_`, `index_reduce`, `index_reduce_`
- flat gather/scatter primitives: `take`, `put_`
- indexed write primitives: `scatter`, `scatter_`, `scatter_add`,
  `scatter_add_`, `scatter_reduce`, `scatter_reduce_`,
  `scatter_reduce_sum`, `scatter_reduce_amax`, `scatter_reduce_amin`,
  `masked_fill`, `masked_fill_`, `masked_scatter`, `masked_scatter_`,
  `masked_select`, `where`, `where_`
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

## Accessors

`Series` has namespace-like accessors for related method families:

- `stat` -> [`SeriesStatAccessor`](series_stat_accessor_api_reference.md)
- `linearalg` ->
  [`SeriesLinearAlgAccessor`](series_linearalg_accessor_api_reference.md)
- `traverse` ->
  [`SeriesTraverseAccessor`](series_traverse_accessor_api_reference.md)

All three accessor references are complete across their public surfaces. Start
with [`Series::stat`](api/series/stat.md),
[`Series::linearalg`](api/series/linearalg.md), or
[`Series::traverse`](api/series/traverse.md) for the entrypoint itself, or jump
straight to a dedicated accessor reference from the sidebar. `traverse` is the
value-driven search family — one call from a condition to the coordinates,
matched values, and flat offsets of every position that satisfies it.

```{toctree}
:hidden:
:maxdepth: 1

api/series/abs
api/series/abs2
api/series/acos
api/series/acosh
api/series/add
api/series/addcdiv
api/series/addcmul
api/series/addmm
api/series/addmv
api/series/addr
api/series/adjoint
api/series/all
api/series/allclose
api/series/amax
api/series/amin
api/series/aminmax
api/series/angle
api/series/any
api/series/argmax
api/series/argmin
api/series/argsort
api/series/asin
api/series/asinh
api/series/atan
api/series/atan2
api/series/atanh
api/series/baddbmm
api/series/beta
api/series/bernoulli
api/series/bernoulli_
api/series/bincount
api/series/bitwise_and
api/series/bitwise_not
api/series/bitwise_or
api/series/bitwise_xor
api/series/block
api/series/bmm
api/series/broadcast_to
api/series/cartprod
api/series/cbrt
api/series/cauchy_
api/series/ceil
api/series/center
api/series/chunk
api/series/clamp_max
api/series/clamp_min
api/series/clear_sensitivity
api/series/clone
api/series/comp
api/series/conj
api/series/contiguous
api/series/copy_
api/series/copysign
api/series/corrcoef
api/series/count
api/series/count_nonzero
api/series/cos
api/series/cosh
api/series/cov
api/series/cross
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
api/series/digamma
api/series/dim
api/series/dist
api/series/div
api/series/dsplit
api/series/dtype
api/series/eq
api/series/equal
api/series/erf
api/series/erfc
api/series/erfcinv
api/series/erfinv
api/series/exp
api/series/exp2
api/series/expand
api/series/expand_as
api/series/exponential_
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
api/series/float_power
api/series/floor
api/series/floor_divide
api/series/fmax
api/series/fmin
api/series/fmod
api/series/frac
api/series/frexp
api/series/gather
api/series/gcd
api/series/ge
api/series/geometric_
api/series/ger
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
api/series/heaviside
api/series/histc
api/series/histogram
api/series/hsplit
api/series/hypot
api/series/i0
api/series/igamma
api/series/igammac
api/series/imag
api/series/index
api/series/index_add
api/series/index_copy
api/series/index_fill
api/series/index_put
api/series/index_put_
api/series/index_reduce
api/series/index_select
api/series/inner
api/series/intersect1d
api/series/intersection
api/series/intersection_along
api/series/invalid
api/series/invalid_index
api/series/is_complex
api/series/is_contiguous
api/series/is_cpu
api/series/is_cuda
api/series/is_floating_point
api/series/is_gpu
api/series/is_leaf
api/series/is_mps
api/series/is_nonzero
api/series/is_prime
api/series/is_same_size
api/series/is_set_to
api/series/is_signed
api/series/isclose
api/series/isfinite
api/series/isin
api/series/isinf
api/series/isnan
api/series/kron
api/series/kthvalue
api/series/kurtosis
api/series/lcm
api/series/ldexp
api/series/le
api/series/leaky_relu
api/series/lerp
api/series/lgamma
api/series/linearalg
api/series/log_normal_
api/series/log
api/series/log_softmax
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
api/series/masked_fill
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
api/series/moveaxis
api/series/movedim
api/series/mul
api/series/multinomial
api/series/mvlgamma
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
api/series/no_grad
api/series/nonzero
api/series/norm
api/series/normal_
api/series/numel
api/series/outer
api/series/permute
api/series/pile
api/series/polygamma
api/series/pos
api/series/pow
api/series/prelu
api/series/prime
api/series/prod
api/series/put_
api/series/quantile
api/series/random_
api/series/rad2deg
api/series/ravel
api/series/real
api/series/reciprocal
api/series/relu
api/series/remainder
api/series/renorm
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
api/series/select_scatter
api/series/sensitivity
api/series/setdiff1d
api/series/setdiff_along
api/series/setxor1d
api/series/setxor_along
api/series/sgn
api/series/sigmoid
api/series/sign
api/series/signbit
api/series/silu
api/series/sin
api/series/sinc
api/series/sinh
api/series/size
api/series/sizes
api/series/skewness
api/series/slice
api/series/slice_scatter
api/series/softmax
api/series/softplus
api/series/sort
api/series/split
api/series/sqrt
api/series/square
api/series/squeeze
api/series/standardize
api/series/stat
api/series/std
api/series/storage_version
api/series/stride
api/series/sub
api/series/sum
api/series/swapaxes
api/series/swapdims
api/series/t
api/series/take
api/series/take_along_dim
api/series/tan
api/series/tanh
api/series/tensor
api/series/tensor_split
api/series/tile
api/series/to
api/series/topk
api/series/trace
api/series/tracks_slope
api/series/tracks_slope_
api/series/transpose
api/series/traverse
api/series/traverse_put
api/series/traverse_put_
api/series/traverse_slice
api/series/tril
api/series/tril_
api/series/triu
api/series/triu_
api/series/trunc
api/series/unbind
api/series/unfold
api/series/unique
api/series/union1d
api/series/unionize
api/series/unionize_along
api/series/uniform_
api/series/unite
api/series/unsqueeze
api/series/var
api/series/view
api/series/vsplit
api/series/where
api/series/where_
api/series/xlogy
api/series/zeta
series_stat_accessor_api_reference
series_linearalg_accessor_api_reference
series_traverse_accessor_api_reference
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
</div>
