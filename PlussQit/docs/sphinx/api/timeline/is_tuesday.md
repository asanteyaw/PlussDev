# Timeline::is_tuesday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_tuesday() const
```

Aliases: `tue_is()` and `is_tue()`.

## Select Local Tuesdays

`is_tuesday()` marks timestamps whose local date is Tuesday. It returns a
boolean Series with unchanged rank, shape, and ordering. Missing timestamps
produce `false`; validity must be tracked separately when that distinction is
important.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_tuesday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, true, false, false, false, false, false, false]
```

The one-hot result follows directly from the example containing one occurrence
of every weekday. Real event data may contain many Tuesdays or none at all;
the method imposes no frequency assumptions.

## Working With The Mask

Use it where the named day is part of the rule, such as a Tuesday publication
cycle. For a numeric day feature or a dynamically selected weekday, use
`weekday()` instead of branching through seven named methods.

## Gotchas And Failure Modes

- Timezone conversion may move a late-night instant into Monday or Wednesday.
- The method does not reorder or filter by itself.
- Tuesday is a business day under the built-in weekday-only policy.

## See Also

- [`is_monday`](is_monday.md)
- [`is_wednesday`](is_wednesday.md)
- [`weekday_name`](weekday_name.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_monday.html"><span class="label">Previous</span><span class="title">is_monday</span></a>
  <a class="next-link" href="is_wednesday.html"><span class="label">Next</span><span class="title">is_wednesday</span></a>
</div>
<!-- pluss-hand-authored -->
