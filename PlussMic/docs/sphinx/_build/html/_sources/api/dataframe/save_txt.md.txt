<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 642d701c91c7 -->

# DataFrame::save_txt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_txt(std::string_view filepath) const -> void
DataFrame::save_txt(std::string_view filepath, const pluss::loader::Config& config) const -> void
DataFrame::save_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const -> void
```

## What It Does

Writes the frame to a delimited text file — internally, `save_txt()` is
[`save_csv()`](save_csv.md) with one difference: if the `config` doesn't explicitly
set `sep`/`delimiter`/`separator`, the field delimiter defaults to a tab (`'\t'`)
instead of a comma. Every other writing behavior (missing-value representation,
quoting, encoding) is identical to `save_csv()`.

## Parameters

**filepath** (`std::string_view`)

Destination file path.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline settings scribble, recognizing the same keys
[`save_csv()`](save_csv.md)'s `Config` overload does (`addcases`, `header`, `sep`,
`nullrep`, and so on).

## Returns

`void`
    Performs the write and returns nothing.

## Examples

Example 1. Save with the default tab delimiter.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({101, 102, 103}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Thread(pluss::make_series({2, 1, 4}))},
});

orders.save_txt("orders_out.txt");
```

Resulting `orders_out.txt` (fields separated by real tab characters, shown here as
gaps):

```text
id	city	units
101	Accra	2
102	Tema	1
103	Kumasi	4
```

Example 2. Override the default and write comma-separated output instead.

```cpp
orders.save_txt("orders_out_comma.txt", {{"sep", std::string(",")}});
```

Resulting `orders_out_comma.txt`:

```text
id,city,units
101,Accra,2
102,Tema,1
103,Kumasi,4
```

Setting `sep` explicitly overrides the tab default — at that point `save_txt()`
behaves identically to calling [`save_csv()`](save_csv.md) with the same `sep`.

## Notes

- `save_txt()` and [`save_csv()`](save_csv.md) share the exact same writer; the only
  difference is the tab-vs-comma default delimiter, and only when `sep` isn't set
  explicitly in `config`.
- There is no dedicated positional overload the way `save_csv()` has — use `config`
  for anything beyond the plain filepath-only call.
- The read-side counterpart, [`load_txt()`](load_txt.md), does **not** default to a
  tab separator — you must specify `sep` explicitly when reading a file `save_txt()`
  wrote with its tab default.

## See Also

- [`DataFrame::save_csv`](save_csv.md): the shared writer this method wraps, and the
  fuller explanation of every write-side setting.
- [`DataFrame::load_txt`](load_txt.md): the read-side counterpart — note its lack of a
  matching tab default.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_parquet.html">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
