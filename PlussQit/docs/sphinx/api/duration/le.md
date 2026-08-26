<!-- pluss-hand-authored -->

# Duration::le

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::le(const Duration& other) const;
Series Duration::operator<=(const Duration& other) const;  // le(other)
```

## What It Does

Compares two `Duration` values elementwise, testing whether `self` is less
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

Example 1. `1` hour is less than or equal to `90` minutes.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::series::print(one_hour.le(ninety_min));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 hour <= 90 minutes` is `true` (`60 <= 90`).

## Notes

- `le(other)` is `lt(other) || eq(other)` — true whenever either the
  strict-less-than or the equality comparison is true.
- `operator<=` is documented sugar for `le` — nothing is computed differently
  between the two spellings.

## See Also

- [`lt`](lt.md): the strict counterpart.
- [`ge`](ge.md): the opposite-direction non-strict comparison.
- [`eq`](eq.md), [`ne`](ne.md): equality comparisons.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
