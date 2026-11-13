<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3b538890444 -->

# DataFrame::slice

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a contiguous positional case range `[start, stop)` — every row from `start` up
to, but not including, `stop`. Both endpoints support Python-style negative indices
(`-1` means "the last case," `-2` the second-to-last, and so on), and both are clamped
into range rather than raising, so it is always safe to pass generous or approximate
bounds. Case labels on the result are preserved exactly as they were on the source
frame — a slice never renumbers.

Internally, `slice(start, stop)` is `take(indices)` for the computed contiguous index
range, so its result-shape guarantees (feature order, dtypes, case labels) are
identical to [`take()`](take.md)'s.

## Parameters

**start** (`std::int64_t`)

The first case position to include. A negative value is interpreted relative to the
end of the frame (`-1` is the last case), then the whole result is clamped into
`[0, ncases()]`.

**stop** (`std::int64_t`)

The case position to stop before (exclusive). Negative values are normalized the same
way as `start`. If the normalized `stop` ends up before the normalized `start`, the
result is an empty (zero-case) frame rather than an error.

## Returns

`DataFrame`
    A new frame holding the cases in `[start, stop)`, in their original order, with
    every feature and the source frame's original case labels preserved.

## Examples

Example 1. Slice a contiguous middle range using ordinary positive indices.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Slate(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto middle = orders.slice(1, 4);
std::cout << middle;
```

### Output

```text
===============================================================
     | order_id  city   product     units  unit_price  discount
---------------------------------------------------------------
1    | 102       Accra  Headphones  5      45.5        0.05    
2    | 103       Tema   Laptop      1      1150        NULL    
3    | 104       Tema   Monitor     3      320         0.08    
---------------------------------------------------------------
[3 cs x 6 feat]
```

Three cases come back — `1`, `2`, `3` — since `stop = 4` is exclusive. Case labels are
the original positions, not renumbered from `0`.

Example 2. Use negative indices to slice relative to the end of the frame.

```cpp
auto near_the_end = orders.slice(-3, -1);
std::cout << near_the_end;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
3    | 104       Tema    Monitor     3      320         0.08    
4    | 105       Kumasi  Headphones  4      42          0.05    
----------------------------------------------------------------
[2 cs x 6 feat]
```

`-3` normalizes to `6 - 3 = 3` and `-1` normalizes to `6 - 1 = 5`, so this returns
`[3, 5)` — cases `3` and `4`. Case `5`, the actual last row, is excluded, exactly as
`-1` in Python-style slicing means "stop right before the last element."

Example 3. Pass a `stop` well beyond the frame's size.

```cpp
auto to_the_end = orders.slice(4, 100);
std::cout << to_the_end;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
4    | 105       Kumasi  Headphones  4      42          0.05    
5    | 106       Kumasi  Monitor     2      NULL        0.12    
----------------------------------------------------------------
[2 cs x 6 feat]
```

`stop = 100` is clamped down to `ncases() = 6`, so this is equivalent to
`orders.slice(4, 6)` — a convenient way to say "from case `4` to the end" without
having to know or compute the frame's exact length first.

## Notes

- `slice()` never renumbers case labels; the result carries the source frame's
  original labels for the surviving rows.
- Both endpoints are clamped, never an error — an out-of-range `start`/`stop` degrades
  gracefully instead of throwing, unlike a single out-of-range position passed to
  [`take()`](take.md), which does raise.
- For "from the top" or "from the bottom" shortcuts, [`head()`](head.md) and
  [`tail()`](tail.md) read more clearly than an equivalent `slice()` call.
- For an arbitrary (non-contiguous, reorderable, repeatable) set of positions, use
  [`take()`](take.md) instead.

## See Also

- [`DataFrame::take`](take.md): the more general operation `slice()` is built on top
  of.
- [`DataFrame::head`](head.md) / [`DataFrame::tail`](tail.md): common special cases of
  a slice, from the top or bottom respectively.
- [`DataFrame::view`](view.md): the same `[start, stop)` semantics, wrapped in a
  `DataFrameView` with an optional auto-render toggle.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skew.html">
    <span class="label">Previous</span>
    <span class="title">skew</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
