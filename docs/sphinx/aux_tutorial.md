# Aux Tutorial (Comprehensive)

This tutorial is the practical, exhaustive onboarding path for `PlussQit/src/aux`.

If you are new to Pluss APIs, start here first, then use the shorter API guides for lookup.

## What You Get From `aux`

`aux` exists to remove repetitive glue code around day-to-day workflows:

- cleaner output/logging (`OutputEngine`)
- functional-style value pipelines (`suppose`, `let_as`)
- map/zip transforms (`transmit`)
- axis-wise transforms over Pluss tensors (`transform_axis`)
- reproducible seed management (`rng_seed`, `next_rng_seed`)
- safe scalar extraction from `Series` (`to_scalar`, `as_f64`, ...)
- practical path management for project/data/output/model locations (`pathing`)
- mini-batch loading and split planning for tensor workflows (`DataLoader` and split helpers)

## Include Strategy

If you want the full core PlussQit surface + aux:

```cpp
#include <pluss/pluss.h>
```

If you want table/file loaders from PlussMic as well:

```cpp
#include <pluss/mic.h>
```

## Tutorial Structure

1. Output and developer ergonomics
2. Functional helpers and value pipelines
3. Mapping and axis transforms
4. Reproducibility and scalar conversion
5. Pathing and runtime filesystem conventions
6. Data loading and splitting for ML/training loops
7. End-to-end workflows
8. Troubleshooting and operational notes

---

## 1) OutputEngine: Practical Output API

`OutputEngine` is the standard way to print mixed types without manual loops.
It supports primitives, STL containers, `optional`, `variant`, tuples, maps, and Pluss types with stream insertion support.

### 1.1 API Signatures (Public)

`pluss::OutputEngine OutputEngine(os_or_options)`  
Inputs:
- `os_or_options`: output target (`std::ostream`) and/or `OutputOptions`.
Output:
- initialized `OutputEngine`.

`OutputEngine::to(os) -> OutputEngine&`  
Inputs:
- `os`: new output stream sink.
Output:
- same engine (chainable).

`OutputEngine::write(args...) -> OutputEngine&`  
Inputs:
- `args...`: values to print separated by configured separator.
Output:
- same engine (chainable), no trailing newline.

`OutputEngine::writeln(args...) -> OutputEngine&`  
Inputs:
- `args...`: values to print.
Output:
- same engine (chainable), includes newline.

`OutputEngine::writef(pattern, args...) -> OutputEngine&`  
Inputs:
- `pattern`: format pattern.
- `args...`: values bound to the pattern.
Output:
- same engine (chainable), no trailing newline.

`OutputEngine::writefln(pattern, args...) -> OutputEngine&`  
Inputs:
- `pattern`: format pattern.
- `args...`: values bound to the pattern.
Output:
- same engine (chainable), includes newline.

`OutputEngine::endln() -> OutputEngine&`  
Inputs:
- none.
Output:
- same engine (chainable), newline emitted.

`OutputEngine::flush() -> OutputEngine&`  
Inputs:
- none.
Output:
- same engine (chainable), stream flushed.

`OutputEngine::log(level, args...) -> OutputEngine&`  
`OutputEngine::{trace|debug|info|warn|error}(args...) -> OutputEngine&`  
Inputs:
- `level`: log level (for `log`).
- `args...`: message payload.
Output:
- same engine (chainable), line with optional level prefix.

### 1.2 Arguments and Behavior

- `separator`: token inserted between arguments in `write`/`writeln`.
- `level_prefixes`: prefix text used by level methods.
- `min_level`: suppresses log calls below threshold.
- `auto_flush`: flushes stream after line-style methods.
- `to(os)`: retargets output stream at runtime.

### 1.3 Basic usage

```cpp
pluss::OutputEngine out;
out.writeln("rows", 150, "features", 4);
out.write("objective", 0.0314).endln();
```

Expected output:

```text
rows 150 features 4
objective 0.0314
```

### 1.4 Formatted usage (`writef`/`writefln`)

```cpp
pluss::OutputEngine out;
std::string model = "heston";
int iter = 12;
double loss = 0.000923;

out.writefln("model={} iter={} loss={:.6f}", model, iter, loss);
```

Expected output:

```text
model=heston iter=12 loss=0.000923
```

### 1.5 Container printing without manual loops

```cpp
pluss::OutputEngine out;
std::vector<std::string> names{"Open", "High", "Low", "Close"};
std::map<std::string, int> counts{{"ok", 7}, {"bad", 2}};
std::optional<double> maybe = 3.14;
std::variant<int, std::string> v = std::string("alpha");

auto tup = std::tuple{42, std::string("x"), 2.5};
out.writeln("names", names);
out.writeln("counts", counts, "maybe", maybe, "variant", v, "tuple", tup);
```

Expected output:

```text
names [Open, High, Low, Close]
counts {ok: 7, bad: 2} maybe 3.14 variant alpha tuple (42, x, 2.5)
```

