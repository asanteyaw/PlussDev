<!-- pluss-hand-authored -->

# Locus::save, Locus::load, Locus::read

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::save(std::string_view directory, const StoreOptions& options = {}) const -> void
static Locus::load(std::string_view directory, const LoadOptions& options = {}) -> Locus
static Locus::read(std::string_view directory, const StoreOptions& options = {}) -> Locus
```

## What It Does

`save()` writes every catalog to `directory`, one stream at a time
(`Catalog::save_streams`), so each stream's extent, nulls, and kind round-trip
exactly -- a ragged catalog comes back exactly as ragged as it was saved, with no
padding and no trimming. Declared relationships, table metadata, and saved join plans
are written alongside and round-trip too.

`read()` is the matching load: it reads a directory `save()` wrote, and also accepts a
directory written by `Nexus::save()` (rectangular tables) -- either way you get a
fully reconstructed `Locus` back, links and all. `load()` is a different, lower-level
entry point: it reads a directory of loose table files directly through the
`DataFrame` loaders, with no expectation that the directory was produced by `save()`
or that it carries relationship/metadata information at all.

## Parameters

| Parameter | Meaning |
|---|---|
| `directory` | The directory to write to or read from. |
| `options` | `StoreOptions` -- file kind (CSV, Parquet, ...) and related settings, for `save`/`read`. |
| `options` (`load`) | `LoadOptions` -- how to interpret a directory of loose table files. |

## Returns

`save` returns `void`. `load` and `read` return a newly constructed `Locus`.

## Errors

Throws when `directory` cannot be written to (`save`) or does not contain data in the
expected shape (`load`/`read`).

## Examples

Example 1. Round-tripping a linked workspace through `save()`/`read()`.

```cpp
const auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

const auto dir = fs::temp_directory_path() / "locus_docs_persistence_example";
fs::remove_all(dir);

shop.save(dir.string());
std::cout << "save() wrote a directory: " << fs::exists(dir) << '\n';

const auto back = Locus::read(dir.string());
std::cout << "read() keys match: " << (back.keys() == shop.keys()) << '\n';
std::cout << "read() nrelationships: " << back.nrelationships() << '\n';
std::cout << "restored join still works, rows: " << back.join("orders", "customers").get("amount")->size() << '\n';

fs::remove_all(dir);
```

### Output

```text
save() wrote a directory: true
read() keys match: true
read() nrelationships: 1
restored join still works, rows: 3
```

The restored `Locus` joins exactly like the original (`3` matched rows, the same
inner-join result [`join`](join.md)'s own first example produces) -- the declared link
survived the round trip, not just the raw catalog data.

## See Also

- [`join`](join.md): the join a restored `Locus` can run immediately, with no
  relationships to redeclare.
- [`link`](link.md): what gets written and restored alongside the catalog data.
