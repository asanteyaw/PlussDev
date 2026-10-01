<!-- pluss-hand-authored -->

# Duration::sub

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::operator-(const Duration& other) const;
Duration& Duration::operator-=(const Duration& other);
```

## What It Does

`Duration - Duration` combines two elapsed-time values elementwise,
broadcasting via the same `SeriesIterator` machinery every other N-D op in
Pluss uses. `-=` is ordinary compound-assignment sugar around the binary
operator — it mutates `self` in place and returns `Duration&`, rather than
building a new `Duration` and reassigning.

## Parameters

**other** (`const Duration&`)

The other duration. Broadcasts against `self` under ordinary N-D rules.

## Returns

`Duration` for `operator-`
    A new duration holding the elementwise difference.

`Duration&` for `operator-=`
    A reference to the (now-mutated) `self`.

## Examples

Example 1. Subtract one duration from another.

```cpp
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
std::cout << pluss::duration::print(one_day - one_hour);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 23:00:00.000000000
```

`1 day - 1 h = 23 h`.

Example 2. `-=` mutates `self` in place and is verified identical to the
binary operator it wraps.

```cpp
auto compound_sub = one_day;
compound_sub -= one_hour;
std::cout << "diff=" << pluss::series::print(compound_sub.total().hours()) << "\n";
std::cout << "matches_binary=" << pluss::series::print(compound_sub.eq(one_day - one_hour));
```

### Output

```text
diff=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=23.0000
matches_binary=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Notes

- `Duration` also combines with `Timeline`: `Timeline - Duration` and
  `Timeline - Timeline` (which itself produces a `Duration`) both exist,
  documented on [`Timeline::sub_duration`](../timeline/sub_duration.md) and
  [`Timeline::diff`](../timeline/diff.md).
- Subtracting always combines two `Duration`s — there is no
  `Duration - <plain number>` overload. To scale a single `Duration` by a
  plain count instead, use [`mul`](mul.md)/[`div`](div.md).

## See Also

- [`add`](add.md): the complementary operation.
- [`neg`](neg.md): unary negation — `a - b` and `a + (-b)` are equivalent.
- [`construction`](construction.md): building the operands used here.
- [`Timeline::sub_duration`](../timeline/sub_duration.md): the `Timeline`-side
  use of `Duration` subtraction.
