<!-- pluss-hand-authored -->

# Duration::lt

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::lt(const Duration& other) const;
Series Duration::operator<(const Duration& other) const;  // lt(other)
```

## What It Does

Compares two `Duration` values elementwise, testing whether `self` is
strictly less than `other` by underlying elapsed-nanosecond magnitude,
broadcasting under ordinary N-D rules, and returns a `Series` of booleans.

## Parameters

**other** (`const Duration&`)

The duration to compare against. Broadcasts against `self`.

## Returns

`Series`
    A boolean `Series` at the broadcasted shape, one comparison result per
    position.

## Examples

Example 1. `1` hour is less than `90` minutes.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::series::print(one_hour.lt(ninety_min));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 hour < 90 minutes` is `true` (`60 < 90`).

## Notes

- Comparisons are purely by elapsed magnitude, including sign — a negative
  `Duration` compares as less than a positive one, same as ordinary
  signed-number comparison.
- `operator<` is documented sugar for `lt` — nothing is computed differently
  between the two spellings.

## See Also

- [`le`](le.md): the non-strict counterpart.
- [`gt`](gt.md), [`ge`](ge.md): the opposite-direction comparisons.
- [`eq`](eq.md), [`ne`](ne.md): equality comparisons.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ne.html">
    <span class="label">Previous</span>
    <span class="title">ne</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
