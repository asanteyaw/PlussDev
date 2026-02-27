# Series Guide (Usage-First)

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>
```

## How to read this page

- Every snippet is written as real usage, not pseudo API listing.
- Comments explain why/when to use a method.
- The bottom section includes the full current CodeM method-call cookbook extracted from demo coverage.

## Setup Once, Reuse Everywhere

```cpp
// Reusable fixtures used in examples below.
auto x = pluss::make_series(std::vector<double>{-3, -2, -1, 0, 1, 2, 3}, {7});
auto y = pluss::make_series(std::vector<double>{ 3,  2,  1, 0,-1,-2,-3}, {7});
auto m = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto mm = pluss::make_series(std::vector<double>{1,2,3,4,5,6,7,8,9}, {3,3});

// Index/mask helpers for gather/scatter/masked ops.
auto idx23 = pluss::make_series(std::vector<std::int64_t>{2,1,0,0,1,2}, {2,3});
auto flat_idx = pluss::make_series(std::vector<std::int64_t>{0,3,5}, {3});
auto mask23 = pluss::Series::ones({2,3}, pluss::ScalarType::Bool);
```

## Construction: Real Ways You Will Use

```cpp
// Scalar / literal data
auto s0 = pluss::make_series(3.14);
auto s1 = pluss::make_series(true);
auto s2 = pluss::make_series({1.0, 2.0, 3.0});
auto s3 = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

// Flat host data + explicit shape
std::vector<double> flat{1,2,3,4,5,6};
auto s4 = pluss::make_series(flat, {2,3});
```

```cpp
// Factory families
auto z = pluss::zeros({2,3});
auto o = pluss::ones({2,3});
auto f = pluss::full({2,3}, 7.5);
auto e = pluss::empty({2,3}, pluss::ScalarType::Float64);

// Ranges / random
auto r0 = pluss::arange(0, 10);
auto r1 = pluss::linspace(-1.0, 1.0, 9);
auto rn = pluss::randn({2,3});
auto ri = pluss::randint(0, 10, {2,3});
auto nb = pluss::bernoulli({2,3}, 0.25);
```

## Shape and Indexing Methods

```cpp
// reshape/view: change logical layout (same data if possible)
auto v = m.view({3,2});
auto r = m.reshape({3,2});

// transpose/permute: reorder axes
auto t = m.transpose(0, 1);
auto p = mm.permute({1, 0});

// rank edits
auto u = x.unsqueeze(0);      // [7] -> [1,7]
auto s = u.squeeze();         // [1,7] -> [7]
auto s0 = u.squeeze(0);       // remove a specific axis

// flatten/select/take/nonzero
auto fl = mm.flatten(0, 1);   // [3,3] -> [9]
auto row1 = mm.select(0, 1);  // select axis0 index1
auto picked = m.take(flat_idx);
auto nz = x.gt(0.0).nonzero();
```

## Arithmetic and Comparison

```cpp
// scalar arithmetic
auto a1 = x.add(1.5);
auto a2 = x.sub(1.5);
auto a3 = x.mul(2.0);
auto a4 = x.div(2.0);
auto a5 = x.pow(2.0);

// elementwise arithmetic
auto b1 = x.add(y);
auto b2 = x.sub(y);
auto b3 = x.mul(y);
auto b4 = x.div(y.add(1.0));
auto b5 = x.pow(y.mul(0.0).add(2.0));

// comparisons return bool-like mask Series
auto c1 = x.eq(0.0);
auto c2 = x.ne(0.0);
auto c3 = x.lt(0.0);
auto c4 = x.le(0.0);
auto c5 = x.gt(0.0);
auto c6 = x.ge(0.0);
```

## Bitwise and Logical

```cpp
auto i = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6}, {2,3});

auto bw1 = i.bitwise_and(i.add(1));
auto bw2 = i.bitwise_or(i.add(1));
auto bw3 = i.bitwise_xor(i.add(1));
auto bw4 = i.bitwise_not();

auto l1 = x.gt(0.0).logical_and(x.lt(3.0));
auto l2 = x.gt(0.0).logical_or(x.lt(-2.0));
auto l3 = x.gt(0.0).logical_xor(x.lt(0.0));
auto l4 = x.gt(0.0).logical_not();
```

## Unary Math and Numeric Checks

```cpp
// common unary transforms
auto u1 = x.abs();
auto u2 = x.exp();
auto u3 = x.log1p();
auto u4 = x.sqrt();

// trig/hyperbolic
auto t1 = x.sin();
auto t2 = x.cos();
auto t3 = x.tan();
auto t4 = x.sinh();
auto t5 = x.cosh();
auto t6 = x.tanh();

// rounding and special
auto r1 = x.floor();
auto r2 = x.ceil();
auto r3 = x.round();
auto r4 = x.trunc();
auto r5 = x.reciprocal();

// finite checks
auto f1 = x.isnan();
auto f2 = x.isinf();
auto f3 = x.isfinite();
```

## Reductions, Norms, and Scans

```cpp
// axis reductions
auto s = mm.sum(1, false);
auto mn = mm.mean(1, true);
auto mx = mm.amax(1, false);
auto mi = mm.amin(1, false);
auto aall = mm.gt(0.0).all(1, false);
auto aany = mm.gt(7.0).any(1, false);