### 1.6 Pluss types (Series, Note, Timeline)

```cpp
pluss::OutputEngine out;
auto s = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
out.info("series", s);
```

Expected output (exact shape metadata may vary by build):

```text
[INFO] series pluss::Series(shape=[3], ...)
```

### 1.7 Level filtering and custom prefixes

```cpp
pluss::OutputOptions opts;
opts.separator = " | ";
opts.min_level = pluss::OutputLevel::Info;
opts.level_prefixes = {"[T]", "[D]", "[I]", "[W]", "[E]"};

pluss::OutputEngine out{std::cout, opts};
out.debug("hidden");
out.info("epoch", 3, "loss", 0.42);
out.warnf("{} exceeded threshold {}", "grad_norm", 5.0);
```

Expected output:

```text
[I] epoch | 3 | loss | 0.42
[W] grad_norm exceeded threshold 5
```

### 1.8 Redirecting sink to `std::ostringstream`

```cpp
std::ostringstream oss;
pluss::OutputEngine out;
out.to(oss).writeln("a", 1, "b", 2);

std::string captured = oss.str();
```

Expected `captured`:

```text
a 1 b 2
```

### 1.9 Operational notes

- `OutputEngine` is thread-safe for writes (internal lock).
- This is about clean and flexible output, not SIMD/vectorized text rendering.
- For ultra-high-frequency logging, consider batching or lowering verbosity level.

---

## 2) `suppose` and `let_as`: Lightweight Value Pipelines

### 2.1 API Signatures (Public)

`pluss::suppose(value) -> let_box<T>`  
Inputs:
- `value`: any value to start a pipeline.
Output:
- pipeline box that supports `then/transform/tap/into/value`.

`pluss::suppose.as<To>(value) -> let_box<To>`  
Inputs:
- `value`: source value.
- `To`: target type.
Output:
- typed pipeline box.

`let_box<T>::then(fn) -> let_box<U>`  
Inputs:
- `fn`: transform callable from current value to next value.
Output:
- new pipeline box with transformed type.

`let_box<T>::tap(fn) -> let_box<T>`  
Inputs:
- `fn`: side-effect callable.
Output:
- same value pipeline.

`let_box<T>::into<U>() -> let_box<U>`  
Inputs:
- `U`: destination type.
Output:
- converted pipeline.

`let_box<T>::value() -> T`  
Inputs:
- none.
Output:
- final materialized value.

`let_as(T1, T2, ...) name = value`  
Inputs:
- allowed type set + initial value.
Output:
- constrained variant-style value with fixed allowed types.

### 2.2 Example: transform pipeline

```cpp
auto result = suppose(std::vector<int>{1, 2, 3, 4})
  .tap([](const auto& xs) {
    if (xs.empty()) throw std::runtime_error("empty input");
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
  .value();
```

Expected output idea (if printed):

```text
20
```

### 2.3 Example: explicit conversion with `into<U>()`

```cpp
auto msg = suppose(42)
  .into<double>()
  .then([](double x) { return std::string("value=") + std::to_string(x); })
  .value();
```

Expected output:

```text
value=42.000000
```

### 2.4 Example: restricted variant with `let_as`

```cpp
let_as(int, std::string, double) token = 5;
token = std::string("ready");

token.visit([](const auto& v) {
  std::cout << v << "\n";
});
```

Expected output:

```text
ready
```

### 2.5 Notes

- Use `suppose` for concise local transformations.
- Use `let_as` when value domain must stay constrained.
- Prefer regular named variables for long business logic branches.

---

## 3) `transmit`: Map/Zip Primitive

### 3.1 API Signatures (Public)

`pluss::transmit(fn, value) -> mapped_value`  
Inputs:
- `fn`: unary callable.
- `value`: scalar/object value.
Output:
- transformed value.

`pluss::transmit(fn, range, opts = {}) -> std::vector<out_t>`  
Inputs:
- `fn`: unary callable (`fn(elem)` or `fn(elem, index)`).
- `range`: iterable source.
- `opts`: execution options (`seq`/`par`, grain size, workers).
Output:
- transformed vector.

`pluss::transmit(fn, first, last, opts = {}) -> std::vector<out_t>`  
Inputs:
- iterator pair and transform callable.
Output:
- transformed vector.

`pluss::transmit(fn, range_a, range_b) -> std::vector<out_t>`  
Inputs:
- binary callable + two ranges.
Output:
- zipped transform result (up to min length).

`pluss::transmit_into(fn, range, out_it) -> OutIt`  
Inputs:
- `fn`: transform callable.
- `range`: source range.
- `out_it`: destination output iterator.
Output:
- final output iterator position.

### 3.2 Callable forms

Unary:

- `fn(elem)`
- `fn(elem, index)`

Binary zip:

- `fn(a, b)`
- `fn(a, b, index)`

### 3.3 Example A: scalar value

```cpp
auto y = pluss::transmit([](double x) { return x * 1.5; }, 2.0);
```

