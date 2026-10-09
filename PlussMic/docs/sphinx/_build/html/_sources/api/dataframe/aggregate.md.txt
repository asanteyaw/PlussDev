<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d03113246cdc -->

# DataFrame::aggregate

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::aggregate(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = false) const -> DataFrameView
```

## What It Does

Collapses the whole frame down to a single summary row: each `SummarySpec` in `specs`
names a source feature, a reduction (`SummaryOp`), and an output column name, and
`aggregate()` computes one scalar per spec, packing all of them side by side into a
one-row `DataFrameView` labeled `"summary"`. It is the "dashboard row" counterpart to
[`describe()`](describe.md) — where `describe()` gives you a fixed, canned set of
statistics per feature, `aggregate()` lets you hand-pick exactly which statistics on
exactly which features go into a single, custom summary row, in whatever order you
like.

Internally, `aggregate()` is a thin `DataFrameView` wrapper around the frame-oriented
`summarize()`; use `summarize()` directly if you want the plain one-row `DataFrame`
without the view's auto-render behavior.

## Parameters

**specs** (`pluss::RangeView<SummarySpec>`)

An ordered list of `SummarySpec{output, feature, op}` instructions, one per output
column:

- **`feature`** (`std::string`): the source column to reduce. Cannot be empty.
- **`op`** (`SummaryOp`, default `Mean`): the reduction to apply —
  `Count`, `Ndistinct`, `Sum`, `Mean`, `Median`, `Min`, `Max`, `Std`, `Var`, or `Prod`.
  `Count` and `Ndistinct` accept a feature of any kind (they count non-null values or
  distinct non-null values respectively); every other op requires `feature` to be a
  numeric `Series` and throws `"DataFrame::summarize: numeric reductions require
  numeric Series feature"` otherwise.
- **`output`** (`std::string`): the name of the resulting column. Leave it empty to
  auto-generate one as `"{feature}_{op_name}"` (for example, `amount_max` for
  `{"", "amount", SummaryOp::Max}`) — note the auto-generated name for `Ndistinct` is
  `"{feature}_ndistinct"`, not `"{feature}_nunique"`.

Two specs that resolve to the same output name (whether both explicit or one
auto-generated) are rejected with `"DataFrame::summarize: duplicate output name
'<name>'"` — give every spec a distinct `output`, or vary the auto-generated name by
using different `feature`/`op` combinations.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, following the same convention as
[`head()`](head.md): `false` returns a quiet view you print explicitly; `true` allows
the view to auto-render itself in a temporary-expression context.

## Returns

`DataFrameView`
    A one-row view (case label `"summary"`) with one output column per spec, in the
    order the specs were given.

## Examples

Example 1. Build a compact summary row from three different reductions.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"region", Slate(pluss::make_note({"East", "West", "East", "West", "East"}))},
    {"amount", Slate(pluss::make_series({120.0, 85.0, 200.0, 60.0, 95.0}))},
});

auto summary = orders.aggregate({
    {"total_amount", "amount", DataFrame::SummaryOp::Sum},
    {"avg_amount", "amount", DataFrame::SummaryOp::Mean},
    {"n_orders", "region", DataFrame::SummaryOp::Count},
});
std::cout << summary;
```

### Output

```text
============================================
        | total_amount  avg_amount  n_orders
--------------------------------------------
summary | 560           112         5       
--------------------------------------------
[1 cs x 3 feat]
```

`total_amount` and `avg_amount` both reduce the numeric `amount` feature; `n_orders`
counts non-null entries of `region` instead — `Count` works on the `Note` feature
without complaint, since it only needs to count present values, not do arithmetic on
them.

Example 2. Leave `output` empty and let PlussMic auto-name the result column.

```cpp
auto summary = orders.aggregate({
    {"", "amount", DataFrame::SummaryOp::Max},
});
std::cout << summary;
```

### Output

```text
====================
        | amount_max
--------------------
summary | 200       
--------------------
[1 cs x 1 feat]
```

The output column is named `amount_max` — `"{feature}_{op_name}"` — since no explicit
name was given.

Example 3. Pass `render = true` and let the returned view print itself, with
no explicit `std::cout` at all.

```cpp
orders.aggregate({
    {"total_amount", "amount", DataFrame::SummaryOp::Sum},
}, true);
```

### Output

```text
======================
        | total_amount
----------------------
summary | 560         
----------------------
[1 cs x 1 feat]
```

Every other example on this page explicitly writes `std::cout << summary;`
after capturing the result in a variable. Here, the call is a bare statement:
the temporary `DataFrameView` is destroyed at the end of that statement
without ever being assigned or streamed, and `render = true` makes exactly
that destruction trigger an automatic print. With the default `render =
false`, the same bare-statement call would produce no output at all.

Example 4. A non-numeric feature under a numeric-only `SummaryOp`, and two
specs that collide on the same output name, each throw.

```cpp
try {
  auto bad = orders.aggregate({
      {"bad", "region", DataFrame::SummaryOp::Sum},
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto dup = orders.aggregate({
      {"x", "amount", DataFrame::SummaryOp::Sum},
      {"x", "amount", DataFrame::SummaryOp::Mean},
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::summarize: numeric reductions require numeric Series feature
DataFrame::summarize: duplicate output name 'x'
```

`region` is a `Note` feature; `Sum` needs an actual numeric `Series` to add
up, so it throws rather than treating text as `0`. The second call gives two
different specs the same explicit output name (`"x"`) — rejected up front,
before either reduction runs, exactly like an auto-generated name collision
would be.

## Notes

- Duplicate output names (whether explicit or auto-generated) are rejected up front,
  before any reduction runs — give each spec a distinct output name.
- `Count`/`Ndistinct` tolerate any feature kind; every other `SummaryOp` requires a
  numeric `Series` feature and throws otherwise.
- `Sum` on an all-null numeric feature (after dropping nulls) returns `0`, and `Prod`
  returns `1` — the identity element for each operation — rather than `NaN`; every
  other op returns `NaN` in that case.
- `aggregate()` always collapses to exactly one row; for a per-group breakdown instead
  of a single summary row, use [`group`](group.md).

## See Also

- [`DataFrame::describe`](describe.md): a fixed, canned set of per-feature statistics,
  computed without hand-writing `SummarySpec`s.
- [`DataFrame::group`](group.md): the grouped analogue — one summary row per key
  instead of one row for the whole frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="align.html">
    <span class="label">Next</span>
    <span class="title">align</span>
  </a>
</div>
