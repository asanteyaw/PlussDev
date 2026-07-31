# View invariants: non-overlapping & dense

PlussQit enforces (in debug builds) that certain view-producing operations only create
**non-overlapping and dense** strided layouts.

This is important for:

- safe in-place writes across views
- predictable interop with external libraries (e.g., LibTorch)
- avoiding subtle aliasing bugs where different indices map to the same memory offset

## Definition (practical)

A strided layout is treated as non-overlapping and dense when, up to permutation of dimensions,
strides match a contiguous packing rule:

- smallest non-trivial stride is 1
- next stride equals product of previous sizes, etc.

This matches how many tensor libraries define “dense, non-overlapping” views.

## Where enforced

- `SeriesImpl` constructor (debug builds)
- `from_torch` conversion requires a non-overlapping & dense tensor

For operations that can produce general strided views (like `transpose`), we still allow them,
but in-place operations that combine aliasing + broadcasting may conservatively materialize RHS.