Expected output:

```text
3
```

### 3.4 Example B: unary range with index

```cpp
std::vector<int> xs{10, 10, 10};
auto ys = pluss::transmit([](int x, std::size_t i) {
  return x + static_cast<int>(i);
}, xs);
```

Expected output:

```text
[10, 11, 12]
```

### 3.5 Example C: zip two ranges

```cpp
std::vector<int> a{1, 2, 3};
std::vector<int> b{10, 20, 30};
auto z = pluss::transmit([](int x, int y) { return x + y; }, a, b);
```

Expected output:

```text
[11, 22, 33]
```

### 3.6 Example D: `transmit_into`

```cpp
std::vector<int> in{1, 2, 3, 4};
std::vector<int> out;
out.reserve(in.size());
pluss::transmit_into([](int x) { return x * x; }, in, std::back_inserter(out));
```

Expected output:

```text
[1, 4, 9, 16]
```

### 3.7 Example E: parallel map

```cpp
std::vector<double> vals(100000, 2.0);
pluss::transmit_options opts;
opts.exec = pluss::transmit_exec::par;
opts.grain_size = 2048;
opts.workers = 8;

auto out = pluss::transmit([](double x, std::size_t i) {
  return x + static_cast<double>(i % 7);
}, vals, opts);
```

Expected output behavior:

```text
out.size() == vals.size()
parallel scheduling enabled for random-access sized range
```

### 3.8 Notes

- Parallel path applies to unary range overload only.
- For tiny ranges, `seq` is usually faster due to lower overhead.
- Zip stops at the shorter range length.

---

## 4) `transform_axis`: Axis-wise Apply for Pluss Types

### 4.1 API Signatures (Public)

`pluss::transform_axis(fn, Series, axis, opts = {}) -> std::vector<out_t>`  
`pluss::transform_axis(fn, Note, axis, opts = {}) -> std::vector<out_t>`  
`pluss::transform_axis(fn, Timeline, axis, opts = {}) -> std::vector<out_t>`  
`pluss::transform_axis(fn, TimeIndex, axis = 0, opts = {}) -> std::vector<out_t>`  
Inputs:
- `fn`: callable applied to each axis slice.
- data object: one of `Series`, `Note`, `Timeline`, `TimeIndex`.
- `axis`: target axis (negative allowed).
- `opts`: execution options.
Output:
- vector of per-slice results.

### 4.2 Axis semantics

- Supports negative axis values (`-1` = last axis).
- `Series/Note/Timeline`: requires `dim() > 0`.
- `TimeIndex`: effectively 1D; axis must resolve to 0.

### 4.3 Example A: Series row means

```cpp
auto x = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {2,3});
auto row_means = pluss::transform_axis(
  [](const pluss::Series& row) {
    return row.mean().item<double>();
  },
  x,
  1
);
```

Expected output:

```text
[2.0, 5.0]
```

### 4.4 Example B: Note per-slice string length sums

```cpp
auto n = pluss::make_note({"ab", "c", "def", "gh"}, {2,2});
auto lengths = pluss::transform_axis(
  [](const pluss::Note& slice) {
    int total = 0;
    for (const auto& s : slice.host<std::string>().vector()) total += static_cast<int>(s.size());
    return total;
  },
  n,
  1
);
```

Expected output:

```text
[3, 5]
```

### 4.5 Example C: Timeline by-day summary

```cpp
auto t = pluss::make_timeline({
  "2024-01-01T10:00:00Z",
  "2024-01-01T11:00:00Z",
  "2024-01-02T10:00:00Z",
  "2024-01-02T12:00:00Z"
}, {2,2});

auto first_hours = pluss::transform_axis(
  [](const pluss::Timeline& slice) {
    return slice.hour().host<std::int32_t>().vector().front();
  },
  t,
  1
);
```

Expected output:

```text
[10, 10]
```

### 4.6 Notes

- Returned value is `std::vector<out_t>` where `out_t` is inferred from your callable.
- Use `opts.exec = par` for heavy per-slice callable workloads.

---

## 5) RNG Seed Control

### 5.1 API Signatures (Public)

`pluss::rng_seed(seed) -> void`  
Inputs:
- `seed`: explicit global RNG seed.
Output:
- none.

`pluss::rng_seed() -> std::uint64_t`  
Inputs:
- none.
Output:
- current global RNG seed.

`pluss::next_rng_seed() -> std::uint64_t`  
Inputs:
- none.
Output:
- next deterministic seed in stream.

`pluss::seed_or_next(seed) -> std::uint64_t`  
Inputs:
- `seed`: requested seed (`0` means consume next seed).
Output:
- chosen seed value.

### 5.2 Example

```cpp
pluss::rng_seed(12345);
auto a = pluss::next_rng_seed();
auto b = pluss::next_rng_seed();

auto c = pluss::seed_or_next(0);     // next stream value
auto d = pluss::seed_or_next(99);    // forced 99
```

