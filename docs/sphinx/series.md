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
auto x = pluss::value(std::vector<double>{-3, -2, -1, 0, 1, 2, 3}, {7});
auto y = pluss::value(std::vector<double>{ 3,  2,  1, 0,-1,-2,-3}, {7});
auto m = pluss::value(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto mm = pluss::value(std::vector<double>{1,2,3,4,5,6,7,8,9}, {3,3});

// Index/mask helpers for gather/scatter/masked ops.
auto idx23 = pluss::make_series(std::vector<std::int64_t>{2,1,0,0,1,2}, {2,3});
auto flat_idx = pluss::make_series(std::vector<std::int64_t>{0,3,5}, {3});
auto mask23 = pluss::Series::ones({2,3}, pluss::ScalarType::Bool);
```

## Construction: Real Ways You Will Use

```cpp
// Scalar / literal data
auto s0 = pluss::value(3.14);
auto s1 = pluss::value(true);
auto s2 = pluss::value({1.0, 2.0, 3.0});
auto s3 = pluss::value({{1.0, 2.0}, {3.0, 4.0}});

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
auto x = pluss::value(std::vector<double>{-3,-2,-1,0,1,2,3}, {7});
auto y = pluss::value(std::vector<double>{3,2,1,0,-1,-2,-3}, {7});
auto pos = pluss::value(std::vector<double>{0.2,0.5,1.0,2.0,3.0,4.0,5.0}, {7});
auto unit = pluss::value(std::vector<double>{-0.9,-0.5,-0.1,0.0,0.1,0.5,0.9}, {7});
auto m = pluss::value(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto n = pluss::value(std::vector<double>{6,5,4,3,2,1}, {2,3});
auto mm = pluss::value(std::vector<double>{1,2,3,4,5,6,7,8,9}, {3,3});
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

- Signature:
  `Series view(self, std::span<const std::int64_t>)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.
- Example:
  `auto out = m.view(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));`
- Signature:
  `Series reshape(self, std::span<const std::int64_t>)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.
- Example:
  `auto out = m.reshape(std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));`
- Signature:
  `Series transpose(self, std::int64_t, std::int64_t)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.
- Example:
  `auto out = m.transpose(0, 1);`
- Signature:
  `Series permute(self, std::span<const std::int64_t>)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::span<const std::int64_t>`.
- Example:
  `auto out = mm.permute(std::span<const std::int64_t>(perm_10.data(), perm_10.size()));`
- Signature:
  `Series unsqueeze(self, std::int64_t)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.
- Example:
  `auto out = x.unsqueeze(0);`
- Signature:
  `Series squeeze(self)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. No additional arguments.
- Example:
  `auto out = x.unsqueeze(0).squeeze();`
- Signature:
  `Series squeeze(self, std::int64_t)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t`.
- Example:
  `auto out = x.unsqueeze(0).squeeze(0);`
- Signature:
  `Series flatten(self, std::int64_t, std::int64_t)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.
- Example:
  `auto out = mm.flatten(0, 1);`
- Signature:
  `Series select(self, std::int64_t, std::int64_t)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t, std::int64_t`.
- Example:
  `auto out = mm.select(0, 1);`
- Signature:
  `Series take(self, const Series&)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const Series&`.
- Example:
  `auto out = gather_src.take(flat_idx);`
- Signature:
  `Series nonzero(self)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. No additional arguments.
- Example:
  `auto out = cond.nonzero();`
- Signature:
  `Series add(self, double)`
- Explanation:
  Applies `add` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.add(1.5);`
- Signature:
  `Series sub(self, double)`
- Explanation:
  Applies `sub` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.sub(1.5);`
- Signature:
  `Series mul(self, double)`
- Explanation:
  Applies `mul` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.mul(2.0);`
- Signature:
  `Series div(self, double)`
- Explanation:
  Applies `div` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.div(2.0);`
- Signature:
  `Series pow(self, double)`
- Explanation:
  Applies `pow` computation to `self`. Arguments: `double`.
- Example:
  `auto out = pos.pow(2.0);`
- Signature:
  `Series maximum(self, double)`
- Explanation:
  Applies `maximum` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.maximum(0.0);`
- Signature:
  `Series minimum(self, double)`
- Explanation:
  Applies `minimum` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.minimum(0.0);`
- Signature:
  `Series eq(self, double)`
- Explanation:
  Applies `eq` to `self`. Arguments: `double`.
- Example:
  `auto out = x.eq(0.0);`
- Signature:
  `Series ne(self, double)`
- Explanation:
  Applies `ne` to `self`. Arguments: `double`.
- Example:
  `auto out = x.ne(0.0);`
- Signature:
  `Series lt(self, double)`
- Explanation:
  Applies `lt` to `self`. Arguments: `double`.
- Example:
  `auto out = x.lt(0.0);`
- Signature:
  `Series le(self, double)`
- Explanation:
  Applies `le` to `self`. Arguments: `double`.
- Example:
  `auto out = x.le(0.0);`
- Signature:
  `Series gt(self, double)`
- Explanation:
  Applies `gt` to `self`. Arguments: `double`.
- Example:
  `auto out = x.gt(0.0);`
- Signature:
  `Series ge(self, double)`
- Explanation:
  Applies `ge` to `self`. Arguments: `double`.
- Example:
  `auto out = x.ge(0.0);`
- Signature:
  `Series add(self, const Series&)`
- Explanation:
  Applies `add` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.add(y);`
- Signature:
  `Series sub(self, const Series&)`
- Explanation:
  Applies `sub` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.sub(y);`
- Signature:
  `Series mul(self, const Series&)`
- Explanation:
  Applies `mul` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.mul(y);`
- Signature:
  `Series div(self, const Series&)`
- Explanation:
  Applies `div` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = pos.div(pos.add(1.0));`
- Signature:
  `Series pow(self, const Series&)`
- Explanation:
  Applies `pow` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = pos.pow(pos.mul(0.0).add(2.0));`
- Signature:
  `Series bitwise_and(self, const Series&)`
- Explanation:
  Applies `bitwise_and` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = bi.bitwise_and(bi.add(1));`
- Signature:
  `Series bitwise_or(self, const Series&)`
- Explanation:
  Applies `bitwise_or` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = bi.bitwise_or(bi.add(1));`
- Signature:
  `Series bitwise_xor(self, const Series&)`
- Explanation:
  Applies `bitwise_xor` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = bi.bitwise_xor(bi.add(1));`
- Signature:
  `Series bitwise_not(self)`
- Explanation:
  Applies `bitwise_not` computation to `self`. No additional arguments.
- Example:
  `auto out = bi.bitwise_not();`
- Signature:
  `Series logical_and(self, const Series&)`
- Explanation:
  Applies `logical_and` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.gt(0.0).logical_and(x.lt(3.0));`
- Signature:
  `Series logical_or(self, const Series&)`
- Explanation:
  Applies `logical_or` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.gt(0.0).logical_or(x.lt(-2.0));`
- Signature:
  `Series logical_xor(self, const Series&)`
- Explanation:
  Applies `logical_xor` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.gt(0.0).logical_xor(x.lt(0.0));`
- Signature:
  `Series logical_not(self)`
- Explanation:
  Applies `logical_not` computation to `self`. No additional arguments.
- Example:
  `auto out = x.gt(0.0).logical_not();`
- Signature:
  `Series exp(self)`
- Explanation:
  Applies `exp` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.exp();`
- Signature:
  `Series log(self)`
- Explanation:
  Applies `log` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.log();`
- Signature:
  `Series sqrt(self)`
- Explanation:
  Applies `sqrt` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.sqrt();`
- Signature:
  `Series sin(self)`
- Explanation:
  Applies `sin` computation to `self`. No additional arguments.
- Example:
  `auto out = x.sin();`
- Signature:
  `Series cos(self)`
- Explanation:
  Applies `cos` computation to `self`. No additional arguments.
- Example:
  `auto out = x.cos();`
- Signature:
  `Series tan(self)`
- Explanation:
  Applies `tan` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.tan();`
- Signature:
  `Series asin(self)`
- Explanation:
  Applies `asin` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.asin();`
- Signature:
  `Series acos(self)`
- Explanation:
  Applies `acos` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.acos();`
- Signature:
  `Series atan(self)`
- Explanation:
  Applies `atan` computation to `self`. No additional arguments.
- Example:
  `auto out = x.atan();`
- Signature:
  `Series sinh(self)`
- Explanation:
  Applies `sinh` computation to `self`. No additional arguments.
- Example:
  `auto out = x.sinh();`
- Signature:
  `Series cosh(self)`
- Explanation:
  Applies `cosh` computation to `self`. No additional arguments.
- Example:
  `auto out = x.cosh();`
- Signature:
  `Series tanh(self)`
- Explanation:
  Applies `tanh` computation to `self`. No additional arguments.
- Example:
  `auto out = x.tanh();`
- Signature:
  `Series floor(self)`
- Explanation:
  Applies `floor` computation to `self`. No additional arguments.
- Example:
  `auto out = x.div(2.0).floor();`
- Signature:
  `Series ceil(self)`
- Explanation:
  Applies `ceil` computation to `self`. No additional arguments.
- Example:
  `auto out = x.div(2.0).ceil();`
- Signature:
  `Series round(self)`
- Explanation:
  Applies `round` computation to `self`. No additional arguments.
- Example:
  `auto out = x.div(2.0).round();`
- Signature:
  `Series trunc(self)`
- Explanation:
  Applies `trunc` computation to `self`. No additional arguments.
- Example:
  `auto out = x.div(2.0).trunc();`
- Signature:
  `Series reciprocal(self)`
- Explanation:
  Applies `reciprocal` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.reciprocal();`
- Signature:
  `Series sigmoid(self)`
- Explanation:
  Applies `sigmoid` computation to `self`. No additional arguments.
- Example:
  `auto out = x.sigmoid();`
- Signature:
  `Series relu(self)`
- Explanation:
  Applies `relu` computation to `self`. No additional arguments.
- Example:
  `auto out = x.relu();`
- Signature:
  `Series log1p(self)`
- Explanation:
  Applies `log1p` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.log1p();`
- Signature:
  `Series expm1(self)`
- Explanation:
  Applies `expm1` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.expm1();`
- Signature:
  `Series erf(self)`
- Explanation:
  Applies `erf` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.erf();`
- Signature:
  `Series erfc(self)`
- Explanation:
  Applies `erfc` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.erfc();`
- Signature:
  `Series lgamma(self)`
- Explanation:
  Applies `lgamma` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.lgamma();`
- Signature:
  `Series asinh(self)`
- Explanation:
  Applies `asinh` computation to `self`. No additional arguments.
- Example:
  `auto out = x.asinh();`
- Signature:
  `Series acosh(self)`
- Explanation:
  Applies `acosh` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.add(1.0).acosh();`
- Signature:
  `Series atanh(self)`
- Explanation:
  Applies `atanh` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.atanh();`
- Signature:
  `Series isnan(self)`
- Explanation:
  Applies `isnan` to `self`. No additional arguments.
- Example:
  `auto out = x.isnan();`
- Signature:
  `Series isinf(self)`
- Explanation:
  Applies `isinf` to `self`. No additional arguments.
- Example:
  `auto out = x.isinf();`
- Signature:
  `Series isfinite(self)`
- Explanation:
  Applies `isfinite` to `self`. No additional arguments.
- Example:
  `auto out = x.isfinite();`
- Signature:
  `Series sum(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.sum(1, false);`
- Signature:
  `Series mean(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.mean(1, true);`
- Signature:
  `Series amax(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.amax(1, false);`
- Signature:
  `Series amin(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.amin(1, false);`
- Signature:
  `Series all(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.gt(0.0).all(1, false);`
- Signature:
  `Series any(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.gt(7.0).any(1, false);`
- Signature:
  `Series mean(self)`
- Explanation:
  Computes a reduction or summary from `self`. No additional arguments.
- Example:
  `auto out = mm.mean();`
- Signature:
  `Series norm(self, double)`
- Explanation:
  Applies `norm` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.norm(2.0);`
- Signature:
  `Series var(self, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `bool`.
- Example:
  `auto out = x.var(false);`
- Signature:
  `Series std(self, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `bool`.
- Example:
  `auto out = x.std(false);`
- Signature:
  `Series cumsum(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.cumsum(1, false);`
- Signature:
  `Series cumprod(self, std::int64_t, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto out = mm.cumprod(1, false);`
- Signature:
  `Series where(self, const Series&, const Series&)`
- Explanation:
  Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, const Series&`.
- Example:
  `auto out = x.where(cond, y);`
- Signature:
  `Series where(self, const Series&, double)`
- Explanation:
  Selects values from `self` or fallback inputs based on a boolean condition/mask. Arguments: `const Series&, double`.
- Example:
  `auto out = x.where(cond, 42.0);`
- Signature:
  `Series clamp_min(self, double)`
- Explanation:
  Applies `clamp_min` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.clamp_min(0.0);`
- Signature:
  `Series clamp_max(self, double)`
- Explanation:
  Applies `clamp_max` computation to `self`. Arguments: `double`.
- Example:
  `auto out = x.clamp_max(0.0);`
- Signature:
  `Series clamp_min(self, const Series&)`
- Explanation:
  Applies `clamp_min` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.clamp_min(y);`
- Signature:
  `Series clamp_max(self, const Series&)`
- Explanation:
  Applies `clamp_max` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.clamp_max(y);`
- Signature:
  `Series nan_to_num(self, double, double, double)`
- Explanation:
  Applies `nan_to_num` computation to `self`. Arguments: `double, double, double`.
- Example:
  `auto out = x.nan_to_num(0.0, 1e9, -1e9);`
- Signature:
  `Series masked_select(self, const Series&)`
- Explanation:
  Applies `masked_select` to `self`. Arguments: `const Series&`.
- Example:
  `auto out = m.masked_select(mask);`
- Signature:
  `Series masked_scatter(self, const Series&, const Series&)`
- Explanation:
  Applies `masked_scatter` to `self`. Arguments: `const Series&, const Series&`.
- Example:
  `auto out = m.masked_scatter(mask, n);`
- Signature:
  `Series gather(self, std::int64_t, const Series&)`
- Explanation:
  Applies `gather` computation to `self`. Arguments: `std::int64_t, const Series&`.
- Example:
  `auto out = gather_src.gather(1, gather_idx);`
- Signature:
  `Series scatter(self, std::int64_t, const Series&, const Series&)`
- Explanation:
  Applies `scatter` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.
- Example:
  `auto out = scatter_base.scatter(1, gather_idx, scatter_val);`
- Signature:
  `Series scatter_add(self, std::int64_t, const Series&, const Series&)`
- Explanation:
  Applies `scatter_add` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&`.
- Example:
  `auto out = scatter_base.scatter_add(1, gather_idx, scatter_val);`
- Signature:
  `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool)`
- Explanation:
  Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.
- Example:
  `auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true);`
- Signature:
  `Series scatter_reduce_sum(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`
- Explanation:
  Applies `scatter_reduce_sum` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.
- Example:
  `auto out = scatter_base.scatter_reduce_sum(1, gather_idx, scatter_val, true, 0.0);`
- Signature:
  `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool)`
- Explanation:
  Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.
- Example:
  `auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true);`
- Signature:
  `Series scatter_reduce_amax(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`
- Explanation:
  Applies `scatter_reduce_amax` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.
- Example:
  `auto out = scatter_base.scatter_reduce_amax(1, gather_idx, scatter_val, true, -1e9);`
- Signature:
  `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool)`
- Explanation:
  Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool`.
- Example:
  `auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true);`
- Signature:
  `Series scatter_reduce_amin(self, std::int64_t, const Series&, const Series&, bool, double initial_scalar)`
- Explanation:
  Applies `scatter_reduce_amin` computation to `self`. Arguments: `std::int64_t, const Series&, const Series&, bool, double initial_scalar`.
- Example:
  `auto out = scatter_base.scatter_reduce_amin(1, gather_idx, scatter_val, true, 1e9);`
- Signature:
  `Series unique(self, bool)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `bool`.
- Example:
  `auto out = i.unique(true);`
- Signature:
  `Series bincount(self, std::int64_t minlength)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t minlength`.
- Example:
  `auto out = i.sub(1.0).to(ScalarType::Int64).bincount(0);`
- Signature:
  `Series histc(self, std::int64_t, double, double)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, double, double`.
- Example:
  `auto out = pos.histc(5, 0.0, 5.0);`
- Signature:
  `Series& cumsum_(self, std::int64_t, bool)`
- Explanation:
  In-place variant of `cumsum`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto t = x.clone(); t.cumsum_(1, false);`
- Signature:
  `Series& cumprod_(self, std::int64_t, bool)`
- Explanation:
  In-place variant of `cumprod`; it mutates `self` and returns `self`. Arguments: `std::int64_t, bool`.
- Example:
  `auto t = x.clone(); t.cumprod_(1, false);`
- Signature:
  `Series& scatter_(self, std::int64_t, const Series&, const Series&)`
- Explanation:
  In-place variant of `scatter`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.
- Example:
  `auto t = scatter_base.clone(); t.scatter_(1, gather_idx, scatter_val);`
- Signature:
  `Series& scatter_add_(self, std::int64_t, const Series&, const Series&)`
- Explanation:
  In-place variant of `scatter_add`; it mutates `self` and returns `self`. Arguments: `std::int64_t, const Series&, const Series&`.
- Example:
  `auto t = scatter_base.clone(); t.scatter_add_(1, gather_idx, scatter_val);`
- Signature:
  `Series& masked_scatter_(self, const Series&, const Series&)`
- Explanation:
  In-place variant of `masked_scatter`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&`.
- Example:
  `auto t = m.clone(); t.masked_scatter_(mask, n);`
- Signature:
  `Series& put_(self, const Series&, const Series&, bool)`
- Explanation:
  In-place variant of `put`; it mutates `self` and returns `self`. Arguments: `const Series&, const Series&, bool`.
- Example:
  `auto t = gather_src.clone(); t.put_(flat_idx, flat_src, false);`
- Signature:
  `Series pdf(self)`
- Explanation:
  Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. No additional arguments.
- Example:
  `auto out = pos.pdf();`
- Signature:
  `Series pdf(self, double stddev)`
- Explanation:
  Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `double stddev`.
- Example:
  `auto out = pos.pdf(1.5);`
- Signature:
  `Series pdf(self, double mean, double stddev)`
- Explanation:
  Computes Gaussian probability density values for `self` using the provided mean/stddev parameters. Arguments: `double mean, double stddev`.
- Example:
  `auto out = pos.pdf(0.0, 1.5);`
- Signature:
  `Series cdf(self)`
- Explanation:
  Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. No additional arguments.
- Example:
  `auto out = pos.cdf();`
- Signature:
  `Series cdf(self, double stddev)`
- Explanation:
  Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `double stddev`.
- Example:
  `auto out = pos.cdf(1.5);`
- Signature:
  `Series cdf(self, double mean, double stddev)`
- Explanation:
  Computes Gaussian cumulative distribution values for `self` using the provided mean/stddev parameters. Arguments: `double mean, double stddev`.
- Example:
  `auto out = pos.cdf(0.0, 1.5);`
- Signature:
  `Series dct(self)`
- Explanation:
  Applies `dct` computation to `self`. No additional arguments.
- Example:
  `auto out = x.dct();`
- Signature:
  `Series dct(self, std::int64_t dim)`
- Explanation:
  Applies `dct` computation to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = x.dct(0);`
- Signature:
  `Series idct(self)`
- Explanation:
  Applies `idct` computation to `self`. No additional arguments.
- Example:
  `auto out = x.dct().idct();`
- Signature:
  `Series idct(self, std::int64_t dim)`
- Explanation:
  Applies `idct` computation to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = x.idct(0);`
- Signature:
  `Series dst(self)`
- Explanation:
  Applies `dst` computation to `self`. No additional arguments.
- Example:
  `auto out = x.dst();`
- Signature:
  `Series dst(self, std::int64_t dim)`
- Explanation:
  Applies `dst` computation to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = x.dst(0);`
- Signature:
  `Series idst(self)`
- Explanation:
  Applies `idst` computation to `self`. No additional arguments.
- Example:
  `auto out = x.dst().idst();`
- Signature:
  `Series idst(self, std::int64_t dim)`
- Explanation:
  Applies `idst` computation to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = x.idst(0);`
- Signature:
  `Series hilbert(self)`
- Explanation:
  Applies `hilbert` computation to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert();`
- Signature:
  `Series hilbert(self, std::int64_t dim)`
- Explanation:
  Applies `hilbert` computation to `self`. Arguments: `std::int64_t dim`.
- Example:
  `auto out = x.hilbert(0);`
- Signature:
  `Series histcounts(self, std::int64_t, double, double)`
- Explanation:
  Computes a reduction or summary from `self`. Arguments: `std::int64_t, double, double`.
- Example:
  `auto out = pos.histcounts(5, 0.0, 5.0);`
- Signature:
  `Series skewness(self)`
- Explanation:
  Applies `skewness` computation to `self`. No additional arguments.
- Example:
  `auto out = x.skewness();`
- Signature:
  `Series kurtosis(self)`
- Explanation:
  Applies `kurtosis` computation to `self`. No additional arguments.
- Example:
  `auto out = x.kurtosis();`
- Signature:
  `Series nanvar(self)`
- Explanation:
  Applies `nanvar` computation to `self`. No additional arguments.
- Example:
  `auto out = x.nanvar();`
- Signature:
  `Series nanstd(self)`
- Explanation:
  Applies `nanstd` computation to `self`. No additional arguments.
- Example:
  `auto out = x.nanstd();`
- Signature:
  `Series ediff1d(self)`
- Explanation:
  Applies `ediff1d` computation to `self`. No additional arguments.
- Example:
  `auto out = x.ediff1d();`
- Signature:
  `Series movsum(self)`
- Explanation:
  Computes moving-window sums along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movsum();`
- Signature:
  `Series movsum(self, std::int64_t window)`
- Explanation:
  Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movsum(3);`
- Signature:
  `Series movsum(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window sums along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movsum(2, 1);`
- Signature:
  `Series movmean(self)`
- Explanation:
  Computes moving-window means along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movmean();`
- Signature:
  `Series movmean(self, std::int64_t window)`
- Explanation:
  Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movmean(3);`
- Signature:
  `Series movmean(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window means along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movmean(2, 1);`
- Signature:
  `Series movmax(self)`
- Explanation:
  Computes moving-window maxima along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movmax();`
- Signature:
  `Series movmax(self, std::int64_t window)`
- Explanation:
  Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movmax(3);`
- Signature:
  `Series movmax(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window maxima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movmax(2, 1);`
- Signature:
  `Series movmin(self)`
- Explanation:
  Computes moving-window minima along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movmin();`
- Signature:
  `Series movmin(self, std::int64_t window)`
- Explanation:
  Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movmin(3);`
- Signature:
  `Series movmin(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window minima along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movmin(2, 1);`
- Signature:
  `Series movstd(self)`
- Explanation:
  Computes moving-window standard deviation along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movstd();`
- Signature:
  `Series movstd(self, std::int64_t window)`
- Explanation:
  Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movstd(3);`
- Signature:
  `Series movstd(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window standard deviation along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movstd(2, 1);`
- Signature:
  `Series movvar(self)`
- Explanation:
  Computes moving-window variance along a dimension (default is last dimension). No additional arguments.
- Example:
  `auto out = x.movvar();`
- Signature:
  `Series movvar(self, std::int64_t window)`
- Explanation:
  Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window`.
- Example:
  `auto out = x.movvar(3);`
- Signature:
  `Series movvar(self, std::int64_t window, std::int64_t dim)`
- Explanation:
  Computes moving-window variance along a dimension (default is last dimension). Arguments: `std::int64_t window, std::int64_t dim`.
- Example:
  `auto out = mm.movvar(2, 1);`
- Signature:
  `Series pdist2(self, const Series&)`
- Explanation:
  Applies `pdist2` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = m.pdist2(n);`
- Signature:
  `Series cdist(self, const Series&)`
- Explanation:
  Applies `cdist` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = m.cdist(n);`
- Signature:
  `Series normr(self)`
- Explanation:
  Applies `normr` computation to `self`. No additional arguments.
- Example:
  `auto out = m.normr();`
- Signature:
  `Series normc(self)`
- Explanation:
  Applies `normc` computation to `self`. No additional arguments.
- Example:
  `auto out = m.normc();`
- Signature:
  `Series standardize(self)`
- Explanation:
  Applies `standardize` computation to `self`. No additional arguments.
- Example:
  `auto out = x.standardize();`
- Signature:
  `Series center(self)`
- Explanation:
  Applies `center` computation to `self`. No additional arguments.
- Example:
  `auto out = x.center();`
- Signature:
  `Series besselj(self)`
- Explanation:
  Applies `besselj` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.besselj();`
- Signature:
  `Series bessely(self)`
- Explanation:
  Applies `bessely` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.bessely();`
- Signature:
  `Series besseli(self)`
- Explanation:
  Applies `besseli` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.besseli();`
- Signature:
  `Series besselk(self)`
- Explanation:
  Applies `besselk` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.besselk();`
- Signature:
  `Series beta(self, double)`
- Explanation:
  Applies `beta` computation to `self`. Arguments: `double`.
- Example:
  `auto out = pos.beta(2.0);`
- Signature:
  `Series betaln(self, double)`
- Explanation:
  Applies `betaln` computation to `self`. Arguments: `double`.
- Example:
  `auto out = pos.betaln(2.0);`
- Signature:
  `Series erfcinv(self)`
- Explanation:
  Applies `erfcinv` computation to `self`. No additional arguments.
- Example:
  `auto out = unit.abs().erfcinv();`
- Signature:
  `Series zeta(self)`
- Explanation:
  Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. No additional arguments.
- Example:
  `auto out = pos.zeta();`
- Signature:
  `Series zeta(self, double)`
- Explanation:
  Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `double`.
- Example:
  `auto out = pos.zeta(1.0);`
- Signature:
  `Series zeta(self, const Series&)`
- Explanation:
  Computes the Hurwitz zeta-style transform for `self` with scalar/series parameter overloads. Arguments: `const Series&`.
- Example:
  `auto out = pos.zeta(y.abs().add(1.0));`
- Signature:
  `Series airy(self)`
- Explanation:
  Applies `airy` computation to `self`. No additional arguments.
- Example:
  `auto out = pos.airy();`
- Signature:
  `Series unwrap(self)`
- Explanation:
  Applies `unwrap` computation to `self`. No additional arguments.
- Example:
  `auto out = x.unwrap();`
- Signature:
  `Series unwrap(self, double period)`
- Explanation:
  Applies `unwrap` computation to `self`. Arguments: `double period`.
- Example:
  `auto out = x.unwrap(3.141592653589793);`
- Signature:
  `Series unwrap(self, double period, std::int64_t dim)`
- Explanation:
  Applies `unwrap` computation to `self`. Arguments: `double period, std::int64_t dim`.
- Example:
  `auto out = mm.unwrap(3.141592653589793, 1);`
- Signature:
  `Series cplxpair(self)`
- Explanation:
  Applies `cplxpair` computation to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert().cplxpair();`
- Signature:
  `Series vdot(self, const Series&)`
- Explanation:
  Applies `vdot` computation to `self`. Arguments: `const Series&`.
- Example:
  `auto out = x.vdot(y);`
- Signature:
  `Series real(self)`
- Explanation:
  Applies `real` to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert().real();`
- Signature:
  `Series imag(self)`
- Explanation:
  Applies `imag` to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert().imag();`
- Signature:
  `Series angle(self)`
- Explanation:
  Applies `angle` to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert().angle();`
- Signature:
  `Series fftshift(self)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. No additional arguments.
- Example:
  `auto out = mm.fftshift();`
- Signature:
  `Series fftshift(self, const std::vector<std::int64_t>& dims)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.
- Example:
  `auto out = mm.fftshift(std::vector<std::int64_t>{0,1});`
- Signature:
  `Series ifftshift(self)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. No additional arguments.
- Example:
  `auto out = mm.fftshift().ifftshift();`
- Signature:
  `Series ifftshift(self, const std::vector<std::int64_t>& dims)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& dims`.
- Example:
  `auto out = mm.ifftshift(std::vector<std::int64_t>{0,1});`
- Signature:
  `Series abs2(self)`
- Explanation:
  Applies `abs2` to `self`. No additional arguments.
- Example:
  `auto out = x.hilbert().abs2();`
- Signature:
  `Series addbmm(self, const Series& batch1, const Series& batch2)`
- Explanation:
  Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2`.
- Example:
  `auto out = pluss::Series::zeros({2,2}, pluss::ScalarType::Float64).addbmm(pluss::Series::ones({3,2,4}, pluss::ScalarType::Float64), pluss::Series::ones({3,4,2}, pluss::ScalarType::Float64));`
- Signature:
  `Series addbmm(self, const Series& batch1, const Series& batch2, double beta, double alpha)`
- Explanation:
  Computes `beta * self + alpha * batch_matmul(batch1, batch2)` using batched matrix products. Arguments: `const Series& batch1, const Series& batch2, double beta, double alpha`.
- Example:
  `auto out = pluss::Series::zeros({2,2}, pluss::ScalarType::Float64).addbmm(pluss::Series::ones({3,2,4}, pluss::ScalarType::Float64), pluss::Series::ones({3,4,2}, pluss::ScalarType::Float64), 0.5, 2.0);`
- Signature:
  `Series view_as(self, const Series& other)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const Series& other`.
- Example:
  `auto out = pluss::value(std::vector<double>{0,1,2,3,4,5}, {6}).view_as(pluss::Series::ones({3,2}, pluss::ScalarType::Float64));`
- Signature:
  `Series vsplit(self, std::int64_t sections)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `std::int64_t sections`.
- Example:
  `auto out = pluss::value(std::vector<double>{0,1,2,3,4,5,6,7,8,9,10,11}, {6,2}).vsplit(2);`
- Signature:
  `Series vsplit(self, const std::vector<std::int64_t>& split_points)`
- Explanation:
  Changes shape/indexing/view behavior for `self`. Arguments: `const std::vector<std::int64_t>& split_points`.
- Example:
  `auto out = pluss::value(std::vector<double>{0,1,2,3,4,5,6,7,8,9,10,11}, {6,2}).vsplit(std::vector<std::int64_t>{2,4});`
- Signature:
  `Series xlogy(self, const Series& other)`
- Explanation:
  Computes `self * log(other)` with scalar/series overloads. Arguments: `const Series& other`.
- Example:
  `auto out = x.abs().xlogy(y.abs().add(1.0));`
- Signature:
  `Series xlogy(self, double scalar)`
- Explanation:
  Computes `self * log(other)` with scalar/series overloads. Arguments: `double scalar`.
- Example:
  `auto out = x.abs().xlogy(2.0);`
- Signature:
  `Series& xlogy_(self, const Series& other)`
- Explanation:
  In-place form of `xlogy`; mutates `self`. Arguments: `const Series& other`.
- Example:
  `auto t = x.abs().clone(); t.xlogy_(y.abs().add(1.0));`
- Signature:
  `Series& xlogy_(self, double scalar)`
- Explanation:
  In-place form of `xlogy`; mutates `self`. Arguments: `double scalar`.
- Example:
  `auto t = x.abs().clone(); t.xlogy_(2.0);`
- Signature:
  `Series& zero_(self)`
- Explanation:
  In-place reset: fills `self` with zeros and returns `self`. No additional arguments.
- Example:
  `auto t = x.clone(); t.zero_();`
