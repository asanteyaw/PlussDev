# `pluss::tils::as_<dtype>(const Series&)`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/aux/scalar_cast.h>
```

`T as_<dtype>(const Series& x)` reads `x` back as a single host value of type `T`. The series must hold exactly one element; a series with more is refused. This overload is the typed shortcut for `to_scalar<T>(x)`.

## Parameters

| Parameter | Meaning |
|---|---|
| `x` | A one-element series of the matching dtype. |

## Returns

The single host value held by `x`.

## Errors

A series that does not hold exactly one element is refused.

## Practical Examples

### Example 1

Example 1 reads a one-element series back as a double, and shows the refusal for a two-element series.

```cpp
// Taking one host number out of a Series. An average computed by a pipeline is a one-element Series,
// and the extraction reads it back as a plain number. A Series with more than one element cannot be
// read as one number, and the extraction refuses it.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

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

The one-element series reads back as 42.5, and the two-element series is refused rather than silently picking one value.

### Example 2

Example 2 reads a complex value back.

```cpp
// Complex numbers keep both parts through a Series and back out. A signal's gain is a complex value,
// and the program stores it, reads it back, and checks both parts survived unchanged.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const pluss::codem::Complex128 gain{0.6, -0.8};
  const auto stored = pluss::tils::as_c128(gain);
  const auto back = pluss::tils::as_c128(stored);

  std::cout << "real part kept: " << (back.real() == 0.6) << '\n';
  std::cout << "imaginary part kept: " << (back.imag() == -0.8) << '\n';
  return 0;
}
```

Output

```text
real part kept: 1
imaginary part kept: 1
```

The real and imaginary parts both come back unchanged.

### Example 3

Example 3 reads an integer value back, not a float.

```cpp
// Extracting a plain integer, not a float. The extraction form takes the type from the call, as_i64
// here, and requires the series to hold exactly one element.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto count = pluss::make_series(std::vector<std::int64_t>{42}, {}, cfg);
  const std::int64_t value = pluss::tils::as_i64(count);
  std::cout << "extracted: " << value << '\n';
  return 0;
}
```

Output

```text
extracted: 42
```

The extracted value is the plain integer the series held.

## See Also

- [`to_scalar`](to-scalar.md) -- the general form.
- [Construction form](construction-form.md) -- the reverse direction.