// global stats
auto gmean = mm.mean();
auto n2 = x.norm(2.0);
auto v = x.var(false);
auto st = x.std(false);

// cumulative
auto cs = mm.cumsum(1, false);
auto cp = mm.cumprod(1, false);
```

## Masking, Where, Gather, Scatter

```cpp
auto cond = x.gt(0.0);

// where: choose from two values by condition
auto w1 = x.where(cond, y);     // branch Series vs Series
auto w2 = x.where(cond, 42.0);  // branch Series vs scalar

// clamp family
auto cm1 = x.clamp_min(0.0);
auto cm2 = x.clamp_max(0.0);
auto cm3 = x.nan_to_num(0.0, 1e9, -1e9);

// mask extraction / scatter
auto ms = m.masked_select(mask23);
auto src = pluss::make_series(std::vector<double>{10,20,30,40,50,60}, {2,3});
auto g = src.gather(1, idx23);

auto base = pluss::Series::zeros({2,3}, pluss::ScalarType::Float64);
auto sc = base.scatter(1, idx23, src);
auto sca = base.scatter_add(1, idx23, src);
auto scr = base.scatter_reduce_sum(1, idx23, src, true);
```

## Integer Analytics

```cpp
auto ints = pluss::make_series(std::vector<std::int64_t>{1,2,3,3,2,1,5}, {7});

auto uq = ints.unique(true);
auto bc = ints.bincount(0);
auto hc = ints.to(pluss::ScalarType::Float64).histc(5, 0.0, 5.0);
```

## In-Place Methods: Safe Pattern

```cpp
// In-place methods end with `_`.
// Clone first if you need the original unchanged.
auto t = x.clone();
t.add_(1.0)
 .mul_(2.0)
 .clamp_min_(0.0)
 .nan_to_num_(0.0, 1e9, -1e9);
```

Useful in-place families in current demo coverage:

- unary in-place: `exp_ log_ sqrt_ sin_ cos_ tanh_ ...`
- arithmetic in-place: `add_ sub_ mul_ div_ pow_`
- bounds/mask in-place: `clamp_min_ clamp_max_ where_ nan_to_num_`
- index update in-place: `scatter_ scatter_add_ masked_scatter_ put_`

## Signal/Stats Extras (CodeM Surface)

```cpp
// signal transforms
auto dct = x.dct();
auto idct = dct.idct();
auto hil = x.hilbert();
auto hst = x.histcounts(8, -3.0, 3.0);

// distribution/stat helpers
auto pdf = x.abs().pdf(0.0, 1.5);
auto cdf = x.abs().cdf(0.0, 1.5);
auto sk = x.skewness();
auto ku = x.kurtosis();

