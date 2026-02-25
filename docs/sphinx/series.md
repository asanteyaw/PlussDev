# Series Guide

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>
```

## Mental model

`Series` is the numeric/bool tensor-like core type in PlussQit.

- Factories construct data (`value`, `make_series`, `zeros`, `randn`, ...).
- Methods transform/query data (`reshape`, `mean`, `gather`, ...).
- Conversion is explicit (`to(platform)`, `to(dtype)`).

For exhaustive call forms, use [Series: All Methods Usage](series_all_methods_usage).

## Common workflow

```cpp
auto x = pluss::normal(0.0, 1.0, {20});
auto y = x.abs().add(1.0).log();
auto mean = y.mean();
auto stdv = y.std(false);
```

## Construction quick recipes

```cpp
// scalar / literal
auto a = pluss::value(3.14);
auto v = pluss::value({1.0, 2.0, 3.0});
auto m = pluss::value({{1.0, 2.0}, {3.0, 4.0}});

// host buffer + shape
std::vector<double> flat{1,2,3,4,5,6};
auto x = pluss::make_series(flat, {2,3});

// factory family
auto z = pluss::zeros({2,3});
auto o = pluss::ones({2,3});
auto f = pluss::full({2,3}, 7.5);
auto r = pluss::randn({2,3});
auto i = pluss::randint(0, 10, {2,3});
```

## High-value method families

```cpp
// shape/indexing
auto mt = m.transpose(0, 1);
auto mr = m.reshape({3,2});
auto g  = m.gather(1, idx);

// reductions
auto s = m.sum(1, false);
auto am = m.amax(1, false);

// masking/scatter
auto masked = m.masked_select(mask);
auto out = base.scatter_add(1, idx, src);
```

## Demos

- `PlussQit/examples/CodeM/series_usage_demo.cpp`
- `PlussQit/examples/CodeM/series_methods_demo.cpp`
- `PlussQit/examples/CodeM/stats_demo.cpp`
- `PlussQit/examples/CodeM/signal_demo.cpp`
