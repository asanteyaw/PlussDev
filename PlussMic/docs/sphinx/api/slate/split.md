<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e0b7a23804b5 -->

# Slate::split

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
DataFrame Slate::split(std::string_view delimiter,
                        std::int64_t limit = -1,
                        std::string prefix = "part") const

DataFrame Slate::split(std::string_view pattern,
                        const TextOptions& options) const
```

## What It Does

Splits every cell of a `Note` (string) slate on a delimiter, expanding the result into
one output column per split position — column `part_0` (or `<prefix>_0`) holds each
row's first piece, `part_1` its second, and so on. Requires the slate to be
`Note`-kind. There are two overloads with different delimiter semantics:

- **`split(delimiter, limit, prefix)`**: always a literal substring split (never
  regex), with an optional `limit` on how many splits to perform and a customizable
  column-name `prefix` (default `"part"`).
- **`split(pattern, options)`**: `pattern` is literal or regex depending on
  `options.regex`; `options.limit` caps the split count and `options.prefix` names the
  columns (the same `TextOptions` struct used across the string/text family — see
  [`replace()`](replace.md)).

Both overloads produce ragged results: a row with fewer pieces than the widest row is
`NULL` in the extra columns, and `limit` caps the number of splits performed (a split
of `"a/b/c"` with `limit = 1` yields two pieces, `["a", "b/c"]` — the remainder after
the capped split stays joined, it isn't dropped further).

```{warning} The two overloads compute the output column count differently for invalid rows
The single-delimiter overload (`split(delimiter, limit, prefix)`) determines the
output column count from **every** row's underlying text — including rows marked
invalid — because it works from a raw, validity-unaware split computed once over the
whole slate. An invalid cell whose underlying stored text happens to contain many
delimiters can inflate the column count even though that row shows `NULL` in every one
of them. The `(pattern, options)` overload does not have this quirk: it only computes
each row's split (and therefore only contributes to the column count) for rows that
are actually valid. See Example 4 for a side-by-side demonstration with identical data
under both overloads.
```

## Parameters

**delimiter** / **pattern** (`std::string_view`)
    The literal delimiter (first overload) or literal/regex pattern (second overload,
    depending on `options.regex`) to split on. An empty delimiter throws
    `"Slate::split: delimiter cannot be empty"`.

**limit** (`std::int64_t`, first overload, default `-1`)
    Maximum number of splits to perform (producing at most `limit + 1` pieces); `-1`
    means unlimited.

**prefix** (`std::string`, first overload, default `"part"`)
    Base name for the output columns: `<prefix>_0`, `<prefix>_1`, ...

**options** (`TextOptions`, second overload)
    `options.regex` (default `false`): treat `pattern` as regex. `options.limit`
    (default `-1`): cap the number of splits. `options.prefix` (default `"part"`):
    base name for output columns. `options.case_sensitive` is **not** used by
    `split()`.

## Returns

`DataFrame`
    A new frame with one `<prefix>_0`, `<prefix>_1`, ... feature per split position (as
    many as the widest split needs). Rows with fewer pieces are `NULL` in the extra
    columns.

## Examples

Example 1. A plain literal split with no limit — the default `"part"` prefix, and a
row with no delimiter at all.

```cpp
using namespace pluss::table;

auto paths = Slate(pluss::make_note({"a/b/c", "x/y", "solo", "z"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << paths;

auto out1 = paths.split("/");
std::cout << out1;
```

### Output (before)

```text
0  a/b/c
1  x/y
2  solo
3  NULL

ptype: Note
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

Row 0's three-way split drives the output to three columns; row 1's two-piece split
leaves `part_2` `NULL`; row 2 has no `/` at all, so its entire original text lands in
`part_0` and the rest is `NULL`.

Example 2. `limit` caps the number of splits — the remainder after the limit is
reached stays joined together in the last column, not dropped.

```cpp
auto out2 = paths.split("/", 1);
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

With `limit = 1`, row 0's `"a/b/c"` splits only once, into `"a"` and the still-joined
`"b/c"` — not three separate pieces. Row 1 (`"x/y"`) only had one delimiter anyway, so
it's unaffected by the cap.

Example 3. The second overload: a regex pattern (splitting on a comma with optional
surrounding whitespace) and a custom column prefix.

```cpp
auto rows = Slate(pluss::make_note({"a, b,c", "x , y", "single", "z"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << rows;

auto out3 = rows.split(R"(\s*,\s*)", {.regex = true, .prefix = "field"});
std::cout << out3;
```

### Output (before)

```text
0  a, b,c
1  x , y
2  single
3  NULL

ptype: Note
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

The regex `\s*,\s*` treats the comma plus any surrounding spaces as a single
delimiter, so `"x , y"` splits cleanly into `"x"` and `"y"` (no leftover spaces). The
`options.prefix = "field"` renames the output columns from `part_N` to `field_N`.

Example 4. The overload difference from the warning above, made concrete: identical
data, but the single-delimiter overload's column count is inflated by an invalid
row's underlying text, while the `(pattern, options)` overload ignores that row
entirely.

```cpp
auto mixed = Slate(pluss::make_note({"a/b", "x", "1/2/3/4"}),
    pluss::make_series(std::vector<bool>{true, true, false}));

auto via_overload1 = mixed.split("/");
auto via_overload2 = mixed.split("/", TextOptions{});
std::cout << via_overload1;
std::cout << via_overload2;
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

Row 2 was constructed invalid with the underlying text `"1/2/3/4"` — four pieces'
worth of delimiters, even though the cell is invisible as `NULL` and was never meant
to contribute real data. The first overload's column count (4) is driven by that
invisible text; the second overload's column count (2) is driven only by the two
genuinely valid rows.

Example 5. A non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.split("/");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::split: expected Note payload
```

## Notes

- Requires the calling slate to be `Note`-kind.
- An empty delimiter/pattern throws `"Slate::split: delimiter cannot be empty"`.
- `limit` caps the *number of splits*, not the *number of output columns* directly —
  the capped remainder stays joined in the last piece (Example 2).
- The two overloads compute the output width differently for invalid rows — see the
  warning above and Example 4.
- `options.case_sensitive` has no effect on `split()`.

## See Also

- [`Slate::partition`](partition.md): a related but simpler split — always exactly 3
  columns, first-occurrence-only, empty string (not `NULL`) for a missing separator.
- [`Slate::extract`](extract.md): regex capture-group extraction, another
  column-expanding method.
- [`Slate::indicate`](indicate.md): another method that honors `options.prefix` for
  its output columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
