# `pluss::tils::as_<dtype>(value, Config)`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/kit/scalar_cast.h>
```

`Series as_<dtype>(T value, Config cfg = {})` builds a one-element series directly from a host value of the matching type `T`, without converting through another dtype first. Use it when you already have the host value and need it as a one-element series of a specific dtype, such as a parameter for a call that expects a `Series`.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The host value. Its type matches the dtype named in the function, for example a `double` for `as_f64` or a `std::int64_t` for `as_i64`. |
| `cfg` | The platform and pinning to build the series with. |

## Returns

A one-element `Series` of the named dtype, holding `value`.

## Errors

None.

## Practical Examples

### Example 1

Example 1 builds a one-element series from a large integer and from a double.

```cpp
// Creating a one-element Series from a host value of a chosen type. The scalar form keeps the value's
// type exact, so a count stored as int64 stays an integer on the way in and out.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

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

Each value keeps its exact type and its exact digits, read back the same way it went in.

### Example 2

Example 2 builds a one-element series from a complex host value.

```cpp
// Complex numbers keep both parts through a Series and back out. A signal's gain is a complex value,
// and the program stores it, reads it back, and checks both parts survived unchanged.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

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

The complex value's real and imaginary parts are both kept through the round trip.

## See Also

- [`as_dtype`](as-dtype.md) -- the untyped form this is built from.
- [Extraction form](extraction-form.md) -- the reverse direction.
