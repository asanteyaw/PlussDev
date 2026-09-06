<!-- pluss-hand-authored -->

# Timeline::dim

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::dim() const
```

## What It Tells You

`dim()` returns the rank: the number of axes needed to locate one temporal
position. A scalar instant has rank `0`, a temporal vector has rank `1`, and a
rectangular calendar table has rank `2`.

Rank and element count answer different questions. A shape `[1]` has one axis
and one element; it is still a vector. A shape `[2, 0, 4]` has three axes and
no elements. Use [`numel()`](numel.md) for cardinality and
[`is_scalar()`](is_scalar.md) when the scalar contract matters.

## Returns

The number of logical axes as `std::int64_t`. A valid `Timeline` is restricted
to ranks `0` through `3`.

## Examples

Example 1. Nesting determines rank.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto vector = pluss::timeline::make({
    "2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"
});
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});

std::cout << "scalar dim=" << scalar.dim() << "\n"
          << "vector dim=" << vector.dim() << "\n"
          << "matrix dim=" << matrix.dim() << "\n";
```

### Output

```text
scalar dim=0
vector dim=1
matrix dim=2
```

Example 2. One value does not imply rank zero.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto singleton = pluss::timeline::make({"2024-02-29T12:30:00Z"});

std::cout << "scalar: dim=" << scalar.dim()
          << ", numel=" << scalar.numel() << "\n"
          << "singleton: dim=" << singleton.dim()
          << ", numel=" << singleton.numel() << "\n";
```

### Output

```text
scalar: dim=0, numel=1
singleton: dim=1, numel=1
```

## Gotchas And Failure Modes

- `dim() == 0` means scalar, not empty.
- `dim() == 1` says only that one coordinate is required; it says nothing
  about the vector's length, ordering, or uniqueness.
- Rank is structural. Changing timezone metadata does not change it.
- Factories reject shapes with more than three axes.

## Summary

Use `dim()` to reason about address structure, `sizes()` for each axis extent,
and `numel()` for total cardinality.

## See Also

- [`Timeline::sizes`](sizes.md)
- [`Timeline::numel`](numel.md)
- [`Timeline::is_scalar`](is_scalar.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html"><span class="label">Previous</span><span class="title">sizes</span></a>
  <a class="next-link" href="numel.html"><span class="label">Next</span><span class="title">numel</span></a>
</div>
