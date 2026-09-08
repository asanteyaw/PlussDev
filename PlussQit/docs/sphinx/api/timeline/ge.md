# Timeline::ge

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::ge(const Timeline& other) const
Series Timeline::operator>=(const Timeline& other) const
```

## Closing The Lower Boundary

`ge()` reports positions at or after the right-hand instant. It is the
inclusive counterpart to `gt()`.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto start = pluss::timeline::make("2024-01-01T02:00:00Z");
std::cout << pluss::series::print(times >= start) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

To form a closed interval, combine `times >= start` with `times <= end` using
boolean `Series` operations. Keep the result as a mask; call `all()` or `any()`
only when the algorithm asks one aggregate question.

`NaT >= value` and `value >= NaT` are `false`. Non-broadcastable shapes are
rejected.

## Building A Closed Interval

The two inclusive relations compose directly into a closed interval mask:

```cpp
const auto samples = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
    "2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"});
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-01T02:00:00Z");
const auto inside = (samples >= start).logical_and(samples <= end);
std::cout << pluss::series::print(inside) << '\n';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, false]
```

Change either inclusive operator to its strict counterpart to open that side
of the interval. The mask remains shape-preserving and can be passed directly
to a low-level selection operation.

## See Also

- [`Timeline::gt`](gt.md)
- [`Timeline::le`](le.md)
- [`Timeline operators`](operators.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html"><span class="label">Previous</span><span class="title">gt</span></a>
  <a class="next-link" href="isnat.html"><span class="label">Next</span><span class="title">isnat</span></a>
</div>
