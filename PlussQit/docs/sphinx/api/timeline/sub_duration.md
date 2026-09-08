# Timeline::sub_duration

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::sub_duration(const Duration& duration) const
```

## What It Does

Shifts every defined timestamp backward by the supplied duration.

This is the direct counterpart to [`add_duration`](add_duration.md). Use it when
your intention is "move these timestamps earlier" rather than computing a
distance between two temporal payloads.

## Parameters

**d**

The duration to subtract.

The practical effect depends on the scale of the duration you build:

- days for rolling calendar anchors backward
- hours for session shifts
- minutes or seconds for intraday event alignment

## Returns

`Timeline`
    A timeline shifted backward with the same shape as the input.

## Examples

Example 1. Move one scalar timestamp backward by one day.

```cpp
auto scalar = pluss::timeline::make("2024-11-01T09:00:00Z");
auto out = scalar.sub_duration(pluss::duration::days(1));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-10-31 09:00:00
```

Example 2. Roll a whole vector back by fifteen minutes.

```cpp
auto base = pluss::timeline::make({"2024-11-01T09:00:00Z", "2024-11-01T10:15:30Z", "2024-12-31T23:59:59Z", "2025-01-01T00:00:00Z"});
auto out = base.sub_duration(pluss::duration::minutes(15));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 08:45:00, 2024-11-01 10:00:30, 2024-12-31 23:44:59, 2024-12-31 23:45:00 ]
```

## Operator Sugar, Broadcasting, NaT, And Timezone

`sub_duration` shares its full arithmetic model with
[`add_duration`](add_duration.md) -- same unit factories, same broadcasting
rules, same `NaT`-from-either-side propagation, same left-operand-only
timezone preservation. See that page's "Full Scope" and "Broadcasting"
sections for the complete unit-factory sweep and the
broadcasting/`NaT`/timezone examples; this page just confirms
`t - d` is exactly `t.sub_duration(d)`, not an approximation of it:

```cpp
const auto t0 = pluss::timeline::make("1999-04-17T21:15:23Z");
const auto via_operator = t0 - pluss::duration::hours(3);
const auto via_method = t0.sub_duration(pluss::duration::hours(3));
std::cout << pluss::series::print(via_operator.eq(via_method)) << "\n";
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

As with `operator+`, there is no reversed `duration - timeline` form --
`Timeline` must be the left-hand operand. There is a compound-assignment
form, `t -= d`, verified identical to the binary operator:

```cpp
auto t_mut = t0;
t_mut -= pluss::duration::hours(3);
std::cout << pluss::timeline::print(t_mut) << "\n";
std::cout << pluss::series::print(t_mut.eq(t0 - pluss::duration::hours(3))) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
1999-04-17 18:15:23

Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Notes

- Absent instants propagate through the subtraction, from either operand.
- Shape is preserved, subject to the same broadcasting rules as `add_duration`.
- A result outside the representable nanosecond epoch range throws.
- Operator sugar also exists: `t - d`, identical to `sub_duration`; `t -= d`
  is its compound-assignment form.

## See Also

- [`Timeline::add_duration`](add_duration.md)
- [`Timeline::diff`](diff.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_duration.html">
    <span class="label">Previous</span>
    <span class="title">add_duration</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
