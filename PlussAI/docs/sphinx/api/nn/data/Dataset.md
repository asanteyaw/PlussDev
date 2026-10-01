# `pluss::loader::Dataset`

The map-style dataset interface every source [`DataLoader`](DataLoader.md)
batches from ultimately satisfies -- either directly (a custom subclass)
or through [`SeriesDataset`](SeriesDataset.md), the concrete
implementation already covering the common "data is already whole
`Series` fields" case. Two pure virtual methods every subclass must
implement (`size`, `get`), and two with default implementations a
subclass overrides to opt into `DataLoader`'s vectorized batching
(`materialize_fields`, `batch_dim`).

## Signature

```cpp
virtual pluss::loader::Dataset::~Dataset() = default;
virtual std::int64_t pluss::loader::Dataset::size() const = 0;
virtual pluss::loader::DataBatch pluss::loader::Dataset::get(std::int64_t index) const = 0;
virtual std::vector<pluss::Series> pluss::loader::Dataset::materialize_fields() const;
virtual std::int64_t pluss::loader::Dataset::batch_dim() const noexcept;
```

`Dataset` has no public constructor of its own -- it is an abstract
interface; construct a concrete subclass instead.

## Construction

Not applicable to `Dataset` itself. Subclass it and implement `size`/
`get` (required) -- that alone is enough to use the subclass with
`DataLoader`, via its per-sample `get()` fallback; additionally override
`materialize_fields` once the subclass's data can be produced as whole
`Series` up front, for `DataLoader`'s faster vectorized batching path
(see [`materialize_fields`](Dataset/materialize_fields.md)). See
[`SeriesDataset`](SeriesDataset.md) for the concrete implementation this
reference's own `DataLoader` examples use most often instead of a custom
subclass.

## State

None on `Dataset` itself -- every observable field belongs to a concrete
subclass.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `size()` | `int64_t` | [`size`](Dataset/size.md) |
| `get(index)` | `DataBatch` | [`get`](Dataset/get.md) |
| `materialize_fields()` | `vector<Series>` | [`materialize_fields`](Dataset/materialize_fields.md) |
| `batch_dim()` | `int64_t` | [`batch_dim`](Dataset/batch_dim.md) |

## Changelog

- **0.1** — the map-style dataset contract: `size`/`get` required,
  `materialize_fields`/`batch_dim` overridable for vectorized batching.

## See Also

- [`SeriesDataset`](SeriesDataset.md) -- the concrete implementation for
  data that already exists as whole `Series` fields.
- [`DataLoader`](DataLoader.md) -- the consumer every `Dataset` subclass
  exists to feed.
- [`DataBatch`](DataBatch.md) -- what `get` returns.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Dataset/Introduction
Dataset/batch_dim
Dataset/get
Dataset/materialize_fields
Dataset/size
```
<!-- apiref:toctree:end -->
