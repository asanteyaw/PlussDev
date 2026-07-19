<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ef82b71d1d4b -->

# DataFrame::head

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::head(std::int64_t n = 5, bool render = false) -> DataFrameView
```

## What It Does

Returns a new frame containing the first `n` cases (rows) of the current frame, in their
existing order, with every feature preserved. It is the cheapest way to eyeball what a
frame actually looks like — column names, dtypes-by-example, and a handful of real
values — without printing the entire thing, which matters once a frame has more than a
screenful of rows.

`head()` is a read-only, non-mutating view: it does not change case order, does not
drop or rename features, and does not recompute anything. If `n` is greater than or equal
to the number of cases in the frame, the entire frame is returned unchanged. If `n` is
`0`, the result has the same features but zero cases — useful for grabbing an
empty-but-typed frame with the same schema, for example as a seed value before
repeatedly calling [`extend`](extend.md) in a loop.

`head()` looks at position, not value: it always returns the *first* `n` cases as
currently ordered on the frame. If you want the first `n` cases after sorting by some
key, call [`sort_values`](sort_values.md) first and then `head()` the result — the two
compose exactly like `df.sort_values(...).head(...)` reads.

## Parameters

**n** (`std::int64_t`, default `5`)

Number of cases to keep, counted from the top. Defaults to `5` rows. Negative values are
not a supported "everything except the last
`|n|` rows" idiom here — pass an explicit non-negative row count. Values larger than
`ncases()` are clamped to the full frame instead of erroring.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`.

- `false` (the default): you get an ordinary "quiet" view back, and you decide
  explicitly when and how to print it (with `show()`, `to_string()`, or by streaming it
  with `operator<<`).
- `true`: the returned view is allowed to auto-render itself the moment it reaches the
  end of a temporary expression's lifetime without being captured or streamed anywhere
  (for instance, calling `df.head(3, true);` as its own statement will print the result
  immediately). This exists for quick, interactive exploration; prefer `false` and an
  explicit print call inside real application code so output timing stays predictable.

## Returns

`DataFrameView`
    A view over a new frame holding the first `n` cases and every feature of the source
    frame, in the source frame's feature order. `DataFrameView` behaves like a
    `DataFrame` everywhere it matters (it inherits from it) — the "view" distinction only
    affects auto-render behavior via `render`, plus a source/operation label used when a
    GUI viewer window is opened.

## Examples

Example 1. Preview the first three rows of a six-row order log.

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

auto preview = orders.head(3);
std::cout << preview;
```

### Output

```text
===============================================================
     | order_id  city   product     units  unit_price  discount
---------------------------------------------------------------
0    | 101       Accra  Laptop      2      1200        0.1     
1    | 102       Accra  Headphones  5      45.5        0.05    
2    | 103       Tema   Laptop      1      1150        NULL    
---------------------------------------------------------------
[3 cs x 6 feat]
```

All six features are still present; only the case count changed from `6` to `3`. Row `2`
carries a `NULL` for `discount` exactly as it did in the source frame — `head()` never
alters values, only which cases are included.

Example 2. Use the default argument to grab a standard five-row preview.

```cpp
auto preview = orders.head();  // same as orders.head(5)
```

Calling `head()` with no arguments returns the first `5` rows, the default most people
reach for first when opening an unfamiliar frame.

Example 3. Ask for more rows than the frame has.

```cpp
auto preview = orders.head(100);  // orders only has 6 cases
```

`preview` is equivalent to the full `orders` frame (all `6` cases): `head()` clamps `n`
to `ncases()` rather than raising, so it is always safe to call with a generously large
`n` when you simply want "the whole frame, if it's small" without checking its size
first.

## Notes

- `head()` composes naturally with the rest of the pipeline-style API:
  `orders.sort_values({{.feature = "unit_price", .ascending = false}}).head(3)` gives you
  the three highest-priced orders without a separate sort-then-slice step.
- For the tail end of a frame instead of the top, use [`tail(n, render)`](tail.md), which
  has an identical signature and clamping behavior.
- `head()` never triggers a copy of the underlying column storage beyond what is needed
  to build the smaller result frame — cheap enough to call repeatedly while exploring
  data interactively.

## See Also

- [`DataFrame::tail`](tail.md): the same idea from the bottom of the frame.
- [`DataFrame::sample`](sample.md): a random subset of cases instead of a positional
  prefix.
- [`DataFrame::slice`](slice.md): an arbitrary `[start, stop)` case range.
- [`DataFrame::info`](info.md): a structural summary when you want dtypes and null
  counts rather than a value preview.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="info.html">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>
