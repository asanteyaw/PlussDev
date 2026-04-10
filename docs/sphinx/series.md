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
- For deeper, output-backed walkthroughs of `traverse`, `stat`, and `linearalg`, see `series_query_user_guide`.

## Setup Once, Reuse Everywhere

```cpp
// Reusable fixtures used in examples below.
auto x = pluss::make_series({-3, -2, -1, 0, 1, 2, 3});
auto y = pluss::make_series({ 3,  2,  1, 0, -1, -2, -3});
auto m = pluss::make_series({
  {1,2,3},
  {4,5,6}
});
auto mm = pluss::make_series({
  {1,2,3},
  {4,5,6},
  {7,8,9}
});

// Index/mask helpers for gather/scatter/masked ops.
auto idx23 = pluss::make_series({
  {2,1,0},
  {0,1,2}
});
auto flat_idx = pluss::make_series({0,3,5});
auto mask23 = pluss::ones({2,3}, pluss::ScalarType::Bool);
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
auto blk = mm.block(1, 2, 0, 2); // 2D block: rows [1..2], cols [0..1] -> [[4,5],[7,8]]
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

auto base = pluss::zeros({2,3}, pluss::ScalarType::Float64);
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
auto pdf = x.abs().stat.pdf(0.0, 1.5);
auto logpdf = x.abs().stat.logpdf(0.0, 1.5);
auto logcdf = x.abs().stat.logcdf(0.0, 1.5);
auto cdf = x.abs().stat.cdf(0.0, 1.5);
auto sk = x.stat.skewness();
auto ku = x.stat.kurtosis();

// moving window helpers
auto ms = x.stat.movsum(3);
auto mmn = x.stat.movmean(3);
auto mstd = x.stat.movstd(3);
```
