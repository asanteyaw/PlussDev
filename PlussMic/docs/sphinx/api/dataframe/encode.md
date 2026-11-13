<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c1bb75008010 -->

# DataFrame::encode

[Back to DataFrame API Reference](../../dataframe_api_reference)

```{note} New method
`DataFrame::encode()` did not exist prior to this page — it was added to close a real
gap in the categorical-encoding surface (see "What It Does" below) and is documented
here for the first time alongside its implementation.
```

## Signature

```cpp
DataFrame DataFrame::encode(std::string_view feature, EncodeMode mode = EncodeMode::OneHot, const EncodeOptions& options = {}) const
```

## What It Does

Encodes a single categorical feature into a machine-learning-friendly numeric form, in
one of two modes:

- **`EncodeMode::OneHot`** (the default): replaces `feature` with one `Bool` indicator
  column per distinct value — the standard "dummy variable" / `get_dummies()`-style
  encoding. Each new column is named `"{prefix}{value}"` and holds `true` exactly where
  the source row equals that value, `false` elsewhere, and `NULL` for any row where the
  source itself was missing (every indicator column agrees on which rows are `NULL` —
  a missing category never silently reads as "none of the above").
- **`EncodeMode::Ordinal`**: replaces `feature` in place with a single `Int64` column of
  integer codes, one per distinct value — internally, this mode is a thin wrapper
  around [`factor()`](factor.md), keeping only the `codes` and discarding the level
  list `factor()` would otherwise also give you.

Before `encode()`, the closest existing tools were [`factor()`](factor.md) (which
returns a *separate* `CategoryEncoding{codes, levels}` pair rather than a modified
frame) and [`note().indicate()`](../../dataframe_note_accessor_api_reference) (which does genuine multi-label indicator
expansion for delimiter-separated tag strings, and returns only the new columns rather
than the whole frame). `encode()` is the direct, single-call "turn this one column into
model-ready numbers, in place, on the whole frame" entry point either mode's use case
was previously missing.

## Parameters

**feature** (`std::string_view`)

The feature to encode. Must exist on the frame; throws
`"DataFrame::encode: unknown feature '<name>'"` otherwise. Works on a feature of any
`Slate` kind (`Series`, `Note`, `Timeline`, `TimeIndex`) — "distinct value" is
determined the same way [`factor()`](factor.md) determines it, not restricted to text.

**mode** (`EncodeMode`, default `OneHot`)

`OneHot` or `Ordinal`, as described above.

**options** (`const EncodeOptions&`)

- **`prefix`** (`std::string`, default empty, `OneHot` only): prefix for each new
  indicator column's name. Leave empty to default to `"{feature}_"`.
- **`drop_first`** (`bool`, default `false`, `OneHot` only): omit the indicator column
  for the first-encountered distinct value. Useful when you specifically need
  `k - 1` indicator columns for `k` categories (avoiding the "dummy variable trap" of
  perfectly collinear indicators expected by some downstream linear models).
- **`sort_levels`** (`bool`, both modes): `false` (the default) orders distinct values
  by first appearance in the frame; `true` sorts them lexicographically instead. This
  determines both the indicator-column order in `OneHot` mode and the code assignment
  in `Ordinal` mode.
- **`ordered`** (`bool`, default `false`, `Ordinal` only): forwarded directly to
  [`factor()`](factor.md)'s own `ordered` flag.

## Returns

`DataFrame`
    A new frame with `feature` replaced: by its indicator columns (`OneHot`, inserted
    at `feature`'s original position, expanding the frame by `level_count - 1` columns,
    or `level_count - 2` with `drop_first`) or by a single `Int64` code column
    (`Ordinal`, same column count as before). Every other feature is unchanged.

## Examples

Example 1. One-hot encode a categorical feature with the defaults, including a
missing value.

```cpp
using namespace pluss::table;

// The last row's city is missing; construct that NULL cell directly via an
// explicit companion Bool validity Series (see count.md).
auto city_valid = pluss::make_series({true, true, true, true, false}, pluss::ScalarType::Bool);

auto customers = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3, 4, 5}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", ""}), city_valid)},
    {"score", Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}))},
});
std::cout << customers;
```

### Output

