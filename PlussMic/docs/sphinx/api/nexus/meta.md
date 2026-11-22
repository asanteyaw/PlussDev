<!-- pluss-hand-authored -->

# Nexus::meta

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

`meta(key)` reads a table's declared [`TableMeta`](table_meta.md) back (the same read as
[`table_meta(key)`](table_meta.md)), and `meta(key, value)` attaches or replaces it:
grain, primary and candidate keys, a time field, a case-label policy, and free-text
source/provenance/note fields. Follows this class's usual immutable/mutable pairing: the
plain `meta(key, value)` returns a new `Nexus` with the change applied, leaving the source
untouched; `meta_(key, value)` mutates the current handle in place and returns a reference
to it. The bulk setter overload mirrors [`inject()`](inject.md): if you already know several
table metadata records at once, pass them as one brace list, `std::map`, or
`std::unordered_map` instead of chaining `.meta(...).meta(...).meta(...)`.

There is no partial-update form: each call replaces the table's entire `TableMeta` with
the value you pass. If a table already has metadata and you only want to change one field,
read it back with `meta(key)` first, modify the copy, and pass the whole thing back.

## Parameters

**key** (`std::string_view`)

The table to read or describe. For the setters it must already exist in the workspace;
an unknown key throws `"Nexus::meta: unknown table '<key>'"`. The getter returns
`std::nullopt` instead.

**value** (`const TableMeta&`)

The metadata to attach, replacing anything previously declared for `key`. Accepted
as-is: see [`table_meta()`](table_meta.md)'s note that `TableMeta` itself is never
validated at attachment time.

**values** (brace list of `{key, TableMeta}` pairs)

The bulk form: one entry per table, each applied in order with the same checks as the
one-table setter.

## Returns

For `meta(key)`: `std::optional<TableMeta>`
    The declared metadata, or `std::nullopt` when none is declared.

For `meta(key, value)` and `meta(values)`: `Nexus`
    A new workspace with the metadata attached; the source `Nexus` is unchanged.

For `meta_(...)`: `Nexus&`
    A reference to the same, now-mutated workspace.

## Examples

Example 1. Declare metadata with `meta(key, value)` and read it back with `meta(key)`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Shelf(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Shelf(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta info;
info.grain = "one row per customer";
info.primary_key = {"customer_id"};

auto tagged = biz.meta("customers", info);
auto fetched = tagged.meta("customers");

std::cout << std::boolalpha << fetched.has_value() << "\n";
std::cout << *fetched->grain;
```

### Output

```text
true
one row per customer
```

Example 2. Bulk metadata declaration keeps multi-table setup shorter.

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

Example 3. The mutating form changes the current handle in place, and setting metadata on
an unknown table throws.

```cpp
auto mutable_biz = biz;
mutable_biz.meta_("customers", info);

std::cout << std::boolalpha << biz.has_meta("customers") << " " << mutable_biz.has_meta("customers") << "\n";
try {
  auto bad = biz.meta("missing", info);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
false true
Nexus::meta: unknown table 'missing'
```

`mutable_biz` was a separate copy of `biz` before the mutating call; `biz` itself is still
untouched.

Example 4. Each call replaces the whole `TableMeta`, not just the fields you set; reading
it back first is the way to do a partial update.

```cpp
TableMeta grain_only;
grain_only.grain = "one row per customer";
auto step1 = biz.meta("customers", grain_only);

auto updated = *step1.meta("customers");
updated.source = "crm_export";
auto step2 = step1.meta("customers", updated);

std::cout << std::boolalpha << step1.meta("customers")->source.has_value() << " "
          << step2.meta("customers")->source.has_value();
```

### Output

```text
false true
```

`step1`'s `TableMeta` never had `source` set, so it's still absent there; `step2` carries
the merged copy built by reading `step1`'s metadata first.

## Notes

- No partial-update form exists: every call replaces the table's entire `TableMeta`. See
  Example 4 for the read-modify-write pattern.
- An unknown `key` throws from the setters (and from each entry of the bulk form, one table
  at a time); `TableMeta` field values themselves are never validated at attachment time.
- [`table_meta(key)`](table_meta.md) is an exact alias for the getter overload.

## See Also

- [`Nexus::table_meta`](table_meta.md): the explicitly-named getter, including the full
  `TableMeta` field list.
- [`Nexus::has_meta`](has_meta.md): check presence without unwrapping an `std::optional`.
- [`Nexus::grain_audit`](grain_audit.md): the primary consumer of a declared `primary_key`.

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
