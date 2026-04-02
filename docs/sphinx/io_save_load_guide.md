# Pluss I/O Save/Load Guide

This guide covers the unified serialization API now available in `pluss::io` and top-level aliases `pluss::save` / `pluss::load`.

## Quick Start

```cpp
#include <pluss/pluss.h>

pluss::Series x = pluss::randn({3, 4});
pluss::save(x, "tensor.plio");

auto x_loaded = pluss::load<pluss::Series>("tensor.plio");
```

## API Surface

Core API (`<pluss/io.h>`, also re-exported by `<pluss/core.h>` / `<pluss/pluss.h>`):

```cpp
namespace pluss::io {
  enum class Compression : std::uint8_t { None, Zstd, Lz4 };

  struct SaveOptions {
    Compression compression = Compression::None;
    int32_t compression_level = 0;
    bool create_parent_dirs = false;
    bool include_platform_hint = true;
    bool canonicalize_series_storage = true;
    bool warn_on_platform_downgrade = true;
  };

  struct LoadOptions {
    bool preserve_saved_platform = false;
    std::optional<pluss::codem::Platform> target_platform{};
    bool strict_platform = false;
  };

  template <class T> void save(const T& value, std::string_view path, const SaveOptions& = {});
  template <class T> void save(const T& value, std::ostream& out, const SaveOptions& = {});

  template <class T> T load(std::string_view path, const LoadOptions& = {});
  template <class T> T load(std::istream& in, const LoadOptions& = {});

  template <class T> void load(T& out, std::string_view path, const LoadOptions& = {});
  template <class T> void load(T& out, std::istream& in, const LoadOptions& = {});
}
```

Aliases:

```cpp
pluss::save(...); // same as pluss::io::save(...)
pluss::load<...>(...); // same as pluss::io::load<...>(...)
```

## Supported Types

- `pluss::Series`
- `pluss::Note`
- `pluss::Timeline`
- `pluss::TimeIndex`
- `std::vector<T>` where `T` is supported
- `std::list<T>` where `T` is supported
- `std::deque<T>` where `T` is supported
- `std::array<T, N>` where `T` is supported
- `pluss::StackArray<pluss::Series>`

Nested containers are supported as long as inner element types are supported.

## Usage Patterns

### 1) Save/load by path

```cpp
pluss::Note n = pluss::make_note({"alpha", "beta"});
pluss::save(n, "note.plio");

auto n2 = pluss::load<pluss::Note>("note.plio");
```

### 2) Out-parameter load (Torch-like style)

```cpp
pluss::Series y;
pluss::load(y, "tensor.plio");
```

### 3) Stream-based I/O

```cpp
std::stringstream ss;
pluss::save(pluss::arange(10), ss);

auto z = pluss::load<pluss::Series>(ss);
```

### 4) Containers

```cpp
std::vector<pluss::Series> batch = {
  pluss::randn({2, 2}),
  pluss::ones({2, 2})
};
pluss::save(batch, "batch_vec.plio");
auto batch2 = pluss::load<std::vector<pluss::Series>>("batch_vec.plio");

pluss::StackArray<pluss::Series> stack{pluss::zeros({3}), pluss::ones({3})};
pluss::save(stack, "stack_series.plio");
auto stack2 = pluss::load<pluss::StackArray<pluss::Series>>("stack_series.plio");
```

### 5) Save options

```cpp
pluss::io::SaveOptions save_opts;
save_opts.create_parent_dirs = true;
save_opts.include_platform_hint = true;
pluss::save(pluss::randn({4, 4}), "artifacts/tensors/a.plio", save_opts);
```

### 6) Load options for `Series`

```cpp
pluss::io::LoadOptions load_opts;
load_opts.preserve_saved_platform = true; // use saved platform hint when available
load_opts.strict_platform = false;        // fallback to CPU with warning if unavailable

auto s = pluss::load<pluss::Series>("tensor.plio", load_opts);
```

Or force a specific platform:

```cpp
pluss::io::LoadOptions load_opts;
load_opts.target_platform = pluss::codem::Platform::CPU;
auto s = pluss::load<pluss::Series>("tensor.plio", load_opts);
```

## Behavior Notes

- Files are schema-versioned with format magic checks.
- Current on-disk payload is uncompressed and little-endian.
- `Series` is serialized as canonical contiguous CPU payload for portability.
- `Note` preserves byte content and bytes-vs-utf8 mode.
- `Timeline` preserves epoch nanos + timezone metadata.
- `TimeIndex` preserves underlying timeline and sortedness intent.
- `Compression::Zstd` / `Compression::Lz4` are currently accepted as options but not yet emitted; runtime warns and writes `Compression::None`.

## Model/Workflow Bridge (PlussAI)

When using `<pluss/ai.h>` (or `<pluss/ai/ml.h>`), `pluss::io::save/load` also has model/workflow bridge overloads that delegate to existing NodeJ/Lymph serialization.

### NodeJ Flow

```cpp
pluss::save(flow, "flow.bin", train_batch, train_options);

auto bundle = pluss::load<pluss::ai::nodej::io::FlowBundle>("flow.bin");
auto flow2  = pluss::load<pluss::ai::nodej::Flow>("flow.bin");
```

### Lymph Workflow

```cpp
pluss::save(workflow, "workflow.bin", train_batch, train_options);
auto restored = pluss::load<pluss::ai::lymph::Workflow>("workflow.bin");
```

### Lymph WorkflowModel

```cpp
pluss::save(workflow_model, "workflow_model.bin", train_batch, train_options);
auto restored_model = pluss::load<pluss::ai::lymph::WorkflowModel>("workflow_model.bin");
```

## Error/Warning Model

The API uses `PLUSS_CHECK` and `PLUSS_WARN` for diagnostics:

- hard failures for invalid file headers, schema mismatches, shape/payload mismatches, and unsupported type contracts;
- warnings for recoverable behavior (for example platform fallback or ignored model-load platform flags).
