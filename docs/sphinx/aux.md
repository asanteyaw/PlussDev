# Aux User Guide

This guide covers the user-facing utilities in `PlussTils` (notably `pluss/tils/aux.h`):

- functional mini-pipeline helpers (`suppose`, `let_as`)
- mapping and axis transforms (`transmit`, `transform_axis`)
- output/printing (`OutputEngine`)
- RNG seed control (`rng_seed`, `next_rng_seed`)
- scalar extraction (`to_scalar`, `as_f64`, ...)
- tensor data iteration and splitting (`DataLoader`, `split_fields`, ...)

For full onboarding with end-to-end workflows and extended examples, read [Aux Tutorial](aux_tutorial.md).

## Include

```cpp
#include <pluss/tils.h>
```

Environment-variable helpers are documented in [PlussTils Env Guide](tils_env.md).

## 1. Output Engine

### API Signature

`pluss::OutputEngine OutputEngine(os_or_options)`
Inputs:
- `os_or_options`: optional output stream and/or `OutputOptions`.
Output:
- initialized output engine.

`OutputEngine::write(args...) -> OutputEngine&`
`OutputEngine::writeln(args...) -> OutputEngine&`
`OutputEngine::writef(pattern, args...) -> OutputEngine&`
`OutputEngine::writefln(pattern, args...) -> OutputEngine&`
Inputs:
- values and optional format pattern.
Output:
- same engine for chaining.

`OutputEngine::endln() -> OutputEngine&`
`OutputEngine::flush() -> OutputEngine&`
`OutputEngine::{trace|debug|info|warn|error}(args...) -> OutputEngine&`
Inputs:
- none or log message payload.
Output:
- same engine for chaining.

### Arguments

- `separator`: token inserted between `write/writeln` args.
- `level_prefixes`: string labels used by `trace/debug/info/warn/error`.
- `min_level`: suppresses lower-severity logs.
- `auto_flush`: flush after line-oriented calls.
- `to(std::ostream&)`: redirect output target at runtime.

### Rendering Rules and Extensibility

`write*()` is not limited to stream-insertable types.

Current rendering order:
- string-like values
- `std::optional`
- `std::variant`
- `std::pair`
- tuple-like values
- map-like ranges
- generic ranges (for example `std::vector`, `std::array`)
- ADL extension hook: `output_render(std::ostream&, const T&)`
- stream insertion: `operator<<(std::ostream&, const T&)`

If your type is not covered by the built-in categories, prefer one of these:

1. define `operator<<` for your public type
2. define an ADL renderer in the same namespace:

```cpp
namespace mylib {
struct Metrics {
  double loss{};
  int epoch{};
};

inline void output_render(std::ostream& os, const Metrics& m) {
  os << "Metrics{loss=" << m.loss << ", epoch=" << m.epoch << "}";
}
} // namespace mylib

pluss::OutputEngine out;
out.writeln(mylib::Metrics{.loss = 0.023, .epoch = 12});
```

The ADL path avoids patching `OutputEngine` for each custom type and keeps the API scalable.

### Example A: Basic printing

```cpp
pluss::OutputEngine out;
out.writeln("rows", 150, "features", 4);
out.write("sum", 42).endln();
```

Expected output:

```text
rows 150 features 4
sum 42
```

### Example B: Format-style printing

```cpp
pluss::OutputEngine out;
std::string name = "Ada";
int age = 31;

auto pi = 3.1415926535;
out.writefln("name={} age={}", name, age);
out.writefln("pi={:.3f}", pi);
```

Expected output:

```text
name=Ada age=31
pi=3.142
```

### Example C: STL and Pluss types

```cpp
pluss::OutputEngine out;
std::vector<std::string> feats{"Open", "High", "Low", "Close"};
std::map<std::string, int> counts{{"ok", 7}, {"bad", 1}};
auto s = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});

out.writeln("features", feats);
out.writeln("counts", counts);
out.info("series", s);
```

Expected output (shape/details may vary slightly by build):

```text
features [Open, High, Low, Close]
counts {ok: 7, bad: 1}
[INFO] series pluss::Series(shape=[3], ...)
```

