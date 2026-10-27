# `pluss::tils::as_dtype`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/aux/scalar_cast.h>

Config as_dtype(Config cfg, ScalarType dtype);
Series as_dtype(const Series& x, ScalarType dtype, Config cfg = {});
Series as_dtype(double value, ScalarType dtype, Config cfg = {});
```

`as_dtype` is the untyped form the thirteen named `as_<dtype>` functions call internally. Use it when the target dtype is a runtime value, for example read from configuration or chosen by a caller, rather than one you can name directly in the source. The three overloads mirror the config, conversion, and construction forms of the named functions; there is no untyped overload for extraction, because the return type of an extraction has to be a compile-time type.

## Parameters

| Parameter | Meaning |
|---|---|
| `cfg` | The configuration to tag, or to build the result with. |
| `dtype` | The target dtype, as a runtime `ScalarType` value. |
| `x` | The series to convert. |
| `value` | A host double to build a one-element series from. |

## Returns

Matches the named forms: a tagged `Config`, a converted `Series`, or a one-element `Series`.

## Errors

None.

## Practical Examples

### Example 1

Example 1 tags a configuration with a dtype chosen at runtime.

```cpp
// as_dtype(Config, ScalarType) is the generic form the named helpers (as_f64, as_i32, ...) call
// internally. Use it when the dtype is chosen at runtime, not known at the call site.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto base = pluss::Config{}.with_platform(pluss::pCPU);
  const pluss::ScalarType chosen = pluss::ScalarType::Int32;
  const auto cfg = pluss::tils::as_dtype(base, chosen);
  const auto values = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, cfg);
  std::cout << "dtype chosen at runtime: " << values.dtype() << '\n';
  return 0;
}
```

Output

```text
dtype chosen at runtime: int32
```

The series built from the tagged configuration reports the runtime-chosen dtype.

### Example 2

Example 2 converts a series to a dtype named at runtime.

```cpp
// as_dtype(const Series&, ScalarType, Config) converts an existing series, the same job as_i64(x, cfg)
// does, but with the target type given as a value instead of named in the function.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto floats = pluss::make_series(std::vector<double>{3.9, 1.1}, {2}, cfg);
  const auto converted = pluss::tils::as_dtype(floats, pluss::ScalarType::Int64, cfg);
  std::cout << "converted dtype: " << converted.dtype() << '\n';
  for (auto v : converted.host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
converted dtype: int64
 3 1
```

The converted series reports the target dtype, and its values are truncated the same way the named conversion form truncates.

### Example 3

Example 3 builds a one-element series from a host double, with the dtype named at runtime.

```cpp
// as_dtype(double, ScalarType, Config) builds a one-element series from a host double, the same job
// as_f32(value, cfg) does for a chosen type, but the type is a value here, not part of the name.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto value = pluss::tils::as_dtype(2.5, pluss::ScalarType::Float32, cfg);
  std::cout << "dtype: " << value.dtype() << '\n';
  std::cout << "holds one element: " << (value.numel() == 1) << '\n';
  return 0;
}
```

Output

```text
dtype: float32
holds one element: 1
```

The built series reports the runtime-chosen dtype and holds exactly one element.

## See Also

- [Config form](config-form.md)
- [Conversion form](conversion-form.md)
- [Construction form](construction-form.md)
