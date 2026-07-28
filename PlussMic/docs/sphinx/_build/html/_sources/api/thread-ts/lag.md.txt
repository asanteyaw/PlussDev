<!-- pluss-hand-authored -->

# TimeSeriesAccessor::lag

[Back to TimeSeriesAccessor API Reference](../../thread_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::lag(std::int64_t periods = 1) const -> Thread
```

## What It Does

Shifts the bound thread `periods` positions forward: `out[i] = x[i - periods]` where
that index is in bounds, `NULL` otherwise. A negative `periods` delegates to
[`lead()`](lead.md) with the sign flipped — `lag(-periods)` and `lead(periods)` are the
same operation. Restricted to a numeric `Series` thread; anything else throws.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift forward. Negative values behave as
    [`lead(-periods)`](lead.md) instead.

## Returns

`Thread`
    A new thread, same length as the bound source, shifted by `periods` positions.

## Examples

Example 1. Lag a numeric thread by one position.

```cpp
using namespace pluss::table;

auto load = Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = load.ts().lag(1);
std::cout << out;
```

### Output

```text
0  NULL
1  10
2  20
3  15

ptype: Series
```

Each position now shows the previous position's value: position `1` shows position
`0`'s `10`, position `2` shows position `1`'s `20`, and so on. Position `0` has no
predecessor, so it's `NULL`. This is the same operation
[`Thread::shift(periods)`](../thread/shift.md) performs.

Example 2. A negative `periods` behaves like `lead()` with the sign flipped.

```cpp
auto out2 = load.ts().lag(-1);
auto out3 = load.ts().lead(1);
std::cout << out2;
std::cout << out3;
```

### Output

```text
0  20
1  15
2  30
3  NULL

ptype: Series
0  20
1  15
2  30
3  NULL

ptype: Series
```

`lag(-1)` and `lead(1)` produce identical results — `lag()` with a negative `periods`
just forwards to `lead()` with the sign flipped internally.

Example 3. A pre-existing missing entry in the source thread stays `NULL` after
shifting, even though its underlying storage is a real number.

```cpp
auto with_gap = Thread(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out4 = with_gap.ts().lag(1);
std::cout << out4;
```

### Output

```text
0  NULL
1  10
2  NULL
3  30

ptype: Series
```

Position `1` (the masked-missing `20.0`) shifts into position `2` — and stays
`NULL` there, exactly as it was before the shift, rather than surfacing the real
`20.0` sitting underneath the mask. Positions `1` and `3` still correctly pick up
`10.0` and `30.0` from the real, valid positions `0` and `2`.

Example 4. A non-numeric thread throws.

```cpp
auto words = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = words.ts().lag(1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
TimeSeriesAccessor::lag: source must be Series
```

## Notes

- `lag(periods)` with a negative `periods` is exactly `lead(-periods)` — the two
  methods share one implementation, differing only in shift direction.
- Equivalent to [`Thread::shift(periods)`](../thread/shift.md) for a numeric `Series`
  thread — vacated positions become `NULL`, values shifted past the far edge are
  dropped, not wrapped around.
- A pre-existing missing entry in the source thread is honored, not just the
  `NULL`s the shift itself introduces — see Example 3.
- Restricted to numeric `Series` payloads; a `Note`/`Timeline`/`TimeIndex` thread
  throws `"TimeSeriesAccessor::lag: source must be Series"`; a non-numeric `Series`
  throws `"TimeSeriesAccessor::lag: source Series must be numeric"`.

## See Also

- [`TimeSeriesAccessor::lead`](lead.md): the mirror-image shift.
- [`TimeSeriesAccessor::lag_block`](lag_block.md): computing several lags of the
  same thread at once, each as its own `DataFrame` column.
- [`Thread::shift`](../thread/shift.md): the equivalent top-level operation outside the
  `ts()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="lag_block.html">
    <span class="label">Next</span>
    <span class="title">lag_block</span>
  </a>
</div>
