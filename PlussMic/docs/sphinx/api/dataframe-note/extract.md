<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6867479ea607 -->

# DataFrameNoteAccessor::extract

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names = {}) const -> DataFrame
```

## What It Does

Searches every cell of the **single selected text feature** for the first
match of a regular expression `pattern`, and expands each capture group
into its own output column — one column per group, in group order. It's
the capture-group counterpart to [`findall()`](findall.md): where
`findall()` collects every match's *entire* matched text, `extract()`
takes apart the *first* match into its named or numbered pieces.

```{important} extract() requires exactly one selected feature
Like [`findall()`](findall.md), [`split()`](split.md), [`partition()`](partition.md),
and [`indicate()`](indicate.md), `extract()` operates on exactly one
column. Calling it through an accessor built from more than one feature
throws `"DataFrame::nt().extract requires one feature"`. Build the
accessor from a single feature: `df.features({"code"}).nt().extract(...)`.
```

`pattern` must contain at least one capture group — `extract("no groups
here")` throws `"Slate::extract: pattern needs a capture group"`. The
search is unanchored (`regex_search`, like [`has()`](has.md)'s regex mode,
not a whole-cell [`fullmatch()`](fullmatch.md)) and only the **first**
match in each cell is used; if `pattern` could match more than once,
everything after the first match is ignored for that row.

**Naming the output columns.** By default, columns are named `group_0`,
`group_1`, ... in capture-group order. Passing `names` overrides this —
but `names.size()` must exactly equal the number of capture groups in
`pattern`, or the call throws `"Slate::extract: names must match capture
groups"`.

**Optional groups that don't participate.** A capture group inside an
optional construct (`(...)?`, one alternative of a `|`, and so on) can
match successfully overall while that particular group captures nothing.
When that happens, the group's cell for that row is `NULL` — genuinely
different from that row simply not matching `pattern` at all, but
indistinguishable from it in the output alone (both show as `NULL`). See
Example 2.

## Parameters

**pattern** (`std::string_view`)
    A regular expression pattern (ECMAScript syntax) containing at least
    one capture group `(...)`.

**names** (`pluss::msharp::RangeView::Of<std::string_view>`, default `{}`)
    Column names for each capture group, in order. Leave empty for the
    default `group_0`, `group_1`, ... naming, or supply exactly one name
    per capture group.

## Returns

`DataFrame`
    A new frame with the selected column replaced by one column per
    capture group in `pattern`. A row where `pattern` didn't match at all
    is `NULL` in every group column; a row where it matched but a
    particular optional group didn't participate is `NULL` in just that
    group's column.

## Examples

Example 1. Pull a structured code apart into its prefix and numeric parts,
using the default `group_N` column names.

```cpp
using namespace pluss::table;

auto codes = DataFrame({
    {"code", Slate(pluss::make_note(
        {"ID-4521", "REF-99", "bad", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << codes;

auto out1 = codes.features({"code"}).nt().extract(R"(([A-Z]+)-(\d+))");
std::cout << out1;
```

### Output (before)

```text
==============
     | code   
--------------
0    | ID-4521
1    | REF-99 
2    | bad    
3    | NULL   
--------------
[4 cs x 1 feat]
```

### Output

```text
=======================
     | group_0  group_1
-----------------------
0    | ID       4521   
1    | REF      99     
2    | NULL     NULL   
3    | NULL     NULL   
-----------------------
[4 cs x 2 feat]
```

Row 0's `"ID-4521"` splits into `group_0 = "ID"` and `group_1 = "4521"`.
Row 2 (`"bad"`) doesn't match the pattern at all, so both output columns
are `NULL` for that row — not empty strings.

Example 2. The same extraction with custom column names, plus a case where
a group is *part of a successful match* but individually captures nothing.

```cpp
auto out2 = codes.features({"code"}).nt().extract(
    R"(([A-Z]+)-(\d+))", {"prefix", "number"});
std::cout << out2;

auto partial = DataFrame({
    {"code", Slate(pluss::make_note(
        {"ID-4521", "ID-", "REF99", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
auto out3 = partial.features({"code"}).nt().extract(R"(([A-Z]+)-?(\d+)?)");
std::cout << out3;
```

### Output (`names = {"prefix", "number"}`)

```text
=====================
     | prefix  number
---------------------
0    | ID      4521  
1    | REF     99    
2    | NULL    NULL  
3    | NULL    NULL  
---------------------
[4 cs x 2 feat]
```

### Output (optional trailing group)

```text
=======================
     | group_0  group_1
-----------------------
0    | ID       4521   
1    | ID       NULL   
2    | REF      99     
3    | NULL     NULL   
-----------------------
[4 cs x 2 feat]
```

`([A-Z]+)-?(\d+)?` makes both the hyphen and the digits optional. Row 1
(`"ID-"`) still matches overall (the letters `"ID"` alone are enough), so
`group_0` is `"ID"` — but the digit group never captured anything for this
row, so `group_1` is `NULL`, distinct from row 3's "the whole row is
invalid" `NULL`.

Example 3. Two throwing cases: a pattern with no capture group, and a
`names` list that doesn't match the group count.

```cpp
try {
  auto no_groups = codes.features({"code"}).nt().extract("[A-Z]+");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

try {
  auto bad_names = codes.features({"code"}).nt().extract(
      R"(([A-Z]+)-(\d+))", {"only_one_name"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::extract: pattern needs a capture group
Slate::extract: names must match capture groups
```

## Notes

- Requires the accessor to be built from exactly **one** feature — see the
  important callout above.
- Uses an unanchored search (`regex_search`) and only the **first** match
  per cell — a `pattern` that could match multiple times in a cell still
  only produces one row's worth of group values.
- `pattern` must contain at least one capture group; `names`, if given,
  must have exactly one entry per group.
- A row with no match at all is `NULL` in every output column; a row that
  matched but left one particular optional group empty is `NULL` in just
  that column — see Example 2.

## See Also

- [`DataFrameNoteAccessor::findall`](findall.md): collect every match's
  full text (not broken into groups), one output column per match
  position instead of per capture group.
- [`DataFrameNoteAccessor::matches`](matches.md) / [`has()`](has.md):
  test whether a pattern matches, without pulling out any text.
- [`DataFrameNoteAccessor::split`](split.md): break a cell apart by
  delimiter instead of by regex capture groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="findall.html">
    <span class="label">Next</span>
    <span class="title">findall</span>
  </a>
</div>