### Example D: Options and levels

```cpp
pluss::OutputOptions opts;
opts.separator = " | ";
opts.level_prefixes = {"[T]", "[D]", "[I]", "[W]", "[E]"};
opts.min_level = pluss::OutputLevel::Info;

pluss::OutputEngine out{std::cout, opts};
out.debug("hidden");
out.info("train", 100, "val", 20);
out.warnf("epoch {} has high loss", 3);
```

Expected output:

```text
[I] train | 100 | val | 20
[W] epoch 3 has high loss
```

### Example E: Lymph summary objects print directly

```cpp
pluss::OutputEngine out;
auto X = imputer.learn_map(housing_num);
out.writeln(imputer.summary());
```

Expected output shape:

```text
SimpleImputerSummary{feature_names=[...], statistics=pluss::Series(...)}
```

## 2. suppose / let_as

### API Signature

`pluss::suppose(value) -> let_box<T>`
`pluss::suppose.as<To>(value) -> let_box<To>`
`let_box<T>::then(fn) -> let_box<U>`
`let_box<T>::tap(fn) -> let_box<T>`
`let_box<T>::into<U>() -> let_box<U>`
`let_box<T>::value() -> T`
`let_as(T1, T2, ...) x = value`

### Arguments

- `then/transform`: value transformation.
- `tap`: side effect while preserving the flowing value.
- `into<U>`: explicit conversion step.
- `let_as(...)`: compile-time restricted variant domain.

### Example

```cpp
auto out = suppose(std::vector<int>{1, 2, 3})
  .then([](std::vector<int> v) {
    for (auto& x : v) x *= 10;
    return v;
  })
  .then([](const std::vector<int>& v) {
    return v[0] + v[1] + v[2];
  })
  .value();

let_as(int, std::string) token = 42;
token = std::string("ok");
```

Expected output idea (if printed):

```text
sum=60
token=ok
```

## 3. transmit

### API Signature

`pluss::transmit(fn, value) -> mapped_value`
`pluss::transmit(fn, range, opts = {}) -> std::vector<out_t>`
`pluss::transmit(fn, first, last, opts = {}) -> std::vector<out_t>`
`pluss::transmit(fn, range_a, range_b) -> std::vector<out_t>`
`pluss::transmit_into(fn, range, out_it) -> OutIt`

### Arguments

- `fn` can accept `(elem)` or `(elem, index)`.
- binary overload accepts `(a, b)` or `(a, b, index)`.
- `exec=par` only applies to unary range overload with random-access sized ranges.

### Example A: unary map

```cpp
std::vector<int> xs{1,2,3,4};
auto sq = pluss::transmit([](int x){ return x*x; }, xs);
```

Expected output:

```text
[1, 4, 9, 16]
```

### Example B: zipped map with index

```cpp
std::vector<int> a{1,2,3};
std::vector<int> b{10,20,30};
auto z = pluss::transmit([](int x, int y, std::size_t i){
  return x + y + static_cast<int>(i);
}, a, b);
```

Expected output:

```text
[11, 23, 35]
```

## 4. transform_axis

### API Signature

`pluss::transform_axis(fn, Series, axis, opts = {}) -> std::vector<out_t>`
`pluss::transform_axis(fn, Note, axis, opts = {}) -> std::vector<out_t>`
`pluss::transform_axis(fn, Timeline, axis, opts = {}) -> std::vector<out_t>`
`pluss::transform_axis(fn, TimeIndex, axis = 0, opts = {}) -> std::vector<out_t>`

### Arguments

- `axis`: supports negative indexing (`-1` = last axis).
- `fn`: receives each 1D slice (and optional slice index).
- `opts`: sequential/parallel behavior for slice traversal.

### Example

```cpp
auto x = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto means = pluss::transform_axis(
  [](const pluss::Series& row){ return row.mean().item<double>(); },
  x,
  1
);
```

Expected output:

```text
[2.0, 5.0]
```

## 5. rng_seed utilities

### API Signature

