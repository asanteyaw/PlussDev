<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::split

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrameNoteAccessor::split(std::string_view delimiter,
                              std::int64_t limit = -1,
                              std::string prefix = "part") const -> DataFrame

DataFrameNoteAccessor::split(std::string_view pattern,
                              const TextOptions& options) const -> DataFrame
```

## What It Does

Splits every cell of the **single selected text feature** on a delimiter,
expanding the result into one output column per split position — column
`part_0` (or `<prefix>_0`) holds each row's first piece, `part_1` its
second, and so on, the same column-expansion shape [`findall()`](findall.md)
and [`extract()`](extract.md) use. There are two overloads with different
delimiter semantics:

- **`split(delimiter, limit, prefix)`**: always a literal substring split
  (never regex), with an optional `limit` on how many splits to perform
  and a customizable column-name `prefix` (default `"part"`).
- **`split(pattern, options)`**: `pattern` is literal or regex depending on
  `options.regex`; `options.limit` caps the split count and
  `options.prefix` names the columns (both fields of the same
  `TextOptions` struct used throughout this accessor).

```{important} split() requires exactly one selected feature
Like [`findall()`](findall.md), [`extract()`](extract.md), [`partition()`](partition.md),
and [`indicate()`](indicate.md), `split()` operates on exactly one column —
calling it through a multi-feature accessor throws
`"DataFrame::nt().split requires one feature"`.
```

Both overloads produce ragged results the same way `findall()` does: a row
with fewer pieces than the widest row is `NULL` in the extra columns, and
`limit` caps the number of splits performed (a split of `"a/b/c"` with
`limit = 1` yields two pieces, `["a", "b/c"]` — the remainder after the
capped split stays joined, it isn't dropped).

```{warning} The two overloads compute the output column count differently for invalid rows
The single-delimiter overload (`split(delimiter, limit, prefix)`) determines
the output column count from **every** row's underlying text — including
rows the caller marked invalid — because it works from a raw, validity-unaware
split computed once over the whole column. A constructed-invalid cell whose
underlying stored text happens to contain many delimiters can inflate the
column count even though that row shows `NULL` in every one of them. The
`(pattern, options)` overload does not have this quirk: it only computes
each row's split (and therefore only contributes to the column count) for
rows that are actually valid. See Example 4 for a side-by-side
demonstration with identical data under both overloads.
```

## Parameters

**delimiter** / **pattern** (`std::string_view`)
    The literal delimiter (first overload) or literal/regex pattern (second
    overload, depending on `options.regex`) to split on. An empty delimiter
    throws `"Thread::split: delimiter cannot be empty"`.

**limit** (`std::int64_t`, first overload, default `-1`)
    Maximum number of splits to perform (producing at most `limit + 1`
    pieces); `-1` means unlimited.

**prefix** (`std::string`, first overload, default `"part"`)
    Base name for the output columns: `<prefix>_0`, `<prefix>_1`, ...

**options** (`TextOptions`, second overload)
    `options.regex` (default `false`): treat `pattern` as regex.
    `options.limit` (default `-1`): cap the number of splits.
    `options.prefix` (default `"part"`): base name for output columns.
    `options.case_sensitive` is **not** used by `split()`.

## Returns

`DataFrame`
    A new frame with the selected column replaced by `<prefix>_0`,
    `<prefix>_1`, ... columns (as many as the widest split needs), keeping
    every other feature and the original row labels. Rows with fewer
    pieces are `NULL` in the extra columns.

## Examples

Example 1. A plain literal split with no limit — the default `"part"`
prefix, and a row with no delimiter at all.

```cpp
using namespace pluss::table;

