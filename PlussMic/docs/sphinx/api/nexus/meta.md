<!-- pluss-hand-authored -->

# Nexus::meta, Nexus::meta_

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
std::optional<TableMeta> Nexus::meta(std::string_view key) const

Nexus Nexus::meta(std::string_view key, const TableMeta& value) const
Nexus Nexus::meta(std::initializer_list<std::pair<std::string_view, TableMeta>> values) const

Nexus& Nexus::meta_(std::string_view key, const TableMeta& value)
Nexus& Nexus::meta_(std::initializer_list<std::pair<std::string_view, TableMeta>> values)
```

## What It Does

`meta(key)` reads back the `TableMeta` declared for a table, if any (the same read as [`table_meta(key)`](table_meta.md)). `meta(key, value)`
attaches (or replaces) it and returns a new `Nexus`, leaving the source untouched;
`meta_(key, value)` mutates the current handle in place and returns a reference to it.
The bulk overloads mirror [`inject()`](inject.md): if you already know several table
metadata records at once, pass them as one brace list, `std::map`, or `std::unordered_map`
instead of chaining `.meta(...).meta(...).meta(...)`.

There is no partial-update form — each call replaces the table's entire `TableMeta` with
the value you pass. If a table already has metadata and you only want to change one field,
read it back with `meta(key)` first, modify the copy, and pass the whole thing back.

`TableMeta` is presentation and audit metadata layered on top of a table — it never
changes the table's own values, only what `Nexus` knows *about* the table:

### `TableMeta` fields


| Field | Type | Default |
|---|---|---|
| `grain` | `std::optional<std::string>` | empty |
| `primary_key` | `std::vector<std::string>` | empty |
| `candidate_keys` | `std::vector<std::vector<std::string>>` | empty |
| `time_field` | `std::optional<std::string>` | empty |
| `case_labels` | `CaseLabelPolicy` | `CaseLabelPolicy::Unknown` |
| `source` | `std::optional<std::string>` | empty |
| `provenance` | `std::optional<std::string>` | empty |
| `note` | `std::optional<std::string>` | empty |

`grain` is a free-text description of what one row represents (`"one row per customer"`).
`primary_key` and `candidate_keys` declare which feature(s) should uniquely identify a row
— `primary_key` is the one actually used by [`grain_audit()`](grain_audit.md) and the
default-key overloads of [`validate_keys()`](validate_keys.md)/
[`detect_duplicates()`](detect_duplicates.md); `candidate_keys` are alternate key sets
audited alongside it. `time_field` names the feature holding each row's timestamp, used by
[`pull()`](pull.md)'s windowed aggregates. `case_labels` records how the table's row-label
axis was derived — one of `CaseLabelPolicy::Unknown` (the default), `ImplicitRange`,
`ExternalAxis`, `FeatureDerived`, or `Structured`; mostly descriptive — the only place it
affects anything is [`profile()`](profile.md)'s `meta_score` column, which counts it as
one point once it's set to anything other than `Unknown`. `source`/`provenance`/`note` are
free-text bookkeeping fields with no behavioral effect anywhere in `Nexus`.

```{note} TableMeta itself never validates anything
Attaching a `TableMeta` (via `meta()`/`meta()`) does not check that
`primary_key` names a real feature, that `grain` is non-empty, or anything else about
the declared values — `TableMeta` is inert data. Validation happens later, when something
else reads it: [`grain_audit()`](grain_audit.md) throws if a feature named in
`primary_key` doesn't exist on the table, but `meta()` itself accepts any
`TableMeta` you hand it.
```

## Parameters

**key** (`std::string_view`)
    The table to read or attach metadata to. For the getter it does not need to exist in
    the workspace — an unknown key returns `std::nullopt`. For the setters it must already
    exist; an unknown key throws `"Nexus::meta: unknown table '<key>'"`.

**value** (`const TableMeta&`)
    The metadata to attach, replacing anything previously declared for `key`. Accepted
    as-is — `TableMeta` itself is never validated at attachment time.

```{important} An unknown table returns std::nullopt from the getter, it does not throw
Unlike most other lookups on `Nexus` (`frame()`, `apply()`, and friends, which throw
`"unknown key '<key>'"`), `meta(key)` treats "table doesn't exist" and "table exists
but has no metadata" identically — both return `std::nullopt`. If you need to distinguish
the two, check [`has()`](has.md) (table existence) separately from
[`has_meta()`](has_meta.md) (metadata presence).
```

## Returns

`meta(key)`: `std::optional<TableMeta>`
    The declared metadata, or `std::nullopt` if none was ever set (or the key doesn't
    exist at all).

`meta(key, value)` and the bulk `meta(values)`: `Nexus`
    A new workspace with the metadata attached; the source `Nexus` is unchanged.

`meta_(...)`: `Nexus&`
    A reference to the same, now-mutated workspace.

## Examples

Example 1. No metadata declared: `std::nullopt`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Shelf(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

std::cout << std::boolalpha << biz.meta("customers").has_value();
```

