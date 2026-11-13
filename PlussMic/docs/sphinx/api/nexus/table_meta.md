<!-- pluss-hand-authored -->

# Nexus::table_meta

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
std::optional<TableMeta> Nexus::table_meta(std::string_view key) const
```

## What It Does

Reads back the `TableMeta` declared for a table, if any. `TableMeta` is presentation and
audit metadata layered on top of a table — it never changes the table's own values, only
what `Nexus` knows *about* the table:

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
Attaching a `TableMeta` (via `set_table_meta()`/`meta()`) does not check that
`primary_key` names a real feature, that `grain` is non-empty, or anything else about
the declared values — `TableMeta` is inert data. Validation happens later, when something
else reads it: [`grain_audit()`](grain_audit.md) throws if a feature named in
`primary_key` doesn't exist on the table, but `set_table_meta()` itself accepts any
`TableMeta` you hand it.
```

## Parameters

**key** (`std::string_view`)
    The table to look up. Does not need to exist in the workspace — see the important
    callout below for what happens when it doesn't.

```{important} An unknown table returns std::nullopt, it does not throw
Unlike most other lookups on `Nexus` (`frame()`, `apply()`, and friends, which throw
`"unknown key '<key>'"`), `table_meta()` treats "table doesn't exist" and "table exists
but has no metadata" identically — both return `std::nullopt`. If you need to distinguish
the two, check [`has()`](has.md) (table existence) separately from
[`has_table_meta()`](has_table_meta.md) (metadata presence).
```

## Returns

`std::optional<TableMeta>`
    The declared metadata, or `std::nullopt` if none was ever set (or the key doesn't
    exist at all — see the important callout above).

## Examples

Example 1. No metadata declared: `std::nullopt`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

std::cout << std::boolalpha << biz.table_meta("customers").has_value();
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

auto tagged = biz.set_table_meta("customers", meta);
auto fetched = tagged.table_meta("customers");

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
std::cout << std::boolalpha << biz.table_meta("missing").has_value();
```

### Output

```text
false
```

## Notes

- Returns `std::nullopt` both for "table has no metadata" and "table doesn't exist" — see
  the important callout above.
- `TableMeta` itself is inert; nothing in `set_table_meta()`/`table_meta()` validates its
  contents — see the note above.
- [`meta(key)`](meta.md) is an exact alias for this same getter.

## See Also

- [`Nexus::has_table_meta`](has_table_meta.md): check presence without unwrapping the
  `std::optional`.
- [`Nexus::set_table_meta`](set_table_meta.md): attach or replace a table's metadata.
- [`Nexus::grain_audit`](grain_audit.md): the primary consumer of a declared
  `primary_key`/`candidate_keys`.
- [`Nexus::pull`](pull.md): windowed aggregates consume a declared `time_field`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="summary.html">
    <span class="label">Previous</span>
    <span class="title">summary</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
