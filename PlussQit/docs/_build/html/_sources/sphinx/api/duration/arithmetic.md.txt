<!-- pluss-hand-authored -->

# Duration arithmetic

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::operator+(const Duration& other) const;
Duration Duration::operator-(const Duration& other) const;
Duration Duration::operator-() const;
Duration& Duration::operator+=(const Duration& other);
Duration& Duration::operator-=(const Duration& other);

Duration Duration::mul(std::int64_t k) const;
Duration Duration::operator*(std::int64_t k) const;
Duration Duration::div(std::int64_t k) const;
Duration Duration::operator/(std::int64_t k) const;
Duration Duration::floordiv(std::int64_t k) const;
Duration Duration::mod(std::int64_t k) const;
```

## What It Does

`Duration + Duration` and `Duration - Duration` combine two elapsed-time
values elementwise, broadcasting via the same `SeriesIterator` machinery every
other N-D op in Pluss uses. Unary `-Duration` negates (flips "forward in time"
to "backward in time" and back). `+=`/`-=` are ordinary compound-assignment
sugar around the two binary operators.

`mul`/`div`/`floordiv`/`mod` scale a `Duration` by a plain integer count `k` —
not by another `Duration` — since "a duration times a duration" isn't a
meaningful elapsed-time value (compare: `Series * Series` is fine because both
sides are unitless numbers; `Duration * Duration` would need to be an
elapsed-time-squared, which nothing here represents).

## Parameters

**other** (`const Duration&`)

The other duration, for the binary operators. Broadcasts against `self` under
ordinary N-D rules.

**k** (`std::int64_t`)

A plain integer scale/divisor/modulus. `mod`'s `k` is a raw nanosecond count
(not "a number of days" or similar), so computing "remainder after dividing by
3 days" means passing `3 * 86'400 * 1'000'000'000LL`, not `3` — see Example 3.

## Returns

`Duration`
    A new duration holding the elementwise result. `operator+=`/`operator-=`
    instead mutate `self` and return `Duration&`.

## Examples

Example 1. Add and subtract two durations built from different units.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
std::cout << "sum=" << pluss::print_series((ninety_min + one_hour).total_hours()) << "\n";
std::cout << "diff=" << pluss::print_series((one_day - one_hour).total_hours());
```

### Output

```text
sum=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.5000
diff=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=23.0000
```

`90 min + 1 h = 2.5 h`; `1 day - 1 h = 23 h`.

Example 2. Unary negation and scalar `mul`/`div`.

```cpp
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
std::cout << "neg=" << pluss::print_series((-one_hour).total_hours()) << "\n";
std::cout << "times3=" << pluss::print_series((one_hour * 3).total_hours()) << "\n";
std::cout << "div4=" << pluss::print_series((one_day / 4).total_hours());
```

### Output

```text
neg=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-1.0000
times3=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.0000
div4=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=6.0000
```

`-1 h`; `1 h * 3 = 3 h`; `1 day / 4 = 6 h`.

Example 3. `mod` against a raw nanosecond divisor, over a `Duration` array.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
constexpr std::int64_t kThreeDaysNanos = 3LL * 86'400 * 1'000'000'000LL;
std::cout << pluss::print_series(ranged.mod(kThreeDaysNanos).total_days());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 2.0000, 1.0000, 0.0000, 2.0000, 1.0000]
```

`[0, 2, 4, 6, 8, 10]` days each mod `3` days, elementwise: `0, 2, 1, 0, 2, 1`.
`k` had to be `3 * 86'400 * 1'000'000'000` (three days' worth of nanoseconds),
not the bare integer `3` — passing `3` would compute "days mod 3 nanoseconds,"
not "days mod 3 days."

Example 4. `+=`/`-=` mutate `self` in place and are verified identical to
the binary operators they wrap.

```cpp
auto compound_add = one_hour;
compound_add += ninety_min;
std::cout << "sum=" << pluss::print_series(compound_add.total_hours()) << "\n";
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

- `mul`/`div`/`floordiv`/`mod` all take a plain `std::int64_t`, not a
  `Duration` — there is no "divide one duration by another" operation on this
  surface (that would produce a unitless ratio, not a `Duration`, and isn't
  implemented here).
- `mod`'s divisor is always raw nanoseconds, regardless of what unit you're
  conceptually thinking in — convert explicitly, as in Example 3.
- `Duration` also combines with `Timeline`: `Timeline + Duration` and
  `Timeline - Duration` both exist, documented on
  [`Timeline::add_duration`](../timeline/add_duration.md).

## See Also

- [`construction`](construction.md): building the operands used here.
- [`eq`](eq.md): comparing two durations instead of combining them.
- [`Timeline::add_duration`](../timeline/add_duration.md): the `Timeline`-side
  use of `Duration` arithmetic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="construction.html">
    <span class="label">Previous</span>
    <span class="title">construction</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
