<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c5e3fc795627 -->

# `pluss::loader::Dataset::get`

[Back to `pluss::loader::Dataset`](../Dataset.md)

## Signature

```cpp
virtual pluss::loader::DataBatch pluss::loader::Dataset::get(std::int64_t index) const = 0;
```

## What It Does

Returns the single sample at `index`, wrapped in a
[`DataBatch`](../DataBatch.md) of one sample each (not a batch of
several). Pure virtual -- every subclass must implement it. Callable
directly, with no `DataLoader` involved at all, for code that wants one
sample on its own. `DataLoader(const Dataset&, options)` also reaches for
`get()` itself, but only as a fallback: when a subclass overrides
[`materialize_fields()`](materialize_fields.md), that vectorized path
supplies every batch instead, and `get()` is never called internally --
see [`materialize_fields`](materialize_fields.md) for exactly which path
runs when.

## Parameters

`std::int64_t index`
: The sample index, in `[0, size())`.

## Returns

`DataBatch` -- one `Series` per field, each with a single-sample shape
(no batch axis of size `> 1`).

## Errors

None from the base contract; a well-behaved subclass throws (via
`PLUSS_CHECK` or equivalent) for `index` outside `[0, size())`, the same
convention [`SeriesDataset::get`](../SeriesDataset/get.md) follows.

## Practical Examples

Three examples: a baseline in-bounds call, an out-of-bounds call against a
well-behaved implementation, and confirming `get()`'s result is
independent of whatever `DataLoader` batching is separately configured.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

class ScalarDataset final : public pluss::loader::Dataset {
public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }

  pluss::loader::DataBatch get(std::int64_t index) const override {
    PLUSS_CHECK(index >= 0 && index < size(), "ScalarDataset::get: index out of range");
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }

  std::vector<pluss::Series> materialize_fields() const override {
    return {pluss::make_series(values_, {static_cast<std::int64_t>(values_.size())})};
  }

private:
  std::vector<double> values_;
};

int main() {
  ScalarDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});
  std::cout << "get(2).x() = " << dataset.get(2).x().item<double>() << "\n";
  std::cout << "get(0).x() = " << dataset.get(0).x().item<double>() << "\n";
  return 0;
}
```

Output

```text
get(2).x() = 30
get(0).x() = 10
```

### Example 2

Calling `get()` with an out-of-bounds index against the same
well-behaved implementation throws, via the same `PLUSS_CHECK` idiom
every dataset/loader type in this family uses.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>
#include <vector>

class ScalarDataset final : public pluss::loader::Dataset {
public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }

  pluss::loader::DataBatch get(std::int64_t index) const override {
    PLUSS_CHECK(index >= 0 && index < size(), "ScalarDataset::get: index out of range");
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }

  std::vector<pluss::Series> materialize_fields() const override {
    return {pluss::make_series(values_, {static_cast<std::int64_t>(values_.size())})};
  }

private:
  std::vector<double> values_;
};

int main() {
  ScalarDataset dataset({10.0, 20.0, 30.0});
  try {
    dataset.get(5);
  } catch (const std::exception& e) {
    // e.what() carries a second "Triggered at: ..." line with the throw
    // site's source location; only the message itself is printed here.
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
caught: ScalarDataset::get: index out of range
```

### Example 3

`get()` reads directly from the dataset's own storage, independent of any
`DataLoader` constructed over it -- calling `get()` before or after
constructing a `DataLoader` (or with different `DataLoaderOptions`)
returns the identical value.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

class ScalarDataset final : public pluss::loader::Dataset {
public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }
  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }
  std::vector<pluss::Series> materialize_fields() const override {
    return {pluss::make_series(values_, {static_cast<std::int64_t>(values_.size())})};
  }

private:
  std::vector<double> values_;
};

int main() {
  ScalarDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});
  std::cout << "get(3) before constructing a DataLoader: " << dataset.get(3).x().item<double>() << "\n";

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 2;
  options.shuffle = true;
  pluss::loader::DataLoader data_loader(dataset, options);
  std::cout << "get(3) after constructing a shuffled DataLoader: " << dataset.get(3).x().item<double>() << "\n";

  return 0;
}
```

Output

```text
get(3) before constructing a DataLoader: 40
get(3) after constructing a shuffled DataLoader: 40
```

## See Also

- [`size`](size.md) -- the valid index range for `get()`.
- [`DataBatch`](../DataBatch.md) -- the type `get()` returns.
- [`materialize_fields`](materialize_fields.md) -- the faster path
  `DataLoader` prefers when a subclass overrides it; `get()` is the
  fallback otherwise.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_dim.html">
    <span class="label">Previous</span>
    <span class="title">batch_dim</span>
  </a>
  <a class="next-link" href="materialize_fields.html">
    <span class="label">Next</span>
    <span class="title">materialize_fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
