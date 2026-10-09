# PlussTils: Scalar casts

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Numbers move between the host program and a Series, and each move names a type. A count stored as an integer should stay an integer. A column of measurements may need to become whole numbers before it is saved. A single result may need to come out as a plain number. The `as_` family of scalar casts handles each of these moves, and this chapter covers each form with a complete program.

## The mental model

The `as_` functions come in four forms, and the argument decides which one you get. With a `Config`, the function returns that configuration with the element type set, so you can pass it to a factory. With a host value, it builds a one-element Series of that type. With a Series, it converts every element to the new type. And with a one-element Series alone, it returns the host value. The last form refuses a Series that does not hold exactly one element.

Conversions truncate toward zero when they go from a float to an integer, and extraction requires exactly one element. `to_scalar<T>` is the general form of extraction, and a trait tells you whether a type can be extracted at all.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: A configuration that sets the element type

A factory call needs an element type. Each `as_` function that takes a configuration returns that configuration with its type set. This program builds two configurations, one for float32 and one for int32, and makes a series from each.

```cpp
// Choosing the element type of a new Series. Each as_ function with a Config argument returns that
// configuration with the element type set, so it can be passed straight to a factory call.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

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

The series take the types the configurations named: float32 and int32. The configuration is the only thing that chose the type, so the factory call does not repeat it.

## Example 2: A host value stored as a typed series

A count can be too large for a double, so it should be stored as an integer from the start. This program stores a 64-bit integer and a double as one-element series, then reads both back.

```cpp
// Creating a one-element Series from a host value of a chosen type. The scalar form keeps the value's
// type exact, so a count stored as int64 stays an integer on the way in and out.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

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

The large integer comes back exactly, with all its digits, and the double comes back as 0.95. Each value kept its own type through the round trip.

## Example 3: Converting a whole column

A column of sensor counts arrives as floats and must be stored as integers. This program converts every element to a 64-bit integer.

```cpp
// Converting a whole column to a different element type. Sensor counts arrive as floats, and the
// database wants integers. The conversion truncates toward zero, so 2.9 becomes 2.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto floats = pluss::make_series(std::vector<double>{2.9, 7.0, 0.4, 11.75}, {4}, cfg);
  const auto ints = pluss::tils::as_i64(floats, cfg);

  std::cout << "integer column is int64: " << (ints.dtype().scalar == pluss::ScalarType::Int64) << '\n';
  std::cout << "values:";
  for (auto v : ints.to_vec<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
```

Output

```text
integer column is int64: 1
values: 2 7 0 11
```

The conversion truncates toward zero, so 2.9 becomes 2 and 11.75 becomes 11. The result is an integer column, so the truncation is part of the stored value.

## Example 4: Extracting one result

A pipeline often ends with a one-element series, such as an average. This program reads one as a plain number, then tries to read a two-element series the same way.

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

The one-element average reads back as 42.5. The two-element column is refused, because a column cannot be one number.

## Example 5: Complex values keep both parts

A complex gain has a real and an imaginary part, and both must survive the trip into a series and back. This program stores a complex value and reads it back.

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

Both parts come back unchanged, so the real part and the imaginary part are each equal to what went in.

## Example 6: A boolean mask from numbers

A mask marks which readings passed a test. This program converts a column of numbers to booleans, where zero is false and anything else is true.

```cpp
// Turning a Series of numbers into a mask of true and false values. The mask marks the readings that
// passed a limit, and the conversion to boolean keeps every reading that is not zero as true.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto flags = pluss::make_series(std::vector<double>{0.0, 1.5, -2.0, 0.0}, {4}, cfg);
  const auto mask = pluss::tils::as_bool(flags, cfg);

  std::cout << "mask dtype is bool: " << (mask.dtype().scalar == pluss::ScalarType::Bool) << '\n';
  std::cout << "mask values:";
  for (bool b : mask.to_vec<bool>()) std::cout << ' ' << b;
  std::cout << '\n';
  return 0;
}
```

Output

```text
mask dtype is bool: 1
mask values: 0 1 1 0
```

The zeros become false, and the non-zero readings, including the negative one, become true.

## Example 7: Checking a type before extracting it

Not every type can be extracted, and a trait says which can. This program asks the trait about a 64-bit integer and a string, then extracts the integer with the general form.

```cpp
// to_scalar is the general form of the extraction. It names the type you want, and a trait tells you
// which types can be extracted at all. Here the program checks the traits before it asks.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

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

The integer can be extracted and the string cannot. The general form returns the integer, which is 128.

## Example 8: A sensor pipeline with typed steps

Readings are rounded to whole degrees before they are stored. This program adds half a degree, converts the column to integers, and reports a count, a sum, and an average. Each step uses one of the casts, so the type at each point is visible.

```cpp
// A small pipeline that reads raw sensor readings, rounds them to whole units for storage, and
// reports a count and an average. Each conversion is one scalar call, so the types are visible at
// each step.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto raw = pluss::make_series(std::vector<double>{21.4, 22.9, 19.2, 25.6}, {4}, cfg);

  // Round to whole degrees by adding a half and truncating, then store as integers.
  const auto shifted = raw.add(pluss::tils::as_f64(0.5, cfg));
  const auto whole = pluss::tils::as_i64(shifted, cfg);

  std::int64_t sum = 0;
  for (std::int64_t v : whole.to_vec<std::int64_t>()) sum += v;
  const auto count = pluss::make_series(std::vector<std::int64_t>{static_cast<std::int64_t>(whole.numel())}, {}, cfg);

  std::cout << "readings: " << pluss::tils::as_i64(count) << '\n';
  std::cout << "rounded sum: " << sum << '\n';
  std::cout << "rounded average: " << static_cast<double>(sum) / static_cast<double>(whole.numel()) << '\n';
  return 0;
}
```

Output

```text
readings: 4
rounded sum: 89
rounded average: 22.25
```

Four readings are rounded, their sum is 89, and their average is 22.25. The integer conversion does the rounding. For positive readings, adding a half and then truncating rounds to the nearest whole degree.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Pick an element type for a new series | a configuration from `as_f64(cfg)` or a similar call | The configuration carries the type to the factory. |
| Store one host value | `as_i64(value, cfg)` or a similar call with a value | The value keeps its type, so large integers stay exact. |
| Convert a column | `as_i64(series, cfg)` or a similar call with a series | Floats truncate toward zero when they become integers. |
| Read one result | `as_f64(series)` on a one-element series | A series with more elements is refused. |
| Check a type first | `is_scalar_extractable_v` and `to_scalar<T>` | Use the trait before you name a type you cannot extract. |

## Pitfalls

**Expecting a float to round when it becomes an integer.** s03 shows 2.9 becomes 2, so the conversion truncates. Add a half first if you want rounding, as s08 does.

**Expecting a column to read as one number.** s04 shows the two-element column is refused by the one-number form.

**Expecting a large integer to survive a double.** s02 shows a 64-bit integer keeps every digit when it is stored with its own type.

## Where next

- [Mapping with transmit](tils-aux-transmit.md) -- producing the values that are cast.
- [Mapping along an axis with translate](tils-aux-translate.md) -- casting the results of a slice function.
- [Print options](tils-print-options.md) -- printing the values once they are read back.
- [Scalar cast reference](../api/core-scalar-cast.md) -- every overload, grouped by what it takes.
