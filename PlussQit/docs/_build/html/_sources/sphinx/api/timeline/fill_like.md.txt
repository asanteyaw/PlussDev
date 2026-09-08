<!-- pluss-hand-authored -->

# Timeline::fill_like

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline Timeline::fill_like(int year, int month, int day) const
Timeline Timeline::fill_like(const std::string& date) const
```

## Fill The Receiver's Shape With One Local Date

`fill_like()` creates a new `Timeline` with exactly the receiver's shape and
timezone, then places the same local midnight in every element. The receiver is
a geometry and timezone template only: none of its instants participate in the
new values.

This differs from [`fill_nat()`](fill_nat.md), which replaces only absent
positions while retaining defined receiver values. `fill_like()` replaces the
entire payload by construction.

## Overloads

| Call form | Accepted input | Interpretation |
| --- | --- | --- |
| `fill_like(year, month, day)` | Three integer calendar fields | Validates the combined civil date. |
| `fill_like(date)` | `YYYY-MM-DD` or `YYYY/MM/DD` text | Parses a date-only token; surrounding ASCII whitespace is ignored. |

Both overloads produce `00:00:00` in the receiver's timezone. The timezone is
not merely attached after constructing UTC midnight: local midnight is resolved
to its proper UTC instant first. This distinction matters whenever local time
has a nonzero UTC offset.

Example 1. A local `[1, 2]` template receives local midnight from integer
fields, while preserving its shape and timezone.

```cpp
const auto like = pluss::timeline::date({{"2024-01-01", "2024-01-02"}})
                      .tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(like.fill_like(2035, 7, 8)) << '\n';
```

### Output

```text
Timeline(shape=[1, 2], tz=America/Edmonton)
[ 2035-07-08 00:00:00 2035-07-08 00:00:00 ]
```

## Date-Text Forms

The text overload is intentionally date-only. A time, UTC suffix, or numeric
offset would compete with the receiver's timezone and undermine the promise of
receiver-local midnight, so datetime text is rejected rather than truncated.
Hyphen and slash separators are equivalent accepted spellings.

Example 2. Both date separators produce the same local date.

```cpp
std::cout << pluss::timeline::print(like.fill_like("2036-03-04")) << '\n';
std::cout << pluss::timeline::print(like.fill_like("2036/03/04")) << '\n';
```

### Output

```text
Timeline(shape=[1, 2], tz=America/Edmonton)
[ 2036-03-04 00:00:00 2036-03-04 00:00:00 ]
Timeline(shape=[1, 2], tz=America/Edmonton)
[ 2036-03-04 00:00:00 2036-03-04 00:00:00 ]
```

## Scalar And Empty Geometry

Unlike the axis-0 structural operations, `fill_like()` never promotes a scalar.
A scalar template produces a scalar. An empty template retains its complete
empty shape; the requested date is still validated, but no payload elements
are allocated.

Example 3. A valid leap-day fill remains scalar.

```cpp
const auto scalar = pluss::timeline::date("2024-01-01");
std::cout << pluss::timeline::print(scalar.fill_like(2040, 2, 29)) << '\n';
```

### Output

```text
Timeline(shape=[], tz=UTC)
2040-02-29 00:00:00
```

Example 4. Empty trailing geometry survives unchanged.

```cpp
const auto cube = pluss::timeline::date({
    {{"2024-01-01", "2024-01-02"}},
    {{"2024-01-03", "2024-01-04"}},
});
std::cout << pluss::timeline::print(
    cube.empty_like().fill_like(2035, 7, 8)) << '\n';
```

### Output

```text
Timeline(shape=[0, 1, 2], tz=UTC)
```

## Absence, Allocation, And Complexity

An absent instant in the receiver has no special status because receiver values
are not consulted. It is replaced just like every other position. The result
owns contiguous epoch-nanosecond storage and requires one write per output
element, so time and payload storage are both linear in `numel()`.

## Validation And Failure Modes

The integer overload rejects invalid civil dates such as February 29 in a
non-leap year, month 13, or day zero. The text overload rejects malformed text,
datetime text, timezone-bearing text, and impossible dates. Timezone loading
and local-midnight resolution also occur before the result is returned.

Example 5. An impossible date and a datetime token fail instead of being
normalized or silently shortened to a date.

```cpp
try {
  (void)like.fill_like(2023, 2, 29);
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
try {
  (void)like.fill_like("2036-03-04T12:00:00");
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
```

### Output

```text
rejected
rejected
```

## See Also

- [`Timeline::empty_like`](empty_like.md): preserve trailing geometry with zero records.
- [`Timeline::fill_nat`](fill_nat.md): replace only absent positions.
- [`pluss::timeline::full`](../timeline_factory/full.md): construct a filled shape directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html"><span class="label">Previous</span><span class="title">eq</span></a>
  <a class="next-link" href="fill_nat.html"><span class="label">Next</span><span class="title">fill_nat</span></a>
</div>
