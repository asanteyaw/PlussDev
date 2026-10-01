<!-- pluss-hand-authored -->

# Duration::fill_nat

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::fill_nat(const Duration& value) const;
```

## What It Does

Replaces every missing (`Duration::kNaT`) position with `value`, leaving
already-valid positions unchanged.

## Parameters

**value** (`const Duration&`)

The replacement for missing positions. Broadcasts against `self`.

## Returns

`Duration`
    A new duration with every missing position replaced by `value`.

## Examples

Example 1. `fill_nat` replaces the missing value with a real `Duration`.

```cpp
auto missing = pluss::duration::nanos(pluss::Duration::kNaT);
auto filled = missing.fill_nat(pluss::duration::hours(2));
std::cout << pluss::duration::print(filled);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 02:00:00.000000000
```

`missing` had no real value (`pluss::duration::print(missing)` would print
`NaT`); after `fill_nat(pluss::duration::hours(2))`, it prints as an ordinary
`2`-hour `Duration`.

## Notes

- Positions that are already valid pass through unchanged — `fill_nat` never
  overwrites a real value.

## See Also

- [`isnat`](isnat.md): testing for the missing positions this replaces.
- [`where`](where.md): the more general conditional-selection operation
  `fill_nat` is a special case of (mask = `isnat()`).
