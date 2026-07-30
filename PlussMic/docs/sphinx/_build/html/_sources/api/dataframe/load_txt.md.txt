<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cf2d815ed24b -->

# DataFrame::load_txt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load_txt(std::string_view filepath) -> DataFrame
static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame
static DataFrame load_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
```

## What It Does

`DataFrame`'s static constructor for plain delimited text files that aren't
necessarily comma-separated — the same underlying parser and type-inference machinery
as [`load_csv()`](load_csv.md), read through a `Config`/`ConfigEntry` settings bundle
instead of `load_csv()`'s rich set of dedicated positional/typed overloads.

## Parameters

**filepath** (`std::string_view`)

Path to the text file to read.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline settings bundle. Recognizes the same kind of keys `load_csv()`'s
`Config` overload does (`sep`, `header`, `ncases`, `case`, `usefeats`, and so on) —
`load_txt()` doesn't default to a different delimiter on the read side; pass
`{"sep", "\t"}` explicitly for tab-separated input, the same as you would for
[`load_csv()`](load_csv.md).

## Returns

`DataFrame`
    A new, fully typed frame, with the same numeric/text type inference behavior as
    [`load_csv()`](load_csv.md).

## Examples

Example 1. Load a tab-separated text file by naming the delimiter explicitly.

Given `orders.txt`:

```text
id	city	units
101	Accra	2
102	Tema	1
103	Kumasi	4
```

```cpp
using namespace pluss::table;

pluss::loader::Config config{{"sep", std::string("\t")}};
auto orders = DataFrame::load_txt("orders.txt", config);
std::cout << orders;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
2    | 103   Kumasi  4    
--------------------------
[3 cs x 3 feat]
```

Example 2. Cap the number of rows read with an inline config entry.

```cpp
auto preview = DataFrame::load_txt("orders.txt", {
    {"sep", std::string("\t")},
    {"ncases", std::int64_t{2}},
});
std::cout << preview;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
--------------------------
[2 cs x 3 feat]
```

## Notes

- `load_txt()` shares its type-inference engine with [`load_csv()`](load_csv.md) —
  numeric-looking columns become `Series`, everything else becomes `Note`, and empty
  cells become missing values, exactly as documented there.
- There is no dedicated `ReadTxtOptions` struct or positional overload the way
  [`load_csv()`](load_csv.md) has — the `Config`/`ConfigEntry` overloads are the only
  way to customize parsing.
- The corresponding writer, [`save_txt()`](save_txt.md), *does* default to a tab
  separator on the write side unless you override it — but `load_txt()`'s read side
  has no such default; specify `sep` explicitly to match whatever delimiter the file
  actually uses.

## See Also

- [`DataFrame::load_csv`](load_csv.md): the fuller explanation of the shared parsing
  and type-inference behavior, plus the richer overload set (typed options,
  `case_col`, `usefeats`, `on_bad_lines`).
- [`DataFrame::save_txt`](save_txt.md): the write-side counterpart.
- [`DataFrame::load`](load.md): the format-switched entry point that can dispatch to
  `load_txt()` via `source_type = "txt"`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_parquet.html">
    <span class="label">Previous</span>
    <span class="title">load_parquet</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
