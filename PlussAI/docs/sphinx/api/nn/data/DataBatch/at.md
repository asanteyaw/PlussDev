<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f05dfa378aab -->

# `pluss::loader::DataBatch::at`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
pluss::Series& pluss::loader::DataBatch::at(std::size_t index);
const pluss::Series& pluss::loader::DataBatch::at(std::size_t index) const;
pluss::Series& pluss::loader::DataBatch::operator[](std::size_t index);
const pluss::Series& pluss::loader::DataBatch::operator[](std::size_t index) const;
```

## What It Does

Returns the field at `index`, by reference. `operator[]` is a plain
forwarding alias for `at` -- both check bounds identically; neither is an
unchecked fast path. Reach for `at`/`operator[]` directly on a batch with
more than two fields, or where the first two fields are not meaningfully
"input, target" -- [`x()`](x.md)/[`y()`](y.md) are convenience wrappers
over `at(0)`/`at(1)` for the common two-field case.

## Parameters

`std::size_t index`
: The field position, in `[0, size())`.

## Returns

`Series&` (or `const Series&` on a `const` batch) -- a reference into the
batch's own `fields` storage; mutating through it mutates the batch.

## Errors

- `index >= size()` throws.

## Practical Examples

Three examples: indexing a multi-field batch, confirming `operator[]`
and `at` reach the identical object, and an out-of-range index.

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
  batch.fields.push_back(pluss::make_series({9.0, 9.0}, {2}));

  std::cout << "at(2) = " << batch.at(2) << "\n";

  return 0;
}
```

Output

```text
at(2) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[9, 9]
```

### Example 2

`operator[]` and `at` reach the exact same underlying `Series` object --
`&batch[0] == &batch.at(0)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  batch.fields.push_back(pluss::make_series({1.0, 2.0}, {2}));

  std::cout << "&batch[0] == &batch.at(0): " << std::boolalpha << (&batch[0] == &batch.at(0)) << "\n";

  return 0;
}
```

Output

```text
&batch[0] == &batch.at(0): true
```

### Example 3

An out-of-range index throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  batch.fields.push_back(pluss::make_series({1.0}, {1}));

  try {
    batch.at(5);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::DataBatch::at: index out of range
```

## See Also

- [`size`](size.md) -- valid indices are `[0, size())`.
- [`x`](x.md) / [`y`](y.md) -- convenience wrappers over `at(0)`/`at(1)`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
