# `pluss::loader::DataBatch`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## Signature

```cpp
#include <pluss/tils/auxiliary/data_loader.h>

std::vector<Series> fields;

bool empty() const noexcept;
std::size_t size() const noexcept;
Series& at(std::size_t index);
Series& operator[](std::size_t index);
Series& x();
bool has_y() const noexcept;
Series& y();
```

## What It Does

`DataBatch` holds the fields of one batch, in the order a dataset or a `DataLoader` produced them. `at` and `operator[]` read a field by position, with the same bounds check either way. `x` is a shorthand for the first field, and `y` is a shorthand for the second; `has_y` tells you whether a second field exists before you call `y`. A default-constructed batch has no fields, and reading `x` on one is refused.

## Parameters

| Parameter | Meaning |
|---|---|
| `index` | The field's position, from `at` or `operator[]`. |

## Returns

A reference to the field's `Series`. `empty` and `size` describe the field count, not any one field's shape.

## Errors

`at` and `operator[]` refuse an out-of-range index. `x` refuses a batch with no fields, and `y` refuses one with fewer than two.

## Practical Examples

### Example 1

Example 1 reads fields by position with at and operator[].

```cpp
// A batch holds one Series per field, in the order the dataset or the loader produced them. at()
// and operator[] read a field by position, with the same bounds check either way.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  loader::DataBatch batch{};
  batch.fields.push_back(pluss::make_series(std::vector<double>{1, 2, 3}, {3}, cfg));
  batch.fields.push_back(pluss::make_series(std::vector<double>{0, 1, 0}, {3}, cfg));

  std::cout << "size: " << batch.size() << '\n';
  std::cout << "at(0) numel: " << batch.at(0).numel() << '\n';
  std::cout << "operator[](1) numel: " << batch[1].numel() << '\n';
  return 0;
}
```

Output

```text
size: 2
at(0) numel: 3
operator[](1) numel: 3
```

Both read the same field the same way, and the batch reports its field count through size.

### Example 2

Example 2 reads the first two fields through x and y, and checks has_y.

```cpp
// x() is the first field, and y() is the second. has_y() tells you whether a second field exists
// before you reach for it, since a batch with only one field has no y() to call.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  loader::DataBatch two_fields{};
  two_fields.fields.push_back(pluss::make_series(std::vector<double>{1, 2}, {2}, cfg));
  two_fields.fields.push_back(pluss::make_series(std::vector<double>{9, 8}, {2}, cfg));
  std::cout << "has_y: " << two_fields.has_y() << '\n';
  std::cout << "x is the first field: " << (two_fields.x().host<double>() == two_fields.at(0).host<double>()) << '\n';
  std::cout << "y is the second field: " << (two_fields.y().host<double>() == two_fields.at(1).host<double>()) << '\n';

  loader::DataBatch one_field{};
  one_field.fields.push_back(pluss::make_series(std::vector<double>{5}, {1}, cfg));
  std::cout << "one field has_y: " << one_field.has_y() << '\n';
  return 0;
}
```

Output

```text
has_y: true
x is the first field: true
y is the second field: true
one field has_y: false
```

x and y match the fields read by position, and has_y is false for a batch with only one field.

### Example 3

Example 3 checks an empty batch and the refusal reading x gives.

```cpp
// A default-constructed batch has no fields. empty() and size() agree about that, and x() refuses
// to read a field that is not there.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <exception>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::DataBatch empty_batch{};
  std::cout << "empty: " << empty_batch.empty() << '\n';
  std::cout << "size: " << empty_batch.size() << '\n';
  bool refused = false;
  try {
    (void)empty_batch.x();
  } catch (const std::exception&) {
    refused = true;
  }
  std::cout << "reading x on an empty batch refused: " << refused << '\n';
  return 0;
}
```

Output

```text
empty: true
size: 0
reading x on an empty batch refused: true
```

A default-constructed batch is empty, has size zero, and refuses to read x.

## See Also

- [`DataLoader`](loader-data-loader.md) -- produces batches of this type.
- [`Dataset`](loader-dataset.md) -- the source a loader reads from.
