<!-- pluss-hand-authored -->

# DataFrame::load_parquet

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load_parquet(std::string_view filepath) -> DataFrame
static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame
static DataFrame load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
```

## What It Does

`DataFrame`'s static constructor for Parquet files, read via Apache Arrow's Parquet
C++ library.

```{caution} Parquet's own schema is not trusted directly — values are re-inferred
Parquet embeds a real, strongly-typed columnar schema (`int64`, `double`, `utf8`, and
so on). `load_parquet()` reads that schema and the underlying columnar data via Arrow,
but then converts every value to its string form and re-runs it through the exact
same numeric/text type-inference engine [`load_csv()`](load_csv.md) uses, rather than
adopting Arrow's schema directly. For ordinary numeric and string columns this
round-trips cleanly, but it means Parquet's own type precision (distinguishing
`int32` from `int64`, for example) is not preserved as-is — the resulting `Series`
dtype comes from PlussMic's own inference, not from the Parquet file's schema.
```

```{important} Requires an Arrow + Parquet C++ build
`load_parquet()` throws *"built without parquet-cpp support"* if PlussMic was
compiled without the optional `PLUSSMIC_HAS_PARQUET_CPP` support — Parquet reading (and
[`save_parquet()`](save_parquet.md) writing) is conditional on that dependency being
available at build time, unlike CSV/TXT/JSON support, which is always present.
```

## Parameters

**filepath** (`std::string_view`)

Path to the Parquet file to read.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline settings bundle, recognizing the same general-purpose keys
(`ncases`, `usefeats`, `case`, and so on) as [`load_csv()`](load_csv.md)'s `Config`
overload — `usefeats` here restricts which Parquet columns are even read out of the
file via Arrow, rather than reading everything and discarding columns afterward.

## Returns

`DataFrame`
    A new, fully typed frame, with the same numeric/text type inference behavior as
    [`load_csv()`](load_csv.md) — not Parquet's own embedded schema.

## Examples

Example 1. Load a Parquet file with the plain filepath-only overload.

```cpp
using namespace pluss::table;

auto orders = DataFrame::load_parquet("orders.parquet");
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

Example 2. Read only a subset of columns directly from the Parquet file.

```cpp
auto narrowed = DataFrame::load_parquet("orders.parquet", {
    {"usefeats", std::vector<std::string>{"city", "units"}},
});
std::cout << narrowed;
```

### Output

```text
=================
     | city    units
-----------------
0    | Accra   2
1    | Tema    1
2    | Kumasi  4
-----------------
[3 cs x 2 feat]
```

`id` is never read out of the underlying Parquet column store at all — `usefeats`
here is pushed down to the Arrow read step, not applied after the fact.

## Notes

- Requires PlussMic to have been built with Arrow + Parquet C++ support; otherwise
  every call throws immediately.
- Dtypes come from PlussMic's own string-based inference (the same engine
  [`load_csv()`](load_csv.md) uses), not directly from Parquet's embedded schema — see
  the caution above.
- `usefeats` is pushed down to the Arrow column read, so unwanted columns are never
  even decoded from disk, unlike a post-hoc [`features()`](features.md) projection.

## See Also

- [`DataFrame::save_parquet`](save_parquet.md): the write-side counterpart, with the
  same Arrow/Parquet-C++ build requirement.
- [`DataFrame::load_csv`](load_csv.md): the fuller explanation of the shared
  type-inference engine this loader defers to.
- [`DataFrame::dtypes`](dtypes.md): check what PlussMic actually inferred after
  loading, since it may not match the source file's Parquet schema exactly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_json.html">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="load_txt.html">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>