// moving window helpers
auto ms = x.movsum(3);
auto mmn = x.movmean(3);
auto mstd = x.movstd(3);
```

## Implemented CodeM API: Signatures + Real Calls

Only methods with concrete runtime routes in `CodeM/compat/Invoke.cpp` are documented here.
Signature convention: `self` is the receiver object (the `Series` you call the method on).

# Complete Series Method Reference: Signatures + Real Calls

```cpp
auto x = pluss::make_series(std::vector<double>{-3,-2,-1,0,1,2,3}, {7});
auto y = pluss::make_series(std::vector<double>{3,2,1,0,-1,-2,-3}, {7});
auto pos = pluss::make_series(std::vector<double>{0.2,0.5,1.0,2.0,3.0,4.0,5.0}, {7});
auto unit = pluss::make_series(std::vector<double>{-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9}, {7});
auto m = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto n = pluss::make_series(std::vector<double>{6,5,4,3,2,1}, {2,3});
auto mm = pluss::make_series(std::vector<double>{1,2,3,4,5,6,7,8,9}, {3,3});
auto cond = x.gt(0.0);
auto mask = pluss::Series::ones({2,3}, pluss::ScalarType::Bool);
auto i = pluss::make_series(std::vector<std::int64_t>{1,2,3,2,1,0,3}, {7});
auto bi = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6,7}, {7});
std::vector<std::int64_t> shape_3x2{3,2};
std::vector<std::int64_t> perm_10{1,0};
auto gather_src = pluss::make_series(std::vector<std::int64_t>{10,20,30,40,50,60}, {2,3});
auto gather_idx = pluss::make_series(std::vector<std::int64_t>{2,1,0,0,1,2}, {2,3});
auto scatter_base = pluss::Series::zeros({2,3}, pluss::ScalarType::Int64);
auto scatter_val = pluss::make_series(std::vector<std::int64_t>{1,2,3,4,5,6}, {2,3});
auto flat_idx = pluss::make_series(std::vector<std::int64_t>{0,3,5}, {3});
auto flat_src = pluss::make_series(std::vector<std::int64_t>{99,88,77}, {3});
```


## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

### `abs2`

#### Signature

```cpp
Series abs2(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `abs2` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.hilbert().abs2();
```

### `acos`

#### Signature

```cpp
Series acos(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `acos` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.acos();
```

### `acosh`

#### Signature

```cpp
Series acosh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `acosh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.add(1.0).acosh();
```

### `add`

#### Signature

```cpp
Series add(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `add` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series add(self, double)`

#### Examples

```cpp
auto out = x.add(y);
```

### `add`

#### Signature

```cpp
Series add(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `add` computation to `self`. Arguments: `double`.

#### See also

- `Series add(self, const Series&)`

#### Examples

```cpp
auto out = x.add(1.5);
```

### `addbmm`

#### Signature

```cpp
Series addbmm(self, const Series& batch1, const Series& batch2)
```

#### Parameters

`batch1` : `const Series&`
    Required method argument.

`batch2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2`.

#### See also

- `Series addbmm(self, const Series& batch1, const Series& batch2, double beta, double alpha)`

#### Examples

```cpp
auto out = pluss::Series::zeros({2,2}, pluss::ScalarType::Float64).addbmm(pluss::Series::ones({3,2,4}, pluss::ScalarType::Float64), pluss::Series::ones({3,4,2}, pluss::ScalarType::Float64));
```

### `addbmm`

#### Signature

```cpp
Series addbmm(self, const Series& batch1, const Series& batch2, double beta, double alpha)
```

#### Parameters

`batch1` : `const Series&`
    Required method argument.

`batch2` : `const Series&`
    Required method argument.

`beta` : `double`
    Required method argument.

`alpha` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2, double beta, double alpha`.

#### See also

- `Series addbmm(self, const Series& batch1, const Series& batch2)`

#### Examples

```cpp
auto out = pluss::Series::zeros({2,2}, pluss::ScalarType::Float64).addbmm(pluss::Series::ones({3,2,4}, pluss::ScalarType::Float64), pluss::Series::ones({3,4,2}, pluss::ScalarType::Float64), 0.5, 2.0);
```

### `airy`

#### Signature

```cpp
Series airy(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `airy` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.airy();
```

### `all`

#### Signature

```cpp
Series all(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.gt(0.0).all(1, false);
```

### `amax`

#### Signature

```cpp
Series amax(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.amax(1, false);
```

### `amin`

#### Signature

```cpp
Series amin(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.amin(1, false);
```

### `angle`

#### Signature

```cpp
Series angle(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `angle` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.hilbert().angle();
```

### `any`

#### Signature

```cpp
Series any(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.gt(7.0).any(1, false);
```

### `asin`

#### Signature

```cpp
Series asin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `asin` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.asin();
```

### `asinh`

#### Signature

```cpp
Series asinh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `asinh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.asinh();
```

### `atan`

#### Signature

```cpp
Series atan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `atan` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.atan();
```

### `atanh`

#### Signature

```cpp
Series atanh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `atanh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.atanh();
```

### `besseli`

#### Signature

```cpp
Series besseli(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besseli` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.besseli();
```

### `besselj`

#### Signature

```cpp
Series besselj(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besselj` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.besselj();
```

### `besselk`

#### Signature

```cpp
Series besselk(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `besselk` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.besselk();
```

### `bessely`

#### Signature

```cpp
Series bessely(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bessely` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.bessely();
```

### `beta`

#### Signature

```cpp
Series beta(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `beta` computation to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = pos.beta(2.0);
```

### `betaln`

#### Signature

```cpp
Series betaln(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `betaln` computation to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = pos.betaln(2.0);
```

### `bincount`

#### Signature

```cpp
Series bincount(self, std::int64_t minlength)
```

#### Parameters

`minlength` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t minlength`.

#### Examples

```cpp
auto out = i.sub(1.0).to(ScalarType::Int64).bincount(0);
```

### `bitwise_and`

#### Signature

```cpp
Series bitwise_and(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_and` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = bi.bitwise_and(bi.add(1));
```

### `bitwise_not`

#### Signature

```cpp
Series bitwise_not(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_not` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = bi.bitwise_not();
```

### `bitwise_or`

#### Signature

```cpp
Series bitwise_or(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_or` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = bi.bitwise_or(bi.add(1));
```

### `bitwise_xor`

#### Signature

```cpp
Series bitwise_xor(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `bitwise_xor` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = bi.bitwise_xor(bi.add(1));
```

### `cdf`

#### Signature

```cpp
Series cdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series cdf(self, double stddev)`
- `Series cdf(self, double mean, double stddev)`

#### Examples

```cpp
auto out = pos.cdf();
```

### `cdf`

#### Signature

```cpp
Series cdf(self, double mean, double stddev)
```

#### Parameters

`mean` : `double`
    Required method argument.

`stddev` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `double mean, double stddev`.

#### See also

- `Series cdf(self)`
- `Series cdf(self, double stddev)`

#### Examples

```cpp
auto out = pos.cdf(0.0, 1.5);
```

### `cdf`

#### Signature

```cpp
Series cdf(self, double stddev)
```

#### Parameters

`stddev` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `double stddev`.

#### See also

- `Series cdf(self)`
- `Series cdf(self, double mean, double stddev)`

#### Examples

```cpp
auto out = pos.cdf(1.5);
```

### `cdist`

#### Signature

```cpp
Series cdist(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cdist` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = m.cdist(n);
```

### `ceil`

#### Signature

```cpp
Series ceil(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ceil` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.div(2.0).ceil();
```

### `center`

#### Signature

```cpp
Series center(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `center` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.center();
```

### `clamp_max`

#### Signature

```cpp
Series clamp_max(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_max` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series clamp_max(self, double)`

#### Examples

```cpp
auto out = x.clamp_max(y);
```

### `clamp_max`

#### Signature

```cpp
Series clamp_max(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_max` computation to `self`. Arguments: `double`.

#### See also

- `Series clamp_max(self, const Series&)`

#### Examples

```cpp
auto out = x.clamp_max(0.0);
```

### `clamp_min`

#### Signature

```cpp
Series clamp_min(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_min` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series clamp_min(self, double)`

#### Examples

```cpp
auto out = x.clamp_min(y);
```

### `clamp_min`

#### Signature

```cpp
Series clamp_min(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `clamp_min` computation to `self`. Arguments: `double`.

#### See also

- `Series clamp_min(self, const Series&)`

#### Examples

```cpp
auto out = x.clamp_min(0.0);
```

### `cos`

#### Signature

```cpp
Series cos(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cos` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.cos();
```

### `cosh`

#### Signature

```cpp
Series cosh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cosh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.cosh();
```

### `cplxpair`

#### Signature

```cpp
Series cplxpair(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `cplxpair` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.hilbert().cplxpair();
```

### `cumprod`

#### Signature

```cpp
Series cumprod(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.cumprod(1, false);
```

### `cumprod_`

#### Signature

```cpp
Series& cumprod_(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `cumprod`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto t = x.clone(); t.cumprod_(1, false);
```

### `cumsum`

#### Signature

```cpp
Series cumsum(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.cumsum(1, false);
```

### `cumsum_`

#### Signature

```cpp
Series& cumsum_(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `cumsum`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto t = x.clone(); t.cumsum_(1, false);
```

### `dct`

#### Signature

```cpp
Series dct(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dct` computation to `self`. No additional arguments.

#### See also

- `Series dct(self, std::int64_t dim)`

#### Examples

```cpp
auto out = x.dct();
```

### `dct`

#### Signature

```cpp
Series dct(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dct` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series dct(self)`

#### Examples

```cpp
auto out = x.dct(0);
```

### `div`

#### Signature

```cpp
Series div(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `div` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series div(self, double)`

#### Examples

```cpp
auto out = pos.div(pos.add(1.0));
```

### `div`

#### Signature

```cpp
Series div(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `div` computation to `self`. Arguments: `double`.

#### See also

- `Series div(self, const Series&)`

#### Examples

```cpp
auto out = x.div(2.0);
```

### `dst`

#### Signature

```cpp
Series dst(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dst` computation to `self`. No additional arguments.

#### See also

- `Series dst(self, std::int64_t dim)`

#### Examples

```cpp
auto out = x.dst();
```

### `dst`

#### Signature

```cpp
Series dst(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `dst` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series dst(self)`

#### Examples

```cpp
auto out = x.dst(0);
```

### `ediff1d`

#### Signature

```cpp
Series ediff1d(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ediff1d` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.ediff1d();
```

### `eq`

#### Signature

```cpp
Series eq(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `eq` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.eq(0.0);
```

### `erf`

#### Signature

```cpp
Series erf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erf` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.erf();
```

### `erfc`

#### Signature

```cpp
Series erfc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erfc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.erfc();
```

### `erfcinv`

#### Signature

```cpp
Series erfcinv(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `erfcinv` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.abs().erfcinv();
```

### `exp`

#### Signature

```cpp
Series exp(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `exp` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.exp();
```

### `expm1`

#### Signature

```cpp
Series expm1(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `expm1` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.expm1();
```

### `fftshift`

#### Signature

```cpp
Series fftshift(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series fftshift(self, const std::vector<std::int64_t>& dims)`

#### Examples

```cpp
auto out = mm.fftshift();
```

### `fftshift`

#### Signature

```cpp
Series fftshift(self, const std::vector<std::int64_t>& dims)
```

#### Parameters

`dims` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.

#### See also

- `Series fftshift(self)`

#### Examples

```cpp
auto out = mm.fftshift(std::vector<std::int64_t>{0,1});
```

### `flatten`

#### Signature

```cpp
Series flatten(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto out = mm.flatten(0, 1);
```

### `floor`

#### Signature

```cpp
Series floor(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `floor` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.div(2.0).floor();
```

### `gather`

#### Signature

```cpp
Series gather(self, std::int64_t, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `gather` computation to `self`. Arguments: `std::int64_t, const Series&`.

#### Examples

```cpp
auto out = gather_src.gather(1, gather_idx);
```

### `ge`

#### Signature

```cpp
Series ge(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ge` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.ge(0.0);
```

### `gt`

#### Signature

```cpp
Series gt(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `gt` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.gt(0.0);
```

### `hilbert`

#### Signature

```cpp
Series hilbert(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `hilbert` computation to `self`. No additional arguments.

#### See also

- `Series hilbert(self, std::int64_t dim)`

#### Examples

```cpp
auto out = x.hilbert();
```

### `hilbert`

#### Signature

```cpp
Series hilbert(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `hilbert` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series hilbert(self)`

#### Examples

```cpp
auto out = x.hilbert(0);
```

### `histc`

#### Signature

```cpp
Series histc(self, std::int64_t, double, double)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `double`
    Required method argument.

`arg3` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, double, double`.

#### Examples

```cpp
auto out = pos.histc(5, 0.0, 5.0);
```

### `histcounts`

#### Signature

```cpp
Series histcounts(self, std::int64_t, double, double)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `double`
    Required method argument.

`arg3` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, double, double`.

#### Examples

```cpp
auto out = pos.histcounts(5, 0.0, 5.0);
```

### `idct`

#### Signature

```cpp
Series idct(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idct` computation to `self`. No additional arguments.

#### See also

- `Series idct(self, std::int64_t dim)`

#### Examples

```cpp
auto out = x.dct().idct();
```

### `idct`

#### Signature

```cpp
Series idct(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idct` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series idct(self)`

#### Examples

```cpp
auto out = x.idct(0);
```

### `idst`

#### Signature

```cpp
Series idst(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idst` computation to `self`. No additional arguments.

#### See also

- `Series idst(self, std::int64_t dim)`

#### Examples

```cpp
auto out = x.dst().idst();
```

### `idst`

#### Signature

```cpp
Series idst(self, std::int64_t dim)
```

#### Parameters

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `idst` computation to `self`. Arguments: `std::int64_t dim`.

#### See also

- `Series idst(self)`

#### Examples

```cpp
auto out = x.idst(0);
```

### `ifftshift`

#### Signature

```cpp
Series ifftshift(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series ifftshift(self, const std::vector<std::int64_t>& dims)`

#### Examples

```cpp
auto out = mm.fftshift().ifftshift();
```

### `ifftshift`

#### Signature

```cpp
Series ifftshift(self, const std::vector<std::int64_t>& dims)
```

#### Parameters

`dims` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.

#### See also

- `Series ifftshift(self)`

#### Examples

```cpp
auto out = mm.ifftshift(std::vector<std::int64_t>{0,1});
```

### `imag`

#### Signature

```cpp
Series imag(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `imag` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.hilbert().imag();
```

### `isfinite`

#### Signature

```cpp
Series isfinite(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isfinite` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.isfinite();
```

### `isinf`

#### Signature

```cpp
Series isinf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isinf` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.isinf();
```

### `isnan`

#### Signature

```cpp
Series isnan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `isnan` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.isnan();
```

### `kurtosis`

#### Signature

```cpp
Series kurtosis(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `kurtosis` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.kurtosis();
```

### `le`

#### Signature

```cpp
Series le(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `le` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.le(0.0);
```

### `lgamma`

#### Signature

```cpp
Series lgamma(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `lgamma` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.lgamma();
```

### `log`

#### Signature

```cpp
Series log(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `log` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.log();
```

### `log1p`

#### Signature

```cpp
Series log1p(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `log1p` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.log1p();
```

### `logical_and`

#### Signature

```cpp
Series logical_and(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_and` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = x.gt(0.0).logical_and(x.lt(3.0));
```

### `logical_not`

#### Signature

```cpp
Series logical_not(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_not` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.gt(0.0).logical_not();
```

### `logical_or`

#### Signature

```cpp
Series logical_or(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_or` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = x.gt(0.0).logical_or(x.lt(-2.0));
```

### `logical_xor`

#### Signature

```cpp
Series logical_xor(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `logical_xor` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = x.gt(0.0).logical_xor(x.lt(0.0));
```

### `lt`

#### Signature

```cpp
Series lt(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `lt` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.lt(0.0);
```

### `masked_scatter`

#### Signature

```cpp
Series masked_scatter(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `masked_scatter` to `self`. Arguments: `const Series&, const Series&`.

#### Examples

```cpp
auto out = m.masked_scatter(mask, n);
```

### `masked_scatter_`

#### Signature

```cpp
Series& masked_scatter_(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `masked_scatter`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&`.

#### Examples

```cpp
auto t = m.clone(); t.masked_scatter_(mask, n);
```

### `masked_select`

#### Signature

```cpp
Series masked_select(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `masked_select` to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = m.masked_select(mask);
```

### `maximum`

#### Signature

```cpp
Series maximum(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `maximum` computation to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.maximum(0.0);
```

### `mean`

#### Signature

```cpp
Series mean(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### See also

- `Series mean(self, std::int64_t, bool)`

#### Examples

```cpp
auto out = mm.mean();
```

### `mean`

#### Signature

```cpp
Series mean(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### See also

- `Series mean(self)`

#### Examples

```cpp
auto out = mm.mean(1, true);
```

### `minimum`

#### Signature

```cpp
Series minimum(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `minimum` computation to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.minimum(0.0);
```

### `movmax`

#### Signature

```cpp
Series movmax(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmax(self, std::int64_t window)`
- `Series movmax(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmax();
```

### `movmax`

#### Signature

```cpp
Series movmax(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmax(self)`
- `Series movmax(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmax(3);
```

### `movmax`

#### Signature

```cpp
Series movmax(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmax(self)`
- `Series movmax(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movmax(2, 1);
```

### `movmean`

#### Signature

```cpp
Series movmean(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmean(self, std::int64_t window)`
- `Series movmean(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmean();
```

### `movmean`

#### Signature

```cpp
Series movmean(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmean(self)`
- `Series movmean(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmean(3);
```

### `movmean`

#### Signature

```cpp
Series movmean(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmean(self)`
- `Series movmean(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movmean(2, 1);
```

### `movmin`

#### Signature

```cpp
Series movmin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movmin(self, std::int64_t window)`
- `Series movmin(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmin();
```

### `movmin`

#### Signature

```cpp
Series movmin(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movmin(self)`
- `Series movmin(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movmin(3);
```

### `movmin`

#### Signature

```cpp
Series movmin(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movmin(self)`
- `Series movmin(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movmin(2, 1);
```

### `movstd`

#### Signature

```cpp
Series movstd(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movstd(self, std::int64_t window)`
- `Series movstd(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movstd();
```

### `movstd`

#### Signature

```cpp
Series movstd(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movstd(self)`
- `Series movstd(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movstd(3);
```

### `movstd`

#### Signature

```cpp
Series movstd(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movstd(self)`
- `Series movstd(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movstd(2, 1);
```

### `movsum`

#### Signature

```cpp
Series movsum(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movsum(self, std::int64_t window)`
- `Series movsum(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movsum();
```

### `movsum`

#### Signature

```cpp
Series movsum(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movsum(self)`
- `Series movsum(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movsum(3);
```

### `movsum`

#### Signature

```cpp
Series movsum(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movsum(self)`
- `Series movsum(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movsum(2, 1);
```

### `movvar`

#### Signature

```cpp
Series movvar(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). No additional arguments.

#### See also

- `Series movvar(self, std::int64_t window)`
- `Series movvar(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movvar();
```

### `movvar`

#### Signature

```cpp
Series movvar(self, std::int64_t window)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window`.

#### See also

- `Series movvar(self)`
- `Series movvar(self, std::int64_t window, std::int64_t dim)`

#### Examples

```cpp
auto out = x.movvar(3);
```

### `movvar`

#### Signature

```cpp
Series movvar(self, std::int64_t window, std::int64_t dim)
```

#### Parameters

`window` : `std::int64_t`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.

#### See also

- `Series movvar(self)`
- `Series movvar(self, std::int64_t window)`

#### Examples

```cpp
auto out = mm.movvar(2, 1);
```

### `mul`

#### Signature

```cpp
Series mul(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `mul` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series mul(self, double)`

#### Examples

```cpp
auto out = x.mul(y);
```

### `mul`

#### Signature

```cpp
Series mul(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `mul` computation to `self`. Arguments: `double`.

#### See also

- `Series mul(self, const Series&)`

#### Examples

```cpp
auto out = x.mul(2.0);
```

### `nan_to_num`

#### Signature

```cpp
Series nan_to_num(self, double, double, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

`arg2` : `double`
    Required method argument.

`arg3` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nan_to_num` computation to `self`. Arguments: `double, double, double`.

#### Examples

```cpp
auto out = x.nan_to_num(0.0, 1e9, -1e9);
```

### `nanstd`

#### Signature

```cpp
Series nanstd(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nanstd` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.nanstd();
```

### `nanvar`

#### Signature

```cpp
Series nanvar(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `nanvar` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.nanvar();
```

### `ne`

#### Signature

```cpp
Series ne(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `ne` to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.ne(0.0);
```

### `nonzero`

#### Signature

```cpp
Series nonzero(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### Examples

```cpp
auto out = cond.nonzero();
```

### `norm`

#### Signature

```cpp
Series norm(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `norm` computation to `self`. Arguments: `double`.

#### Examples

```cpp
auto out = x.norm(2.0);
```

### `normc`

#### Signature

```cpp
Series normc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `normc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = m.normc();
```

### `normr`

#### Signature

```cpp
Series normr(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `normr` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = m.normr();
```

### `pdf`

#### Signature

```cpp
Series pdf(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. No additional arguments.

#### See also

- `Series pdf(self, double stddev)`
- `Series pdf(self, double mean, double stddev)`

#### Examples

```cpp
auto out = pos.pdf();
```

### `pdf`

#### Signature

```cpp
Series pdf(self, double mean, double stddev)
```

#### Parameters

`mean` : `double`
    Required method argument.

`stddev` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `double mean, double stddev`.

#### See also

- `Series pdf(self)`
- `Series pdf(self, double stddev)`

#### Examples

```cpp
auto out = pos.pdf(0.0, 1.5);
```

### `pdf`

#### Signature

```cpp
Series pdf(self, double stddev)
```

#### Parameters

`stddev` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `double stddev`.

#### See also

- `Series pdf(self)`
- `Series pdf(self, double mean, double stddev)`

#### Examples

```cpp
auto out = pos.pdf(1.5);
```

### `pdist2`

#### Signature

```cpp
Series pdist2(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pdist2` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = m.pdist2(n);
```

### `permute`

#### Signature

```cpp
Series permute(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto out = mm.permute(std::span<const std::int64_t>(perm_10.data(), perm_10.size()));
```

### `pow`

#### Signature

```cpp
Series pow(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pow` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series pow(self, double)`

#### Examples

```cpp
auto out = pos.pow(pos.mul(0.0).add(2.0));
```

### `pow`

#### Signature

```cpp
Series pow(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `pow` computation to `self`. Arguments: `double`.

#### See also

- `Series pow(self, const Series&)`

#### Examples

```cpp
auto out = pos.pow(2.0);
```

### `put_`

#### Signature

```cpp
Series& put_(self, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `bool`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `put`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&, bool`.

#### Examples

```cpp
auto t = gather_src.clone(); t.put_(flat_idx, flat_src, false);
```

### `real`

#### Signature

```cpp
Series real(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `real` to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.hilbert().real();
```

### `reciprocal`

#### Signature

```cpp
Series reciprocal(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `reciprocal` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.reciprocal();
```

### `relu`

#### Signature

```cpp
Series relu(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `relu` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.relu();
```

### `reshape`

#### Signature

```cpp
Series reshape(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto out = m.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### `round`

#### Signature

```cpp
Series round(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `round` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.div(2.0).round();
```

### `scatter`

#### Signature

```cpp
Series scatter(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto out = scatter_base.scatter(1, gather_idx, scatter_val);
```

### `scatter_`

#### Signature

```cpp
Series& scatter_(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `scatter`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto t = scatter_base.clone(); t.scatter_(1, gather_idx, scatter_val);
```

### `scatter_add`

#### Signature

```cpp
Series scatter_add(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_add` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto out = scatter_base.scatter_add(1, gather_idx, scatter_val);
```

### `scatter_add_`

#### Signature

```cpp
Series& scatter_add_(self, std::int64_t, const Series&, const Series&)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place variant of `scatter_add`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.

#### Examples

```cpp
auto t = scatter_base.clone(); t.scatter_add_(1, gather_idx, scatter_val);
```

### `scatter_reduce_amax`

#### Signature

```cpp
Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_amax`

#### Signature

```cpp
Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.

#### See also

- `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true, -1e9);
```

### `scatter_reduce_amin`

#### Signature

```cpp
Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_amin`

#### Signature

```cpp
Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.

#### See also

- `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true, 1e9);
```

### `scatter_reduce_sum`

#### Signature

```cpp
Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.

#### See also

- `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true);
```

### `scatter_reduce_sum`

#### Signature

```cpp
Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

`arg3` : `const Series&`
    Required method argument.

`arg4` : `bool`
    Required method argument.

`initial_scalar` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.

#### See also

- `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool)`

#### Examples

```cpp
auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true, 0.0);
```

### `select`

#### Signature

```cpp
Series select(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto out = mm.select(0, 1);
```

### `sigmoid`

#### Signature

```cpp
Series sigmoid(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sigmoid` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.sigmoid();
```

### `sin`

#### Signature

```cpp
Series sin(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sin` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.sin();
```

### `sinh`

#### Signature

```cpp
Series sinh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sinh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.sinh();
```

### `skewness`

#### Signature

```cpp
Series skewness(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `skewness` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.skewness();
```

### `sqrt`

#### Signature

```cpp
Series sqrt(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sqrt` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = pos.sqrt();
```

### `squeeze`

#### Signature

```cpp
Series squeeze(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. No additional arguments.

#### See also

- `Series squeeze(self, std::int64_t)`

#### Examples

```cpp
auto out = x.unsqueeze(0).squeeze();
```

### `squeeze`

#### Signature

```cpp
Series squeeze(self, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.

#### See also

- `Series squeeze(self)`

#### Examples

```cpp
auto out = x.unsqueeze(0).squeeze(0);
```

### `standardize`

#### Signature

```cpp
Series standardize(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `standardize` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.standardize();
```

### `std`

#### Signature

```cpp
Series std(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto out = x.std(false);
```

### `sub`

#### Signature

```cpp
Series sub(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sub` computation to `self`. Arguments: `const Series&`.

#### See also

- `Series sub(self, double)`

#### Examples

```cpp
auto out = x.sub(y);
```

### `sub`

#### Signature

```cpp
Series sub(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `sub` computation to `self`. Arguments: `double`.

#### See also

- `Series sub(self, const Series&)`

#### Examples

```cpp
auto out = x.sub(1.5);
```

### `sum`

#### Signature

```cpp
Series sum(self, std::int64_t, bool)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.

#### Examples

```cpp
auto out = mm.sum(1, false);
```

### `take`

#### Signature

```cpp
Series take(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = gather_src.take(flat_idx);
```

### `tan`

#### Signature

```cpp
Series tan(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `tan` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = unit.tan();
```

### `tanh`

#### Signature

```cpp
Series tanh(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `tanh` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.tanh();
```

### `transpose`

#### Signature

```cpp
Series transpose(self, std::int64_t, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

`arg2` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.

#### Examples

```cpp
auto out = m.transpose(0, 1);
```

### `trunc`

#### Signature

```cpp
Series trunc(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `trunc` computation to `self`. No additional arguments.

#### Examples

```cpp
auto out = x.div(2.0).trunc();
```

### `unique`

#### Signature

```cpp
Series unique(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto out = i.unique(true);
```

### `unsqueeze`

#### Signature

```cpp
Series unsqueeze(self, std::int64_t)
```

#### Parameters

`arg1` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.

#### Examples

```cpp
auto out = x.unsqueeze(0);
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. No additional arguments.

#### See also

- `Series unwrap(self, double period)`
- `Series unwrap(self, double period, std::int64_t dim)`

#### Examples

```cpp
auto out = x.unwrap();
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self, double period)
```

#### Parameters

`period` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. Arguments: `double period`.

#### See also

- `Series unwrap(self)`
- `Series unwrap(self, double period, std::int64_t dim)`

#### Examples

```cpp
auto out = x.unwrap(3.141592653589793);
```

### `unwrap`

#### Signature

```cpp
Series unwrap(self, double period, std::int64_t dim)
```

#### Parameters

`period` : `double`
    Required method argument.

`dim` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `unwrap` computation to `self`. Arguments: `double period, std::int64_t dim`.

#### See also

- `Series unwrap(self)`
- `Series unwrap(self, double period)`

#### Examples

```cpp
auto out = mm.unwrap(3.141592653589793, 1);
```

### `var`

#### Signature

```cpp
Series var(self, bool)
```

#### Parameters

`arg1` : `bool`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. Arguments: `bool`.

#### Examples

```cpp
auto out = x.var(false);
```

### `vdot`

#### Signature

```cpp
Series vdot(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Applies `vdot` computation to `self`. Arguments: `const Series&`.

#### Examples

```cpp
auto out = x.vdot(y);
```

### `view`

#### Signature

```cpp
Series view(self, std::span<const std::int64_t>)
```

#### Parameters

`arg1` : `std::span<const std::int64_t>`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.

#### Examples

```cpp
auto out = m.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### `view_as`

#### Signature

```cpp
Series view_as(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const Series& other`.

#### Examples

```cpp
auto out = pluss::make_series(std::vector<double>{0,1,2,3,4,5}, {6}).view_as(pluss::Series::ones({3,2}, pluss::ScalarType::Float64));
```

### `vsplit`

#### Signature

```cpp
Series vsplit(self, const std::vector<std::int64_t>& split_points)
```

#### Parameters

`split_points` : `const std::vector<std::int64_t>&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& split_points`.

#### See also

- `Series vsplit(self, std::int64_t sections)`

#### Examples

```cpp
auto out = pluss::make_series(std::vector<double>{0,1,2,3,4,5,6,7,8,9,10,11}, {6,2}).vsplit(std::vector<std::int64_t>{2,4});
```

### `vsplit`

#### Signature

```cpp
Series vsplit(self, std::int64_t sections)
```

#### Parameters

`sections` : `std::int64_t`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t sections`.

#### See also

- `Series vsplit(self, const std::vector<std::int64_t>& split_points)`

#### Examples

```cpp
auto out = pluss::make_series(std::vector<double>{0,1,2,3,4,5,6,7,8,9,10,11}, {6,2}).vsplit(2);
```

### `where`

#### Signature

```cpp
Series where(self, const Series&, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, const Series&`.

#### See also

- `Series where(self, const Series&, double)`

#### Examples

```cpp
auto out = x.where(cond, y);
```

### `where`

#### Signature

```cpp
Series where(self, const Series&, double)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

`arg2` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, double`.

#### See also

- `Series where(self, const Series&, const Series&)`

#### Examples

```cpp
auto out = x.where(cond, 42.0);
```

### `xlogy`

#### Signature

```cpp
Series xlogy(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `self * log(other)` with scalar/series overloads. Arguments: `const Series& other`.

#### See also

- `Series xlogy(self, double scalar)`

#### Examples

```cpp
auto out = x.abs().xlogy(y.abs().add(1.0));
```

### `xlogy`

#### Signature

```cpp
Series xlogy(self, double scalar)
```

#### Parameters

`scalar` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes `self * log(other)` with scalar/series overloads. Arguments: `double scalar`.

#### See also

- `Series xlogy(self, const Series& other)`

#### Examples

```cpp
auto out = x.abs().xlogy(2.0);
```

### `xlogy_`

#### Signature

```cpp
Series& xlogy_(self, const Series& other)
```

#### Parameters

`other` : `const Series&`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place form of `xlogy`; mutates `self`. Arguments: `const Series& other`.

#### See also

- `Series& xlogy_(self, double scalar)`

#### Examples

```cpp
auto t = x.abs().clone(); t.xlogy_(y.abs().add(1.0));
```

### `xlogy_`

#### Signature

```cpp
Series& xlogy_(self, double scalar)
```

#### Parameters

`scalar` : `double`
    Required method argument.

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place form of `xlogy`; mutates `self`. Arguments: `double scalar`.

#### See also

- `Series& xlogy_(self, const Series& other)`

#### Examples

```cpp
auto t = x.abs().clone(); t.xlogy_(2.0);
```

### `zero_`

#### Signature

```cpp
Series& zero_(self)
```

#### Parameters

`None`

#### Returns

`Series&`
    Return value produced by this method call.

#### Notes

In-place reset: fills `self` with zeros and returns `self`. No additional arguments.

#### Examples

```cpp
auto t = x.clone(); t.zero_();
```
### `zeta`

#### Signature

```cpp
Series zeta(self)
```

#### Parameters

`None`

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. No additional arguments.

#### See also

- `Series zeta(self, double)`
- `Series zeta(self, const Series&)`

#### Examples

```cpp
auto out = pos.zeta();
```

### `zeta`

#### Signature

```cpp
Series zeta(self, const Series&)
```

#### Parameters

`arg1` : `const Series&`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `const Series&`.

#### See also

- `Series zeta(self)`
- `Series zeta(self, double)`

#### Examples

```cpp
auto out = pos.zeta(y.abs().add(1.0));
```

### `zeta`

#### Signature

```cpp
Series zeta(self, double)
```

#### Parameters

`arg1` : `double`
    Required method argument.

#### Returns

`Series`
    Return value produced by this method call.

#### Notes

Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `double`.

#### See also

- `Series zeta(self)`
- `Series zeta(self, const Series&)`

#### Examples

```cpp
auto out = pos.zeta(1.0);
```
