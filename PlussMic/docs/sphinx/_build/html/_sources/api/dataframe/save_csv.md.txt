<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 79eca98416cb -->

# DataFrame::save_csv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_csv(std::string_view filepath) const -> void

DataFrame::save_csv(std::string_view filepath,
                     bool add_cases,
                     bool header = true,
                     char sep = ',',
                     std::string null_rep = "",
                     std::optional<std::string> case_label = std::nullopt,
                     std::optional<std::string> float_format = std::nullopt,
                     char quotechar = '"',
                     bool doublequote = true,
                     std::string line_terminator = "\n",
                     char decimal = '.',
                     std::string encoding = "utf-8") const -> void

DataFrame::save_csv(std::string_view filepath, const pluss::loader::Config& config) const -> void

DataFrame::save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const -> void

DataFrame::save_csv(std::string_view filepath,
                     std::initializer_list<pluss::loader::ConfigEntry> config_entries) const -> void
```

## What It Does

Writes the frame to a delimited text file. The plain `save_csv(filepath)` overload
covers the common case — comma-separated, header row written, case labels *not* written
as their own column — while the fully positional overload exposes every formatting knob
CSV round-tripping tends to need: the delimiter, the missing-value marker, whether the
case axis should be exported as a real column, numeric formatting, quoting rules, line
endings, and text encoding.

The single detail worth deciding deliberately every time: **`add_cases`**. When `false`
(the effective default via the filepath-only overload), the file contains only the
frame's ordinary features — reloading it with `load_csv()` gives you back a frame with a
fresh, auto-generated `0..N-1` case index, not the original one. When `true`, the current
case axis is written out as its own leading column (named via `case_label`, or a default
name if you leave it unset), so a later `load_csv(..., case_col = <that name>)` can
restore the exact case labels the frame had when it was saved.

## Parameters

**filepath** (`std::string_view`)

Destination file path.

**add_cases** (`bool`)

When `true`, the case axis is exported as its own column so case labels survive a
save/load round trip. When `false`, only the ordinary features are written.

**header** (`bool`, default `true`)

Whether to write feature names as the first line of the file.

**sep** (`char`, default `','`)

Field delimiter to write between values.

**null_rep** (`std::string`, default `""`)

The literal text written for a missing value. The default is an empty field; pass
something like `"NA"` or `"NULL"` if the consuming system needs missing values to be
visually unambiguous rather than an empty CSV field.

**case_label** (`std::optional<std::string>`, default unset)

Column name to use for the exported case-axis column when `add_cases=true`. Leave unset
to use a default name.

**float_format** (`std::optional<std::string>`, default unset)

A `fmt`-style format string applied to every floating-point value written. Leave unset
to use the library's default numeric formatting (enough significant digits to round-trip
typical values without excess trailing digits).

**quotechar** (`char`, default `'"'`)

Character used to quote fields that contain the delimiter, the quote character itself,
or a line break.

**doublequote** (`bool`, default `true`)

When `true`, an embedded quote character inside a field is escaped by doubling it
(standard CSV quoting); when `false`, a different escaping convention is used instead.

**line_terminator** (`std::string`, default `"\n"`)

Line ending written between rows.

**decimal** (`char`, default `'.'`)

Decimal point character used when formatting floating-point values — set to `','` for
locales that expect a comma decimal separator (note this then requires a non-comma
`sep`, such as `';'`, to keep fields unambiguous).

**encoding** (`std::string`, default `"utf-8"`)

Text encoding used when writing the file.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`) / **options**
(`const pluss::loader::WriteCsvOptions&`)

Equivalent settings expressed as a reusable config object, inline entries, or a typed
options struct, for callers who prefer not to spell out every positional parameter at
the call site.

## Returns

`void`
    Performs the write and returns nothing. Inspect the file on disk (or catch an
    exception from the loader layer on a write failure) to confirm the result — there is
    no in-memory object to print afterward.

## Examples

Example 1. Save with the simple, filepath-only overload.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Slate(pluss::make_series({101, 102, 103}))},
    {"city",     Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units",    Slate(pluss::make_series({2, 1, 4}))},
});

orders.save_csv("orders_out.csv");
```

Resulting `orders_out.csv`:

```text
order_id,city,units
101,Accra,2
102,Tema,1
103,Kumasi,4
```

No case-axis column is written — reloading this file gives back a frame with a fresh
`0, 1, 2` index rather than whatever case labels `orders` had before saving (in this
example, they happen to coincide, since `orders` itself uses the default index).

Example 2. Export the case axis as its own named column.

```cpp
orders.save_csv("orders_with_case.csv", /*add_cases=*/true, /*header=*/true, ',',
                 /*null_rep=*/"", /*case_label=*/std::string{"case_id"});
```

Resulting `orders_with_case.csv`:

```text
case_id,order_id,city,units
0,101,Accra,2
1,102,Tema,1
2,103,Kumasi,4
```

Loading this file back with
`DataFrame::load_csv("orders_with_case.csv", ',', std::nullopt, std::vector<std::string>{}, std::string{"case_id"})`
restores `case_id` as the case axis instead of leaving it as an ordinary feature.

## Notes

- `save_csv` never mutates the frame it is called on — it is a pure side-effecting write.
- Missing values are written as `null_rep` (an empty field by default), never as the
  literal text `"nan"` or `"NULL"` unless you set `null_rep` to that string explicitly.
- Set `float_format` when downstream tooling expects a fixed number of decimal places
  (for example `"{:.2f}"` for currency-style output) instead of the default
  significant-digits formatting.

## See Also

- [`DataFrame::load_csv`](load_csv.md): the read-side counterpart, including how
  `case_col` restores a column written with `add_cases=true`.
- [`DataFrame::save_excel`](save_excel.md) / [`save_json`](save_json.md) /
  [`save_parquet`](save_parquet.md): writers for other destination formats.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_as.html">
    <span class="label">Previous</span>
    <span class="title">save_as</span>
  </a>
  <a class="next-link" href="save_excel.html">
    <span class="label">Next</span>
    <span class="title">save_excel</span>
  </a>
</div>
