<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f448a4472ce0 -->

# DataFrameLabelAccessor::reset

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::reset(std::string case_feature = {}) const -> DataFrame
DataFrameLabelAccessor::reset(const DataFrame::ResetLabelOptions& options) const -> DataFrame
```

## What It Does

Clears the case labels back to the default integer index. Unless dropped, the
old case-label values are first materialized back as ordinary feature
column(s). By default — with no name supplied — each level's own name is
kept (matching pandas' `reset_index()`, which preserves a named `Index`'s
name); pass an explicit name only when you want to override it.

## Parameters

**case_feature** (`std::string`, default `{}`, empty)

The name to give the restored feature column. Left empty (the default), each
level keeps its own name (or `"level_"` + its position, if the level itself
has no name). Pass a non-empty string to override that name — valid only when
there's exactly one case-label level (or an unstructured axis).

**options** (`const DataFrame::ResetLabelOptions&`)

`drop` (discard the old values entirely instead of restoring them as
features), `names` (one name per level, positionally — leave **empty** to keep
each level's own name instead of overriding it), and `prefix` (fallback prefix
for any level that has no name of its own).

## Returns

`DataFrame`
    A new frame with default integer case labels, and (unless `drop`) the old
    case-label values restored as feature column(s).

## Examples

Example 1. The default overload, called with no arguments, keeps each level's
own name.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region", true); // drop_feature=true
std::cout << labeled;

auto out = labeled.label().reset();
std::cout << out;
```

### Output (before)

```text
=============
      | sales
-------------
east  | 100  
west  | 200  
east  | 150  
north | 90   
-------------
[4 cs x 1 feat]
```

### Output (reset(), no override)

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

With no argument, the restored column keeps the level's own name —
`"region"` — rather than being forced to some fixed default.

Example 2. Pass a name explicitly to override the level's own name.

```cpp
auto out2 = labeled.label().reset("custom_name");
std::cout << out2;
```

### Output

```text
=========================
     | custom_name  sales
-------------------------
0    | east         100  
1    | west         200  
2    | east         150  
3    | north        90   
-------------------------
[4 cs x 2 feat]
```

Passing a non-empty name always overrides the level's own name with the one
given.

Example 3. `drop = true` discards the old values entirely.

```cpp
auto out3 = labeled.label().reset(DataFrame::ResetLabelOptions{.drop = true});
std::cout << out3;
```

### Output

```text
============
     | sales
------------
0    | 100  
1    | 200  
2    | 150  
3    | 90   
------------
[4 cs x 1 feat]
```

## Notes

- Calling `reset()` with no arguments keeps each level's own name (or falls
  back to `"level_"` + position for an unnamed level) — this matches pandas'
  `reset_index()`, which preserves a named `Index`'s name. Pass a non-empty
  `case_feature` string only when you want to override that name.
- `reset(DataFrame::ResetLabelOptions{})` with `options.names` left empty is
  equivalent to the no-argument single-string overload.
- On a structured frame, if `options.names` is non-empty it must have exactly
  one entry per level.
- With `drop = true`, the old case-label values are discarded — there's no way
  to recover them afterward.

## See Also

- [`DataFrameLabelAccessor::set`](set.md): the operation `reset()` undoes.
- [`DataFrameLabelAccessor::drop`](drop.md): removes case-label levels without
  ever restoring them as features, even without `drop = true`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeated.html">
    <span class="label">Previous</span>
    <span class="title">repeated</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
