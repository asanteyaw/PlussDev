# `pluss::tils::as_<dtype>(const Series&, Config)`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/kit/scalar_cast.h>
```

`Series as_<dtype>(const Series& x, Config cfg = {})` converts every element of `x` to `<dtype>` and returns a new series. Converting a float to an integer dtype truncates toward zero. The input series is not modified.

## Parameters

| Parameter | Meaning |
|---|---|
| `x` | The series to convert. |
| `cfg` | The platform and pinning to build the result with. The dtype comes from the function name, not from this value. |

## Returns

A new `Series` with the chosen dtype, holding `x`'s values converted.

## Errors

None.

## Practical Examples

### Example 1

Example 1 converts a floating-point column to a 64-bit integer column.

```cpp
// Converting a whole column to a different element type. Sensor counts arrive as floats, and the
// database wants integers. The conversion truncates toward zero, so 2.9 becomes 2.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto floats = pluss::make_series(std::vector<double>{2.9, 7.0, 0.4, 11.75}, {4}, cfg);
  const auto ints = pluss::tils::as_i64(floats, cfg);

  std::cout << "integer column is int64: " << (ints.dtype().scalar == pluss::ScalarType::Int64) << '\n';
  std::cout << "values:";
  for (auto v : ints.host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
integer column is int64: 1
values: 2 7 0 11
```

Each value is truncated toward zero, so 2.9 becomes 2 and 11.75 becomes 11.

### Example 2

Example 2 converts a column of numbers to a boolean mask.

```cpp
// Turning a Series of numbers into a mask of true and false values. The mask marks the readings that
// passed a limit, and the conversion to boolean keeps every reading that is not zero as true.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto flags = pluss::make_series(std::vector<double>{0.0, 1.5, -2.0, 0.0}, {4}, cfg);
  const auto mask = pluss::tils::as_bool(flags, cfg);

  std::cout << "mask dtype is bool: " << (mask.dtype().scalar == pluss::ScalarType::Bool) << '\n';
  std::cout << "mask values:";
  for (bool b : mask.host<bool>()) std::cout << ' ' << b;
  std::cout << '\n';
  return 0;
}
```

Output

```text
mask dtype is bool: 1
mask values: 0 1 1 0
```

Zero becomes false, and every non-zero value, including a negative one, becomes true.

### Example 3

Example 3 converts to a narrower floating-point type.

```cpp
// Converting down to a narrower float type. The values are the same numbers, stored in a smaller type.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto wide = pluss::make_series(std::vector<double>{1.5, 2.5}, {2}, cfg);
  const auto narrow = pluss::tils::as_f32(wide, cfg);
  std::cout << "dtype: " << narrow.dtype() << '\n';
  for (float v : narrow.host<float>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
dtype: float32
 1.5 2.5
```

The values keep their magnitude, now stored as 32-bit floats instead of 64-bit ones.

## See Also

- [`as_dtype`](as-dtype.md) -- the untyped form this is built from.
