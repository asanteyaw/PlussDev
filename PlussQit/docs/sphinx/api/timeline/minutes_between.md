# Timeline::minutes_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::minutes_between(const Timeline& other) const
```

## Count Complete Elapsed Minutes

`minutes_between()` divides the signed epoch-nanosecond difference between two
scalar instants by exactly 60,000,000,000. It returns only complete minutes and
truncates any seconds or fractional nanoseconds toward zero. The result is a
native `std::int64_t`, so it can be used directly in scalar scheduling and
timeout logic.

The operation measures instants rather than displayed clock fields. Different
timezone labels are allowed, and two differently displayed values return zero
when they represent the same instant.

## Example: A Composite Interval In Minutes

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");

std::cout << start.minutes_between(end) << '\n';
```

### Output

```text
1563
```

One day contributes 1,440 minutes, two hours contribute 120, and three more
minutes produce 1,563. The final 4.5 seconds are incomplete and are discarded.

## Example: Reverse Direction Truncates Toward Zero

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");

std::cout << end.minutes_between(start) << '\n';
```

### Output

```text
-1563
```

The discarded 4.5 seconds do not force the result down to `-1564`. This
symmetry makes `a.minutes_between(b) == -b.minutes_between(a)` whenever the
mathematical quotient is representable.

## Example: Invalid Scalar Distance

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");

try {
  (void)start.minutes_between(pluss::timeline::make(""));
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

`NaT` has no elapsed distance, and vectors or matrices are not implicitly
reduced. Use [`span()`](span.md) for elementwise or broadcasted arrays. This
scalar method is constant-time, allocation-free, overflow-safe, and leaves both
operands unchanged.

## See Also

- [`hours_between`](hours_between.md): count complete elapsed hours.
- [`seconds_between`](seconds_between.md): retain completed seconds.
- [`add_minutes`](add_minutes.md): move by a fixed number of minutes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute_scalar.html"><span class="label">Previous</span><span class="title">minute_scalar</span></a>
  <a class="next-link" href="month.html"><span class="label">Next</span><span class="title">month</span></a>
</div>
<!-- pluss-hand-authored -->
