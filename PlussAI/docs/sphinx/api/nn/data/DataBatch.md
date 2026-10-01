# `pluss::loader::DataBatch`

One batch's aligned `Series` fields -- what
[`Dataset::get`](Dataset/get.md) returns for a single sample, and what
[`DataLoader`](DataLoader.md) produces for a whole batch, in exactly the
same shape. `x()`/`y()` name the first two fields by convention (input,
target); `at(index)`/`operator[]` reach any field by position.

## Signature

```cpp
pluss::loader::DataBatch::DataBatch() = default;
```

## Construction

```cpp
pluss::loader::DataBatch batch;
batch.fields.push_back(x_series);
batch.fields.push_back(y_series);
```

A plain aggregate around one public member, `fields` -- construct it
default and push fields directly, or receive one already built from
`Dataset::get`/`DataLoader` iteration.

## State

`fields` -- `vector<Series>`
: The batch's own field list, in the same order every constructing
  `Dataset`/`DataLoader` produced them. Every method on this page reads
  or indexes into this one member.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `size()` | `size_t` | [`size`](DataBatch/size.md) |
| `empty()` | `bool` | [`empty`](DataBatch/empty.md) |
| `at(index)` / `operator[](index)` | `Series&` (or `const Series&`) | [`at`](DataBatch/at.md) |
| `x()` | `Series&` (or `const Series&`) | [`x`](DataBatch/x.md) |
| `y()` | `Series&` (or `const Series&`) | [`y`](DataBatch/y.md) |
| `has_y()` | `bool` | [`has_y`](DataBatch/has_y.md) |

## Changelog

- **0.1** — a plain aligned-field-list batch, with `x()`/`y()`
  convenience accessors for the first two fields.

## See Also

- [`Dataset::get`](Dataset/get.md) -- returns one `DataBatch` per sample.
- [`DataLoader`](DataLoader.md) -- iterating it (or calling
  [`next`](DataLoader/next.md)/[`batch_at`](DataLoader/batch_at.md))
  produces one `DataBatch` per batch.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

DataBatch/Introduction
DataBatch/at
DataBatch/empty
DataBatch/has_y
DataBatch/size
DataBatch/x
DataBatch/y
```
<!-- apiref:toctree:end -->
