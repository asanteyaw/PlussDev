# Timeline::ceil

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::ceil(std::string_view unit) const
```

## Snap Forward In Local Time

`ceil()` returns the least requested-unit boundary that is not earlier than each local wall time. Already aligned values remain unchanged; use it for upper bucket boundaries rather than for a strictly-later boundary.

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
std::cout << pluss::timeline::print(values.ceil("H")) << '\n';
```

```text
Timeline(shape=[5], tz=UTC)
[ 1970-01-01 00:00:00, 1970-01-01 00:00:00, 1970-01-01 01:00:00, 1970-01-01 02:00:00, NaT ]
```

## Example 2

```cpp
const auto spring = pluss::timeline::make(
    "2024-03-10T08:30:00Z").tz_convert("America/Edmonton");
try { (void)spring.ceil("H"); }
catch (const std::exception&) { std::cout << "rejected\n"; }
```

```text
rejected
```

## Edge Behavior

A nominal boundary inside a spring-forward gap is rejected rather than shifted silently. Unsupported units and nanosecond overflow also fail; `NaT` propagates.

All supported Timeline ranks are accepted; the operation is shape-preserving, not a reduction.

## See Also

- [`floor`](floor.md)
- [`ceil`](ceil.md)
- [`round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argsort.html"><span class="label">Previous</span><span class="title">argsort</span></a>
  <a class="next-link" href="concat.html"><span class="label">Next</span><span class="title">concat</span></a>
</div>
<!-- pluss-hand-authored -->
