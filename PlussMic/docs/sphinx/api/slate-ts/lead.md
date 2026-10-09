<!-- pluss-hand-authored -->

# TimeSeriesAccessor::lead

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::lead(std::int64_t periods = 1) const -> Slate
```

## What It Does

Shifts the bound slate `periods` positions backward: `out[i] = x[i + periods]` where
that index is in bounds, `NULL` otherwise. A negative `periods` delegates to
[`lag()`](lag.md) with the sign flipped — `lead(-periods)` and `lag(periods)` are the
same operation. Restricted to a numeric `Series` slate; anything else throws.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift backward. Negative values behave as
    [`lag(-periods)`](lag.md) instead.

## Returns

`Slate`
    A new slate, same length as the bound source, led by `periods` positions.

## Examples

Example 1. Lead a numeric slate by one position.

```cpp
using namespace pluss::table;

auto load = Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = load.ts().lead(1);
std::cout << out;
```

### Output

```text
0  20
1  15
2  30
3  NULL

ptype: Series
```

Each position now shows the next position's value: position `0` shows position `1`'s
`20`, position `1` shows position `2`'s `15`, and so on. Position `3` has no
successor, so it's `NULL` — the `NULL` lands at the opposite end from
[`lag()`](lag.md).

Example 2. A pre-existing missing entry in the source slate is honored, not just
the `NULL`s the shift itself introduces.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = with_gap.ts().lead(1);
std::cout << out2;
```

### Output

```text
0  NULL
1  30
2  40
3  NULL

ptype: Series
```

Position `0` (which would otherwise show position `1`'s value) comes back `NULL`
because position `1`'s `20.0` is masked-missing — the real number underneath the
mask never surfaces. Positions `1` and `2` correctly pick up `30.0` and `40.0`
from the real, valid positions `2` and `3`.

Example 3. A non-numeric slate throws.

```cpp
auto words = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = words.ts().lead(1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
TimeSeriesAccessor::lead: source must be Series
```

## Notes

- `lead(periods)` with a negative `periods` is exactly `lag(-periods)` — the two
  methods share one implementation, differing only in shift direction.
- A pre-existing missing entry in the source slate is honored, not just the
  `NULL`s the shift itself introduces — see Example 2.
- Restricted to numeric `Series` payloads; a `Note`/`Timeline`/`TimeIndex` slate
  throws `"TimeSeriesAccessor::lead: source must be Series"`; a non-numeric `Series`
  throws `"TimeSeriesAccessor::lead: source Series must be numeric"`.

## See Also

- [`TimeSeriesAccessor::lag`](lag.md): the mirror-image shift.
- [`Slate::shift`](../slate/shift.md): the equivalent top-level operation (a negative
  `shift` argument corresponds to a lead) outside the `ts()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="log_returns.html">
    <span class="label">Next</span>
    <span class="title">log_returns</span>
  </a>
</div>
