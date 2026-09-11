# Timeline::le

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::le(const Timeline& other) const
Series Timeline::operator<=(const Timeline& other) const
```

## Closing The Upper Boundary

`le()` reports positions where the left instant is earlier than or exactly
equal to the right instant. It is the upper-bound predicate for a closed
interval.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto end = pluss::timeline::make("2024-01-01T02:00:00Z");
std::cout << pluss::series::print(times <= end) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, true]
```

Compare this with `times < end`, whose second element is `false`. That one bit
is the difference between including and excluding the upper endpoint.

`NaT <= value` and `value <= NaT` are both `false`. Shapes broadcast by
trailing dimensions; incompatible shapes throw.

## Composing Interval Predicates

`le()` is commonly the right half of an interval expression. For values in
`[start, end]`, combine `values.ge(start)` and `values.le(end)` with the
boolean Series operation `logical_and()`. Do not reduce either side first:
the elementwise masks must remain aligned until they are combined.

For a half-open interval `[start, end)`, retain `ge(start)` but replace this
method with `lt(end)`. That choice is about endpoint membership, not rounding
or timestamp precision.

## Failure Modes

- Undefined operands are rejected.
- Non-broadcastable shapes are rejected.
- A scalar comparison returns a rank-zero boolean Series, not a native bool.
- An absent endpoint does not define an unbounded interval; its comparisons
  are `false`.

## See Also

- [`Timeline::lt`](lt.md)
- [`Timeline::ge`](ge.md)
- [`Timeline::where`](where.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html"><span class="label">Previous</span><span class="title">lt</span></a>
  <a class="next-link" href="leap_scalar.html"><span class="label">Next</span><span class="title">leap_scalar</span></a>
</div>
