<!-- pluss-hand-authored -->

# Nexus::read

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
static Nexus::read(std::string_view directory, const StoreOptions& options = {}) -> Nexus
```

## What It Does

Reconstructs a `Nexus` from a directory previously written by [`save()`](save.md) — the exact
inverse on the persistence side.

`read()` is for Nexus stores, not raw table folders. If you want to ingest a directory of CSV,
JSON, Parquet, or text tables directly into a fresh in-memory workbench, use [`load()`](load.md)
instead.

## Parameters

**directory** (`std::string_view`)
    The directory a prior [`save()`](save.md) call wrote to.

**options** (`const StoreOptions&`, default `{}`)
    Must match what [`save()`](save.md) was called with.

## Returns

`Nexus`
    A workspace equivalent to the one that was saved — same tables, same declared metadata, same
    relationships, same saved views, same [`fingerprint()`](fingerprint.md).

## Examples

Example 1. A full round trip.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 2.0}))},
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
std::cout << std::boolalpha << loaded.has_table_meta("customers") << " "
          << *loaded.table_meta("customers")->grain;
```

### Output

```text
2 1
true one row per customer
```

## Notes

- `read()` is the persisted-store counterpart to [`save()`](save.md).
- `load()` is now reserved for canonical raw-file ingestion.
- `read()` reconstructs `TableMeta`, `Relationship`s, and saved views, not just the raw tables.

## See Also

- [`Nexus::save`](save.md): the write-side counterpart.
- [`Nexus::load`](load.md): raw directory ingestion instead of store restoration.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="links.html">
    <span class="label">Previous</span>
    <span class="title">links</span>
  </a>
  <a class="next-link" href="map_frames.html">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>