auto paths = DataFrame({
    {"path", Thread(pluss::make_note(
        {"a/b/c", "x/y", "solo", "z"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << paths;

auto out1 = paths.features({"path"}).nt().split("/");
std::cout << out1;
```

### Output (before)

```text
============
     | path 
------------
0    | a/b/c
1    | x/y  
2    | solo 
3    | NULL 
------------
[4 cs x 1 feat]
```

### Output

```text
=============================
     | part_0  part_1  part_2
-----------------------------
0    | a       b       c     
1    | x       y       NULL  
2    | solo    NULL    NULL  
3    | NULL    NULL    NULL  
-----------------------------
[4 cs x 3 feat]
```

Row 0's three-way split drives the output to three columns; row 1's
two-piece split leaves `part_2` `NULL`; row 2 has no `/` at all, so its
entire original text lands in `part_0` and the rest is `NULL`.

Example 2. `limit` caps the number of splits — the remainder after the
limit is reached stays joined together in the last column, not dropped.

```cpp
auto out2 = paths.features({"path"}).nt().split("/", 1);
std::cout << out2;
```

### Output

```text
=====================
     | part_0  part_1
---------------------
0    | a       b/c   
1    | x       y     
2    | solo    NULL  
3    | NULL    NULL  
---------------------
[4 cs x 2 feat]
```

With `limit = 1`, row 0's `"a/b/c"` splits only once, into `"a"` and the
still-joined `"b/c"` — not three separate pieces. Row 1 (`"x/y"`) only had
one delimiter anyway, so it's unaffected by the cap.

Example 3. The second overload: a regex pattern (splitting on a comma with
optional surrounding whitespace) and a custom column prefix.

```cpp
auto rows = DataFrame({
    {"csv", Thread(pluss::make_note(
        {"a, b,c", "x , y", "single", "z"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << rows;

auto out3 = rows.features({"csv"}).nt().split(
    R"(\s*,\s*)", {.regex = true, .prefix = "field"});
std::cout << out3;
```

### Output (before)

```text
=============
     | csv   
-------------
0    | a, b,c
1    | x , y 
2    | single
3    | NULL  
-------------
[4 cs x 1 feat]
```

### Output

```text
================================
     | field_0  field_1  field_2
--------------------------------
0    | a        b        c      
1    | x        y        NULL   
2    | single   NULL     NULL   
3    | NULL     NULL     NULL   
--------------------------------
[4 cs x 3 feat]
```

The regex `\s*,\s*` treats the comma plus any surrounding spaces as a
single delimiter, so `"x , y"` splits cleanly into `"x"` and `"y"` (no
leftover spaces). The `options.prefix = "field"` renames the output columns
from `part_N` to `field_N`.

Example 4. The overload difference from the warning above, made concrete:
identical data, but the single-delimiter overload's column count is
inflated by an invalid row's underlying text, while the `(pattern,
options)` overload ignores that row entirely.

```cpp
auto mixed = DataFrame({
    {"path", Thread(pluss::make_note({"a/b", "x", "1/2/3/4"}),
        pluss::make_series(std::vector<bool>{true, true, false}))},
});
std::cout << mixed;

auto via_overload1 = mixed.features({"path"}).nt().split("/");
std::cout << via_overload1;

auto via_overload2 = mixed.features({"path"}).nt().split("/", TextOptions{});
std::cout << via_overload2;
```

### Output (before — row 2 is invalid, its underlying text is invisible here)

```text
===========
     | path
-----------
0    | a/b 
1    | x   
2    | NULL
-----------
[3 cs x 1 feat]
```

### Output (`split("/")`, first overload)

```text
=====================================
     | part_0  part_1  part_2  part_3
-------------------------------------
0    | a       b       NULL    NULL  
1    | x       NULL    NULL    NULL  
2    | NULL    NULL    NULL    NULL  
-------------------------------------
[3 cs x 4 feat]
```

### Output (`split("/", TextOptions{})`, second overload)

```text
=====================
     | part_0  part_1
---------------------
0    | a       b     
1    | x       NULL  
2    | NULL    NULL  
---------------------
[3 cs x 2 feat]
```

Row 2 was constructed invalid with the underlying text `"1/2/3/4"` — four
pieces' worth of delimiters, even though the cell displays as `NULL` and
was never meant to contribute real data. The first overload's column count
(4) is driven by that invisible text; the second overload's column count
(2) is driven only by the two genuinely valid rows, exactly what a reader
would expect from looking at the printed `path` column alone.

## Notes

- Requires the accessor to be built from exactly **one** feature.
- An empty delimiter/pattern throws `"Thread::split: delimiter cannot be
  empty"`.
- `limit` caps the *number of splits*, not the *number of output columns*
  directly — the capped remainder stays joined in the last piece (Example 2).
- The two overloads compute the output width differently for invalid
  rows — see the warning above and Example 4.
- `options.case_sensitive` has no effect on `split()`.

## See Also

- [`DataFrameNoteAccessor::partition`](partition.md): a related but
  simpler split — always exactly 3 columns, first-occurrence-only, empty
  string (not `NULL`) for a missing separator.
- [`DataFrameNoteAccessor::findall`](findall.md) / [`extract()`](extract.md):
  other single-feature, column-expanding methods on this accessor.
- [`DataFrameNoteAccessor::indicate`](indicate.md): another method that
  honors `options.prefix` for its output columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
