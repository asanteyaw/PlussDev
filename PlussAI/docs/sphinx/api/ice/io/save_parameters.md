# `pluss::ai::ice::io::save_parameters`

Writes a generic, versioned, checksummed parameter-list artifact -- a
fixed-order dump of shape + values for any `std::vector<Series*>`.
What [`Model::save`](../model/Model/save.md) and every `models_*`
catalog entry's own `save` ultimately call for the weights-only portion
of a checkpoint; exposed standalone for saving raw parameters with no
`Model` involved at all, or for a hand-written `Model` subclass that
wants the identical artifact format without going through `Model::save`.

## Signature

```cpp
void pluss::ai::ice::io::save_parameters(std::string_view filepath, const std::vector<pluss::Series*>& parameters);
```

## What It Does

Writes `parameters` to `filepath` in a fixed order -- shape then flat
values per parameter, prefixed by a magic line, schema version, count,
and an FNV-1a checksum over the body.
[`load_parameters`](load_parameters.md) is the only supported reader;
the format is not meant to be hand-edited or read by anything else.
Values round-trip through `double` on both write and read, independent
of the parameter's own dtype -- see
[`load_parameters`](load_parameters.md) for how the dtype/platform on
read are decided.

## Parameters

`std::string_view filepath`
: Where to write. Overwrites unconditionally if it already exists.

`const std::vector<Series*>& parameters`
: The parameters to save, in the exact order
  [`load_parameters`](load_parameters.md) must later be called with --
  nothing about the artifact records parameter *names*, only position.

## Returns

`void`.

## Errors

- Throws if `filepath` is empty.
- Throws if `parameters` is empty.
- Throws if the file cannot be opened or written to.

## Practical Examples

Three examples: a direct save of two raw parameters, confirming
[`load_parameters`](load_parameters.md) round-trips the exact values
back into a fresh, differently-initialized parameter set, and the
count-mismatch failure from loading into the wrong number of
parameters.

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
  std::cout << "saved ok\n";

  return 0;
}
```

Output

```text
saved ok
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
  Series b = pluss::make_series({0.5F}, {1}, f32);
  ice::io::save_parameters("./io_params_demo.artifact", {&w, &b});

  // A fresh set of same-shaped parameters, starting at different values.
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

### Example 3

Loading into the wrong number of parameters -- one where the artifact
holds two -- throws rather than silently loading a partial result.

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

  Series w_only = pluss::make_series({0.0F, 0.0F, 0.0F, 0.0F}, {2, 2}, f32);
  try {
    ice::io::load_parameters("./io_params_demo.artifact", {&w_only});
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

- [`load_parameters`](load_parameters.md) -- the matching reader.
- [`Model::save`](../model/Model/save.md) -- calls this for the
  weights-only portion of every `Model` subclass's own save.
- [`save_checkpoint`](save_checkpoint.md) -- the full, resumable
  artifact this is one piece of.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
