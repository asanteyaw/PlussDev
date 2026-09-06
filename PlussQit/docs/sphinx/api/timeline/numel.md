<!-- pluss-hand-authored -->

# Timeline::numel

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::numel() const
```

## What It Tells You

`numel()` returns the number of logical timestamp positions, equal to the
product of all entries in [`sizes()`](sizes.md). It counts positions, including
positions that hold `NaT`; it does not count only valid timestamps.

A scalar contains one position, so its count is `1` even though its shape is
empty. If any shape extent is zero, the product and therefore the element count
is zero.

This is the right query for allocation sizing, flat export lengths, loops over
all positions, and emptiness tests. It does not tell you rank or how values are
arranged.

## Returns

The logical number of positions as `std::int64_t`.

## Examples

Example 1. Compare rank and cardinality across common shapes.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});

std::cout << "scalar: dim=" << scalar.dim()
          << ", numel=" << scalar.numel() << "\n"
          << "matrix: dim=" << matrix.dim()
          << ", numel=" << matrix.numel() << "\n";
```

### Output

```text
scalar: dim=0, numel=1
matrix: dim=2, numel=4
```

The matrix count is four even though one position is `NaT`: missingness is a
value property, not an absent array slot.

Example 2. Test emptiness by cardinality.

```cpp
auto empty = pluss::timeline::epoch({2, 0, 4});
std::cout << std::boolalpha
          << "dim=" << empty.dim()
          << ", numel=" << empty.numel()
          << ", empty=" << (empty.numel() == 0) << "\n";
```

### Output

```text
dim=3, numel=0, empty=true
```

Example 3. Flat epoch export has exactly `numel()` entries.

```cpp
auto times = pluss::timeline::epoch({2, 3});
auto ticks = times.to_epoch_nanos_vector();
std::cout << "numel=" << times.numel()
          << ", exported=" << ticks.size() << "\n";
```

### Output

```text
numel=6, exported=6
```

## Gotchas And Failure Modes

- `numel() == 1` does not prove that the timeline is scalar. Shapes `[1]` and
  `[1, 1]` also contain one position.
- `NaT` contributes one to `numel()` like every other stored timestamp.
- `numel()` is not a byte count. Epoch export stores each position as one
  `std::int64_t`.
- Use `numel() == 0`, not `dim() == 0`, to test emptiness.

## See Also

- [`Timeline::sizes`](sizes.md)
- [`Timeline::dim`](dim.md)
- [`Timeline::to_epoch_nanos_vector`](to_epoch_nanos_vector.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dim.html"><span class="label">Previous</span><span class="title">dim</span></a>
  <a class="next-link" href="is_scalar.html"><span class="label">Next</span><span class="title">is_scalar</span></a>
</div>
