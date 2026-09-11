# Timeline::is_saturday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_saturday() const
```

## Classify Local Saturdays

`is_saturday()` marks each instant whose local civil date is Saturday. It is a
named, fixed-day predicate: use it when the weekday itself expresses the rule,
and use `weekday()` when the target weekday is supplied dynamically. The
method classifies independently; it does not sort, filter, aggregate, or
assume that the Timeline is regular.

Saturday is one half of the fixed weekend rule and is excluded by the built-in workweek predicate.

## Result Contract

The result is a newly allocated boolean `Series` with exactly the receiver's
rank, shape, and ordering. Scalar input remains rank zero, matrices remain
matrices, and empty extents remain empty. `NaT` maps to `false`, so validity
must be retained separately when absence cannot share the false state.

The receiver is unchanged. Weekday is determined after converting each instant
to the Timeline's current timezone. A timezone conversion near midnight can
therefore change the answer without changing the represented instant. One call
performs one shape-preserving pass and has no options or scalar-rank restriction.

## Example 1: Select Saturday From A Complete Week

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_saturday());
```

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, true, false, false]
```

The absent final element is false, just like every non-matching valid day.

## Example 2: Relate The Name To Weekday Numbering

```cpp
std::cout << std::boolalpha
          << week.is_saturday().equal(week.weekday().eq(5)) << '\n';
```

```text
true
```

Saturday has code `5` under the Monday-zero convention. The named call
avoids scattering that numeric convention through fixed scheduling rules.

## Example 3: Preserve A Two-Dimensional Layout

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
     "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z"},
    {"2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
     "2024-03-10T12:00:00Z", ""},
});
pluss::series::print(std::cout, matrix.is_saturday());
```

```text
Series(shape=[2,4], strides=[4,1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=
⎡ false false false false ⎤
⎣ false  true false false ⎦
```

The mask remains position-aligned with the temporal matrix; no flattening takes place.

## Example 4: All Named Days Partition Valid Instants

```cpp
const auto any_day = week.is_monday().logical_or(week.is_tuesday())
    .logical_or(week.is_wednesday()).logical_or(week.is_thursday())
    .logical_or(week.is_friday()).logical_or(week.is_saturday())
    .logical_or(week.is_sunday());
std::cout << std::boolalpha << any_day.equal(week.isvalid()) << '\n';
```

```text
true
```

Exactly one named predicate is true for each valid instant. None is true for `NaT`.

## Gotchas And Failure Modes

- `is_saturday()` returns a mask, not the matching Timeline values.
- `false` can mean another weekday or `NaT`.
- Choose the operational timezone before classification.
- Calendar holidays do not change weekday identity.
- Valid Timeline values of every supported rank are accepted.

## See Also

- [`weekday`](weekday.md)
- [`is_business_day`](is_business_day.md)
- [`is_weekend`](is_weekend.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_quarter_start.html"><span class="label">Previous</span><span class="title">is_quarter_start</span></a>
  <a class="next-link" href="is_scalar.html"><span class="label">Next</span><span class="title">is_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
