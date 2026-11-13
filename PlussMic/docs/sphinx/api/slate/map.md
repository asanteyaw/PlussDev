<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8178c3fd0d8c -->

# Slate::map

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::map(const std::function<SlateValue(const SlateValue&)>& fn) const
```

## What It Does

Calls `fn` once **per row**, each time with a single-row `SlateValue` wrapping that row's
value, and stitches the per-row results back into one new `Slate` of the same length. This
is the row-by-row functional surface on `Slate` — reach for it when the transformation
genuinely needs to run in scalar terms (branching on the value, building a different kind
of result per row) rather than as a vectorized `Series`/`Note`/`Timeline` operation.

`SlateValue` is a thin single-row wrapper: read the value out with `.to_dbl()`,
`.to_str()`, `.as_series()`, etc., and build the return value by wrapping a fresh one-row
`Slate` back into a `SlateValue` — `SlateValue(Slate(...))`.

Invalid (`NULL`) rows are **not** passed to `fn` at all — they're skipped and carried
through as `NULL` in the output, at whatever payload kind the *other* rows established.

```{important} Every row's callback result must share the same payload kind
The first valid row's result establishes the output kind (`Series`, `Note`, `Timeline`, or
`TimeIndex`). If a later row's result has a different kind, `map()` throws — see Example 4.
If every row is invalid, `fn` is never called at all and `map()` returns `*this` unchanged.
```

## Parameters

**fn** (`const std::function<SlateValue(const SlateValue&)>&`)
    Called once per valid row with that row's value wrapped as a `SlateValue`. Must return
    a `SlateValue` wrapping exactly one row, of a consistent payload kind across all calls.
    Must not be empty — see Example 5.

## Returns

`Slate`
    A new slate of the same length as `*this`, built by concatenating each row's callback
    result. `NULL` rows are preserved without invoking `fn`.

## Examples

Example 1. Double each numeric value, row by row.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.map([](const SlateValue& v) {
  return SlateValue(Slate(pluss::make_series({v.to_dbl() * 2.0})));
});
std::cout << out;
```

### Output

```text
0  20
1  40
2  60
ptype: Series
```

Example 2. Turn each text value into its own length, as a new `Note`-kind slate — the
callback can change payload kind entirely, as long as every row agrees on the new kind.

```cpp
auto names = Slate(pluss::make_note({"ada", "bola", "chen"}));

auto out = names.map([](const SlateValue& v) {
  auto text = v.to_str();
  return SlateValue(Slate(pluss::make_note(std::vector<std::string>{
      std::to_string(text.size())})));
});
std::cout << out;
```

### Output

```text
0  3
1  4
2  4
ptype: Note
```

Example 3. Invalid rows are skipped — `fn` never sees them, and they stay `NULL` in the
output.

```cpp
auto readings = Slate(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));  // row 1 is NULL

auto out = readings.map([](const SlateValue& v) {
  return SlateValue(Slate(pluss::make_series({v.to_dbl() + 100.0})));
});
std::cout << out;
```

### Output

```text
0  101
1  NULL
2  103
ptype: Series
```

Example 4. Returning a different payload kind for different rows throws.

```cpp
auto values = Slate(pluss::make_series({1.0, 2.0}));
try {
  auto out = values.map([](const SlateValue& v) -> SlateValue {
    if (v.to_dbl() < 1.5) {
      return SlateValue(Slate(pluss::make_series({v.to_dbl()})));
    }
    return SlateValue(Slate(pluss::make_note(std::vector<std::string>{"x"})));
  });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::map: output kinds must be consistent
```

Example 5. An empty (default-constructed) `std::function` throws immediately.

```cpp
auto slate = Slate(pluss::make_series({1.0}));
try {
  std::function<SlateValue(const SlateValue&)> empty_fn;
  auto out = slate.map(empty_fn);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::map: callback is empty
```

## Notes

- Runs row by row, calling `fn` once per **valid** row — this is meaningfully slower than a
  vectorized `Series`/`Note` operation for large slates; prefer the vectorized surface
  (arithmetic, [`compare`](compare.md), [`cast`](cast.md), text methods) when it covers what
  you need.
- Invalid rows never reach `fn` and are preserved as `NULL`, at whatever kind the valid rows
  established.
- All valid rows' results must share one payload kind — see the important callout above.
- [`Slate::transform`](transform.md)'s single-argument `SlateValue`-returning overload is
  a pure alias for `map()`.

## See Also

- [`Slate::transform`](transform.md): dispatches to `map()` for the `SlateValue`-callback
  overload, and to [`fold`](fold.md) for the `Slate`-callback overload.
- [`Slate::fold`](fold.md) / [`Slate::flow`](flow.md): whole-slate (not per-row)
  functional transforms — call `fn` once with the entire slate instead of once per row.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lxor.html">
    <span class="label">Previous</span>
    <span class="title">lxor</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
