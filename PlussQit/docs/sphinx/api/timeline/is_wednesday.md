# Timeline::is_wednesday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_wednesday() const
```

`is_wednesday()` is the sole public spelling.

## Select Local Wednesdays

`is_wednesday()` evaluates every timestamp independently and marks local
Wednesdays. The result is boolean and preserves the Timeline's complete shape;
`NaT` is represented by `false`.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_wednesday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, true, false, false, false, false, false]
```

This named predicate is useful for readable midweek rules. It is equivalent in
meaning to `weekday() == 2`, because weekday numbering starts with Monday at
zero, but avoids embedding that numeric convention at the call site.

## Gotchas And Failure Modes

- The method returns a mask; it does not return the matching timestamps.
- Absent instants do not form a third boolean state.
- The current timezone, not the timestamp's original text suffix, determines
  the local weekday after conversion.

## See Also

- [`weekday`](weekday.md)
- [`is_tuesday`](is_tuesday.md)
- [`is_thursday`](is_thursday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_tuesday.html"><span class="label">Previous</span><span class="title">is_tuesday</span></a>
  <a class="next-link" href="is_thursday.html"><span class="label">Next</span><span class="title">is_thursday</span></a>
</div>
<!-- pluss-hand-authored -->
