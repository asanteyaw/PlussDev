<!-- pluss-hand-authored -->

# Nexus::summary

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::summary() const -> DataFrame
```

## What It Does

Returns the compact table inventory for the workspace.

`summary()` is intentionally small. It answers only three questions:

- what is each table called?
- how many rows does it have?
- how many features does it have?

That narrow scope is deliberate. If you need metadata completeness, declared-key
health, or relationship diagnostics, step up to [`profile()`](profile.md) or
[`scan()`](scan.md). `summary()` stays focused on table identity and size.

## Parameters

`None`

## Returns

`DataFrame`
    One row per table, with exactly these columns:

    - `table`
    - `ncases`
    - `nfeatures`

The result follows the current workspace order exactly.

## Examples

Example 1. `summary()` returns one compact row per table.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

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

auto summary = nx.summary();

std::cout << summary.ncases() << " " << summary.nfeatures() << "\n";
for (std::int64_t i = 0; i < summary.ncases(); ++i) {
  auto idx = static_cast<std::size_t>(i);
  std::cout << summary.features("table").as_note().at(idx) << " "
            << summary.features("ncases").as_series().to_vec<std::int64_t>()[idx] << " "
            << summary.features("nfeatures").as_series().to_vec<std::int64_t>()[idx] << "\n";
}
```

### Output

```text
3 3
loan 3 3
account 3 3
trans 3 3
```

Example 2. `summary()` follows the current workspace order rather than sorting.

That matters after projection or reordering operations. Here the workspace is
reduced to two tables in a deliberate `trans`, then `loan` order, and `summary()`
reports them in that same order.

```cpp
auto reordered = nx.select({"trans", "loan"}).summary();

for (std::int64_t i = 0; i < reordered.ncases(); ++i) {
  if (i > 0) {
    std::cout << " ";
  }
  std::cout << reordered.features("table").as_note().at(static_cast<std::size_t>(i));
}
```

### Output

```text
trans loan
```

Example 3. `summary()` intentionally omits audit and relationship columns.

This is the cleanest way to see the method's boundary. Metadata-readiness and
relationship-confidence fields are not part of the contract here.

```cpp
std::cout << std::boolalpha
          << summary.get("meta_ready").has_value() << " "
          << summary.get("confidence").has_value();
```

### Output

```text
false false
```

## Notes

- `summary()` is non-mutating.
- The column names are `ncases` / `nfeatures`, not `rows` / `cols` as used by
  [`profile()`](profile.md) and [`scan()`](scan.md).
- `summary()` does not inspect or report relationship health, metadata readiness,
  or key validity.

## See Also

- [`Nexus::profile`](profile.md): richer per-table diagnostics.
- [`Nexus::scan`](scan.md): table and relationship diagnostics in one frame.
- [`Nexus::ntables`](ntables.md): just the table count when even `summary()` is
  more detail than you need.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="store.html">
    <span class="label">Previous</span>
    <span class="title">store</span>
  </a>
  <a class="next-link" href="table_meta.html">
    <span class="label">Next</span>
    <span class="title">table_meta</span>
  </a>
</div>
