<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ecbb90c08950 -->

# DataFrame::tabelize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame::tabelize(const pluss::kindle::Catalog& catalog) -> DataFrame
static DataFrame::tabelize(const pluss::kindle::Catalog& catalog, const pluss::kindle::TabelizePolicy& policy) -> DataFrame
```

## What It Does

The reverse of [`kindelize()`](kindelize.md): rebuilds a `DataFrame` from a
`pluss::kindle::Catalog`'s keyed collection of `Stream`s, aligning them back into one
shared table. Each `Catalog` key becomes one output feature, keyed by that same
name; the frame's case labels are reconstructed from the streams' own attached
labels according to `policy`. This is a **static** factory method — call it as
`DataFrame::tabelize(catalog)`, not on an existing frame instance.

When every stream in `catalog` already carries the exact same set of case labels
(as is true right after [`kindelize()`](kindelize.md) with its default
`attach_labels = true`), the default `TabelizePolicy` reconstructs the original frame
exactly — `tabelize()` and `kindelize()` round-trip.

## Parameters

**catalog** (`const pluss::kindle::Catalog&`)

The catalog to convert. Must be a defined `Catalog`; an undefined one throws
`"DataFrame::tabelize requires a defined Catalog"`.

**policy** (`const pluss::kindle::TabelizePolicy&`, optional)

Controls how streams are aligned back into one shared table when their label sets
don't already match exactly. Key fields:

- **`align`** (`Align`, default `Inner`): how to reconcile streams whose label sets
  differ — `Inner` (only labels common to every stream), `Left`/`Right` (exactly one
  stream's labels), `Outer` (union of every stream's labels), or `Strict` (every
  stream's labels must already match the anchor exactly).
- **`align_by`** (`AlignBy`, default `Label`): align by label value, or by plain
  position (`Pos`) — falls back to `Pos` automatically if any stream lacks labels.
- **`fill`** (`FillPolicy`, default `Null`): how to fill a cell with no matching
  source row — `Null`, a `fill_scalar`, or forward/backward fill.
- **`include_label_feature`** (`bool`, default `false`): when `true`, also adds the
  reconstructed case-label thread as its own extra feature, named by
  **`label_feature`** (default `"label"`).

## Returns

`DataFrame`
    A new frame with one feature per catalog key (in the catalog's key order), plus
    one extra `label_feature`-named feature when `policy.include_label_feature` is
    `true`.

## Examples

Example 1. Round-trip a frame through `kindelize()` and back through `tabelize()`
with the default policy.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"sales", Thread(pluss::make_series({100.0, 150.0, 90.0}))},
}, Thread(pluss::make_note({"o1", "o2", "o3"})));
std::cout << orders;
```

### Output (before)

```text
==========================
     | id    city    sales
--------------------------
o1   | 1     Accra   100  
o2   | 2     Tema    150  
o3   | 3     Kumasi  90   
--------------------------
[3 cs x 3 feat]
```

```cpp
auto catalog = orders.kindelize();
auto rebuilt = DataFrame::tabelize(catalog);
std::cout << rebuilt;
```

### Output (after)

```text
==========================
     | id    city    sales
--------------------------
o1   | 1     Accra   100  
o2   | 2     Tema    150  
o3   | 3     Kumasi  90   
--------------------------
[3 cs x 3 feat]
```

`rebuilt` is identical to `orders`: `kindelize()`'s default `attach_labels = true`
gave every stream the same three case labels, so `tabelize()`'s default `Inner`
alignment finds every label common to every stream — which is all three — and
reconstructs the original table exactly.

Example 2. Materialize the reconstructed case labels as their own extra feature.

```cpp
pluss::kindle::TabelizePolicy policy;
policy.include_label_feature = true;

auto with_label_feature = DataFrame::tabelize(catalog, policy);
std::cout << with_label_feature;
```

### Output

```text
=================================
     | id    city    sales  label
---------------------------------
o1   | 1     Accra   100    o1   
o2   | 2     Tema    150    o2   
o3   | 3     Kumasi  90     o3   
---------------------------------
[3 cs x 4 feat]
```

A fourth feature, `"label"` (`policy.label_feature`'s default), appears holding the
same case-label values (`o1`, `o2`, `o3`) as a regular text column, in addition to
them still being the frame's case labels.

## Notes

- `tabelize()` is a **static** method — call it as `DataFrame::tabelize(catalog,
  ...)`, there is no non-static instance overload.
- The default `TabelizePolicy` reconstructs the original frame exactly only when
  every stream in `catalog` shares the same label set; streams with differing label
  sets require choosing an explicit `align`/`fill` policy to control how gaps are
  resolved.
- `include_label_feature`'s collision handling mirrors
  [`kindelize()`](kindelize.md)'s `materialize_label`: `policy.collision` decides
  whether a name clash with `label_feature` throws, gets suffixed, or overwrites.

## See Also

- [`DataFrame::kindelize`](kindelize.md): the reverse conversion this method
  round-trips with.
- [`DataFrame::load_csv`](load_csv.md), [`DataFrame::load_json`](load_json.md): other
  static factory methods for building a frame from an external source.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