Expected output behavior:

```text
a, b, c deterministic for same initial seed
a != b, b != c
 d == 99
```

### 5.3 Notes

- Use one global seed per run for reproducibility.
- `next_rng_seed()` is useful for deterministic sub-streams (dataset split, model init, augmentation, etc.).

---

## 6) Scalar Cast Helpers

### 6.1 API Signatures (Public)

`pluss::to_scalar<T>(x) -> T`  
Inputs:
- `x`: scalar `Series` (`numel() == 1`).
- `T`: output scalar type.
Output:
- scalar value extracted as `T`.

`pluss::as_f64(x) -> double`  
`pluss::as_f32(x) -> float`  
`pluss::as_bool(x) -> bool`  
`pluss::as_i16(x) -> int16_t`  
`pluss::as_i32(x) -> int32_t`  
`pluss::as_i64(x) -> int64_t`  
`pluss::as_u16(x) -> uint16_t`  
`pluss::as_u32(x) -> uint32_t`  
`pluss::as_u64(x) -> uint64_t`  
Inputs:
- `x`: scalar `Series`.
Output:
- value cast into requested primitive type.

### 6.2 Preconditions

`x` must be:

- defined
- scalar (`numel() == 1`)

Otherwise the helper throws/check-fails.

### 6.3 Example

```cpp
auto s = pluss::make_series(std::vector<double>{42.25}, {1});
double v1 = pluss::to_scalar<double>(s);
double v2 = pluss::as_f64(s);
```

Expected output:

```text
v1 = 42.25
v2 = 42.25
```

---

## 7) Pathing Utilities (`pluss::loader` namespace)

Pathing is centered on a simple single-root API (`file_path(...)`) with two add-ons:
- explicit absolute paths via `absolute_path(...)`
- multi-root contexts via `PathContext`

### 7.1 API Signatures (Public)

`pluss::loader::root_path() -> path`  
`pluss::loader::set_root_path(root, resolve_mode = RootResolve::WorkingDirectory) -> void`  
`pluss::loader::reset_root_path() -> void`

`pluss::loader::path_policy() -> PathPolicy`  
`pluss::loader::set_path_policy(policy) -> void`

`pluss::loader::file_path(subpath, options = {}) -> PathRef`  
`pluss::loader::file_dir(subpath, options = {}) -> DirRef`  
`pluss::loader::absolute_path(path, options = {}) -> PathRef`  
`pluss::loader::join_path(base, parts...) -> path`

`PathRef::string() -> std::string`  
`DirRef::string() -> std::string`  
`DirRef::file(subpath, options = {}) -> PathRef`

`pluss::loader::PathContext` (add-on):
- `PathContext::from_root(root, resolve_mode = RootResolve::WorkingDirectory) -> PathContext`
- `ctx.set_root_path(...)`, `ctx.root_path()`, `ctx.reset_root_path()`
- `ctx.set_path_policy(...)`, `ctx.path_policy()`
- `ctx.file_path(subpath, options = {}) -> PathRef`
- `ctx.file_dir(subpath, options = {}) -> DirRef`
- `ctx.absolute_path(path, options = {}) -> PathRef`

`pluss::loader::UrlContext` (internet-root add-on):
- `UrlContext::from_root("https://...") -> UrlContext`
- `u.file_path(subpath) -> UrlRef`
- `u.file_paths({...}) -> std::vector<UrlRef>`
- `u.file_path_map({...}) -> std::unordered_map<std::string, UrlRef>`
- `UrlRef::string() -> std::string`

### 7.2 Knobs and policies

`RootResolve`:
- `WorkingDirectory`: relative roots resolve from current process working directory.
- `CallerFile`: relative roots resolve from the file location where `set_root_path` is called.

`PathPolicy` (global):
- `canonicalize_root` (default `true`): normalize/absolute/canonicalize root.
- `auto_create_dirs` (default `false`): enable implicit directory creation for `file_dir(...)` and parent-directory creation for file targets.
- `force_replace_dirs` (default `false`): if a directory creation target exists, remove and recreate.
- `strip_leading_separators` (default `true`): legacy compatibility knob; rooted inputs are always treated as root-relative/parent-relative subpaths.

`PathOptions` (per call):
- `create_dirs`: optional override for creation behavior.
- `force_replace_dirs`: optional override for replacement behavior.

### 7.3 Resolution rules (exact behavior)

1. Default root resolution:
- `PLUSS_ROOT_PATH` env var if set.
- else `PLUSS_PROJECT_ROOT` env var if set.
- else nearest parent containing `.git`, `CMakeLists.txt`, or `PlussQit` (up to 12 levels).
- else current working directory.

2. `set_root_path(...)` accepts absolute or relative root:
- absolute stays absolute.
- relative resolves per `RootResolve`.

3. `file_path(subpath)` always appends to root:
- `"/all_data/train.csv"` and `"all_data/train.csv"` both resolve to `"<root>/all_data/train.csv"`.

