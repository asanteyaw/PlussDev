<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ac1340edc12c -->

# `pluss::loader::DataBatch::y`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
pluss::Series& pluss::loader::DataBatch::y();
const pluss::Series& pluss::loader::DataBatch::y() const;
```

## What It Does

Returns the second field -- `at(1)`, under the "target" name by
convention. Unlike [`x()`](x.md), a batch is not guaranteed to have a
second field (a single-field, `x`-only dataset produces batches `y()`
cannot serve) -- check [`has_y()`](has_y.md) first when the field count
is not already known to be `>= 2`.

## Parameters

None.

## Returns

`Series&` (or `const Series&`) -- a reference into the batch's own
storage.

## Errors

- Throws if the batch has fewer than two fields.

## Practical Examples

Three examples: `y()` on a two-field batch, `y()` throwing on a
single-field batch, and `x()`/`y()` read together from a real `DataLoader`
iteration.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  batch.fields.push_back(pluss::make_series({1.0, 2.0}, {2}));
  batch.fields.push_back(pluss::make_series({0.0, 1.0}, {2}));

  std::cout << "y() = " << batch.y() << "\n";

  return 0;
}
```

Output

```text
y() = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch one_field;
  one_field.fields.push_back(pluss::make_series({5.0}, {1}));

  try {
    one_field.y();
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::DataBatch::y: batch does not contain a second field
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});

  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x, y}, options);

  for (auto batch : data_loader) {
    std::cout << "y() = " << batch.y() << "\n";
  }

  return 0;
}
```

Output

```text
y() = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
y() = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

## See Also

- [`has_y`](has_y.md) -- check before calling `y()` on a batch that might
  have fewer than two fields.
- [`x`](x.md) -- the always-safe first-field counterpart.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="x.html">
    <span class="label">Previous</span>
    <span class="title">x</span>
  </a>
</div>
<!-- apiref:nav:end -->
