# Series Query, Stats, and Linear Algebra Guide

This page is task-first and example-heavy, following the same practical pattern used by PyTorch docs: start from a concrete operation, run it, inspect the result, then scale to related patterns.

## Surface map

- Core `Series` methods: shape ops, indexing, transforms, math/reductions.
- `series.stat.*`: statistical/distribution/moving-window helpers.
- `series.linearalg.*`: matrix/vector routines.
- `series.traverse.*`: SQL-like match/select workflows over tensor coordinates.

`Series` does not expose a `stats()` or `linear_alg()` name. The current public names are `stat` and `linearalg`.

## One setup block

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

using pluss::Slicing::SliceAll;
using pluss::Slicing::SliceFrom;
using pluss::Slicing::SliceRange;
```

## 1) Traverse (query engine)

### 1.1 View-only traversal

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0},
  {30.0, 31.0, 32.0, 33.0}
});

std::vector<pluss::codem::ExtractSpec> specs{
  SliceAll(),
  SliceRange(0, 4, 2)
};

auto view = x.traverse_slice(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(specs));
```

Output (from `plussqit_codem_traverse_demo`):

```text
traverse_slice(all, 0:4:2):
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[10, 12],
  [20, 22],
  [30, 32]]
```

### 1.2 Find matches and select values

```cpp
auto m = x.traverse.find_ge(22.0);
auto coords = m.coords();
auto linear = m.linear_indices();
auto vals = m.select_from(x);
```

Output:

```text
traverse.find_ge(22).coords:
pluss::Series(shape=[6,2], strides=[2,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=[[1, 2],
  [1, 3],
  [2, 0],
  [2, 1],
  [2, 2],
  [2, 3]]

traverse.find_ge(22).linear_indices:
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[6, 7, 8, 9, 10, 11]

traverse.find_ge(22).select_from(x):
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[22, 23, 30, 31, 32, 33]
```

### 1.3 Mask-based query and first match

```cpp
auto mask = x.ge(21.0).logical_and(x.le(31.0));
auto all_hits = x.traverse.find_all(mask);
auto first_hit = x.traverse.find_first(mask);
```

Output:

```text
traverse.find_all(mask[21..31]).coords:
pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[1, 1],
  [1, 2],
  [1, 3],
  [2, 0],
  [2, 1]]

traverse.find_first(mask[21..31]).coords:
pluss::Series(shape=[1,2], strides=[2,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1, 1]]
```

### 1.4 Scoped query (`on` selector) + `select_from`

This is the pattern you can use when one series defines the match and another aligned series provides values.

```cpp
std::vector<pluss::codem::ExtractSpec> on_specs{
  SliceAll(),
  SliceFrom(1)
};

auto x_tail = x.traverse_slice(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(on_specs));
auto q = x.traverse(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(on_specs)).find_btw(21.0, 32.0, true);
auto picked = q.select_from(x_tail);
```

Output:

```text
traverse(on=all,1:).find_btw(21,32).coords:
pluss::Series(shape=[5,2], strides=[2,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[1, 0],
  [1, 1],
  [1, 2],
  [2, 0],
  [2, 1]]

traverse(on=all,1:).select_from(x_tail):
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[21, 22, 23, 31, 32]
```

### 1.5 Traversal writes

```cpp
auto y = x.clone();
std::vector<pluss::codem::ExtractSpec> middle{
  SliceAll(),
  SliceRange(1, 3, 1)
};
y.traverse_put_(pluss::msharp::RangeView::Of<pluss::codem::ExtractSpec>(middle),
                pluss::series::full({3, 2}, -1.0));
```

Output:

```text
traverse_put_(all, 1:3) = -1:
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, -1, -1, 13],
  [20, -1, -1, 23],
  [30, -1, -1, 33]]
```

## 2) Stats accessor (`series.stat.*`)

### 2.1 Core moments

```cpp
auto x = pluss::series::normal(0.0, 1.0, {20});
auto m = x.stat.mean();
auto v = x.var(false);
auto s = x.std(false);
auto sk = x.stat.skewness();
auto ku = x.stat.kurtosis();
```

Output excerpt (`plussqit_codem_stats_demo`):

```text
mean:
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=-0.0084899487

std(unbiased=false):
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=0.88947636

skewness:
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.024844598]

kurtosis:
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.09772825]
```

### 2.2 Distribution helpers

```cpp
auto p = x.stat.pdf();
auto lp = x.stat.logpdf();
auto lc = x.stat.logcdf();
auto c = x.stat.cdf();
```

Output excerpt:

```text
stat.pdf():
pluss::Series(shape=[20], strides=[1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[0.22223912, 0.38390648, 0.064756729, ...]

stat.cdf():
pluss::Series(shape=[20], strides=[1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[0.86031229, 0.60918376, 0.97173477, ...]
```

### 2.3 Moving-window stats

```cpp
auto mm = x.stat.movmean(4);
auto ms = x.stat.movstd(4);
```

Output excerpt:

```text
movmean(window=4):
pluss::Series(shape=[20], strides=[1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[1.0817235, 0.67945786, 1.0886132, ...]

movstd(window=4):
pluss::Series(shape=[20], strides=[1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[0, 0.40226559, 0.66535294, ...]
```

## 3) Linear algebra accessor (`series.linearalg.*`)

### 3.1 Matrix multiply and determinant

```cpp
auto A = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto B = pluss::make_series(std::vector<double>{7,8,9,10,11,12}, {3,2});
auto AB = A.linearalg(B).matmul();

auto G = A.transpose(0, 1).matmul(A).add(pluss::series::eye(3, 3, pluss::ScalarType::Float64));
auto d = G.linearalg.det();
```

Output (`plussqit_codem_linear_alg_demo`):

```text
A.linearalg(B).matmul():
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[58, 64],
  [139, 154]]

(Gram + I).linearalg.det():
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=146
```

### 3.2 Factorizations

```cpp
auto qrq = A.linearalg.qr_q();
auto qrr = A.linearalg.qr_r();
auto svdu = A.linearalg.svd_u();
auto svds = A.linearalg.svd_s();
auto svdvh = A.linearalg.svd_vh();
```

Output excerpt:

```text
subsp::linalg::qr(A).Q:
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-0.24253563, -0.9701425],
  [-0.9701425, 0.24253563]]

subsp::linalg::svd(A).S:
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[9.508032, 0.77286964]
```

## 4) Core Series methods that pair with these accessors

### 4.1 `slice` and `cube`

```cpp
auto x = pluss::make_series({
  {10.0, 11.0, 12.0, 13.0},
  {20.0, 21.0, 22.0, 23.0},
  {30.0, 31.0, 32.0, 33.0}
});

auto s = x.slice(1, 1, 4, 2);
auto c = x.cube().slice(0, 0, 1);
```

Output:

```text
slice(dim=1, start=1, end=4, step=2):
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]

cube().slice(0, 0, 1):
pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1000, 1331, 1728, 2197]]
```

## 5) Practical rules of thumb

- Use `traverse_slice` when the selector is view-representable and you want cheap traversal.
- Use `extract` when you need advanced/materializing indexing behavior.
- Use `TraverseMatch::select_from(other)` when query/match and selected values come from aligned tensors.
- Keep `stat` and `linearalg` for discoverability and namespace hygiene; they remain method-like via dot style (`x.stat.*`, `x.linearalg.*`).

## 6) Reproduce all outputs on your machine

```bash
./build/PlussQit/plussqit_codem_traverse_demo
./build/PlussQit/plussqit_codem_stats_demo
./build/PlussQit/plussqit_codem_linear_alg_demo
```
