# `pluss::tils::to_scalar and is_scalar_extractable_v`

[Back to Scalar casts](../core-scalar-cast.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/kit/scalar_cast.h>

template <class T> T to_scalar(const Series& x);
template <class T> inline constexpr bool is_scalar_extractable_v;
```

`to_scalar<T>` is the general form of the extraction overloads: it reads a one-element series back as a host value of `T`, for any `T` the trait accepts. `is_scalar_extractable_v<T>` is true for an arithmetic type or one of the two CodeM complex types, and false otherwise; a type the trait rejects fails to compile if you still try to extract it, because `to_scalar` asserts on the trait.

## Parameters

| Parameter | Meaning |
|---|---|
| `T` | The host type to read back. Checked against `is_scalar_extractable_v`. |
| `x` | A one-element series. |

## Returns

The single host value held by `x`, as a `T`.

## Errors

A series that does not hold exactly one element is refused. A `T` the trait rejects fails to compile.

## Practical Examples

### Example 1

Example 1 checks the trait for an extractable and a non-extractable type, then extracts a value.

```cpp
// to_scalar is the general form of the extraction. It names the type you want, and a trait tells you
// which types can be extracted at all. Here the program checks the traits before it asks.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto total = pluss::make_series(std::vector<std::int64_t>{128}, {}, cfg);

  std::cout << "int64 can be extracted: " << pluss::tils::is_scalar_extractable_v<std::int64_t> << '\n';
  std::cout << "std::string can be extracted: " << pluss::tils::is_scalar_extractable_v<std::string> << '\n';
  std::cout << "total: " << pluss::tils::to_scalar<std::int64_t>(total) << '\n';
  return 0;
}
```

Output

```text
int64 can be extracted: 1
std::string can be extracted: 0
total: 128
```

A 64-bit integer is extractable and a string is not, and the extracted value matches what the series held.

## See Also

- [Extraction form](extraction-form.md) -- the typed shortcut for this.
