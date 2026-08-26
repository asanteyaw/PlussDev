<!-- pluss-hand-authored -->

# Duration::gt

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::gt(const Duration& other) const;
Series Duration::operator>(const Duration& other) const;  // gt(other)
```

## What It Does

Compares two `Duration` values elementwise, testing whether `self` is
strictly greater than `other` by underlying elapsed-nanosecond magnitude,
broadcasting under ordinary N-D rules, and returns a `Series` of booleans.

## Parameters

**other** (`const Duration&`)

The duration to compare against. Broadcasts against `self`.

## Returns

`Series`
    A boolean `Series` at the broadcasted shape, one comparison result per
    position.

## Examples

Example 1. `1` day is greater than `1` hour.

```cpp
auto one_hour = pluss::duration::hours(1);
auto one_day = pluss::duration::days(1);
std::cout << pluss::series::print(one_day.gt(one_hour));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 day > 1 hour` is `true` (`24 h > 1 h`).

## Notes

- Comparisons are purely by elapsed magnitude, including sign.
- `operator>` is documented sugar for `gt` — nothing is computed differently
  between the two spellings.

## See Also

- [`ge`](ge.md): the non-strict counterpart.
- [`lt`](lt.md), [`le`](le.md): the opposite-direction comparisons.
- [`eq`](eq.md), [`ne`](ne.md): equality comparisons.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