4. Directory creation:
- global via `PathPolicy::auto_create_dirs`.
- per call via `PathOptions::create_dirs`.

5. `absolute_path(path)` is explicit opt-in:
- input must be rooted/absolute.
- resolved value is returned directly (it does not prepend configured root).

### 7.4 Example A: basic root + file lookup

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto train_csv = file_path("all_data/train.csv");
std::cout << root_path() << "\n";
std::cout << train_csv.string() << "\n";
```

Expected output:

```text
/abs/path/to/proj1
/abs/path/to/proj1/all_data/train.csv
```

### 7.5 Example B: relative root from current working directory

```cpp
using namespace pluss::loader;

set_root_path("../../proj1", RootResolve::WorkingDirectory);
auto p = file_path("trained_models/model.bin");
std::cout << root_path() << "\n";
std::cout << p.string() << "\n";
```

Expected output behavior:

```text
root_path() prints an absolute normalized path
p prints <absolute_root>/trained_models/model.bin
```

### 7.6 Example C: relative root from caller file location

```cpp
using namespace pluss::loader;

set_root_path("../proj1", RootResolve::CallerFile);
auto p = file_path("main_outputs/run_01/metrics.txt");
```

Expected output behavior:

```text
root is anchored to the source file location that called set_root_path
```

### 7.7 Example D: leading slash input still binds to root

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto p = file_path("/all_data/housing.csv");
std::cout << p.string() << "\n";
```

Expected output:

```text
/abs/path/to/proj1/all_data/housing.csv
```

### 7.8 Example E: directory paths and trailing slash output

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto model_dir = file_dir("trained_models");
std::cout << model_dir.string() << "\n";
```

Expected output:

```text
/abs/path/to/proj1/trained_models/
```

### 7.9 Example F: attach a file target from directory ref

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto model_dir = file_dir("trained_models");
auto model_file = model_dir.file("model.bin");
std::cout << model_file.string() << "\n";
```

Expected output:

```text
/abs/path/to/proj1/trained_models/model.bin
```

### 7.10 Example G: global creation policy (set once)

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
set_path_policy(PathPolicy{
  .canonicalize_root = true,
  .auto_create_dirs = true,
  .force_replace_dirs = false
});

auto run_dir = file_dir("main_outputs/ngarch_run_01");
auto metrics = run_dir.file("metrics.txt");
```

Expected output behavior:

```text
main_outputs/ngarch_run_01 is created if missing
metrics parent folder exists when writing metrics.txt
```

### 7.11 Example H: per-call override for creation and replacement

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto out = file_dir("saved_images/plots", PathOptions{
  .create_dirs = true,
  .force_replace_dirs = false
});
```

Expected output behavior:

```text
saved_images/plots is created for this call even if global auto_create_dirs=false
```

### 7.12 Example I: explicit one-off absolute path

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj1");
auto tool = absolute_path("/usr/local/bin/tool.exe");
std::cout << tool.string() << "\n";
```

Expected output:

```text
/usr/local/bin/tool.exe
```

### 7.13 Example J: multiple roots with `PathContext`

```cpp
using namespace pluss::loader;

auto proj = PathContext::from_root("/abs/path/to/proj1");
auto ext  = PathContext::from_root("/mnt/shared_data");

