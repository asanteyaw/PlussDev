<!-- pluss-hand-authored -->

# Nexus::set_table_meta

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::set_table_meta(std::string_view key, const TableMeta& meta) const -> Nexus

Nexus& Nexus::set_table_meta_(std::string_view key, const TableMeta& meta)
```

## What It Does

Attaches (or replaces) a table's declared [`TableMeta`](table_meta.md) — grain, primary and
candidate keys, a time field, a case-label policy, and free-text
source/provenance/note fields. Follows this class's usual immutable/mutable pairing: the
plain `set_table_meta()` returns a new `Nexus` with the change applied, leaving the source
untouched; `set_table_meta_()` mutates the current handle in place and returns a reference
to it.

There is no partial-update form — each call replaces the table's entire `TableMeta` with
the value you pass. If a table already has metadata and you only want to change one field,
read it back with [`table_meta()`](table_meta.md) first, modify the copy, and pass the
whole thing back.

## Parameters

**key** (`std::string_view`)
    The table to attach metadata to. Must already exist in the workspace — an unknown key
    throws `"Nexus::set_table_meta: unknown table '<key>'"`.

**meta** (`const TableMeta&`)
    The metadata to attach, replacing anything previously declared for `key`. Accepted
    as-is — see [`table_meta()`](table_meta.md)'s note that `TableMeta` itself is never
    validated at attachment time.

## Returns

For `set_table_meta()`: `Nexus`
    A new workspace with `meta` attached to `key`; the source `Nexus` is unchanged.

For `set_table_meta_()`: `Nexus&`
    A reference to the same, now-mutated workspace.

## Examples

Example 1. Declare metadata on a table, non-mutating form.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"customer_id"};
meta.source = "crm_export";

auto tagged = biz.set_table_meta("customers", meta);

std::cout << std::boolalpha << biz.has_table_meta("customers") << " " << tagged.has_table_meta("customers");
```

### Output

```text
false true
```

`biz` itself never gains metadata — `set_table_meta()` is non-mutating, so `tagged` is a
distinct workspace handle.

Example 2. The mutating form changes the current handle in place.

```cpp
auto mutable_biz = biz;
mutable_biz.set_table_meta_("customers", meta);

std::cout << std::boolalpha << biz.has_table_meta("customers") << " " << mutable_biz.has_table_meta("customers");
```

### Output

```text
false true
```

`mutable_biz` was a separate copy of `biz` before the mutating call; `biz` itself is still
untouched.

Example 3. Setting metadata on an unknown table throws.

```cpp
try {
  auto bad = biz.set_table_meta("missing", meta);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::set_table_meta: unknown table 'missing'
```

Example 4. Each call replaces the whole `TableMeta`, not just the fields you set — reading
it back first is the way to do a partial update.

```cpp
TableMeta grain_only;
grain_only.grain = "one row per customer";
auto step1 = biz.set_table_meta("customers", grain_only);

auto updated = *step1.table_meta("customers");
updated.source = "crm_export";
auto step2 = step1.set_table_meta("customers", updated);

std::cout << std::boolalpha << step1.table_meta("customers")->source.has_value() << " "
          << std::boolalpha << step2.table_meta("customers")->source.has_value();
```

### Output

```text
false true
```

`step1`'s `TableMeta` never had `source` set, so it's still absent there; `step2` carries
the merged copy built by reading `step1`'s metadata first.

## Notes

- No partial-update form exists — every call replaces the table's entire `TableMeta`. See
  Example 4 for the read-modify-write pattern.
- An unknown `key` throws; `TableMeta` field values themselves are never validated at
  attachment time.
- [`meta(key, value)`](meta.md)/[`meta_(key, value)`](meta.md) are exact aliases for these
  two overloads.

## See Also

- [`Nexus::table_meta`](table_meta.md): read back what was declared, including the full
  `TableMeta` field list.
- [`Nexus::has_table_meta`](has_table_meta.md): check presence without unwrapping an
  `std::optional`.
- [`Nexus::grain_audit`](grain_audit.md): the primary consumer of a declared `primary_key`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
