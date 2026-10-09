<!-- pluss-hand-authored -->

# Nexus::grain_audit

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::grain_audit(std::string_view table) const -> DataFrame
```

## What It Does

Runs the same completeness-and-uniqueness check as [`validate_keys()`](validate_keys.md)
against **every** declared key set on a table — the `primary_key` plus every entry in
`candidate_keys` — and returns the results as one row per key set, so you can compare them
side by side instead of calling [`validate_keys()`](validate_keys.md) once per key set
yourself.

```{important} Requires declared metadata with at least one key set
Unlike [`validate_keys()`](validate_keys.md)'s two-argument overload, `grain_audit()` has
no explicit-key-list form — it only audits what's already declared via
[`set_table_meta()`](table_meta.md). A table with no declared `TableMeta` at all, or with
metadata whose `primary_key` is empty and `candidate_keys` is also empty, throws.
```

## Parameters

**table** (`std::string_view`)
    The table to audit. Must already exist in the workspace and have `TableMeta` declared
    with at least one non-empty key set (`primary_key` and/or an entry in
    `candidate_keys`).

## Returns

`DataFrame`
    One row per audited key set, with columns:

    - `table` — the table name (repeated on every row).
    - `key_name` — `"primary"` for the primary key, `"candidate_0"`, `"candidate_1"`, ...
      for each candidate key set, in declaration order.
    - `key_signature` — the key feature names joined with `" + "`.
    - `key_width` — how many features make up that key set.
    - `rows` — total row count in the table.
    - `complete_rows` / `missing_rows` — rows with a non-missing value in every key
      feature, and the complement.
    - `duplicate_rows` — how many (complete) rows are involved in a duplicate key
      combination.
    - `valid` — `true` only when `missing_rows == 0` and `duplicate_rows == 0`, matching
      what [`validate_keys()`](validate_keys.md) would return for that same key set.

## Examples

Example 1. Auditing a declared primary key with a duplicate value.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy", "Cy2"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta meta;
meta.primary_key = {"customer_id"};
auto tagged = biz.set_table_meta("customers", meta);

auto audit = tagged.grain_audit("customers");
std::cout << audit.features({"table", "key_name", "key_signature", "rows", "duplicate_rows", "valid"});
```

### Output

```text
======================================================================
     | table      key_name  key_signature  rows  duplicate_rows  valid
----------------------------------------------------------------------
0    | customers  primary   customer_id    4     2               false
----------------------------------------------------------------------
[1 cs x 6 feat]
```

Example 2. Calling `grain_audit()` on a table with no declared key sets throws.

```cpp
try {
  auto bad = biz.grain_audit("customers");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::grain_audit: no metadata declared for table 'customers'
```

## Notes

- Requires at least one declared key set (`primary_key` and/or `candidate_keys`) — see the
  important callout above.
- One row per audited key set; `key_name` distinguishes `"primary"` from
  `"candidate_N"`.
- `valid` mirrors what [`validate_keys()`](validate_keys.md) would return for that same key
  set.

## See Also

- [`Nexus::validate_keys`](validate_keys.md): the same check, one key set at a time, either
  declared or explicit.
- [`Nexus::detect_duplicates`](detect_duplicates.md): the actual offending rows for one key
  set.
- [`Nexus::table_meta`](table_meta.md): declare `primary_key`/`candidate_keys` before
  auditing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="graph.html">
    <span class="label">Previous</span>
    <span class="title">graph</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
