# Timeline::isnat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::isnat() -> Series
```

## What It Does

Returns a boolean mask marking which timeline entries are missing.

This is the load-bearing predicate for temporal missing-data logic.

## Returns

`Series`
    `true` where the timestamp is missing, `false` otherwise.

## Examples

Example 1. Detect missing timestamps in a mixed timeline.

```cpp
auto mask = with_nat.isnat();
std::cout << pluss::series::print(mask) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

## Notes

- Pair `isnat()` with [`fill_nat`](fill_nat.md) or [`where`](where.md) to repair
  or branch on missing times.
- Use [`isnat_scalar`](isnat_scalar.md) for one scalar timestamp instead of an
  array-shaped result.

## See Also

- [`Timeline::isvalid`](isvalid.md)
- [`Timeline::fill_nat`](fill_nat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="isvalid.html">
    <span class="label">Next</span>
    <span class="title">isvalid</span>
  </a>
</div>
