<!-- pluss-hand-authored -->

# Duration::ge

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::ge(const Duration& other) const;
Series Duration::operator>=(const Duration& other) const;  // ge(other)
```

## What It Does

Compares two `Duration` values elementwise, testing whether `self` is greater
than or equal to `other` by underlying elapsed-nanosecond magnitude,
broadcasting under ordinary N-D rules, and returns a `Series` of booleans.

## Parameters

**other** (`const Duration&`)

The duration to compare against. Broadcasts against `self`.

## Returns

`Series`
    A boolean `Series` at the broadcasted shape, one comparison result per
    position.

## Examples

Example 1. `1` day is greater than or equal to `1` hour.

```cpp
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
std::cout << pluss::series::print(one_day.ge(one_hour));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 day >= 1 hour` is `true`.

## Notes

- `ge(other)` is `gt(other) || eq(other)` — true whenever either the
  strict-greater-than or the equality comparison is true.
- `operator>=` is documented sugar for `ge` — nothing is computed differently
  between the two spellings.

## See Also

- [`gt`](gt.md): the strict counterpart.
- [`le`](le.md): the opposite-direction non-strict comparison.
- [`eq`](eq.md), [`ne`](ne.md): equality comparisons.