auto train_csv = proj.file_path("/all_data/train.csv");
auto vendor    = ext.file_dir("vendor").file("source2.csv");
auto cache     = ext.absolute_path("/tmp/cache/index.bin");
```

Expected output behavior:

```text
proj and ext resolve independently using their own root/policy state
```

### 7.14 Notes

- Keep one `set_root_path(...)` near app startup.
- Use `file_dir(...)` when you intend a folder target.
- Use `.file("file.ext")` on `DirRef` to finalize file targets from folders.
- For reproducible CI runs, set `PLUSS_ROOT_PATH` before launching binaries.
- `absolute_path(...)` is explicit and does not affect root-relative behavior.
- `PathContext` lets you keep multiple roots without replacing global root state.

### 7.15 Diagnostics (`PLUSS_CHECK` / `PLUSS_WARN`)

Pathing now emits stronger diagnostics:

- `PLUSS_CHECK` hard-fails when:
  - `set_root_path(...)` resolves to empty.
  - `file_path(...)` attempts to escape configured root.
  - `absolute_path(...)` receives non-rooted input.
  - `DirRef::file(...)` receives an empty path.
  - `DirRef::file(...)` attempts to escape parent root.
- `PLUSS_WARN` warns (stderr) when:
  - rooted path input is supplied to `file_path(...)`, `file_dir(...)`, or `DirRef::file(...)` (input is rewritten as subpath under root/parent).
  - `absolute_path(...)` cannot be normalized with `std::filesystem::absolute` and falls back to lexical normalization.

---

## 8) Data Loader Family (`pluss::loader` re-exported from aux)

This section covers tensor mini-batching and split planning APIs.

### 8.1 Core API Signatures (Public)

`pluss::loader::SeriesDataset(fields, batch_dim = 0) -> SeriesDataset`  
Inputs:
- `fields`: one or more aligned `Series` tensors.
- `batch_dim`: sample axis.
Output:
- dataset object with indexable samples.

`pluss::loader::DataLoader(dataset_or_fields, options = {}) -> DataLoader`  
Inputs:
- `dataset_or_fields`: `Dataset`, `SeriesDataset`, or raw `std::vector<Series>`.
- `options`: batching/shuffle/seed/device/dtype/collation options.
Output:
- iterable batch loader.

`DataLoader::next() -> optional<DataBatch>`  
Inputs:
- none.
Output:
- next batch if available.

`DataLoader::batch_at(batch_index) -> DataBatch`  
Inputs:
- `batch_index`: zero-based batch position.
Output:
- deterministic batch view for that index.

`DataBatch::x() -> Series`  
`DataBatch::y() -> Series`  
`DataBatch::fields -> std::vector<Series>`  
Inputs:
- none (or field index for `at(i)`).
Output:
- batched tensors.

### 8.2 Split Planning API Signatures (Public)

`pluss::loader::split_fields(fields, split_options) -> DataSplit`  
Inputs:
- `fields`: aligned tensors.
- `split_options`: train/val ratios, shuffle, seed, batch axis.
Output:
- train/val/test field groups.

`pluss::loader::split_loaders(fields, split_options, train_opts, val_opts, test_opts) -> DataLoaderSplit`  
Inputs:
- same `fields` and split policy plus loader options per split.
Output:
- ready-to-iterate train/val/test loaders.

`pluss::loader::time_series_split_indices(n_samples, ts_options) -> TimeSeriesIndexSplit`  
Inputs:
- `n_samples`: total sample count.
- `ts_options`: fold count, test window, train cap, gap.
Output:
- walk-forward index folds.

`pluss::loader::time_series_split_fields(fields, ts_options) -> TimeSeriesFieldSplit`  
`pluss::loader::time_series_split_loaders(fields, ts_options, train_opts, test_opts) -> TimeSeriesLoaderSplit`  
Inputs:
- time-ordered fields and fold options.
Output:
- fold-wise field partitions or fold-wise loaders.

### 8.3 Example A: basic mini-batch loader

```cpp
auto x = pluss::make_series(std::vector<double>{1,2,3,4,5,6}, {6});
auto y = pluss::make_series(std::vector<std::int64_t>{0,0,1,1,0,1}, {6});

pluss::loader::DataLoaderOptions opts;
opts.batch_size = 2;
opts.shuffle = false;

pluss::loader::DataLoader loader({x, y}, opts);

pluss::OutputEngine out;
out.writeln("batch_count", loader.batch_count());

for (auto batch : loader) {
  out.writeln("x shape", batch.x().sizes(), "y shape", batch.y().sizes());
}
```

Expected output behavior:

```text
batch_count 3
x shape [2] y shape [2]
x shape [2] y shape [2]
x shape [2] y shape [2]
```

### 8.4 Example B: shuffle + deterministic seed

```cpp
pluss::loader::DataLoaderOptions opts;
opts.batch_size = 3;
opts.shuffle = true;
opts.seed = 123;

pluss::loader::DataLoader loader({x, y}, opts);

auto b0 = loader.batch_at(0);
auto b1 = loader.batch_at(1);
```

Expected output behavior:

```text
Batches reflect a deterministic shuffled order for seed=123
```

### 8.5 Example C: target dtype/platform conversion

```cpp
pluss::loader::DataLoaderOptions opts;
opts.batch_size = 2;
opts.target_dtype = pluss::ScalarType::Float32;
opts.target_platform = pluss::Platform::CPU;

pluss::loader::DataLoader loader({x, y}, opts);
auto batch = loader.batch_at(0);
```

Expected output behavior:

```text
batch fields converted to requested platform/dtype where applicable
```

### 8.6 Example D: `collate_fn`

```cpp
pluss::loader::DataLoaderOptions opts;
opts.batch_size = 2;
opts.collate_fn = [](pluss::loader::DataBatch b) {
  // Example: center x batch by subtracting mean
  auto m = b.x().mean();
  b.fields[0] = b.x().sub(m);
  return b;
};

pluss::loader::DataLoader loader({x, y}, opts);
auto batch = loader.batch_at(0);
```

Expected output behavior:

```text
First field is transformed by collate_fn before being returned
```

### 8.7 Example E: split into train/val/test fields

```cpp
pluss::loader::DataSplitOptions sopts;
sopts.train_ratio = 0.7;
sopts.val_ratio = 0.2;
sopts.shuffle = true;
sopts.seed = 7;

