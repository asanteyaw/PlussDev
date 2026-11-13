<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7ef94413804f -->

# DataFrameNoteAccessor::partition

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameNoteAccessor::partition(std::string_view separator,
                                            std::string prefix = "part") const
```

## What It Does

Splits every cell of the **single selected text feature** at the *first*
occurrence of a literal `separator`, always producing exactly **three**
output columns: the text before the separator, the separator itself, and
the text after it. It's a narrower, simpler cousin of [`split()`](split.md)
— always literal (no regex), always stops at the first match, and always
exactly three columns regardless of the input.

```{important} partition() requires exactly one selected feature
Like [`split()`](split.md), [`findall()`](findall.md), [`extract()`](extract.md),
and [`indicate()`](indicate.md), `partition()` operates on exactly one
column — calling it through a multi-feature accessor throws
`"DataFrame::nt().partition requires one feature"`.
```

```{important} A missing separator produces empty strings, not NULL
When `separator` doesn't occur in a cell at all, that row's middle and
third columns are set to the **empty string** `""`, not `NULL` — the row
itself is still valid, it simply had nothing to split. This is a
meaningfully different behavior from [`split()`](split.md) and
[`findall()`](findall.md), where a row that produced fewer pieces than the
widest row gets genuine `NULL`s in the missing columns. `partition()`'s
validity for all three output columns is copied directly from the
*original row's own validity* — a valid input row is valid in all three
output columns even when the separator wasn't found; only a row that was
already invalid on input produces `NULL` in all three. See Example 1.
```

## Parameters

**separator** (`std::string_view`)
    The literal text to split on. Must not be empty — `partition("")`
    throws `"Slate::partition: separator cannot be empty"`.

**prefix** (`std::string`, default `"part"`)
    Base name for the three output columns: `<prefix>_0` (before),
    `<prefix>_1` (the separator, or `""` if not found), `<prefix>_2`
    (after, or `""` if not found).

## Returns

`DataFrame`
    A new frame with the selected column replaced by exactly three
    columns: `<prefix>_0`, `<prefix>_1`, `<prefix>_2`.

## Examples

Example 1. The core behavior, including the not-found case's empty strings
and the genuinely-invalid row's `NULL`s, side by side.

```cpp
using namespace pluss::table;

auto entries = DataFrame({
    {"entry", Slate(pluss::make_note(
        {"key=value", "no-equals", "a=b=c", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << entries;

auto out1 = entries.features({"entry"}).nt().partition("=");
std::cout << out1;
```

### Output (before)

```text
================
     | entry    
----------------
0    | key=value
1    | no-equals
2    | a=b=c    
3    | NULL     
----------------
[4 cs x 1 feat]
```

### Output

```text
================================
     | part_0     part_1  part_2
--------------------------------
0    | key        =       value 
1    | no-equals                
2    | a          =       b=c   
3    | NULL       NULL    NULL  
--------------------------------
[4 cs x 3 feat]
```

Row 0 splits cleanly: `"key"`, `"="`, `"value"`. Row 1 has no `=` at all —
its `part_1` and `part_2` cells are **empty strings** (the blank space
between the pipes above, not the literal text `"NULL"`), because the row
itself is valid. Row 2 (`"a=b=c"`) partitions only at the *first* `=`:
`"a"`, `"="`, `"b=c"` — the second `=` is left inside `part_2` untouched.
Row 3 was invalid on input, so all three of its output columns are
genuinely `NULL`.

Example 2. A custom column prefix.

```cpp
auto out2 = entries.features({"entry"}).nt().partition("=", "kv");
std::cout << out2;
```

### Output

```text
=============================
     | kv_0       kv_1  kv_2 
-----------------------------
0    | key        =     value
1    | no-equals             
2    | a          =     b=c  
3    | NULL       NULL  NULL 
-----------------------------
[4 cs x 3 feat]
```

Same values as Example 1, just under `kv_0`/`kv_1`/`kv_2` instead of the
default `part_0`/`part_1`/`part_2`.

Example 3. An empty separator throws immediately.

```cpp
try {
  auto bad = entries.features({"entry"}).nt().partition("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::partition: separator cannot be empty
```

## Notes

- Requires the accessor to be built from exactly **one** feature.
- Always literal (no regex mode) and always stops at the *first*
  occurrence — for a regex-aware or multi-way split, use [`split()`](split.md)
  or [`findall()`](findall.md).
- Always produces exactly three output columns, regardless of the input.
- A missing separator yields empty strings in the second and third
  columns, not `NULL` — see the important callout above. Only a row that
  was already invalid on input produces `NULL` in any of the three
  columns.
- An empty `separator` throws `"Slate::partition: separator cannot be
  empty"`.

## See Also

- [`DataFrameNoteAccessor::split`](split.md): the more general,
  possibly-regex, unlimited-pieces sibling of this method — and one with
  genuine `NULL`-padding instead of empty strings for missing pieces.
- [`DataFrameNoteAccessor::findall`](findall.md) / [`extract()`](extract.md):
  other single-feature, column-expanding methods on this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
