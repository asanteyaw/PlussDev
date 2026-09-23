<!-- pluss-hand-authored -->

# Nexus::save

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::save(std::string_view directory, const StoreOptions& options = {}) const -> void
```

## What It Does

Persists the entire workspace to disk under `directory`: every table's data, declared
[`TableMeta`](table_meta.md), declared [`Relationship`](relationships.md)s, and saved
views — everything [`load()`](load.md) needs to reconstruct an equivalent `Nexus` later.

```cpp
struct StoreOptions {
  StoreKind kind{StoreKind::Parquet};
  std::string manifest{"manifest.csv"};
  std::string schema{"schema.csv"};
  std::string graph{"graph.csv"};
  std::string views{"views.csv"};
  std::string tables{"tables"};
};
```

`kind` picks the per-table file format (`Csv`, `Json`, or `Parquet` — see the important
callout below). The rest of `StoreOptions` are just file/directory names, relative to
`directory`: `tables` is the subdirectory each table is written into (one file per table,
named `<index>_<sanitized-key>.<ext>`); `schema` and `graph` are exactly
[`schema()`](schema.md)'s and [`graph()`](graph.md)'s own output, written as CSV
regardless of `kind`; `views` records every saved view's [`JoinPlan`](plan.md), one row
per step (also always CSV); `manifest` is a small one-row index recording `kind`,
[`fingerprint()`](fingerprint.md), table/relationship counts, and the other four file
names, which is what [`load()`](load.md) reads first to find everything else.

```{important} The default StoreKind::Parquet requires an optional build dependency
Saving with the default `StoreOptions{}` (or any explicit `options.kind =
StoreKind::Parquet`) requires the library to have been built with Arrow + Parquet C++
support. On a build without it, saving throws
`"loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and
reconfigure)"` — the same way [`DataFrame::save_as("parquet", ...)`](../dataframe/save_as.md)
would for a single table. `StoreKind::Csv` and `StoreKind::Json` have no such
dependency.
```

## Parameters

**directory** (`std::string_view`)
    The target directory. Created (along with the `tables` subdirectory) if it doesn't
    already exist.

**options** (`const StoreOptions&`, default `{}`)
    File format and naming, as described above.

## Returns

`void`

## Examples

Example 1. Saving with `StoreKind::Csv`, since the default `Parquet` needs an optional
dependency this build doesn't have.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

TableMeta customers_meta;
customers_meta.grain = "one row per customer";
customers_meta.primary_key = {"customer_id"};
auto tagged = biz.set_table_meta("customers", customers_meta);

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = tagged.add_relationship(rel);

StoreOptions options;
options.kind = StoreKind::Csv;
linked.save("/tmp/nexus_biz_store", options);

auto loaded = Nexus::read("/tmp/nexus_biz_store", options);
std::cout << loaded.ntables() << " " << loaded.nrelationships() << "\n";
std::cout << std::boolalpha << (loaded.fingerprint() == linked.fingerprint());
```

### Output

```text
2 1
true
```

Example 2. Attempting to save with the default `StoreKind::Parquet` on a build without
Arrow + Parquet C++ support.

```cpp
try {
  linked.save("/tmp/nexus_biz_store_default");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)
```

## Notes

- `directory` (and its `tables` subdirectory) is created if it doesn't exist; existing
  files at the target paths are overwritten.
- The default `StoreKind::Parquet` requires an optional build dependency — see the
  important callout above. Use `StoreKind::Csv`/`Json` for a dependency-free round trip.
- Every one of `manifest`/`schema`/`graph`/`views` is always written as CSV, independent
  of `kind` — `kind` only controls the per-table data files.

## See Also

- [`Nexus::read`](read.md): the exact inverse of this method.
- [`Nexus::fingerprint`](fingerprint.md): recorded in the manifest, and useful for
  confirming a round trip preserved everything.
- [`Nexus::schema`](schema.md) / [`graph`](graph.md): the exact content written to
  `schema.csv`/`graph.csv`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
