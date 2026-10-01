<!-- pluss-hand-authored -->

# Duration::ne

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::ne(const Duration& other) const;
Series Duration::operator!=(const Duration& other) const;  // ne(other)
```

## What It Does

Compares two `Duration` values elementwise by their underlying
elapsed-nanosecond magnitude for inequality, broadcasting under ordinary N-D
rules, and returns a `Series` of booleans — the exact negation of [`eq`](eq.md)
at every position.

## Parameters

**other** (`const Duration&`)

The duration to compare against. Broadcasts against `self`.

## Returns

`Series`
    A boolean `Series` at the broadcasted shape, one comparison result per
    position.

## Examples

Example 1. `1` hour does not equal `90` minutes.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::series::print(one_hour.ne(ninety_min));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 hour != 90 minutes` is `true` (`60 min != 90 min`).

## Notes

- `ne(other)` is always the elementwise negation of `eq(other)` — there is no
  case where they disagree.
- `operator!=` is documented sugar for `ne` — nothing is computed differently
  between the two spellings.

## See Also

- [`eq`](eq.md): the comparison this negates.
- [`lt`](lt.md), [`le`](le.md), [`gt`](gt.md), [`ge`](ge.md): the ordering
  comparisons.