```text
==========================
     | id    city    score
--------------------------
0    | 1     Accra   10   
1    | 2     Tema    20   
2    | 3     Accra   30   
3    | 4     Kumasi  40   
4    | 5     NULL    50   
--------------------------
[5 cs x 3 feat]
```

```cpp
auto one_hot = customers.encode("city");
std::cout << one_hot;
```

### Output

```text
======================================================
     | id    city_Accra  city_Tema  city_Kumasi  score
------------------------------------------------------
0    | 1     true        false      false        10   
1    | 2     false       true       false        20   
2    | 3     true        false      false        30   
3    | 4     false       false      true         40   
4    | 5     NULL        NULL       NULL         50   
------------------------------------------------------
[5 cs x 5 feat]
```

`city` is replaced, in place, by three `Bool` columns — `city_Accra`, `city_Tema`,
`city_Kumasi`, in first-seen order, prefixed with the default `"city_"`. Row `4`
(originally a missing city) is `NULL` in all three, not `false` in all three — the
distinction matters downstream, since "no city recorded" and "confirmed to be none of
these three cities" are different claims. `id` and `score` are untouched, and the new
columns land exactly where `city` used to be.

Example 2. Drop the first indicator column to avoid the dummy-variable trap.

```cpp
EncodeOptions drop_opts;
drop_opts.drop_first = true;

auto reduced = customers.encode("city", EncodeMode::OneHot, drop_opts);
std::cout << reduced;
```

### Output

```text
==========================================
     | id    city_Tema  city_Kumasi  score
------------------------------------------
0    | 1     false      false        10   
1    | 2     true       false        20   
2    | 3     false      false        30   
3    | 4     false      true         40   
4    | 5     NULL       NULL         50   
------------------------------------------
[5 cs x 4 feat]
```

`city_Accra` (the first-seen level) is omitted. A row can now still be inferred as
`Accra` when both remaining indicators are `false` — the classic "one fewer column
than categories" encoding some linear models expect to avoid collinear predictors.

Example 3. Ordinal mode: integer codes in place of the text, rather than new columns.

```cpp
auto ordinal = customers.encode("city", EncodeMode::Ordinal);
std::cout << ordinal;
```

### Output

```text
========================
     | id    city  score
------------------------
0    | 1     0     10   
1    | 2     1     20   
2    | 3     0     30   
3    | 4     2     40   
4    | 5     NULL  50   
------------------------
[5 cs x 3 feat]
```

`city` keeps its name and position, but is now an `Int64` `Series` of codes
(`Accra -> 0`, `Tema -> 1`, `Kumasi -> 2`, assigned in first-seen order) instead of a
`Note` of text. The originally-missing row `4` stays `NULL` rather than becoming a
fourth, spurious category code.

## Notes

- `OneHot` mode only ever compares distinct-value *keys* (the same string-key
  comparison [`factor()`](factor.md) uses via `unify_key_parts`), so it works on
  numeric or calendar features too, not just `Note` text — a small-cardinality
  `Series` of status codes can be one-hot encoded exactly the same way.
- A missing source value propagates as `NULL` to every generated indicator column in
  `OneHot` mode, and to the single code column in `Ordinal` mode — it is never
  silently coerced into `false` or into its own numeric code.
- `sort_levels` controls category order (and therefore both indicator-column order and
  which value gets dropped by `drop_first`, or which value gets code `0`) — pass
  `true` for a stable, data-independent ordering across repeated runs on frames whose
  row order might change.
- `Ordinal` mode is a thin convenience wrapper: `customers.encode(feature,
  EncodeMode::Ordinal, opts)` is equivalent to taking `factor(feature, opts.ordered,
  opts.sort_levels).codes` and splicing it back into the frame at `feature`'s position
  — reach for [`factor()`](factor.md) directly if you also need the level list.

## See Also

- [`DataFrame::factor`](factor.md): the lower-level label-encoding primitive
  `encode()`'s `Ordinal` mode wraps, returning both codes and the level list
  separately rather than a modified frame.
- [`DataFrame::cast`](cast.md): change a feature's dtype or payload kind outright,
  for conversions that aren't about categorical levels at all.
- [`DataFrame::bucketize`](bucketize.md): turn a *continuous* numeric feature into a
  small number of discrete bucket codes, upstream of a categorical encoding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
