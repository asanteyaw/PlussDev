# Timeline::is_saturday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_saturday() const
```

`is_saturday()` is the sole public spelling.

## Select Local Saturdays

`is_saturday()` returns `true` only for local Saturdays. It is the specific-day
half of the broader `is_weekend()` predicate. The output has boolean dtype,
retains the Timeline's shape, and maps `NaT` to `false`.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_saturday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, true, false, false]
```

Combining `is_saturday()` and `is_sunday()` with logical OR reproduces
`is_weekend()` for all valid timestamps. All three return `false` at `NaT`, so
the identity also holds there.

## Gotchas And Failure Modes

- Saturday is always weekend under this built-in convention.
- A domain with a different work week needs its own policy mask.
- The local timezone determines when Saturday begins and ends.

## See Also

- [`is_weekend`](is_weekend.md)
- [`is_friday`](is_friday.md)
- [`is_sunday`](is_sunday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_friday.html"><span class="label">Previous</span><span class="title">is_friday</span></a>
  <a class="next-link" href="is_sunday.html"><span class="label">Next</span><span class="title">is_sunday</span></a>
</div>
<!-- pluss-hand-authored -->
