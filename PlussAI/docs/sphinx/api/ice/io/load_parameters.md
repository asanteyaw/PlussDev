# `pluss::ai::ice::io::load_parameters`

Restores values written by [`save_parameters`](save_parameters.md)
into an already-constructed, identically-shaped parameter list. What
[`Model::load`](../model/Model/load.md) and every `models_*` catalog
entry's own `load` ultimately call for the weights-only portion of a
checkpoint.

## Signature

```cpp
void pluss::ai::ice::io::load_parameters(std::string_view filepath, const std::vector<pluss::Series*>& parameters);
```

## What It Does

Reads the artifact at `filepath`, verifies its checksum, and overwrites
each of `parameters` in place with the matching saved values --
`parameters` must already exist with the right count and shapes; this
never allocates new parameters, only fills existing ones. Each
parameter is reconstructed on whichever platform/dtype it already has
(`param->platform()`/`param->scalar_type()`), not whatever the artifact
was originally saved from -- loading a `Float32` CPU artifact's values
into a `Float64` MPS parameter works and casts automatically.

## Parameters

`std::string_view filepath`
: The artifact [`save_parameters`](save_parameters.md) wrote.

`const std::vector<Series*>& parameters`
: The parameters to overwrite, in the same order they were saved in.
  Must match the saved count exactly, and each one's element count
  must match its saved counterpart -- see Example 3/the next page's
  Example 3 for the two distinct failure modes.

## Returns

`void` -- mutates `parameters` in place.

## Errors

- Throws if `filepath` is empty, cannot be opened, or fails the
  checksum (truncated/corrupted artifact).
- Throws if the artifact's schema is unsupported.
- Throws if `parameters.size()` does not match the saved count.
- Throws if any individual parameter's element count does not match
  its saved counterpart.

## Practical Examples

Three examples: a direct round trip into a fresh, differently-valued
parameter set, loading across platforms/dtypes, and the per-parameter
size-mismatch failure (distinct from the whole-list count mismatch
[`save_parameters`'s own page](save_parameters.md) shows).

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series({1.0F, 2.0F, 3.0F, 4.0F}, {2, 2}, f32);
  Series b = pluss::make_series({0.5F}, {1}, f32);
  ice::io::save_parameters("./io_params_demo.artifact", {&w, &b});

  Series w2 = pluss::make_series({0.0F, 0.0F, 0.0F, 0.0F}, {2, 2}, f32);
  Series b2 = pluss::make_series({0.0F}, {1}, f32);
  std::cout << "before load: b2 = " << b2.item<float>() << "\n";

  ice::io::load_parameters("./io_params_demo.artifact", {&w2, &b2});
  std::cout << "after load:  b2 = " << b2.item<float>() << "\n";
  std::cout << "w2 matches w: " << (w.sub(w2).abs().amax().item<float>() == 0.0F) << "\n";

  return 0;
}
```

Output

```text
before load: b2 = 0
after load:  b2 = 0.5
w2 matches w: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series({1.0F, 2.0F, 3.0F, 4.0F}, {2, 2}, f32);
  ice::io::save_parameters("./io_params_demo.artifact", {&w});

  // A Float64 parameter -- the artifact's values (saved from a Float32
  // Series) cast automatically to whatever dtype the destination has.
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  Series w_f64 = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {2, 2}, f64);
  ice::io::load_parameters("./io_params_demo.artifact", {&w_f64});
  std::cout << "dtype == Float64: " << (w_f64.scalar_type() == ScalarType::Float64) << "\n";
  std::cout << "values match: " << (w_f64.sub(w.to(ScalarType::Float64)).abs().amax().item<double>() == 0.0) << "\n";

  return 0;
}
```

Output

```text
dtype == Float64: 1
values match: 1
```

### Example 3

Same parameter *count* as the artifact (one), but a different element
count -- `3` instead of the saved `4` -- throws a size-mismatch error
distinct from [`save_parameters`'s own Example 3](save_parameters.md)
(a whole-list count mismatch).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series({1.0F, 2.0F, 3.0F, 4.0F}, {2, 2}, f32);
  Series b = pluss::make_series({0.5F}, {1}, f32);
  ice::io::save_parameters("./io_params_demo.artifact", {&w, &b});

  Series w_wrong_size = pluss::make_series({0.0F, 0.0F, 0.0F}, {3}, f32);
  Series b2 = pluss::make_series({0.0F}, {1}, f32);
  try {
    ice::io::load_parameters("./io_params_demo.artifact", {&w_wrong_size, &b2});
    std::cout << "no error\n";
  } catch (const std::exception&) {
    std::cout << "threw\n";
  }

  return 0;
}
```

Output

```text
threw
```

## See Also

- [`save_parameters`](save_parameters.md) -- the matching writer.
- [`Model::load`](../model/Model/load.md) -- calls this for the
  weights-only portion of every `Model` subclass's own load.
- [`load_checkpoint`](load_checkpoint.md) -- the full, resumable
  artifact this is one piece of.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
