# MSharp SnapPtr (intrusive reference counting)

`pluss::msharp::SnapPtr<T>` is PlussQit’s internal intrusive smart pointer used for
long-lived shared objects (e.g., `codem::SeriesImpl`).

## Why SnapPtr?

For performance and predictability.

- `std::shared_ptr` typically allocates a separate control block.
- Intrusive refcounting stores the count *inside the object*, enabling:
  - fewer allocations
  - better locality
  - cheaper copies

This design is inspired by common industry practice for tensor handles and graph nodes.

## Components

### `RefCounted`
A base class that provides a thread-unsafe (single-threaded) reference count:

- `retain()`
- `release()`
- `use_count()`

> Note: if you later require cross-thread sharing, swap `std::size_t` to `std::atomic<std::size_t>`.

### `SnapPtr<T>`
A pointer that calls `retain/release` appropriately.

### `make_snap<T>(args...)`
Allocates with `new` and returns a `SnapPtr` **without** an extra retain.

## Usage in CodeM

`Series` holds a `SnapPtr<SeriesImpl>`.
All `Series` copies are cheap; they share the same underlying implementation object.
Views share the same underlying `Storage` but may carry different `sizes/strides/storage_offset`.
