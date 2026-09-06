<!-- pluss-hand-authored -->

# Timeline::to_epoch_nanos_flat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Timeline::to_epoch_nanos_flat() const
```

## What It Does

`to_epoch_nanos_flat()` is the explicit-flat spelling of
[`to_epoch_nanos_vector()`](to_epoch_nanos_vector.md). Both return the same
owning `std::vector<std::int64_t>`, in the same logical row-major order, with
the same `NaT` representation.

Use this spelling where “flatten the temporal array” communicates intent at
the call site. Use `to_epoch_nanos_vector()` where the destination container
type is the more important fact. There is no precision, layout, ownership, or
timezone difference between them.

## Returns

An owning flat vector of UTC epoch nanoseconds with `numel()` entries.

## Examples

Example 1. Verify the two public spellings are value-equivalent.

```cpp
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});

std::cout << std::boolalpha
          << "flat alias equal: "
          << (matrix.to_epoch_nanos_flat() ==
              matrix.to_epoch_nanos_vector()) << "\n";
```

### Output

```text
flat alias equal: true
```

Example 2. Flattening removes coordinate boundaries, so save the shape when
they matter.

```cpp
auto matrix = pluss::timeline::epoch({2, 3});
auto flat = matrix.to_epoch_nanos_flat();
std::cout << "rank before=" << matrix.dim()
          << ", flat values=" << flat.size() << "\n";
```

### Output

```text
rank before=2, flat values=6
```

## Gotchas And Failure Modes

- “Flat” describes the returned container, not a mutation of the source.
- This is not a borrowed view. Every call allocates an independent vector.
- Do not infer the original shape from the vector length; many shapes have the
  same element count.
- Prefer [`epoch_nanos()`](epoch_nanos.md) when downstream `Series` operations
  or shape preservation are required.

## See Also

- [`Timeline::to_epoch_nanos_vector`](to_epoch_nanos_vector.md)
- [`Timeline::epoch_nanos`](epoch_nanos.md)
- [`Timeline::sizes`](sizes.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_epoch_nanos_vector.html"><span class="label">Previous</span><span class="title">to_epoch_nanos_vector</span></a>
  <a class="next-link" href="epoch_nanos.html"><span class="label">Next</span><span class="title">epoch_nanos</span></a>
</div>
