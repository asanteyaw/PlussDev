# Timeline::round

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::round(std::string_view unit) const
```

## Choose The Nearest Local Boundary

`round()` selects the nearest requested-unit boundary in local wall time. Exact halfway cases use half-even, choosing the neighboring boundary with an even index and avoiding systematic upward bias.

## Units

| Accepted spellings | Boundary |
|---|---|
| `S / s / sec / second` | second |
| `min / m / minute` | minute |
| `H / h / hour` | hour |
| `D / d / day` | local day |

## Result Contract

The method allocates a new Timeline while preserving rank, shape, timezone, ordering, and `NaT` positions. The receiver is not modified. Boundaries are computed in local wall time and then resolved in the current timezone.

## Example 1

```cpp
const auto values = pluss::timeline::make({
    "1969-12-31T23:29:30.123456789Z", "1969-12-31T23:30:00Z",
    "1970-01-01T00:30:00Z", "1970-01-01T01:30:00Z", "",
});
std::cout << pluss::timeline::print(values.round("H")) << '\n';
```

```text
Timeline(shape=[5], tz=UTC)
[ 1969-12-31 23:00:00, 1970-01-01 00:00:00, 1970-01-01 00:00:00, 1970-01-01 02:00:00, NaT ]
```

## Example 2

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-01-01T00:29:00Z", "2024-01-01T00:31:00Z"},
    {"2024-01-01T01:29:00Z", ""},
});
std::cout << pluss::timeline::print(matrix.round("H")) << '\n';
```

```text
Timeline(shape=[2, 2], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-01 01:00:00 ]
[ 2024-01-01 01:00:00                 NaT ]
```

## Edge Behavior

Half-even applies symmetrically on both sides of the epoch. Unsupported units, overflow, and nonexistent local boundaries are rejected.

All supported Timeline ranks are accepted; the operation is shape-preserving, not a reduction.

## See Also

- [`floor`](floor.md)
- [`ceil`](ceil.md)
- [`round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeat.html"><span class="label">Previous</span><span class="title">repeat</span></a>
  <a class="next-link" href="searchsorted_left.html"><span class="label">Next</span><span class="title">searchsorted_left</span></a>
</div>
<!-- pluss-hand-authored -->
