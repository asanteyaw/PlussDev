<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a13c8f867cdd -->

# DataFrame::align

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::align(const AlignSpec& spec) const
DataFrame DataFrame::align(const Slate& new_label) const
```

## What It Does

Reindexes the frame onto a new target case-label axis, a new target feature order, or
both at once — the general-purpose "make this frame line up with a shape I already
know" tool. Every source case is matched to a target case *by label value*, not by
physical row position: a case label that appears in the target axis is looked up
among the source's rows carrying that same label (matched in order, for the rare case
of repeated labels), and a target label that has no match in the source becomes a
brand-new row, filled with the null cell (or with `fill`, if provided).

`align(new_label)` is the shorthand that only touches the case axis, leaving every
feature untouched. `align(spec)` is the full form: an `AlignSpec` can reorder features
independently of case labels, introduce brand-new feature columns that don't exist in
the source at all (also null- or fill-filled), or restrict the output to a smaller
feature subset — all in the same call.

## Parameters

**new_label** (`const Slate&`, case-only overload)

The target case-label axis, in the exact order you want the output rows in. Equivalent
to `align(AlignSpec{.label = new_label})`.

**spec** (`const AlignSpec&`)

- **`label`** (`std::optional<Slate>`, default unset): target case-label axis. Leave
  unset to keep the frame's existing case axis unchanged while still reordering
  features via `features`.
- **`features`** (`std::vector<std::string>`, default empty): target feature order.
  Leave empty to keep every existing feature, in its existing order. A name in this
  list that doesn't exist on the source frame becomes a brand-new, entirely missing
  column in the output (filled with `fill`, if set) rather than an error — `align()`
  never rejects an unknown target feature name the way [`features()`](features.md)
  does.
- **`fill`** (`std::optional<double>`, default unset): a numeric value used to fill
  any newly introduced gap — a target case label absent from the source, or a target
  feature absent entirely — instead of leaving it as `NULL`. `fill` only ever applies
  to `Series`-kind (numeric) columns; a `Note`/`Timeline`/`TimeIndex` gap is always
  `NULL` regardless of `fill`, since there is no sensible way to "fill" a missing
  string or date with a `double`.

## Returns

`DataFrame`
    A new frame reindexed onto the requested case axis and/or feature order. Existing
    cells are carried over unchanged; new gaps are `NULL` unless `fill` says otherwise.

## Examples

Example 1. Reindex onto a new case-label order that includes one label absent from
the source.

```cpp
using namespace pluss::table;

auto prices = DataFrame({
    {"price", Slate(pluss::make_series({10.0, 20.0, 30.0}))},
    {"grade", Slate(pluss::make_note({"A", "B", "C"}))},
}, Slate(pluss::make_note({"p1", "p2", "p3"})));

auto target = Slate(pluss::make_note({"p2", "p4", "p1"}));
auto reindexed = prices.align(target);
std::cout << reindexed;
```

### Output

```text
===================
     | price  grade
-------------------
p2   | 20     B    
p4   | NULL   NULL 
p1   | 10     A    
-------------------
[3 cs x 2 feat]
```

`p4` doesn't exist in `prices`, so it becomes a brand-new row filled with `NULL` in
every feature — including `grade`, a `Note` column, since no `fill` was supplied here.
`p2` and `p1` are matched by label and reordered to the requested position, not left in
their original physical order.

Example 2. Restrict and reorder the feature set, introduce a feature that doesn't
exist at all, and fill every resulting gap with `0.0`.

```cpp
auto out = prices.align(DataFrame::AlignSpec{
    .label = target,
    .features = {"price", "bonus"},
    .fill = 0.0,
});
std::cout << out;
```

### Output

```text
===================
     | price  bonus
-------------------
p2   | 20     0    
p4   | 0      0    
p1   | 10     0    
-------------------
[3 cs x 2 feat]
```

`grade` is dropped entirely since it isn't in the requested `features` list. `bonus`
doesn't exist anywhere on `prices`, so the *entire* column is manufactured from
scratch — and because `fill = 0.0` is set, it comes back fully valid (`0`) rather than
`NULL`, exactly like the `p4` gap in `price`. Without `fill`, both would have been
`NULL` instead.

Example 3. List the same target feature name twice, to see it throw.

```cpp
try {
  auto out = prices.align(DataFrame::AlignSpec{.features = {"price", "price"}});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::align: duplicate target feature 'price'
```

Two identical entries in `features` would mean two output columns both
called `"price"` — ambiguous, so it's rejected rather than silently
producing a duplicate or overwriting one with the other.

## Notes

- Matching is by case-label *value*, not physical position — this is what
  distinguishes `align()` from a plain [`take()`](take.md)-style reorder.
- A missing target feature is manufactured wholesale rather than rejected, unlike
  [`features()`](features.md)'s multi-name projection, which throws on an unknown
  name.
- `fill` only rescues `Series`-kind gaps from `NULL`; text and calendar features stay
  `NULL` regardless.
- [`DataFrame::conform`](conform.md) builds on `align()`: it matches feature order to
  another frame's exact feature list first, then calls the case-only overload of
  `align()` to match that frame's case order too.

## See Also

- [`DataFrame::conform`](conform.md): align both feature order and case order to
  match another frame's structure in a single call.
- [`DataFrame::features`](features.md): a stricter feature-subset projection that
  rejects unknown names instead of manufacturing them.
- [`DataFrame::take`](take.md): reorder/select cases by physical position instead of
  by label.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate.html">
    <span class="label">Previous</span>
    <span class="title">aggregate</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
