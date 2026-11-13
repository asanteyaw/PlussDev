<!-- pluss-hand-authored -->

# Series::narrow_copy

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::narrow_copy(std::int64_t dim, std::int64_t start, std::int64_t length) const
```

## What It Does

`narrow_copy()` takes a contiguous run of `length` positions starting at
`start` along axis `dim`, and returns it as a **new materialized array** — the
same selection a slice would make, but copied rather than viewed.

For a `2 x 4` matrix, `narrow_copy(1, 2, 2)` keeps columns `2` and `3` of every
row. Every other axis is kept in full.

`start` may be negative, counting from the end of the axis. The requested run
must lie fully inside the axis: `start` out of range, a negative `length`, or
`start + length` past the end are all rejected. If you want a lenient selection
that simply stops at the axis end, use the view-returning
[`slice()`](slice.md) instead.

Use `narrow_copy()` (over [`slice()`](slice.md)) when you want the result to own
its storage — for example to hand it to code that mutates in place, or to
detach it from a large parent buffer.

## Parameters

**dim** (`std::int64_t`)

The axis to narrow. Negative values count from the back.

**start** (`std::int64_t`)

The first index to keep. Negative values count from the end of the axis. After
that adjustment it must satisfy `0 <= start <= size(dim)`.

**length** (`std::int64_t`)

How many consecutive indices to keep. Must be `>= 0`, and `start + length` must
not exceed `size(dim)`.

## Returns

`Series`
    A fresh contiguous array with axis `dim` reduced to exactly `length` and
    every other axis intact.

## Gradient

`narrow_copy()` is differentiable. The backward places the upstream gradient
into the `[start, start + length)` slice of a zero array shaped like `self`;
positions outside the kept run receive `0`.

## Examples

Example 1. Keeping the last two columns.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});

std::cout << m.narrow_copy(1, 2, 2);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 4],
  [7, 8]]
```

Example 2. A negative `start` counts from the end of the axis.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.narrow_copy(1, -2, 2);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[2, 3],
  [5, 6]]
```

Example 3. Narrowing the first axis to a single row.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});

std::cout << m.narrow_copy(0, 0, 1);
```

### Output

```text
pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2, 3, 4]]
```

Example 4. A zero `length` at the boundary is allowed and gives an empty axis.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.narrow_copy(1, 3, 0);
```

### Output

```text
pluss::Series(shape=[2,0], strides=[0,1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 5. A `start` past the axis is rejected.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).narrow_copy(1, 5, 1);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
narrow_copy: start 5 is out of range for axis 1 of size 3
```

Example 6. So is a run that starts in range but extends past the end.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).narrow_copy(1, 2, 2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
narrow_copy: start + length (2 + 2) exceeds axis 1 size 3
```

Example 7. A negative `length` is rejected.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).narrow_copy(1, 0, -1);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
narrow_copy: length must be non-negative, got -1
```

Example 8. The backward fills only the kept slice.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});
m.tracks_slope_(true);

m.narrow_copy(1, 1, 2).sum().reverse();

pluss::series::print(std::cout, m.sensitivity());
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 0.0000 1.0000 1.0000 0.0000 ⎤
⎣ 0.0000 1.0000 1.0000 0.0000 ⎦
```

Columns `1` and `2` were kept, so they carry the gradient; columns `0` and `3`
get `0`.

## Gotchas And Failure Modes

- `narrow_copy()` copies. For a zero-copy view of the same range use
  [`slice()`](slice.md).
- The requested run must fit: an out-of-range `start`, a negative `length`, or
  `start + length` past the axis end all raise. [`slice()`](slice.md) is the
  lenient alternative — it stops at the axis end instead.
- Only one axis is narrowed per call.

## Summary

`narrow_copy(dim, start, length)` is a materialized contiguous slice:

- keeps exactly `[start, start + length)` along `dim`; negative `start` counts
  from the end; the run must lie inside the axis or it raises
- always contiguous, owns its storage
- differentiable, backward fills only the kept run

## See Also

- [`Series::slice`](slice.md): the same selection as a zero-copy view, with
  lenient bounds.
- [`Series::unfold`](unfold.md): every sliding window rather than one run.
- [`Series::clone`](clone.md): copy without narrowing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanvar.html">
    <span class="label">Previous</span>
    <span class="title">nanvar</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
