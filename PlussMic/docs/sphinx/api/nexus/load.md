<!-- pluss-hand-authored -->

# Nexus::load

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
static Nexus::load(std::string_view directory, const StoreOptions& options = {}) -> Nexus
```

## What It Does

Reconstructs a `Nexus` from a directory previously written by [`save()`](save.md) — the
exact inverse. A **static** method (called as `Nexus::load(...)`, not on an existing
instance), since there's no workspace to call it on yet.

`load()` reads `options.manifest` first to discover the store's own `kind` and the other
file names/directory, then reads `options.schema`/`options.graph` to reconstruct every
table's [`TableMeta`](table_meta.md) and every declared [`Relationship`](relationships.md),
then each table's own data file from the `tables` subdirectory, and finally
`options.views` (if present) to restore saved views.

```{important} options must match what save() actually used
`load()` doesn't read `options.kind` from the manifest to guess the table file format for
you — it uses `options.kind` (or the manifest's own recorded `kind`, for locating the
right file extension) consistently with whatever `save()` was called with. Pass the same
`StoreOptions` you saved with, especially `kind` — mismatched file extensions won't be
found.
```

## Parameters

**directory** (`std::string_view`)
    The directory a prior [`save()`](save.md) call wrote to.

**options** (`const StoreOptions&`, default `{}`)
    Must match what [`save()`](save.md) was called with — see the important callout above.

## Returns

`Nexus`
    A workspace equivalent to the one that was saved — same tables, same declared
    metadata, same relationships, same saved views, same [`fingerprint()`](fingerprint.md).

## Examples

Example 1. A full round trip.

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

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = tagged.add_relationship(rel);

StoreOptions options;
options.kind = StoreKind::Csv;
linked.save("/tmp/nexus_biz_store", options);

auto loaded = Nexus::load("/tmp/nexus_biz_store", options);

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

- Static — called as `Nexus::load(...)`, not on an existing `Nexus` instance.
- Must be called with the same `StoreOptions` (especially `kind`) that
  [`save()`](save.md) was called with — see the important callout above.
- Fully reconstructs declared `TableMeta` and `Relationship`s, plus any saved views — not
  just the raw table data.

## See Also

- [`Nexus::save`](save.md): the exact inverse of this method.
- [`Nexus::fingerprint`](fingerprint.md): compare before/after to confirm a round trip
  preserved everything.

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
