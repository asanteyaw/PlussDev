<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7cc6b0a60905 -->

# DataFrameLabelAccessor::drop

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameLabelAccessor::drop(pluss::msharp::RangeView::Of<std::string_view> names) const
DataFrame DataFrameLabelAccessor::drop(std::initializer_list<std::string_view> names) const
DataFrame DataFrameLabelAccessor::drop(const DataFrame::LabelDropOptions& options) const
```

## What It Does

Has two entirely different behaviors depending on how it's called:

- **The `names`-only overloads, and `drop(options)` with both `duplicated` and
  `repeated` left `false`**: drop **case-label levels** by name from a
  structured (multi-level) frame — not rows. The remaining levels are
  rejoined into a new flat case-label slate. Feature columns are untouched.
- **`drop(options)` with `duplicated = true` or `repeated = true`**: drop
  **rows** instead, filtering out whichever rows [`duplicated()`](duplicated.md)
  or [`repeated()`](repeated.md) (restricted to `options.names`, if given)
  would mark `true`. This mode requires structured labels only if `options.names`
  is non-empty; with `options.names` empty, it keys off the full flat case
  label.

## Parameters

**names** (name-only overloads)

The level name(s) to remove. Requires a structured frame; every name must
match an existing level.

**options** (`const DataFrame::LabelDropOptions&`)

`names` (level names, or row-filter key names — see above), `duplicated` /
`repeated` (switch to row-filter mode; mutually exclusive), and `keep_first` /
`keep_last` (which occurrence `duplicated`-mode row-filtering spares).

## Returns

`DataFrame`
    Either the same rows with fewer case-label levels, or the same levels with
    fewer rows, depending on which mode was used.

## Examples

Example 1. Drop a level by name.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region", false);
std::cout << labeled;

auto out = labeled.labels().drop({"region"});
std::cout << out;
```

### Output (before)

```text
=====================
      | region  sales
---------------------
east  | east    100  
west  | west    200  
east  | east    150  
north | north   90   
---------------------
[4 cs x 2 feat]
```

### Output (level dropped)

```text
====================
     | region  sales
--------------------
0    | east    100  
1    | west    200  
2    | east    150  
3    | north   90   
--------------------
[4 cs x 2 feat]
```

Dropping the only level leaves the frame with no structured case labels at
all — cases fall back to a default integer index. `region` is still a feature
column (it was never dropped from the features, only from the case-label
structure) and its values are untouched.

Example 2. Row-filter with `duplicated = true` — removes extra occurrences.

```cpp
auto out2 = labeled.labels().drop(DataFrame::LabelDropOptions{.duplicated = true});
std::cout << out2;
```

### Output

```text
=====================
      | region  sales
---------------------
east  | east    100  
west  | west    200  
north | north   90   
---------------------
[3 cs x 2 feat]
```

With `keep_first` defaulting to `true`, the *second* `"east"` row is the
duplicate and gets dropped; the first survives.

Example 3. Row-filter with `repeated = true` — removes every occurrence of a
repeated value.

```cpp
auto out3 = labeled.labels().drop(DataFrame::LabelDropOptions{.repeated = true});
std::cout << out3;
```

### Output

```text
=====================
      | region  sales
---------------------
west  | west    200  
north | north   90   
---------------------
[2 cs x 2 feat]
```

Unlike `duplicated`, `repeated` doesn't spare a first occurrence — since
`"east"` appears more than once at all, **every** `"east"` row is dropped.

## Notes

- `duplicated` and `repeated` cannot both be `true` on the same call.
- In level-drop mode, dropping every level leaves the frame with default
  integer case labels (like [`reset(..., drop=true)`](reset.md), but without
  touching feature columns either way).
- In row-filter mode with `options.names` non-empty, those names key the
  filter but the printed case-label column still shows the **full** flat case
  label, not just the selected level(s) — matching
  [`duplicated(names)`](duplicated.md)'s own display behavior.

## See Also

- [`DataFrameLabelAccessor::duplicated`](duplicated.md),
  [`DataFrameLabelAccessor::repeated`](repeated.md): the row-marking primitives
  this method's row-filter mode is built on.
- [`DataFrameLabelAccessor::reset`](reset.md): clears case-label structure
  while optionally restoring the old values as feature columns, the opposite of
  this method's level-drop mode (which never restores anything).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
