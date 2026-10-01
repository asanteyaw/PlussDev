# `pluss::loader::SeriesDataset`

A [`Dataset`](Dataset.md) backed directly by whole `Series` fields
sharing one sample dimension -- the common case, covering data that
already exists as `Series` without writing a custom `Dataset` subclass.
`materialize_fields()` simply returns the same fields the constructor was
given, so every `SeriesDataset` is immediately usable with
[`DataLoader`](DataLoader.md).

## Signature

```cpp
pluss::loader::SeriesDataset::SeriesDataset();
explicit pluss::loader::SeriesDataset::SeriesDataset(std::vector<pluss::Series> fields, std::int64_t batch_dim = 0);
pluss::loader::SeriesDataset::SeriesDataset(std::initializer_list<pluss::Series> fields, std::int64_t batch_dim = 0);
```

## Construction

```cpp
const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});

pluss::loader::SeriesDataset dataset({x, y});             // batch_dim = 0 (default)
pluss::loader::SeriesDataset dataset2({x, y}, /*batch_dim=*/0);
```

Every field must be *defined*, rank `>= 1`, and share the same size along
`batch_dim` -- construction throws otherwise (via `PLUSS_CHECK`). The
default-constructed form (`SeriesDataset()`) produces an empty dataset,
useful mainly as a placeholder before assignment.

## State

`fields()` -- `const vector<Series>&`
: The fields this dataset was constructed with, unchanged. See
  [`fields`](SeriesDataset/fields.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `size()` | `int64_t` | [`size`](SeriesDataset/size.md) |
| `get(index)` | `DataBatch` | [`get`](SeriesDataset/get.md) |
| `fields()` | `const vector<Series>&` | [`fields`](SeriesDataset/fields.md) |
| `materialize_fields()` | `vector<Series>` | [`materialize_fields`](SeriesDataset/materialize_fields.md) |
| `batch_dim()` | `int64_t` | [`batch_dim`](SeriesDataset/batch_dim.md) |

## Changelog

- **0.1** — a `Dataset` implementation backed directly by whole `Series`
  fields, with validated shared shape along `batch_dim`.

## See Also

- [`Dataset`](Dataset.md) -- the interface this implements.
- [`DataLoader`](DataLoader.md) -- the consumer every `SeriesDataset` is
  built to feed.
- [`DataBatch`](DataBatch.md) -- what `get` returns.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SeriesDataset/Introduction
SeriesDataset/batch_dim
SeriesDataset/fields
SeriesDataset/get
SeriesDataset/materialize_fields
SeriesDataset/size
```
<!-- apiref:toctree:end -->
