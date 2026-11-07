# `pluss::tils::as_<dtype>(Config)`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/kit/scalar_cast.h>
```

`Config as_<dtype>(Config cfg = {})` returns `cfg` with its dtype field set to `<dtype>`. It changes nothing else on the configuration: the platform and pinning you already set stay as they are. Pass the result to a factory such as `pluss::make_series` to build a value of that dtype.

## Parameters

| Parameter | Meaning |
|---|---|
| `cfg` | The configuration to tag. Defaults to a fresh `Config`. |

## Returns

A `Config` with the dtype field set.

## Errors

None.

## Practical Examples

### Example 1

Example 1 tags two different configurations and builds a series from each.

```cpp
// Choosing the element type of a new Series. Each as_ function with a Config argument returns that
// configuration with the element type set, so it can be passed straight to a factory call.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto single = pluss::tils::as_f32(pluss::Config{}.with_platform(pluss::pCPU));
  const auto ints = pluss::tils::as_i32(pluss::Config{}.with_platform(pluss::pCPU));

  const auto ratios = pluss::make_series(std::vector<double>{0.25, 0.5, 0.75}, {3}, single);
  const auto counts = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, ints);

  std::cout << "ratios dtype: " << ratios.dtype() << '\n';
  std::cout << "counts dtype: " << counts.dtype() << '\n';
  return 0;
}
```

Output

```text
ratios dtype: float32
counts dtype: int32
```

Each series reports the dtype its own configuration was tagged with.

### Example 2

Example 2 tags a configuration for booleans.

```cpp
// A second dtype, to show the config form is uniform across the family: every as_<dtype>(Config)
// sets the same field on the configuration, whichever type you pick.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::tils::as_bool(pluss::Config{}.with_platform(pluss::pCPU));
  const auto flags = pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, cfg);
  std::cout << "dtype: " << flags.dtype() << '\n';
  return 0;
}
```

Output

```text
dtype: bool
```

The series built from the tagged configuration holds booleans.

## See Also

- [`as_dtype`](as-dtype.md) -- the untyped form this is built from.
