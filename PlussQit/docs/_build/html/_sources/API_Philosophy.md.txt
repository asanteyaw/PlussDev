# API Philosophy

PlussQit is designed for **maximum usability** in C++.

## One include

End users should be able to write:

```cpp
#include <pluss/pluss.h>
```

and gain access to the complete high-level API.

## Short names for types, namespaced factories

Core types stay flat:

- `pluss::Series`
- `pluss::ScalarType`

Construction factories live in a dedicated namespace per payload type:

- `pluss::series::zeros(...)`, `pluss::series::arange(...)`, ...

with `pluss::make_series(...)` kept as a permanent flat shortcut alongside
`pluss::make_note`/`pluss::make_timeline`/`pluss::make_timeindex`/
`pluss::make_duration` for the common construct-from-data case. Every other
flat factory spelling (`pluss::zeros`, `pluss::full`, ...) is a legacy
compatibility alias for external consumers migrating off the old flat API --
new code should call through `pluss::series::*`/`pluss::note::*`/
`pluss::timeline::*`/`pluss::timeindex::*`/`pluss::duration::*` directly.

Internally, the implementation lives in `pluss::codem`.

## Method-first operations

If a `Series` already exists, operations should be **methods**:

```cpp
auto y = x.exp().add(z).clamp(0.0, 1.0);
```

Factories remain free functions because they *create* a new object:

```cpp
auto x = pluss::series::zeros({2,3});
```

This avoids API duplication and reduces mental load for users.

## Interop

Interop functions are also “creation” operations:

- `pluss::series::from_torch(torch::Tensor)` creates a new `Series` view.

Converting an existing Series is a method:

- `Series::tensor()` creates a `torch::Tensor` view.

## Common method pairs

PlussQit follows the pattern:

- `op(...)` returns a new Series
- `op_(...)` mutates in-place

Examples:

- `add` / `add_`
- `exp` / `exp_`
- `index_put` / `index_put_`
- `scatter_add` / `scatter_add_`


## Operator overloads

PlussQit provides C++ operator overloads as **thin forwarding sugar**:

- `a + b` == `a.add(b)`
- `a += b` == `a.add_(b)`
- `a > b` == `a.gt(b)`
- `a & b` == `a.bitwise_and(b)`

Operators never replace method APIs; they remain a convenience layer.

Printing uses `operator<<` to emit a compact summary + small preview.

## Scalar overloads

For ergonomics, common binary ops also accept `double`:

```cpp
auto y = x + 3.0;
x += 1.0;
a = (x > 0.0);
```

Internally, scalars are converted to 1-element Series and broadcasted.


## Nomenclature deviations from Torch

PlussQit intentionally deviates from Torch naming in select places to keep the API coherent:

- `Series::unite(...)` corresponds to Torch `cat`
- `Series::pile(...)` corresponds to Torch `stack`

Deprecated compatibility aliases `cat/stack` may exist temporarily, but users should prefer `unite/pile`.

## Formatting and printing

`operator<<` provides a compact summary + preview. For richer formatted printing, PlussQit may optionally support `fmt` or C++23 `std::print` behind feature guards in a future patch.

## Config ergonomics

In addition to `Config{}.with_dtype(...)`, PlussQit supports unordered, typed options:

```cpp
auto x = pluss::series::zeros({128,64}, pluss::cfg::dtype(pluss::ScalarType::Float16), pluss::cfg::pinned(true));
```

Or a string-map builder (CPU-only subset):

```cpp
auto cfg = pluss::config({{"dtype","float16"},{"pinned",true}});
auto x = pluss::series::zeros({128,64}, cfg);
```

## Large method surface

PlussQit keeps a large method surface (Torch-inspired) but maintains a single method-first API.
For forward compatibility, many signatures exist early; unimplemented methods throw a clear runtime error.
Declarations are centralized in `CodeM/core/SeriesMethods.inc` to keep things DRY.

## Torch alignment source

Patch 24 generates a large method surface from Torch `native_functions.yaml` (CPU-only subset) to stay aligned with operator naming and signature intent while keeping PlussQit method-first.

## Code generation

PlussQit can regenerate its Torch-aligned method surface from upstream YAML via `tools/codegen/generate_series_methods.py`. Generated files are checked in to keep builds hermetic.

## Kernel tables

To accelerate implementation, PlussQit can route ops through compact kernel tables (see `CodeM/native/cpu/KernelTable.*`). This reduces the amount of boilerplate needed when turning stubs into real kernels.
