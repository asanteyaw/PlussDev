# Aux Utilities Guide

## Includes

```cpp
#include <pluss/pluss.h>
```

The aux layer gives you three user-facing workflows that sit on top of the core `Series` / `Note` / `Timeline` / `TimeIndex` APIs:

- `pluss::Queue` for dynamic, runtime-selected calls across native Pluss types
- `pluss::dispense(...)` / `pluss::dispense_as<T>(...)` for typed method-pointer and string-name dispatch
- `pluss::suppose`, `pluss::transmit`, and `pluss::transform_axis` for lightweight pipelines and higher-level data transforms

## `pluss::Queue`

`Queue` is the dynamic union holder for `Series`, `Note`, `Timeline`, and `TimeIndex`.
Use it when the payload kind is only known at runtime, but you still want to keep values inside the Pluss type system.

### Signature

```cpp
pluss::Queue q(value);
```

### Explanation

`Queue` stores one native Pluss payload and lets you:

- inspect the current kind with `kind()` / `kind_name()`
- pull the concrete payload back out with `as_series()`, `as_note()`, `as_timeline()`, or `as_timeindex()`
- route method names dynamically with `invoke(...)`
- apply a few cross-kind helpers such as `Queue::where(...)`

### Example

```cpp
auto q = pluss::Queue(pluss::make_series({-1.0, 2.0, -3.0}));

std::cout << q.kind_name() << "\n";          // Series
std::cout << q.numel() << "\n";              // 3
std::cout << q.as_series().abs() << "\n";    // concrete Series access

auto out = q.invoke({"abs", "log", "mean"});
std::cout << out.kind_name() << "\n";        // Series
std::cout << out.as_series().item<double>() << "\n";
```

## `Queue::invoke`

### Signature

```cpp
Queue invoke(self, std::string_view method, std::span<const Queue::Arg> args = {}) const;
Queue invoke(self, std::string_view method, std::initializer_list<Queue::Arg> args) const;
Queue invoke(self, std::initializer_list<std::string_view> methods) const;
```

### Explanation

`invoke` sends a method name to the correct backend dispatcher for the payload currently stored in the queue.
It is the runtime-dispatch escape hatch when you cannot write `x.abs()` directly because `x` might be a `Series` today and a `Timeline` tomorrow.

The chained overload `invoke({"abs", "log", "mean"})` is especially useful when you want to assemble a pipeline of method names dynamically.

### Example

```cpp
auto s = pluss::Queue(pluss::make_series({-4.0, 5.0, -6.0}));
auto t = pluss::Queue(pluss::make_timeline({
    "2024-01-15T10:11:12Z",
    "2024-01-16T10:11:12Z",
}));

auto reduced = s.invoke({"abs", "mean"});
auto years = t.invoke("year");
auto idx = t.invoke("to_timeindex", {true});

std::cout << reduced.as_series().item<double>() << "\n";
std::cout << years.as_series().to_vec<std::int32_t>() << "\n";
std::cout << idx.as_timeindex().size() << "\n";
```

## `Queue::where`

### Signature

```cpp
static Queue where(Queue condition, Queue x, Queue y);
```

### Explanation

`Queue::where` performs ternary selection with a `Series` boolean mask.
It is the dynamic equivalent of `Series::where(...)` when the branch payloads are still wrapped in `Queue`.

Current supported branches are:

- `Series` vs `Series`
- `Timeline` vs `Timeline`

The condition must always be a `Series` mask.

### Example

```cpp
auto mask = pluss::Queue(pluss::make_series({true, false, true}));
auto lhs  = pluss::Queue(pluss::make_series({10.0, 20.0, 30.0}));
auto rhs  = pluss::Queue(pluss::make_series({1.0,  2.0,  3.0}));

auto out = pluss::Queue::where(mask, lhs, rhs);
std::cout << out.as_series() << "\n";
```

## `dispense` and `dispense_as`

### Signature

```cpp
std::any dispense(self, pmf, args);
decltype(auto) dispense(self, pmf, std::tuple<...> args);
std::any dispense(self, std::string_view method, args);
template <class R>
R dispense_as(self, ...);
```

### Explanation

`dispense` is the typed dispatch helper. It supports two user-facing styles:

- **member pointer dispatch**: call a known method with a runtime-provided argument pack
- **string-based dispatch**: call a registered method name using the generic lookup registry

Use member-pointer dispatch when you want full C++ type checking and you already know the concrete method.
Use string-based dispatch when the method name is dynamic.
Use `dispense_as<T>(...)` when you want the final result already `std::any_cast` to `T`.

### Example

```cpp
auto s = pluss::make_series({-4.0, 5.0, -6.0});
auto n = pluss::make_note({"apple", "banana", "apricot"});
auto t = pluss::make_timeline("2024-01-15T10:11:12Z");

auto count = pluss::dispense_as<std::int64_t>(
    s,
    &pluss::Series::numel,
    pluss::args{});

pluss::args starts_with_args;
starts_with_args.emplace_back(std::string_view("a"));
auto starts_with_a = pluss::dispense_as<std::vector<std::uint8_t>>(
    n,
    &pluss::Note::starts_with,
    starts_with_args);

auto floored = pluss::dispense_as<pluss::Timeline>(
    t,
    &pluss::Timeline::floor,
    pluss::args{"D"});

std::cout << count << "\n";
std::cout << static_cast<int>(starts_with_a[0]) << ","
          << static_cast<int>(starts_with_a[1]) << ","
          << static_cast<int>(starts_with_a[2]) << "\n";
std::cout << floored.hour().to_vec<std::int32_t>(0) << "\n";
```

