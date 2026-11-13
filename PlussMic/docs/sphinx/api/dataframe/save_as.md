<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e61c84cb74d1 -->

# DataFrame::save_as

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
void DataFrame::save_as(std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config = {}) const
void DataFrame::save_as(std::string_view target_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

## What It Does

A single format-switched write entry point: `target_type` picks which dedicated
writer actually runs, so `save_as("csv", ...)` behaves exactly like calling
[`save_csv()`](save_csv.md) with the same `config`, `save_as("json", ...)` like
[`save_json()`](save_json.md), and so on. `target_type` is matched
case-insensitively against `"csv"`, `"excel"`/`"xlsx"`, `"txt"`/`"text"`, `"json"`,
and `"parquet"` — the exact same set of format names [`load()`](load.md) recognizes
on the read side, and every one of them is supported through this dispatcher (unlike
`load()`, whose non-`Config` overloads only cover `"csv"`/`"txt"`/`"text"`; `save_as()`
has no such asymmetry — every format is reachable regardless of how `config` is
supplied).

Useful when the destination format is only known at runtime (for example, chosen from
a file extension or a user-facing dropdown) rather than hardcoded at the call site.

## Parameters

**target_type** (`std::string_view`)

Format selector: `"csv"`, `"excel"`/`"xlsx"`, `"txt"`/`"text"`, `"json"`, or
`"parquet"`, matched case-insensitively. Any other value throws
`PLUSS_FAIL("DataFrame::save_as: unsupported target type ...")`.

**filepath** (`std::string_view`)

Destination file path.

**config** (`const pluss::loader::Config&`, default `{}`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

Forwarded as-is to whichever dedicated writer `target_type` selects — the recognized
keys are exactly that writer's own `Config` keys (see
[`save_csv()`](save_csv.md), [`save_excel()`](save_excel.md),
[`save_json()`](save_json.md), or [`save_parquet()`](save_parquet.md) for the
per-format key lists).

## Returns

`void`
    Performs the write and returns nothing.

## Examples

Example 1. Pick the format at runtime, dispatching to `save_json()` under the hood.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({101, 102, 103}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Slate(pluss::make_series({2, 1, 4}))},
});

std::string_view fmt = "json";
orders.save_as(fmt, "orders_out.json");
```

Resulting `orders_out.json` — identical to what
`orders.save_json("orders_out.json")` would have written directly:

```json
[
  {
    "id": 101,
    "city": "Accra",
    "units": 2
  },
  {
    "id": 102,
    "city": "Tema",
    "units": 1
  },
  {
    "id": 103,
    "city": "Kumasi",
    "units": 4
  }
]
```

Example 2. Forward format-specific config keys through unchanged — here, `orient`
and `pretty` reach `save_json()` exactly as if called directly.

```cpp
orders.save_as("json", "orders_compact.json", {
    {"orient", std::string("columns")},
    {"pretty", false},
});
```

Resulting `orders_compact.json`:

```json
{"id":[101,102,103],"city":["Accra","Tema","Kumasi"],"units":[2,1,4]}
```

Example 3. Switch `target_type` to `"csv"` — same `config` object shape, different
destination format and file.

```cpp
orders.save_as("csv", "orders_out.csv");
```

Resulting `orders_out.csv`:

```text
id,city,units
101,Accra,2
102,Tema,1
103,Kumasi,4
```

## Notes

- `target_type` matching is case-insensitive, and `"excel"`/`"xlsx"` and `"txt"`/`"text"`
  are accepted as interchangeable spellings — the same aliasing
  [`load()`](load.md) and [`DataFrame::load(source_type, ...)`](load.md) use.
- Unlike `load()`'s `ReadCsvOptions`/positional overloads (which only support
  `"csv"`/`"txt"`/`"text"` and throw for the other three formats), `save_as()` has no
  such gap — every `config`/`config_entries` overload reaches all five formats.
- An unrecognized `target_type` throws immediately, before any file I/O is attempted.
- `save_as()` cannot write a multi-sheet Excel workbook — like
  [`save_excel()`](save_excel.md), it writes exactly one sheet per call.

## See Also

- [`DataFrame::load`](load.md): the read-side counterpart, including the
  overload-dependent format-support asymmetry that `save_as()` does not share.
- [`DataFrame::save_csv`](save_csv.md), [`save_excel`](save_excel.md),
  [`save_json`](save_json.md), [`save_parquet`](save_parquet.md),
  [`save_txt`](save_txt.md): the dedicated writers `save_as()` dispatches to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample.html">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="save_csv.html">
    <span class="label">Next</span>
    <span class="title">save_csv</span>
  </a>
</div>
