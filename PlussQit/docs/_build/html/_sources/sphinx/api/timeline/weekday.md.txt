# Timeline::weekday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::weekday() const
```

Aliases: `dayofweek()`, `dow()`, and `weekday_()`.

## Number The Days From Monday

`weekday()` maps each local date to an `int32` code using this exact convention:

| Day | Code |
| --- | ---: |
| Monday | 0 |
| Tuesday | 1 |
| Wednesday | 2 |
| Thursday | 3 |
| Friday | 4 |
| Saturday | 5 |
| Sunday | 6 |

The method is elementwise and shape-preserving. A missing timestamp maps to
`-1`, which is outside the valid weekday range.

## Example: Verify The Numbering Convention

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.weekday());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[3, 4, 3, -1]
```

Both 31 December 2020 and 29 February 2024 were Thursdays, hence `3`.

## Building Calendar Rules

For a generic numeric feature, `weekday()` is compact and easy to compare. For
intent-revealing predicates, use methods such as `is_monday()`, `is_weekend()`,
or `is_business_day()`. For presentation text, use `weekday_name()` rather than
maintaining a parallel label table.

All of these methods evaluate the local date. Convert to the operating timezone
before classifying a timestamp that lies near midnight.

## Gotchas And Failure Modes

- Monday is `0`; Sunday is `6`.
- `weekday()` is different from `day()` and `dayofyear()`.
- Weekend and business-day policy should use the dedicated predicates rather
  than embedding numeric codes throughout application logic.
- `-1` identifies `NaT`, not an eighth weekday.

## See Also

- [`weekday_name`](weekday_name.md)
- [`is_monday`](is_monday.md)
- [`day`](day.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="millisecond.html"><span class="label">Previous</span><span class="title">millisecond</span></a>
  <a class="next-link" href="dayofyear.html"><span class="label">Next</span><span class="title">dayofyear</span></a>
</div>
<!-- pluss-hand-authored -->
