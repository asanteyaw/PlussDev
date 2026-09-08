<!-- pluss-hand-authored -->

# Timeline::sizes

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
auto Timeline::sizes() const
```

## What It Tells You

`sizes()` returns the logical extent of every axis. A scalar has no axes and
therefore reports `[]`; a vector of two instants reports `[2]`; a two-row,
two-column timeline reports `[2, 2]`.

The returned object is a lightweight read-only range. Iterate it, index it, or
construct an owning `std::vector<std::int64_t>` when the shape must outlive the
timeline. Keeping the public example type as `auto` avoids coupling
application code to the range implementation.

Shape describes arrangement, not chronology. `[2, 2]` says how positions are
addressed; it does not claim the timestamps are sorted, unique, equally spaced,
or in the same calendar period.

## Returns

A read-only range of signed 64-bit extents in outermost-to-innermost order.
Its length equals [`dim()`](dim.md), and the product of its extents equals
[`numel()`](numel.md).

## Examples

Example 1. Read scalar, vector, and matrix shapes without exposing a concrete
range type.

```cpp
auto print_shape = [](const pluss::Timeline& value) {
    std::cout << '[';
    std::size_t i = 0;
    for (auto extent : value.sizes()) {
        if (i++ != 0) std::cout << ',';
        std::cout << extent;
    }
    std::cout << ']';
};

auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto vector = pluss::timeline::make({
    "2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"
});
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});

print_shape(scalar); std::cout << "\n";
print_shape(vector); std::cout << "\n";
print_shape(matrix); std::cout << "\n";
```

### Output

```text
[]
[2]
[2,2]
```

Example 2. Take an owning copy when a separate shape value is required.

```cpp
auto times = pluss::timeline::epoch({2, 3});
std::vector<std::int64_t> shape(times.sizes().begin(), times.sizes().end());

std::cout << "rank=" << shape.size()
          << ", rows=" << shape[0]
          << ", columns=" << shape[1] << "\n";
```

### Output

```text
rank=2, rows=2, columns=3
```

Example 3. Empty axes remain part of the shape.

```cpp
auto empty = pluss::timeline::epoch({2, 0, 4});
std::cout << "dim=" << empty.dim() << ", numel=" << empty.numel() << "\n";
```

### Output

```text
dim=3, numel=0
```

## Gotchas And Failure Modes

- A scalar shape is `[]`, not `[1]`. The latter is a one-element vector.
- An extent of zero does not remove an axis; `[2, 0, 4]` still has rank `3`.
- `Timeline` supports ranks `0` through `3`; factories reject higher-rank
  shapes.
- Treat the returned range as a borrowed view. Copy it before storing it beyond
  the lifetime of its timeline.

## See Also

- [`Timeline::dim`](dim.md)
- [`Timeline::numel`](numel.md)
- [`Timeline::is_scalar`](is_scalar.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html"><span class="label">Previous</span><span class="title">shift</span></a>
  <a class="next-link" href="dim.html"><span class="label">Next</span><span class="title">dim</span></a>
</div>
