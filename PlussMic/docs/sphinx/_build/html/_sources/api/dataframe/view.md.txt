<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bff939dc9118 -->

# DataFrame::view

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::view(std::int64_t start, std::int64_t stop, bool render = false) const -> DataFrameView
```

## What It Does

Returns a `[start, stop)` row range wrapped in a `DataFrameView`, the same renderable
wrapper [`head()`](head.md) and [`tail()`](tail.md) return. `view(start, stop)` uses
the exact same slice normalization as [`slice()`](slice.md) — both endpoints support
Python-style negative indices, both are clamped into range, and `stop` is exclusive on
both ends, positive or negative. The only thing `view()` adds on top of `slice()` is
the `render` toggle: `view(start, stop, render)` is `slice(start, stop)` dressed up in
a `DataFrameView`.

## Parameters

**start** (`std::int64_t`)

The first case position to include. A negative value counts backward from the end of
the frame (`-1` is the last case), then the result is clamped into range — identical
to `slice()`'s `start` handling.

**stop** (`std::int64_t`)

The case position to stop before (exclusive), positive or negative — identical to
`slice()`'s `stop` handling. If the normalized `stop` ends up before the normalized
`start`, the result is an empty (zero-case) frame.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical in behavior to
[`head()`](head.md)'s `render` parameter.

## Returns

`DataFrameView`
    A view over a new frame holding the cases in `[start, stop)` and every feature of
    the source frame, with the source frame's original case labels preserved.

## Examples

Example 1. A contiguous middle range with ordinary positive indices.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto middle = orders.view(1, 4);
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

`orders.view(1, 4)` and `orders.slice(1, 4)` return identical results — cases `1`
through `3` — since `view()` uses `slice()`'s exact endpoint rules.

Example 2. Use a negative `stop` — normalized and made exclusive exactly like
`slice()`.

```cpp
auto near_the_end = orders.view(1, -1);
std::cout << near_the_end;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
1    | 102       Accra   Headphones  5      45.5        0.05    
2    | 103       Tema    Laptop      1      1150        NULL    
3    | 104       Tema    Monitor     3      320         0.08    
----------------------------------------------------------------
[3 cs x 6 feat]
```

`-1` normalizes to `6 - 1 = 5`, so this returns `[1, 5)` — cases `1` through `3`,
excluding case `5` — identical to what `orders.slice(1, -1)` returns. Use
`orders.view(1, orders.ncases())`, or simply [`tail()`](tail.md), when you want the
range to run all the way through the last row.

Example 3. Toggle auto-render.

```cpp
auto quiet = orders.view(0, 2, false);  // ordinary quiet view; print it explicitly
```

With `render = false` (the default), the returned `DataFrameView` never prints itself
on its own — you decide when and how, exactly as with `head()` and `tail()`.

## Notes

- `view()`'s `start` and `stop` handling is identical to `slice()`'s in every respect
  — negative-index normalization, clamping, and an exclusive `stop`. The only
  difference between the two methods is the `render` toggle and the fact that `view()`
  always returns a `DataFrameView` rather than a plain `DataFrame`.
- Case labels on the result are the source frame's original labels, never renumbered
  — same as `slice()`, `head()`, and `tail()`.
- The `render` parameter works exactly as it does on `head()` and `tail()`.

## See Also

- [`DataFrame::slice`](slice.md): the equivalent plain-`DataFrame` method, without the
  `render` toggle.
- [`DataFrame::head`](head.md) / [`DataFrame::tail`](tail.md): common special cases,
  also returning a `DataFrameView`.
- [`DataFrame::observe`](observe.md): a single-row or range view built on the same
  underlying slice mechanics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
