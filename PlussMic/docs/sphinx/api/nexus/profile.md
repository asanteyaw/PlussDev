<!-- pluss-hand-authored -->

# Nexus::profile

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::profile() const -> DataFrame
```

## What It Does

Builds a table-level diagnostics frame for the entire workspace.

If [`summary()`](summary.md) answers "what tables are here and how large are they?",
`profile()` answers the next layer of questions:

- how complete is each table's metadata?
- does a declared primary key look healthy?
- how connected is this table to the relationship graph?
- what is the table's null burden and time-field declaration?

The output is one row per workspace table. Relationships do not get their own rows
here; for the mixed table-plus-relationship command-center view, use [`scan()`](scan.md).

## Parameters

`None`

## Returns

`DataFrame`
    One row per table, with these columns:

    - `table`: table key
    - `rows`, `cols`: row and feature counts
    - `null_cells`, `null_frac`: total missing cells and the fraction of all cells
    - `meta_score`: count of populated metadata fields among `grain`,
      `primary_key`, `candidate_keys`, `time_field`, `case_labels`, `source`,
      `provenance`, and `note`
    - `meta_ready`: `true` only when `grain` and `primary_key` are both present
    - `primary_key`: declared primary-key signature, or empty when none exists
    - `key_missing_rows`, `key_duplicate_rows`: health of the declared primary key
    - `links`, `left_links`, `right_links`: relationship-graph connection counts
    - `time_field`: declared time field, if any
    - `fingerprint`: short per-table fingerprint

For tables with no declared primary key, `key_missing_rows` and
`key_duplicate_rows` are emitted as `-1` sentinels rather than guessed values.

## Examples

Example 1. `profile()` returns one diagnostics row per table.

```cpp
using namespace pluss::table;

auto loan = DataFrame({
    {"loan_id",    Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"account_id", Thread(pluss::make_series({10.0, 20.0, 30.0}))},
    {"amount",     Thread(pluss::make_series({100.0, 150.0, 120.0}))},
});

auto account = DataFrame({
    {"account_id",  Thread(pluss::make_series({10.0, 20.0, 30.0}))},
    {"district_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    {"segment",     Thread(pluss::make_note({"retail", "retail", "vip"}))},
});

auto trans = DataFrame({
    {"trans_id",   Thread(pluss::make_series({1001.0, 1002.0, 1003.0}))},
    {"account_id", Thread(pluss::make_series({10.0, 10.0, 20.0}))},
    {"amount",     Thread(pluss::make_series({50.0, 20.0, 80.0}))},
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

auto nx = Nexus({
    {"loan", loan},
    {"account", account},
    {"trans", trans},
}).meta("loan", loan_meta)
  .meta("account", account_meta)
  .links({loan_account});

auto profile = nx.profile();

std::cout << profile.ncases() << " " << profile.nfeatures() << "\n";
for (std::int64_t i = 0; i < profile.ncases(); ++i) {
  auto idx = static_cast<std::size_t>(i);
  std::cout << profile.features("table").as_note().at(idx) << " "
            << std::boolalpha
            << profile.features("meta_ready").as_series().to_vec<bool>()[idx] << " "
            << profile.features("primary_key").as_note().at(idx) << " "
            << profile.features("links").as_series().to_vec<std::int64_t>()[idx] << " "
            << profile.features("left_links").as_series().to_vec<std::int64_t>()[idx] << " "
            << profile.features("right_links").as_series().to_vec<std::int64_t>()[idx] << " "
            << profile.features("time_field").as_note().at(idx) << "\n";
}
```

### Output

```text
3 15
loan true loan_id 1 1 0 loan_id
account true account_id 1 0 1 
trans false  0 0 0 
```

Example 2. Undeclared tables emit explicit sentinel values for key-health fields.

`trans` has no declared metadata in this fixture. `profile()` does not invent a
primary key for it, and it does not silently reinterpret repeated columns as one.
Instead it marks the table as not metadata-ready and uses `-1` for the primary-key
health columns.

```cpp
std::cout << std::boolalpha
          << profile.features("meta_ready").as_series().to_vec<bool>()[2] << " "
          << profile.features("key_missing_rows").as_series().to_vec<std::int64_t>()[2] << " "
          << profile.features("key_duplicate_rows").as_series().to_vec<std::int64_t>()[2];
```

### Output

```text
false -1 -1
```

Example 3. Declared-key diagnostics respond to transformed table contents.

Here the `account` table is rewritten with a duplicate `account_id`. Because that
column is declared as the primary key, `profile()` surfaces the duplicate pressure
immediately.

```cpp
auto broken = nx.apply("account", [](const DataFrame&) {
  return DataFrame({
      {"account_id",  Thread(pluss::make_series({10.0, 10.0, 30.0}))},
      {"district_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
      {"segment",     Thread(pluss::make_note({"retail", "retail", "vip"}))},
  });
});

auto broken_profile = broken.profile();
std::cout << broken_profile.features("key_duplicate_rows").as_series().to_vec<std::int64_t>()[1] << " "
          << broken_profile.features("key_missing_rows").as_series().to_vec<std::int64_t>()[1];
```

### Output

```text
2 0
```

## Notes

- `profile()` is non-mutating.
- The current implementation caches the result until the workspace schema or table
  contents change.
- Automatic key health is computed only for the declared primary key, not for every
  candidate feature that looks interesting.
- `links` counts all adjacent relationships for the table, while `left_links` and
  `right_links` split that count by direction in the declared graph.

## See Also

- [`Nexus::summary`](summary.md): the smaller inventory-only view.
- [`Nexus::scan`](scan.md): include relationship rows and validation state in the
  same output frame.
- [`Nexus::deduce`](deduce.md): infer plausible relationships that have not been
  declared.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ntables.html">
    <span class="label">Previous</span>
    <span class="title">ntables</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
