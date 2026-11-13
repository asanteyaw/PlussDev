<!-- pluss-hand-authored -->

# Nexus::scan

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
DataFrame Nexus::scan() const
```

## What It Does

Builds a unified diagnostics table that mixes table rows and relationship rows in
one place.

This is the closest thing `Nexus` has to a command-center overview. Table rows tell
you how declared primary keys and metadata look. Relationship rows tell you whether
the declared join structure is actually holding up under missing keys, duplicate
keys, and orphan pressure.

Unlike [`profile()`](profile.md), which stays table-only, `scan()` is intentionally
heterogeneous: some rows describe tables, others describe relationships, and the
`kind` column tells you which is which.

## Parameters

`None`

## Returns

`DataFrame`
    One row per table plus one row per declared relationship.

    The output columns are:

    - `kind`: `"table"` or `"relationship"`
    - `name`: table name or relationship name
    - `left`, `right`: row anchors; for table rows, `left` repeats the table name
    - `key_signature`: primary-key or relationship-key signature
    - `rows`, `cols`: populated for table rows, `-1` for relationship rows
    - `left_missing_rows`, `right_missing_rows`
    - `left_duplicate_rows`, `right_duplicate_rows`
    - `left_orphans`, `right_orphans`
    - `valid`
    - `declared_status`, `computed_status`
    - `meta_score`, `meta_ready`
    - `confidence`
    - `note`

The most important distinction is `declared_status` vs `computed_status`:
`declared_status` is what the relationship says it is, while `computed_status` is
what the current data actually supports.

## Examples

Example 1. `scan()` combines table and relationship rows.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto loan = DataFrame({
    {"loan_id",    Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"account_id", Slate(pluss::make_series({10.0, 20.0, 30.0}))},
    {"amount",     Slate(pluss::make_series({100.0, 150.0, 120.0}))},
});

auto account = DataFrame({
    {"account_id",  Slate(pluss::make_series({10.0, 20.0, 30.0}))},
    {"district_id", Slate(pluss::make_series({1.0, 1.0, 2.0}))},
    {"segment",     Slate(pluss::make_note({"retail", "retail", "vip"}))},
});

auto trans = DataFrame({
    {"trans_id",   Slate(pluss::make_series({1001.0, 1002.0, 1003.0}))},
    {"account_id", Slate(pluss::make_series({10.0, 10.0, 20.0}))},
    {"amount",     Slate(pluss::make_series({50.0, 20.0, 80.0}))},
});

TableMeta loan_meta{};
loan_meta.grain = "loan";
loan_meta.primary_key = {"loan_id"};
loan_meta.candidate_keys = {{"loan_id"}, {"account_id", "loan_id"}};
loan_meta.time_field = "loan_id";
loan_meta.case_labels = CaseLabelPolicy::Structured;
loan_meta.source = "core.loan";
loan_meta.note = "loan book";

TableMeta account_meta{};
account_meta.grain = "account";
account_meta.primary_key = {"account_id"};
account_meta.source = "core.account";

Relationship loan_account{};
loan_account.name = "loan_account";
loan_account.left_table = "loan";
loan_account.right_table = "account";
loan_account.keys = {{"account_id", "account_id"}};
loan_account.cardinality = RelationshipCardinality::ManyToOne;
loan_account.confidence = 0.95;
loan_account.status = RelationshipValidationStatus::Declared;
loan_account.note = "loan to account";

auto nx = Nexus({
    {"loan", loan},
    {"account", account},
    {"trans", trans},
}).meta("loan", loan_meta)
  .meta("account", account_meta)
  .links({loan_account});

auto scan = nx.scan();

std::cout << scan.ncases() << " " << scan.nfeatures() << "\n";
for (std::int64_t i = 0; i < scan.ncases(); ++i) {
  auto idx = static_cast<std::size_t>(i);
  std::cout << scan.features("kind").as_note().at(idx) << " "
            << scan.features("name").as_note().at(idx) << " "
            << scan.features("computed_status").as_note().at(idx) << "\n";
}
```

### Output

```text
4 20
table loan validated
table account validated
table trans undeclared
relationship loan_account validated
```

Example 2. Relationship rows expose declared-vs-computed state and audit counts.

This relationship was declared as `"declared"`, but the current data supports a
computed `"validated"` status. All the key-health and orphan counters are zero in
this clean fixture.

```cpp
std::cout << scan.features("declared_status").as_note().at(3) << " "
          << scan.features("computed_status").as_note().at(3) << " "
          << scan.features("left_missing_rows").as_series().host<std::int64_t>()[3] << " "
          << scan.features("right_missing_rows").as_series().host<std::int64_t>()[3] << " "
          << scan.features("left_duplicate_rows").as_series().host<std::int64_t>()[3] << " "
          << scan.features("right_duplicate_rows").as_series().host<std::int64_t>()[3] << " "
          << scan.features("left_orphans").as_series().host<std::int64_t>()[3] << " "
          << scan.features("right_orphans").as_series().host<std::int64_t>()[3] << " "
          << scan.features("confidence").as_series().host<double>()[3];
```

### Output

```text
declared validated 0 0 0 0 0 0 0.95
```

Example 3. When the declared key on one side degrades, `scan()` marks the
relationship invalid.

```cpp
auto broken = nx.apply("account", [](const DataFrame&) {
  return DataFrame({
      {"account_id",  Slate(pluss::make_series({10.0, 10.0, 30.0}))},
      {"district_id", Slate(pluss::make_series({1.0, 1.0, 2.0}))},
      {"segment",     Slate(pluss::make_note({"retail", "retail", "vip"}))},
  });
});

auto broken_scan = broken.scan();
std::cout << broken_scan.features("computed_status").as_note().at(3) << " "
          << broken_scan.features("right_duplicate_rows").as_series().host<std::int64_t>()[3];
```

### Output

```text
invalid 2
```

## Notes

- `scan()` is non-mutating.
- The current implementation caches the default result until the workspace changes.
- Table rows and relationship rows intentionally share the same frame even though
  some columns are only meaningful for one `kind`.
- For relationship rows, `rows` and `cols` are set to `-1` because the row is not
  describing a materialized table.

## See Also

- [`Nexus::profile`](profile.md): table-only diagnostics.
- [`Nexus::deduce`](deduce.md): suggest plausible undeclared relationships.
- [`Nexus::summary`](summary.md): compact inventory only, with no audit detail.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
  <a class="next-link" href="schema.html">
    <span class="label">Next</span>
    <span class="title">schema</span>
  </a>
</div>
