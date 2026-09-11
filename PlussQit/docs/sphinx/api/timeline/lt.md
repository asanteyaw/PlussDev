# Timeline::lt

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::lt(const Timeline& other) const
Series Timeline::operator<(const Timeline& other) const
```

## Instants Before A Boundary

`lt()` reports where the left instant occurs strictly before the right instant.
It compares epoch instants, so timezone presentation does not affect ordering.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto boundary = pluss::timeline::make("2024-01-01T02:00:00Z");
std::cout << pluss::series::print(times < boundary) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

The instant exactly at `02:00` is excluded. Choose [`le()`](le.md) when the
boundary belongs to the interval.

When both operands have shape, comparison is positional after broadcasting.
It does not sort either operand or compare only extrema. A `[2, 1]` Timeline
and a `[3]` Timeline produce a `[2, 3]` mask.

Any comparison involving `NaT` is `false`; an absent instant is unordered.

## Failure Modes

- Incompatible trailing dimensions are rejected.
- Do not use a multi-element result as one control-flow condition; reduce it.
- `lt()` never mutates or reorders either operand.

## See Also

- [`Timeline::le`](le.md)
- [`Timeline::gt`](gt.md)
- [`Timeline::sort`](sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leap_scalar.html"><span class="label">Previous</span><span class="title">leap_scalar</span></a>
  <a class="next-link" href="le.html"><span class="label">Next</span><span class="title">le</span></a>
</div>
