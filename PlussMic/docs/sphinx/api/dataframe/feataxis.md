<!-- pluss-hand-authored -->

# DataFrame::feataxis

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::feataxis() const -> DataFrameFeatureAxisAccessor
```

## What It Does

Enters a small, dedicated accessor for selecting or renaming features by *name
pattern* or by *payload kind*, rather than by an explicit list. `feataxis()` itself
does nothing but bind the accessor to the current frame — the actual work happens on
the accessor it returns, which exposes exactly seven methods:

- `like(needle, case_sensitive = true)`: keep features whose name contains `needle`.
- `match(pattern)`: keep features whose name matches an ECMAScript regex.
- `series()` / `note()` / `timeline()`: keep features of one particular `Thread` kind.
- `prefix(text)` / `suffix(text)`: rename **every** feature on the frame by prepending
  or appending `text` to its name.

This complements [`features(names)`](features.md), which selects an explicit,
already-known list of names — `feataxis()` is for the "I don't want to spell out every
name" case: selecting all the `"score_"`-prefixed columns, or every numeric feature,
without listing them one by one.

Every one of the accessor's seven methods has its own dedicated, example-rich
reference page — start at the
[DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)
and use its left sidebar to jump straight to any method.

## Parameters

`feataxis()` itself takes no parameters. See each accessor method's own signature
above for what it accepts.

## Returns

`DataFrameFeatureAxisAccessor`
    A lightweight accessor bound to the current frame. Every method it exposes returns
    a new `DataFrame` (a selected subset, or the same columns under new names); the
    source frame is never mutated.

## Examples

Example 1. Select every feature whose name contains a substring.

```cpp
using namespace pluss::table;

auto stats = DataFrame({
    {"id",          Thread(pluss::make_series({1, 2, 3}))},
    {"sales_2023",  Thread(pluss::make_series({100.0, 150.0, 120.0}))},
    {"sales_2024",  Thread(pluss::make_series({110.0, 130.0, 140.0}))},
    {"region",      Thread(pluss::make_note({"East", "West", "East"}))},
});
std::cout << stats;
```

### Output

```text
===========================================
     | id    sales_2023  sales_2024  region
-------------------------------------------
0    | 1     100         110         East  
1    | 2     150         130         West  
2    | 3     120         140         East  
-------------------------------------------
[3 cs x 4 feat]
```

```cpp
auto sales_only = stats.feataxis().like("sales");
std::cout << sales_only;
```

### Output

```text
=============================
     | sales_2023  sales_2024
-----------------------------
0    | 100         110       
1    | 150         130       
2    | 120         140       
-----------------------------
[3 cs x 2 feat]
```

`id` and `region` are dropped — neither name contains `"sales"` — leaving only the two
year-over-year sales columns, in their original order.

Example 2. Select every numeric feature by payload kind.

```cpp
auto numeric_only = stats.feataxis().series();
std::cout << numeric_only;
```

### Output

```text
===================================
     | id    sales_2023  sales_2024
-----------------------------------
0    | 1     100         110       
1    | 2     150         130       
2    | 3     120         140       
-----------------------------------
[3 cs x 3 feat]
```

`region` (a `Note` feature) is dropped; `id`, `sales_2023`, and `sales_2024` (all
`Series`) are kept, in their original relative order. The mirror-image
`.note()` and `.timeline()` methods keep only `Note` or `Timeline`/`TimeIndex`
features instead.

Example 3. Rename every feature at once with a prefix.

```cpp
auto renamed = stats.feataxis().prefix("q_");
std::cout << renamed;
```

### Output

```text
=================================================
     | q_id  q_sales_2023  q_sales_2024  q_region
-------------------------------------------------
0    | 1     100           110           East    
1    | 2     150           130           West    
2    | 3     120           140           East    
-------------------------------------------------
[3 cs x 4 feat]
```

Every feature name gets `"q_"` prepended, including `id` and `region` — `.prefix()`
and `.suffix()` are whole-frame renames, unlike `.like()` / `.match()` / `.series()` /
`.note()` / `.timeline()`, which are selective.

## Notes

- `.like()` and `.match()` operate on **feature names**, not on the text values inside
  `Note` columns — pattern-matching a column's text contents is a job for
  [`nt()`](nt.md)'s own text methods instead.
- `.prefix()` / `.suffix()` rename every feature on the frame unconditionally; there is
  no selective "rename only these columns" mode on this accessor — for that, see
  [`rename()`](rename.md).
- `feataxis()` deliberately does not expose a name-existence check or a positional
  setter; use [`get(name).has_value()`](get.md) to check whether a feature exists, and
  [`rename()`](rename.md) for a full positional name replacement.
- An invalid regex passed to `.match()` throws with the message
  `"DataFrame::feataxis().match: invalid regex '<pattern>': <reason>"`.

## See Also

- [`DataFrame::features`](features.md): select an explicit, already-known list of
  feature names instead of a pattern or kind.
- [`DataFrame::rename`](rename.md): full positional replacement of every feature name,
  when `.prefix()` / `.suffix()` aren't the transform you need.
- [`DataFrame::nt`](nt.md): text-value operations on `Note` columns' *contents*,
  as opposed to `feataxis()`'s operations on feature *names*.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factor.html">
    <span class="label">Previous</span>
    <span class="title">factor</span>
  </a>
  <a class="next-link" href="features.html">
    <span class="label">Next</span>
    <span class="title">features</span>
  </a>
</div>
