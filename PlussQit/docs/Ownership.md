# Ownership & lifetime

PlussQit uses an intrusive, atomic reference-counted pointer called `pluss::msharp::SnapPtr<T>`.

## Why intrusive refcounting?

- **Cheap**: shared ownership is a single atomic counter stored inside the object.
- **Stable ABI**: avoids `std::shared_ptr` control block allocation and allows precise layout.
- **View semantics**: `Series` objects share underlying storage/impl; copying a Series is cheap.

## Rules

- Internal shared objects derive from `pluss::msharp::RefCounted`.
- Use `msharp::SnapPtr<T>` for shared ownership.
- Use `msharp::make_snap<T>(...)` to create objects (adopts the initial refcount).

## Thread safety

`RefCounted` uses an atomic counter with release/acquire on the last release to ensure
all prior writes are visible before destruction.
