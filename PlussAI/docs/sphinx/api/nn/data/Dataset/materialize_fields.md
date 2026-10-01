<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c73cc8b598d0 -->

# `pluss::loader::Dataset::materialize_fields`

[Back to `pluss::loader::Dataset`](../Dataset.md)

## Signature

```cpp
virtual std::vector<pluss::Series> pluss::loader::Dataset::materialize_fields() const;
```

## What It Does

Returns this dataset's data as a flat list of whole `Series` fields,
sharing a sample dimension named by [`batch_dim()`](batch_dim.md) --
exactly the shape [`DataLoader`](../DataLoader.md) needs to batch via
`index_select`, its own fast, vectorized path. Defaults to returning an
empty vector. `DataLoader(const Dataset&, options)` calls it once, at
construction: when a subclass overrides it to return real fields, those
become every batch's source; when it is left at the unimplemented
default, `DataLoader` falls back to calling [`get`](get.md) once per
sample instead and assembling the equivalent fields from that (see
Example 2) -- slower, but means the plain `size`/`get`-only subclass
every `Dataset` already has to implement is immediately usable with
`DataLoader`, with nothing else required.

## Parameters

None.

## Returns

`std::vector<Series>` -- one `Series` per field, each sized `size()`
along axis `batch_dim()`, all sharing that same size on that axis. Empty
by default, in which case `DataLoader` falls back to `get()` instead of
using this method's result.

## Errors

None from `materialize_fields()` itself. `DataLoader` construction throws
only if *both* this and `get()` fail to produce any fields at all (see
Example 2's counterpart on [`get`](get.md)).

## Practical Examples

Three examples: overriding it to return real fields directly, a subclass
that leaves it at the unimplemented default and is still usable via the
`get()` fallback, and confirming that when both are implemented,
`materialize_fields()` wins and `get()` is never called.

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
  const auto fields = dataset.materialize_fields();
  std::cout << "materialize_fields().size() = " << fields.size() << "\n";
  std::cout << "fields[0] = " << fields[0] << "\n";
  return 0;
}
```

Output

```text
materialize_fields().size() = 1
fields[0] = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10, 20, 30, 40, 50]
```

### Example 2

A `Dataset` subclass that implements `size`/`get` but never overrides
`materialize_fields()` -- `DataLoader(const Dataset&, options)` still
succeeds, via the `get()` fallback: construction gathers every sample by
calling `get(i)` for `i` in `[0, size())` and assembles the equivalent
fields from those, producing identical batches to what an overridden
`materialize_fields()` would have.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

class GetOnlyDataset final : public pluss::loader::Dataset {
public:
  explicit GetOnlyDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }
  pluss::loader::DataBatch get(std::int64_t index) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(values_[static_cast<std::size_t>(index)]));
    return batch;
  }
  // materialize_fields() intentionally left at its default (empty).

private:
  std::vector<double> values_;
};

int main() {
  GetOnlyDataset dataset({10.0, 20.0, 30.0, 40.0, 50.0});

  pluss::loader::DataLoaderOptions options{};
  options.batch_size = 2;
  pluss::loader::DataLoader data_loader(dataset, options);

  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
batch: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 20]
batch: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[30, 40]
batch: pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[50]
```

### Example 3

When both are implemented, `materialize_fields()` takes priority --
`DataLoader`'s batches come from its `index_select` slices, and `get()`
(deliberately implemented here to return an obviously wrong constant)
never runs.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <vector>

class ScalarDataset final : public pluss::loader::Dataset {
public:
  explicit ScalarDataset(std::vector<double> values) : values_(std::move(values)) {}
  std::int64_t size() const override { return static_cast<std::int64_t>(values_.size()); }

  pluss::loader::DataBatch get(std::int64_t) const override {
    pluss::loader::DataBatch batch;
    batch.fields.push_back(pluss::make_series(-999.0));  // deliberately wrong -- proves get() is unused
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
  pluss::loader::DataLoader data_loader(dataset);
  for (auto batch : data_loader) {
    std::cout << "batch: " << batch.x() << "\n";
  }
  return 0;
}
```

Output

```text
batch: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 20, 30]
```

## See Also

- [`batch_dim`](batch_dim.md) -- the axis every field shares its sample
  count on.
- [`get`](get.md) -- the per-sample fallback `DataLoader` uses when this
  method is left at its default.
- [`DataLoader`](../DataLoader.md) -- the actual consumer of this method's
  result.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
