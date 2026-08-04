<!-- pluss-hand-authored -->

# Duration::add

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::operator+(const Duration& other) const;
Duration& Duration::operator+=(const Duration& other);
```

## What It Does

`Duration + Duration` combines two elapsed-time values elementwise,
broadcasting via the same `SeriesIterator` machinery every other N-D op in
Pluss uses. `+=` is ordinary compound-assignment sugar around the binary
operator — it mutates `self` in place and returns `Duration&`, rather than
building a new `Duration` and reassigning.

## Parameters

**other** (`const Duration&`)

The other duration. Broadcasts against `self` under ordinary N-D rules.

## Returns

`Duration` for `operator+`
    A new duration holding the elementwise sum.

`Duration&` for `operator+=`
    A reference to the (now-mutated) `self`.

## Examples

Example 1. Add two durations built from different units.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::duration::print(ninety_min + one_hour);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 02:30:00.000000000
```

`90 min + 1 h = 2 h 30 min`. `pluss::duration::print` (or `std::cout <<
result` directly, since `Duration` has an `operator<<`) is usually the most
direct way to read a `Duration` result — no unit conversion needed the way
[`total().hours()`](total_hours.md) requires.

Example 2. `+=` mutates `self` in place and is verified identical to the
binary operator it wraps.

```cpp
auto compound_add = one_hour;
compound_add += ninety_min;
std::cout << "sum=" << pluss::print_series(compound_add.total().hours()) << "\n";
std::cout << "matches_binary=" << pluss::print_series(compound_add.eq(one_hour + ninety_min));
```

### Output

```text
sum=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.5000
matches_binary=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Notes

- `Duration` also combines with `Timeline`: `Timeline + Duration` also exists,
  documented on [`Timeline::add_duration`](../timeline/add_duration.md).
- There is no `Duration + <plain number>` overload — adding always combines
  two `Duration`s. To scale a single `Duration` by a plain count, use
  [`mul`](mul.md).

## See Also

- [`sub`](sub.md): the complementary operation.
- [`neg`](neg.md): unary negation, useful for turning `a + b` into `a - (-b)`
  style expressions.
- [`construction`](construction.md): building the operands used here.
- [`Timeline::add_duration`](../timeline/add_duration.md): the `Timeline`-side
  use of `Duration` arithmetic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="construction.html">
    <span class="label">Previous</span>
    <span class="title">construction</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