auto split = pluss::loader::split_fields({x, y}, sopts);
```

Expected output behavior:

```text
train/val/test fields are disjoint and cover all samples
```

### 8.8 Example F: split directly to loaders

```cpp
auto loaders = pluss::loader::split_loaders(
  {x, y},
  {.train_ratio = 0.7, .val_ratio = 0.2, .shuffle = true, .seed = 7},
  {.batch_size = 4},
  {.batch_size = 4},
  {.batch_size = 4}
);

pluss::OutputEngine out;
out.writeln("train batches", loaders.train.batch_count());
out.writeln("val batches", loaders.val.batch_count());
out.writeln("test batches", loaders.test.batch_count());
```

Expected output behavior:

```text
Three loaders ready for train/val/test loops
```

### 8.9 Example G: time-series walk-forward indices

```cpp
auto plan = pluss::loader::time_series_split_indices(
  100,
  {.n_splits = 4, .test_size = 10, .gap = 2}
);

pluss::OutputEngine out;
out.writeln("folds", plan.folds.size());
out.writeln("fold0 train", plan.folds[0].train_indices.size(),
            "test", plan.folds[0].test_indices.size());
```

Expected output behavior:

```text
folds 4
fold0 train <depends on options> test 10
```

### 8.10 Example H: time-series split to loaders

```cpp
auto ts = pluss::loader::time_series_split_loaders(
  {x, y},
  {.n_splits = 3, .test_size = 2, .gap = 0},
  {.batch_size = 2},
  {.batch_size = 2}
);

pluss::OutputEngine out;
out.writeln("fold count", ts.folds.size());
```

Expected output behavior:

```text
Multiple train/test folds returned as DataLoader pairs
```

---

## 9) End-to-End Workflows

This section ties multiple aux components together the way real applications do.

### 9.1 End-to-End A: Reproducible experiment scaffold

```cpp
#include <pluss/pluss.h>

int main() {
  pluss::rng_seed(20260319);

  pluss::loader::set_root_path("./", pluss::loader::RootResolve::WorkingDirectory);
  pluss::loader::set_path_policy(pluss::loader::PathPolicy{
    .canonicalize_root = true,
    .auto_create_dirs = true,
    .force_replace_dirs = false
  });

  pluss::OutputOptions o;
  o.level_prefixes = {"[T]", "[D]", "[I]", "[W]", "[E]"};
  pluss::OutputEngine out{o};

  out.info("seed", pluss::rng_seed());

  auto run_dir = pluss::loader::file_dir("outputs/aux_tutorial_run");
  auto log_path = run_dir.file("metrics.txt", {.create_dirs = true});
  out.info("run_dir", run_dir.string());
  out.info("log_path", log_path.string());

  return 0;
}
```

Expected output:

```text
[I] seed 20260319
[I] run_dir ./outputs/aux_tutorial_run
[I] log_path ./outputs/aux_tutorial_run/metrics.txt
```

### 9.2 End-to-End B: Synthetic supervised training loop skeleton

```cpp
#include <pluss/pluss.h>

int main() {
  // synthetic data: y ~= 2x + noise-like perturbation
  std::vector<double> x_raw;
  std::vector<double> y_raw;
  x_raw.reserve(100);
  y_raw.reserve(100);

  for (int i = 0; i < 100; ++i) {
    double x = static_cast<double>(i) / 10.0;
    double y = 2.0 * x + ((i % 5) - 2) * 0.01;
    x_raw.push_back(x);
    y_raw.push_back(y);
  }

  auto X = pluss::make_series(x_raw, {100, 1});
  auto Y = pluss::make_series(y_raw, {100, 1});

  auto splits = pluss::loader::split_loaders(
    {X, Y},
    {.train_ratio = 0.8, .val_ratio = 0.1, .shuffle = true, .seed = 42},
    {.batch_size = 16, .shuffle = true, .seed = 42},
    {.batch_size = 16},
    {.batch_size = 16}
  );

  pluss::OutputEngine out;
  out.writeln("train batches", splits.train.batch_count());
  out.writeln("val batches", splits.val.batch_count());
  out.writeln("test batches", splits.test.batch_count());

  // pseudo training pass over one epoch
  int step = 0;
  for (auto batch : splits.train) {
    auto xb = batch.x();
    auto yb = batch.y();

    // fake prediction: yhat = 2 * x
    auto yhat = xb.mul(2.0);
    auto loss = yhat.sub(yb).abs().mean();

    out.infof("step={} batch_loss={:.6f}", step++, loss.item<double>());
  }

  return 0;
}
```

Expected output behavior:

```text
train/val/test batch counts printed
per-batch loss lines printed for training batches
```

### 9.3 End-to-End C: Time-series walk-forward evaluation scaffold

```cpp
#include <pluss/pluss.h>

