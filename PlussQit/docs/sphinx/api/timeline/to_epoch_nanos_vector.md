<!-- pluss-hand-authored -->

# Timeline::to_epoch_nanos_vector

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Timeline::to_epoch_nanos_vector() const
```

## What It Does

`to_epoch_nanos_vector()` exports every temporal position into one owning C++
vector of UTC epoch-nanosecond ticks. Values are emitted in logical row-major
order: the last axis changes fastest, then the next-to-last axis, and so on.

The method always flattens. It preserves value order and exact 64-bit
precision, but not shape. Record [`sizes()`](sizes.md) separately when the
receiver will need to reconstruct an N-dimensional timeline, or use
[`epoch_nanos()`](epoch_nanos.md) when a shape-preserving `Series` is more
useful.

Timezone conversion does not change this output because conversion changes
wall-clock presentation, not the represented instants.

## Returns

An owning `std::vector<std::int64_t>` with exactly [`numel()`](numel.md)
entries. A `NaT` position becomes `Timeline::kNaT`.

## Examples

Example 1. Export a vector of exact ticks.

```cpp
auto times = pluss::timeline::make({
    "2024-02-29T12:30:00Z",
    "2024-03-01T00:00:00Z",
});
auto ticks = times.to_epoch_nanos_vector();

std::cout << '[' << ticks[0] << ',' << ticks[1] << "]\n";
```

### Output

```text
[1709209800000000000,1709251200000000000]
```

Example 2. A matrix flattens in logical row-major order, including missing
positions.

```cpp
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});
auto ticks = matrix.to_epoch_nanos_vector();

std::cout << '[';
for (std::size_t i = 0; i < ticks.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << ticks[i];
}
std::cout << "]\n";
```

### Output

```text
[1709209800000000000,1709251200000000000,1709360100000000000,-9223372036854775808]
```

Example 3. Presentation-zone conversion leaves the exported instant intact.

```cpp
auto utc = pluss::timeline::make("2024-02-29T12:30:00Z");
auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << (utc.to_epoch_nanos_vector() ==
              local.to_epoch_nanos_vector()) << "\n";
```

### Output

```text
true
```

Example 4. Preserve the shape beside the flat values for round-trip work.

```cpp
auto source = pluss::timeline::epoch({2, 3});
auto ticks = source.to_epoch_nanos_vector();
std::vector<std::int64_t> shape(source.sizes().begin(), source.sizes().end());
auto rebuilt = pluss::timeline::from_epoch(ticks, shape);

std::cout << "source numel=" << source.numel()
          << ", rebuilt dim=" << rebuilt.dim()
          << ", rebuilt numel=" << rebuilt.numel() << "\n";
```

### Output

```text
source numel=6, rebuilt dim=2, rebuilt numel=6
```

## Gotchas And Failure Modes

- Shape is not encoded in the returned vector.
- Values are UTC epoch ticks even when `tz()` names another zone.
- `NaT` is a sentinel integer, not an omitted element.
- The method allocates and copies. Repeated calls create repeated vectors.
- Calling it on an invalid object state throws rather than returning an empty
  vector.

## See Also

- [`Timeline::to_epoch_nanos_flat`](to_epoch_nanos_flat.md)
- [`Timeline::epoch_nanos`](epoch_nanos.md)
- [`Timeline::host`](host.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html"><span class="label">Previous</span><span class="title">host</span></a>
  <a class="next-link" href="to_epoch_nanos_flat.html"><span class="label">Next</span><span class="title">to_epoch_nanos_flat</span></a>
</div>
