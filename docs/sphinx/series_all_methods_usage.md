# Series: All Methods Usage

This page is the exhaustive usage companion for `Series`.

## Construction: all practical forms

```cpp
#include <pluss/pluss.h>
```

```cpp
// Scalars and nested literals
auto s0 = pluss::make_series(3.14);
auto s1 = pluss::make_series(true);
auto s2 = pluss::make_series({1.0, 2.0, 3.0});
auto s3 = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

// Flat host data + explicit shape
std::vector<double> flat{1,2,3,4,5,6};
auto s4 = pluss::make_series(flat, {2,3});
```

```cpp
// Factory families with dtype / config / options
auto a = pluss::series::empty({2,3}, pluss::ScalarType::Float64);
auto b = pluss::series::zeros({2,3});
auto c = pluss::series::ones({2,3}, pluss::ScalarType::Float32);
auto d = pluss::series::full({2,3}, 7.5);

auto cfg = pluss::config({{"dtype", "Float64"}, {"platform", "CPU"}});
auto e = pluss::series::zeros({2,3}, cfg);
auto f = pluss::series::ones({2,3}, cfg);
```

```cpp
// Ranges and random
auto r0 = pluss::series::arange(0, 10);
auto r1 = pluss::series::arange(0, 10, 2);
auto r2 = pluss::series::linspace(-1.0, 1.0, 11);
auto r3 = pluss::series::eye(4);

auto n0 = pluss::series::rand({2,3});
auto n1 = pluss::series::randn({2,3});
auto n2 = pluss::series::randint(0, 10, {2,3});
auto n3 = pluss::series::normal(0.0, 1.0, {2,3});
auto n4 = pluss::series::bernoulli({2,3}, 0.25);
```

## Method call rules (all overload styles)

```cpp
// Out-of-place
auto y1 = x.add(other);      // Series overload
auto y2 = x.add(1.0);        // scalar overload

// In-place (suffix _)
x.add_(other);
x.add_(1.0);

// Dimensional overloads
auto r = x.mean();
auto rdim = x.mean(1, true);

// Indexing/scatter families
auto g = x.gather(1, index);
auto s = x.scatter_add(1, index, src);
```

## Canonical runnable coverage

- `PlussQit/examples/CodeM/series_methods_demo.cpp` (broadest method coverage)
- `PlussQit/examples/CodeM/series_usage_demo.cpp`
- `PlussQit/examples/CodeM/stats_demo.cpp`
- `PlussQit/examples/CodeM/signal_demo.cpp`

## Real call examples from demo

```{include} ../generated/series_method_usage_from_demo.md
```

## Full generated method-name surface call pattern

This is the complete generated `SeriesMethodsGen.h` method name set and how to invoke each name.

```{include} ../generated/series_generated_method_call_patterns.md
```
