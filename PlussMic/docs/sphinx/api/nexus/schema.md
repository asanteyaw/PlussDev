<!-- pluss-hand-authored -->

# Nexus::schema

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::schema() const -> DataFrame
```

## What It Does

Renders the workspace's table-level metadata as a `DataFrame`, one row per table —
combining structural facts (row/column counts, feature names, dtypes) with whatever
[`TableMeta`](table_meta.md) has been declared. This is the format
[`save()`](save.md) writes to disk (as `schema.csv` by default) and
[`load()`](load.md) reads back to reconstruct declared metadata.

## Parameters

`None`

## Returns

`DataFrame`
    One row per table, with columns:

    - `table` — the table name.
    - `rows`/`cols` — `ncases()`/`nfeatures()`.
    - `feats` — every feature name, joined with an internal separator (not meant for
      display parsing — see [`frame(table).features()`](frame.md) for the actual list).
    - `dtypes` — each feature's dtype, similarly joined.
    - `grain`, `pkey`, `ckeys`, `time`, `labels`, `source`, `provenance`, `note` — the
      corresponding [`TableMeta`](table_meta.md) fields, each blank for a table with no
      declared metadata (or no value set for that particular field).
    - `fingerprint` — a per-table content hash combined with the whole workspace's
      [`fingerprint()`](fingerprint.md), truncated to 8 hex characters.

## Examples

Example 1. A workspace with metadata on one table, none on the other.

```cpp
using namespace pluss::table;

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

auto s = tagged.schema();
std::cout << s.features({"table", "rows", "cols", "grain", "pkey"});
```

### Output

```text
===============================================================
     | table      rows  cols  grain                 pkey
---------------------------------------------------------------
0    | customers  3     2     one row per customer  customer_id
1    | orders     3     2
---------------------------------------------------------------
[2 cs x 5 feat]
```

`orders` has no declared metadata, so its `grain` and `pkey` cells are blank.

## Notes

- This is exactly the format [`save()`](save.md)/[`load()`](load.md) round-trip through
  disk for table-level metadata.
- `feats`/`dtypes` are joined strings meant for the save/load round trip, not for reading
  directly — inspect a table's actual feature list via
  `frame(table).features()`/`frame(table).dtypes()` instead.

## See Also

- [`Nexus::graph`](graph.md): the relationship-level counterpart to this table-level view.
- [`Nexus::table_meta`](table_meta.md): the source of most of this method's columns.
- [`Nexus::save`](save.md) / [`load`](load.md): where this exact format is persisted and
  restored.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
