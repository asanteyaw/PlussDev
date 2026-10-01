<!-- pluss-hand-authored -->

# Duration::isvalid

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::isvalid() const
```

## What It Does

Tests elementwise whether a `Duration` position holds a real value rather
than the `Duration::kNaT` missing-value sentinel — the exact negation of
[`isnat`](isnat.md) at every position.

## Parameters

None — this is a zero-argument accessor on `self`.

## Returns

`Series`
    A boolean `Series` at `self`'s shape, one test result per position.

## Examples

Example 1. A valid duration and an explicitly-missing one.

```cpp
auto one_hour = pluss::duration::hours(1);
auto missing = pluss::duration::nanos(pluss::Duration::kNaT);
std::cout << "valid=" << pluss::series::print(one_hour.isvalid()) << "\n";
std::cout << "missing=" << pluss::series::print(missing.isvalid());
```

### Output

```text
valid=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
missing=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

## Notes

- `isvalid()` is always the elementwise negation of `isnat()` — there is no
  case where they disagree.

## See Also

- [`isnat`](isnat.md): the comparison this negates.
- [`fill_nat`](fill_nat.md): replacing missing positions with a real value.
- [`where`](where.md): conditional selection, another mask-driven operation.