int main() {
  // 24 sequential samples
  std::vector<double> x_raw(24), y_raw(24);
  for (int i = 0; i < 24; ++i) {
    x_raw[i] = static_cast<double>(i);
    y_raw[i] = 0.5 * i;
  }

  auto X = pluss::make_series(x_raw, {24, 1});
  auto Y = pluss::make_series(y_raw, {24, 1});

  auto folds = pluss::loader::time_series_split_loaders(
    {X, Y},
    {.n_splits = 4, .test_size = 4, .gap = 1},
    {.batch_size = 4},
    {.batch_size = 4}
  );

  pluss::OutputEngine out;
  out.writeln("fold_count", folds.folds.size());

  for (std::size_t f = 0; f < folds.folds.size(); ++f) {
    auto& tr = folds.folds[f].train;
    auto& te = folds.folds[f].test;
    out.writefln("fold={} train_batches={} test_batches={}",
                 f,
                 tr.batch_count(),
                 te.batch_count());
  }

  return 0;
}
```

Expected output behavior:

```text
fold_count 4
fold=0 train_batches=... test_batches=...
fold=1 train_batches=... test_batches=...
...
```

---

## 10) Interop With Table/File Loader APIs

`aux` and `pluss::loader` table APIs are meant to compose.

Typical pattern:

1. Load table data from CSV/Excel/JSON/Parquet.
2. Inspect and log with `OutputEngine`.
3. Convert needed fields to tensor structures for modeling.
4. Use aux DataLoader split helpers for training loops.

Pathing interop note:
- `load_csv` / `load_txt` take string paths, so pass `file_path(...).string()` (and similarly from `file_paths` / `file_path_map`).
- `load_csv` / `load_txt` / `load_excel` also accept `http(s)` URLs directly.
- for reusable internet roots, use `UrlContext::from_root("https://...")` then `u.file_path("file.csv").string()`.

Example inspection stage:

```cpp
auto df = pluss::loader::load_csv("data/bank-full.csv", {
  {"ncases", static_cast<std::int64_t>(1000)},
  {"usefeats", std::vector<std::string>{"age", "job", "balance"}}
});

pluss::OutputEngine out;
out.writeln("shape", df.ncases(), df.nfeatures());
out.writeln("features", df.feature_names());
```

Expected output:

```text
shape 1000 3
features [age, job, balance]
```

---

## 11) Troubleshooting and Notes

### 11.1 `to_scalar` fails

Symptom:

- error/check failure when using `to_scalar<T>(x)`.

Checklist:

- ensure `x.defined()`.
- ensure `x.numel() == 1`.
- ensure target type `T` is arithmetic and compatible.

### 11.2 `transmit` parallel doesn’t speed up

Possible causes:

- range too small.
- callable too light.
- non-random-access inputs.
- thread count/grain size mismatch.

Actions:

- benchmark with realistic workload.
- tune `grain_size` and `workers`.
- keep `seq` for short loops.

### 11.3 `transform_axis` axis out of range

- verify `dim() > 0`.
- use `-1` for last axis if rank changes.
- confirm axis for TimeIndex resolves to 0.

### 11.4 Path resolution confusion

- print `root_path()` at startup.
- print representative resolved targets such as `file_path("all_data/train.csv").string()`.
- verify env overrides (`PLUSS_ROOT_PATH`, `PLUSS_PROJECT_ROOT`).

### 11.5 DataLoader shape mismatch

- all fields must share identical sample count on `batch_dim`.
- `batch_dim` can be negative, but must resolve to valid axis per field.

---

## 12) Quick API Checklist

### Output

- `OutputEngine`, `OutputOptions`, `OutputLevel`
- `write`, `writeln`, `writef`, `writefln`, `endln`
- `trace/debug/info/warn/error`

### Functional helpers

- `suppose(...).then(...).value()`
- `suppose.as<T>(...)`
- `let_as(T1, T2, ...)`

### Mapping

- `transmit(fn, value)`
- `transmit(fn, range, opts)`
- `transmit(fn, a, b)`
- `transmit_into(fn, range, out_it)`

### Axis mapping

- `transform_axis(fn, series/note/timeline/timeindex, axis, opts)`

### Scalar + RNG

- `to_scalar<T>(series)` and typed aliases
- `rng_seed`, `next_rng_seed`, `seed_or_next`

### Pathing

- `set_root_path`, `root_path`, `reset_root_path`
- `set_path_policy`, `path_policy`
- `file_path`, `file_dir`, `absolute_path`
- `PathRef::string`, `DirRef::string`, `DirRef::file`

### Tensor data loading

- `SeriesDataset`, `DataLoader`, `DataLoaderOptions`
- `split_fields`, `split_loaders`
- `time_series_split_indices/fields/loaders`

---

## 13) Suggested Learning Order for New Users

1. Start with `OutputEngine` so diagnostics are easy immediately.
2. Learn `suppose` + `transmit` for local transformations.
3. Use `transform_axis` once your tensors become rank > 1.
4. Add `rng_seed` and pathing for reproducible experiments.
5. Move to `DataLoader` and split helpers for training workflows.
6. Integrate with table/file loaders (`load_csv`, `load_excel`, ...).

This path gives fast practical wins while keeping code maintainable as projects scale.
