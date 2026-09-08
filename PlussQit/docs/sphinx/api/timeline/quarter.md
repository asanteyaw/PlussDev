# Timeline::quarter

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::quarter() const
```

`quarter()` is the sole public spelling.

## Group Months Into Calendar Quarters

`quarter()` returns `1`, `2`, `3`, or `4` according to the local calendar
month: January-March, April-June, July-September, and October-December. The
method uses ordinary calendar quarters beginning in January; it does not infer
an organization's fiscal-year offset.

The result is an `int32` Series with exactly the Timeline's shape. `NaT` maps
to `-1`.

## Example: Shape Is Preserved

```cpp
const auto schedule = pluss::timeline::make({
    {"2024-01-01T00:00:00Z", "2024-04-01T00:00:00Z"},
    {"2024-07-01T00:00:00Z", "2024-10-01T00:00:00Z"},
});

pluss::series::print(std::cout, schedule.quarter());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1 2 ⎤
⎣ 3 4 ⎦
```

This demonstrates that extraction is not limited to timestamp columns. Every
position in a multidimensional Timeline is projected independently.

## Calendar Versus Fiscal Quarters

For a calendar year, the returned value can be used directly with `year()` as
a `(year, quarter)` grouping key. If a fiscal year starts in another month,
first define that business rule explicitly; merely relabeling `Q1` would leave
the associated year boundary ambiguous.

## Gotchas And Failure Modes

- Quarter numbering is one-based.
- The method has no fiscal-year-start argument.
- A quarter number alone is not unique; retain the corresponding year.
- `-1` denotes `NaT` and is outside the valid range.

## See Also

- [`month`](month.md)
- [`year`](year.md)
- [`week`](week.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dayofyear.html"><span class="label">Previous</span><span class="title">dayofyear</span></a>
  <a class="next-link" href="quarter_scalar.html"><span class="label">Next</span><span class="title">quarter_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