`pluss::rng_seed(seed) -> void`
`pluss::rng_seed() -> std::uint64_t`
`pluss::next_rng_seed() -> std::uint64_t`
`pluss::seed_or_next(seed) -> std::uint64_t`

### Arguments

- `rng_seed(seed)`: set global deterministic seed state.
- `next_rng_seed()`: deterministic stream-derived next seed.
- `seed_or_next(0)`: convenience for “use next random stream seed”.

### Example

```cpp
pluss::rng_seed(123);
auto s1 = pluss::next_rng_seed();
auto s2 = pluss::next_rng_seed();
```

Expected output behavior:

```text
s1 and s2 are deterministic for the same initial seed
s1 != s2
```

## 6. Scalar extraction helpers

### API Signature

`pluss::to_scalar<T>(x) -> T`
`pluss::as_f64(x) -> double`
`pluss::as_f32(x) -> float`
`pluss::as_bool(x) -> bool`
`pluss::as_i16(x) -> int16_t`
`pluss::as_i32(x) -> int32_t`
`pluss::as_i64(x) -> int64_t`
`pluss::as_u16(x) -> uint16_t`
`pluss::as_u32(x) -> uint32_t`
`pluss::as_u64(x) -> uint64_t`

### Arguments

- input must be a defined scalar `Series` (`numel()==1`), else throws.

### Example

```cpp
auto s = pluss::make_series(std::vector<double>{42.5}, {1});
double v = pluss::as_f64(s);
```

Expected output:

```text
42.5
```

## 7. Tensor DataLoader and splitting utilities

This is the `pluss::loader` data-loader family re-exported from aux (`DataLoader`, `SeriesDataset`, split helpers).

### API Signature

`pluss::loader::SeriesDataset(fields, batch_dim = 0) -> SeriesDataset`
`pluss::loader::DataLoader(dataset_or_fields, options = {}) -> DataLoader`
`DataLoader::next() -> optional<DataBatch>`
`DataLoader::batch_at(batch_index) -> DataBatch`
`DataLoader::batch_count() -> size_t`
`DataLoader::reset(reshuffle = true) -> DataLoader&`
`pluss::loader::split_fields(fields, split_opts) -> DataSplit`
`pluss::loader::split_loaders(...) -> DataLoaderSplit`
`pluss::loader::temporal_split_indices(...) -> TemporalIndexSplit`
`pluss::loader::temporal_split_fields(...) -> TemporalFieldSplit`
`pluss::loader::temporal_split_loaders(...) -> TemporalLoaderSplit`

### Arguments

- `batch_dim`: sample axis for each field.
- `target_platform/target_dtype`: optional batch conversion.
- `collate_fn`: post-batch transformation hook.
- split helpers: ratio-based (`split_fields`) and time-aware folds (`temporal_split_*`).

### Example A: mini-batch iteration

```cpp
auto x = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {6});
auto y = pluss::make_series(std::vector<std::int64_t>{0,0,1,1,0,1}, {6});

pluss::loader::DataLoaderOptions opts;
opts.batch_size = 2;
opts.shuffle = false;

pluss::loader::DataLoader loader({x, y}, opts);
for (auto batch : loader) {
  // batch.x() and batch.y() each contain 2 samples
}
```

Expected output behavior:

```text
batch_count = 3
batch 0 -> samples [0,1]
batch 1 -> samples [2,3]
batch 2 -> samples [4,5]
```

### Example B: train/val/test split loaders

```cpp
pluss::loader::DataSplitOptions sopts;
sopts.train_ratio = 0.7;
sopts.val_ratio = 0.2;
sopts.shuffle = true;
sopts.seed = 7;

auto split = pluss::loader::split_loaders({x, y}, sopts,
  {.batch_size=2}, {.batch_size=2}, {.batch_size=2});
```

Expected output behavior:

```text
train/val/test loaders created with disjoint index sets
train + val + test sample counts == original sample count
```

## Related Guide

For file/table loading (`load_csv`, `load_excel`, `load_json`, `save_*`), path roots, and `Config`-driven loader APIs, see [Loader Guide](loader.md).
