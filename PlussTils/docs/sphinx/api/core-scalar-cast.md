# `pluss::tils scalar casts`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/tils/auxiliary/scalar_cast.h>
```

The scalar-cast family names a dtype directly in the function name, `as_bool`, `as_f32`, `as_f64`, `as_i8`, `as_i16`, `as_i32`, `as_i64`, `as_u8`, `as_u16`, `as_u32`, `as_u64`, `as_c64`, and `as_c128`. Every one of these thirteen names has the same four overloads, which differ only in what you hand it: a `Config` to tag with the dtype, an existing `Series` to convert, a host value to build a one-element `Series` from, or a one-element `Series` to read back out as a host value. Pick the overload by the argument you have, not by looking for a differently named function.

Each overload has its own reference page, since the four do genuinely different jobs even though the name is shared. `as_dtype` is the untyped form the thirteen named functions are built from; reach for it when the dtype is a runtime value instead of one you can name at the call site. `to_scalar<T>` and `is_scalar_extractable_v<T>` are the general form of the extraction overload, for a type that is not one of the thirteen built-in names.

## Parameters

Each page below documents the parameters of its own overload.

## Returns

See each page below.

## Errors

See each page below. The extraction overload is the only one that refuses an input: a `Series` that does not hold exactly one element.

## Practical Examples

### Example 1

Example 1 sets the dtype on a Config and builds a series from it.

```cpp
// Choosing the element type of a new Series. Each as_ function with a Config argument returns that
// configuration with the element type set, so it can be passed straight to a factory call.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

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

Two series, built from two differently typed configurations, report their own dtypes back.

### Example 2

Example 2 builds a one-element series from a host value, keeping its exact type.

```cpp
// Creating a one-element Series from a host value of a chosen type. The scalar form keeps the value's
// type exact, so a count stored as int64 stays an integer on the way in and out.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <cstdint>
#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto orders = pluss::tils::as_i64(std::int64_t{1234567890123}, cfg);
  const auto threshold = pluss::tils::as_f64(0.95, cfg);

  std::cout << "orders holds one value: " << (orders.numel() == 1) << '\n';
  std::cout << "orders read back: " << pluss::tils::as_i64(orders) << '\n';
  std::cout << "threshold read back: " << pluss::tils::as_f64(threshold) << '\n';
  return 0;
}
```

Output

```text
orders holds one value: 1
orders read back: 1234567890123
threshold read back: 0.95
```

The large integer and the fraction each keep the type they were built with, through a round trip.

### Example 3

Example 3 converts an existing column to another dtype.

```cpp
// Converting a whole column to a different element type. Sensor counts arrive as floats, and the
// database wants integers. The conversion truncates toward zero, so 2.9 becomes 2.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

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

The floating-point column becomes an integer column, truncated toward zero.

### Example 4

Example 4 reads one host value back out of a one-element series, and shows the refusal for a larger one.

```cpp
// Taking one host number out of a Series. An average computed by a pipeline is a one-element Series,
// and the extraction reads it back as a plain number. A Series with more than one element cannot be
// read as one number, and the extraction refuses it.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto average = pluss::make_series(std::vector<double>{42.5}, {}, cfg);
  std::cout << "average: " << pluss::tils::as_f64(average) << '\n';

  const auto column = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);
  try {
    (void)pluss::tils::as_f64(column);
    std::cout << "two values read as one\n";
  } catch (const std::exception&) {
    std::cout << "two values refused as one number\n";
  }
  return 0;
}
```

Output

```text
average: 42.5
two values refused as one number
```

The one-element series reads back as a plain number, and the two-element series is refused.

### Example 5

Example 5 names the dtype as a runtime value with as_dtype.

```cpp
// as_dtype(Config, ScalarType) is the generic form the named helpers (as_f64, as_i32, ...) call
// internally. Use it when the dtype is chosen at runtime, not known at the call site.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

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

The dtype set at runtime is the one the series reports.

## See Also

- [Config form](core-scalar-cast/config-form.md)
- [Conversion form](core-scalar-cast/conversion-form.md)
- [Construction form](core-scalar-cast/construction-form.md)
- [Extraction form](core-scalar-cast/extraction-form.md)
- [`as_dtype`](core-scalar-cast/as-dtype.md)
- [`to_scalar`](core-scalar-cast/to-scalar.md)
