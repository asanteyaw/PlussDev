<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8103390ebf3 -->

# DataFrame::query

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::query(std::string_view expression) -> DataFrame
```

Filter rows with a boolean expression written against DataFrame feature names.

`DataFrame::query(...)` is the string-expression row filter for PlussMic. It lets you write one compact predicate instead of building a boolean mask by hand, which is especially useful when you are exploring data interactively or translating logic from notebook-style workflows into C++.

The expression itself is a small language evaluated against feature names, so you can
write `"units >= 10 and region == 'south'"` instead of assembling
`df[(df.units >= 10) & (df.region == 'south')]` by hand. The C++ equivalent of the
boolean-mask style is [`filter()`](filter.md) — reach for `query()` when the predicate
reads more clearly as a single self-contained string, and for `filter()` when the mask is
more naturally built up from existing C++ expressions or reused across calls.

```{warning} Expression Input

Treat the query string as code-like input owned by the developer.

Do not pass untrusted raw user input directly into `DataFrame::query(...)` without validation or a safer higher-level policy layer around it.
```

## Parameters

**expression**

Boolean filter expression to evaluate against each row.

Common patterns include:

- numeric comparison: `units >= 10`
- string comparison: `region == 'south'`
- combined predicates: `units >= 10 and region == 'south'`
- feature-to-feature comparison: `units > price`
- spaced feature names via backticks: `` `First Name` == 'Laurel' ``

The result must be a row-level truth decision.

## Returns

`DataFrame`
    A filtered frame containing only the rows where the expression evaluates to `true`.

    The returned frame preserves:
    - the original feature set,
    - the original feature order,
    - the original case labels for rows that survive the filter.

## See Also

- <a href="filter.html"><code>DataFrame::filter</code></a>: filter rows from an explicit boolean mask instead of a query string.
- <a href="where.html"><code>DataFrame::where</code></a>: row filter using a boolean mask that has already been computed.
- <a href="case_study.html"><code>DataFrame::case_study</code></a>: select rows by position, label, or mask when you do not want an expression parser.

## Notes

The expression is evaluated row by row against the current frame. Feature names are available directly inside the query string.

String literals must be quoted. For example, write `region == 'south'`, not `region == south`.

Feature-to-feature comparison is supported. For example, `units > price` compares the two features row by row.

Use backticks around feature names that contain spaces or punctuation, such as `` `First Name` == 'Laurel' ``.

This method is for row filtering. It does not rewrite values, create columns, or mutate the current frame in place.

The expression is parsed and evaluated by PlussMic's own small expression engine, not by
shelling out to an external interpreter — there is no dependency on Python, a SQL engine,
or any other runtime to use `query()`. The patterns documented above (numeric and string
comparisons, `and` / `or` combinators, quoted literals, and backtick-quoted feature
names) are the supported core; if a more elaborate expression does not parse the way you
expect, fall back to building the mask explicitly with [`filter()`](filter.md), which
accepts any boolean `Thread` you can construct in ordinary C++.

## Examples

Example 1. Filter rows with a combined numeric and string predicate.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"region", pluss::make_note({"south", "south", "middle"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("sales");

auto out = sales.query("units >= 10 and region == 'south'");
std::cout << out;
```

### Output

```text
==================================
     | city   region  units  price
----------------------------------
s1   | Accra  south   10     3
----------------------------------
[1 cs x 4 feat]
```

Example 2. Query a feature whose name contains a space by using backticks.

```cpp
using namespace pluss::table;

auto people = DataFrame({
    {"First Name", pluss::make_note({"Laurel", "Bola", "Laurel"})},
    {"Last Name", pluss::make_note({"Ork", "Mensah", "Kirk"})},
    {"Age", pluss::make_series({21.0, 19.0, 23.0})},
}).title("people");

auto out = people.query("`First Name` == 'Laurel'");
std::cout << out;
```

### Output

```text
==================================
     | First Name  Last Name  Age
----------------------------------
0    | Laurel      Ork        21
2    | Laurel      Kirk       23
----------------------------------
[2 cs x 3 feat]
```

Example 3. Compare one feature against another feature inside the same row.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"region", pluss::make_note({"south", "south", "middle"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("sales");

auto out = sales.query("units > price");
std::cout << out;
```

### Output

```text
===================================
     | city    region  units  price
-----------------------------------
s1   | Accra   south   10     3
s2   | Tema    south   7      4
s3   | Kumasi  middle  12     2.5
-----------------------------------
[3 cs x 4 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put_.html">
    <span class="label">Previous</span>
    <span class="title">put_</span>
  </a>
  <a class="next-link" href="radd.html">
    <span class="label">Next</span>
    <span class="title">radd</span>
  </a>
</div>