## `suppose(...).then(...)`

### Signature

```cpp
auto box = suppose(value);
auto box = suppose.as<T>(value);
box.then(fn);
box.transform(fn);
box.tap(fn);
box.into<U>();
box.value();
```

### Explanation

`suppose` gives you a tiny, value-oriented pipeline object (`let_box<T>`) for ad-hoc transformations.

Use it when you want:

- a readable pipeline for one temporary value
- side effects during a chain with `tap(...)`
- explicit type conversion with `into<U>()`
- a lightweight alternative to writing a named variable for each intermediate

`transform(...)` is an alias for `then(...)`.
`tap(...)` runs a side effect and keeps the same boxed value flowing through the pipeline.

### Example

```cpp
auto tagged = suppose(std::vector<int>{1, 2, 3, 4})
                  .tap([](const auto& xs) {
                    if (xs.empty()) throw std::runtime_error("unexpected empty input");
                  })
                  .then([](std::vector<int> xs) {
                    for (auto& x : xs) x *= 2;
                    return xs;
                  })
                  .then([](const std::vector<int>& xs) {
                    int total = 0;
                    for (int x : xs) total += x;
                    return total;
                  })
                  .then([](int total) {
                    return std::string("total=") + std::to_string(total);
                  })
                  .value();

std::cout << tagged << "\n";
```

## `let_as(...)`

### Signature

```cpp
let_as(T1, T2, ...) name = value;
```

### Explanation

`let_as(...)` builds a restricted tagged union (`let_one_of<...>`) when a value is allowed to be one of a small set of explicit types.

It is useful when you want dynamic behavior, but you still want the compiler to enforce the allowed set.

### Example

```cpp
let_as(int, double, std::string) content = std::string("hello");
content = 42;
content = 3.14;

content.visit([](const auto& v) {
  std::cout << v << "\n";
});
```

## `transmit`

### Signature

```cpp
auto out = pluss::transmit(fn, value);
auto out = pluss::transmit(fn, range, opts = {});
auto out = pluss::transmit(fn, first, last, opts = {});
OutIt out = pluss::transmit_into(fn, range, out_it);
auto out = pluss::transmit(fn, range_a, range_b);
```

### Explanation

`transmit` is the aux mapping/zip primitive.

It supports five common patterns:

- transform one scalar value
- map over a range
- map over an iterator pair
- write mapped output into an existing iterator with `transmit_into`
- zip two ranges together

Your callable may accept either:

- `(elem)` or `(elem, index)` for unary work
- `(a, b)` or `(a, b, index)` for zipped work

Set `transmit_options{.exec = pluss::transmit_exec::par, ...}` when you want the unary range overload to split large random-access ranges across workers.

### Example

```cpp
const std::vector<int> xs{1, 2, 3, 4, 5};
const std::vector<int> ys{10, 20, 30, 40};

auto squared = pluss::transmit([](int x) {
  return x * x;
}, xs);

auto plus_index = pluss::transmit([](int x, std::size_t i) {
  return x + static_cast<int>(i);
}, xs);

auto zipped = pluss::transmit([](int a, int b, std::size_t i) {
  return a + b + static_cast<int>(i);
}, xs, ys);

pluss::transmit_options par;
par.exec = pluss::transmit_exec::par;
par.grain_size = 2;
par.workers = 4;
auto parallel = pluss::transmit([](int x, std::size_t i) {
  return x + static_cast<int>(i);
}, xs, par);
```

## `transform_axis`

### Signature

```cpp
auto out = pluss::transform_axis(fn, series, axis, opts = {});
auto out = pluss::transform_axis(fn, note, axis, opts = {});
auto out = pluss::transform_axis(fn, timeline, axis, opts = {});
auto out = pluss::transform_axis(fn, timeindex, axis = 0, opts = {});
```

### Explanation

`transform_axis` is the Pluss equivalent of NumPy-style `apply_along_axis`.
It walks a higher-rank object slice-by-slice along one axis, passes each 1D slice to your callable, and collects the returned results into a `std::vector`.

Use it when you want to write a slice-wise transform without hand-writing nested loops and repeated `select(...)` calls.

### Example

```cpp
auto s2d = pluss::make_series({
    {1.0, 2.0, 3.0},
    {10.0, 20.0, 30.0},
});

auto row_means = pluss::transform_axis(
    [](const pluss::Series& row) {
      return row.mean().item<double>();
    },
    s2d,
    1);

auto note2d = pluss::make_note({{"alpha", "beta"}, {"gamma", "delta"}});
auto row_char_counts = pluss::transform_axis(
    [](const pluss::Note& row) {
      std::int64_t total = 0;
      for (std::int64_t i = 0; i < row.numel(); ++i) total += static_cast<std::int64_t>(row.at(i).size());
      return total;
    },
    note2d,
    1);

std::cout << row_means[0] << ", " << row_means[1] << "\n";
std::cout << row_char_counts[0] << ", " << row_char_counts[1] << "\n";
```

## When to reach for which tool

- Use `Queue` when the payload kind is runtime-selected.
- Use `dispense` when the method or argument pack is runtime-selected but the concrete object is known.
- Use `suppose(...).then(...)` when you want a readable one-value pipeline.
- Use `transmit` for value/range mapping and zipping.
- Use `transform_axis` for slice-wise work over higher-rank Pluss objects.
