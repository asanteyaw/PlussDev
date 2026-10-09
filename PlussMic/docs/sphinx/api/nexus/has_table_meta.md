<!-- pluss-hand-authored -->

# Nexus::has_table_meta

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::has_table_meta(std::string_view key) const -> bool
```

## What It Does

Reports whether a table currently has [`TableMeta`](table_meta.md) declared — grain,
primary/candidate keys, a time field, and so on. This is the safe way to check before
relying on declared metadata: several auditing methods
([`grain_audit()`](grain_audit.md), the default-key overloads of
[`validate_keys()`](validate_keys.md)/[`detect_duplicates()`](detect_duplicates.md))
require metadata to already be declared and throw otherwise.

```{note} Checks presence, not readiness
`has_table_meta()` only reports whether a `TableMeta` value has been attached at all —
even a `TableMeta{}` with every field left at its default counts as "has metadata". It
does not check whether the declared metadata is actually useful (for example, whether a
primary key was ever set). If you need "does this table have a usable primary key
declared", read the [`primary_key`](table_meta.md) field off [`table_meta()`](table_meta.md)'s
result yourself.
```

## Parameters

**key** (`std::string_view`)
    The table name to check. Unlike most other lookups on this class, an unknown key does
    not throw here — it simply returns `false`.

## Returns

`bool`
    `true` if the table has `TableMeta` declared (via [`set_table_meta()`](set_table_meta.md) or
    [`meta()`](meta.md)), `false` if it doesn't — including for a key that doesn't exist in
    the workspace at all.

## Examples

Example 1. No metadata declared yet.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

std::cout << std::boolalpha << biz.has_table_meta("customers");
```

### Output

```text
false
```

Example 2. After declaring metadata, `has_table_meta()` reports `true` on the returned
workspace — but the original, un-tagged workspace is unaffected.

```cpp
TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"customer_id"};

auto tagged = biz.set_table_meta("customers", meta);

std::cout << std::boolalpha << biz.has_table_meta("customers") << " " << tagged.has_table_meta("customers");
```

### Output

```text
false true
```

Example 3. An unknown key returns `false` rather than throwing.

```cpp
std::cout << std::boolalpha << biz.has_table_meta("missing");
```

### Output

```text
false
```

## Notes

- Reports presence only, not "readiness" — see the note above.
- An unknown table key returns `false`; it does not throw.
- Non-mutating; simply inspects the workspace's current state.

## See Also

- [`Nexus::table_meta`](table_meta.md): read the declared metadata itself.
- [`Nexus::set_table_meta`](set_table_meta.md): attach metadata to a table.
- [`Nexus::grain_audit`](grain_audit.md): one of several methods that requires declared
  metadata and throws when it's missing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_store.html">
    <span class="label">Previous</span>
    <span class="title">has_store</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
