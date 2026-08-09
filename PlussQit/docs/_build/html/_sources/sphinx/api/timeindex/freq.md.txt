# TimeIndex::freq

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::infer_freq() const -> std::string
TimeIndex::freq() const -> std::string
TimeIndex::frequency() const -> std::string
```

## What It Does

Infers a regular frequency code from the spacing of the index.

This is the quickest way to ask whether the axis looks like a regular hourly,
minute, daily, or second-based series.

`freq()` and `frequency()` are convenience aliases for `infer_freq()`.

Think of it as a descriptive read on the axis, not a guarantee that later
operations must obey.

## Returns

`std::string`
    A frequency code such as `"D"`, `"H"`, `"min"`, or `"S"`. If no stable
    frequency can be inferred, the result is empty.

## Examples

Example 1. Infer the cadence of a regular minute-like axis.

```cpp
auto idx = pluss::timeindex::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});

auto f = idx.freq();
std::cout << "f = \"" << f << "\"\n";
```

### Output

```text
f = "min"
```

Example 2. Use the inferred cadence as a quick diagnostic before you choose a
resampling strategy.

```cpp
if (idx.freq().empty()) {
    // irregular axis: rely on explicit resample options
}
```

## Notes

- Frequency inference is a heuristic over observed spacing.
- It is most useful on already-regular axes.
- A non-empty result describes the observed spacing of this index; it does not
  automatically constrain later binning choices.
- If your series is irregular, use explicit resample options instead of relying
  on inferred cadence.

## See Also

- [`TimeIndex::resample_bins`](resample_bins.md)
- [`TimeIndex::is_monotonic_increasing`](is_monotonic_increasing.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="host.html">
    <span class="label">Next</span>
    <span class="title">host</span>
  </a>
</div>
