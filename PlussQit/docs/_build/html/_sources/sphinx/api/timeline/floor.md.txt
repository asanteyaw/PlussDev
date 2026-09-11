# Timeline::floor

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::floor(std::string_view unit) const
```

## Snap Backward In Local Time

`floor()` returns the greatest requested-unit boundary that is not later than each local wall time. It is the lower-label operation for temporal buckets and, unlike truncation toward zero, handles timestamps before 1970 correctly.

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
std::cout << pluss::timeline::print(values.floor("H")) << '\n';
```

```text
Timeline(shape=[5], tz=UTC)
[ 1969-12-31 23:00:00, 1969-12-31 23:00:00, 1970-01-01 00:00:00, 1970-01-01 01:00:00, NaT ]
```

## Example 2

```cpp
const auto t = pluss::timeline::make(
    "2024-03-10T18:00:00Z").tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(t.floor("D")) << '\n';
```

```text
Timeline(shape=[], tz=America/Edmonton)
2024-03-10 00:00:00
```

## Edge Behavior

Day flooring resolves midnight with midnight's own offset. Repeated fallback hours preserve the source occurrence. Unsupported units, overflow, and boundaries inside a spring clock gap are rejected.

All supported Timeline ranks are accepted; the operation is shape-preserving, not a reduction.

## See Also

- [`floor`](floor.md)
- [`ceil`](ceil.md)
- [`round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_nat.html"><span class="label">Previous</span><span class="title">fill_nat</span></a>
  <a class="next-link" href="from_buffer.html"><span class="label">Next</span><span class="title">from_buffer</span></a>
</div>
<!-- pluss-hand-authored -->
