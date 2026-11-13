<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f135e47c2487 -->

# DataFrame::tail

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrameView DataFrame::tail(std::int64_t n = 5, bool render = false) const
```

## What It Does

Returns a new frame containing the last `n` cases (rows) of the current frame, in
their existing order, with every feature preserved — the mirror image of
[`head()`](head.md), which takes from the top instead of the bottom.

`tail()` is a read-only, non-mutating view: it does not change case order, does not
drop or rename features, and does not recompute anything. Unlike [`take()`](take.md)
or [`slice()`](slice.md), case labels on the result are **not** the surviving rows'
original labels renumbered from `0` — they are preserved exactly as they were in the
source frame, so the last three rows of a six-row frame are still labeled `3`, `4`,
`5`, not `0`, `1`, `2`.

## Parameters

**n** (`std::int64_t`, default `5`)

Number of cases to keep, counted from the bottom. Defaults to `5` rows. Values larger
than `ncases()` are clamped to the full frame instead of erroring.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical in behavior to
[`head()`](head.md)'s `render` parameter: `false` (the default) gives you a quiet view
you print explicitly; `true` allows the view to auto-render itself when used as a
standalone temporary expression.

## Returns

`DataFrameView`
    A view over a new frame holding the last `n` cases and every feature of the
    source frame, in the source frame's feature order and with the source frame's
    original case labels.

## Examples

Example 1. Preview the last three rows of a six-row order log.

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

auto recent = orders.tail(3);
std::cout << recent;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
3    | 104       Tema    Monitor     3      320         0.08    
4    | 105       Kumasi  Headphones  4      42          0.05    
5    | 106       Kumasi  Monitor     2      NULL        0.12    
----------------------------------------------------------------
[3 cs x 6 feat]
```

The row labels are `3`, `4`, `5` — the original positions in `orders` — not `0`, `1`,
`2`. `tail()` never renumbers; it only changes which cases are included.

Example 2. Ask for more rows than the frame has.

```cpp
auto whole_frame = orders.tail(100);  // orders only has 6 cases
```

`whole_frame` is equivalent to the full `orders` frame: `tail()` clamps `n` to
`ncases()` rather than raising, exactly like `head()` does.

## Notes

- For the top of a frame instead of the bottom, use [`head(n, render)`](head.md),
  which has an identical signature and clamping behavior.
- Case labels are preserved verbatim from the source frame — if you need a fresh
  `0`-based index on the result, combine this with [`label().set(...)`](labels.md) or
  a reset step rather than expecting `tail()` to renumber for you.
- `tail()` never triggers a copy of the underlying column storage beyond what is
  needed to build the smaller result frame.

## See Also

- [`DataFrame::head`](head.md): the same idea from the top of the frame.
- [`DataFrame::slice`](slice.md): an arbitrary `[start, stop)` case range, of which
  `tail(n)` is really just `slice(ncases() - n, ncases())`.
- [`DataFrame::sample`](sample.md): a random subset of cases instead of a positional
  suffix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