### Output

```text
false
```

Example 2. Reading back a full `TableMeta` after declaring it.

```cpp
TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"customer_id"};
meta.source = "crm_export";

auto tagged = biz.meta("customers", meta);
auto fetched = tagged.meta("customers");

std::cout << std::boolalpha << fetched.has_value() << "\n";
std::cout << *fetched->grain << "\n";
std::cout << fetched->primary_key.front() << "\n";
std::cout << *fetched->source;
```

### Output

```text
true
one row per customer
customer_id
crm_export
```

Example 3. An unknown table also returns `std::nullopt`, exactly like a known-but-untagged
table — see the important callout above.

```cpp
std::cout << std::boolalpha << biz.meta("missing").has_value();
```

### Output

```text
false
```

Example 4. Declare metadata on a table, non-mutating form.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Shelf(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"customer_id"};
meta.source = "crm_export";

auto tagged = biz.meta("customers", meta);

std::cout << std::boolalpha << biz.has_meta("customers") << " " << tagged.has_meta("customers");
```

### Output

```text
false true
```

`biz` itself never gains metadata — `meta()` is non-mutating, so `tagged` is a
distinct workspace handle.

Example 5. The mutating form changes the current handle in place.

```cpp
auto mutable_biz = biz;
mutable_biz.meta_("customers", meta);

std::cout << std::boolalpha << biz.has_meta("customers") << " " << mutable_biz.has_meta("customers");
```

### Output

```text
false true
```

`mutable_biz` was a separate copy of `biz` before the mutating call; `biz` itself is still
untouched.

Example 6. Setting metadata on an unknown table throws.

```cpp
try {
  auto bad = biz.meta("missing", meta);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::meta: unknown table 'missing'
```

Example 7. Each call replaces the whole `TableMeta`, not just the fields you set — reading
it back first is the way to do a partial update.

```cpp
TableMeta grain_only;
grain_only.grain = "one row per customer";
auto step1 = biz.meta("customers", grain_only);

auto updated = *step1.meta("customers");
updated.source = "crm_export";
auto step2 = step1.meta("customers", updated);

std::cout << std::boolalpha << step1.meta("customers")->source.has_value() << " "
          << std::boolalpha << step2.meta("customers")->source.has_value();
```

### Output

```text
false true
```

`step1`'s `TableMeta` never had `source` set, so it's still absent there; `step2` carries
the merged copy built by reading `step1`'s metadata first.

Example 8. Bulk metadata declaration keeps multi-table setup shorter.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Shelf(pluss::make_series({1.0, 2.0}))},
});
auto orders = DataFrame({
    {"order_id",    Shelf(pluss::make_series({10.0, 11.0}))},
    {"customer_id", Shelf(pluss::make_series({1.0, 2.0}))},
    {"ordered_at",  Shelf(pluss::make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-03T00:00:00Z",
    }))},
});

TableMeta customers_meta;
customers_meta.primary_key = {"customer_id"};

TableMeta orders_meta;
orders_meta.primary_key = {"order_id"};
orders_meta.time_field = "ordered_at";

auto tagged = Nexus({
    {"customers", customers},
    {"orders", orders},
}).meta({
    {"customers", customers_meta},
    {"orders", orders_meta},
});

std::cout << tagged.meta("customers")->primary_key.front() << "\n";
std::cout << tagged.meta("orders")->time_field.value();
```

### Output

```text
customer_id
ordered_at
```

## Notes

- `meta(key)` returns `std::nullopt` both for "table has no metadata" and "table doesn't
  exist" — see the important callout above.
- No partial-update form exists — every setter call replaces the table's entire
  `TableMeta`. See the read-modify-write example above.
- An unknown `key` throws from the setters; `TableMeta` field values themselves are never
  validated at attachment time.
- The bulk setter is still validated one table at a time. An unknown key throws the same
  `"Nexus::meta: unknown table '<name>'"` error you would get from the one-table form.

## See Also

- [`Nexus::has_meta`](has_meta.md): check presence without unwrapping an `std::optional`.
- [`Nexus::grain_audit`](grain_audit.md): the primary consumer of a declared
  `primary_key`/`candidate_keys`.
- [`Nexus::pull`](pull.md): windowed aggregates consume a declared `time_field`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="materialize.html">
    <span class="label">Previous</span>
    <span class="title">materialize</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
