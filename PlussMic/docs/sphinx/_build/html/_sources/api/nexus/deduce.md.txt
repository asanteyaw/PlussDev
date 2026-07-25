<!-- pluss-hand-authored -->

# Nexus::deduce

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::deduce(double min_confidence = 0.5) const -> DataFrame
```

## What It Does

Scans the workspace for plausible relationship candidates and returns them as an
advisory `DataFrame`.

`deduce()` does not create relationships, mutate the workspace, or validate that
the suggested joins are semantically correct for your domain. Its job is narrower:
surface "these two tables share a feature name that looks structurally joinable"
signals so you can inspect them before you declare anything with the relationship
APIs.

The current implementation scores candidates from four pieces of evidence:

- shared feature name: base score `0.35`
- left feature is unique: `+0.20`
- right feature is unique: `+0.20`
- left feature is declared as a single-column key: `+0.125`
- right feature is declared as a single-column key: `+0.125`

The result is capped at `1.0`. Composite key declarations do **not** count toward
the declared-key bonus here; the implementation only treats exact one-column key
declarations as evidence for `deduce()`.

```{important} deduce() is name-and-key-structure driven
In the current implementation, `deduce()` does not compare actual value overlap
between the two columns. It reasons from shared feature names, uniqueness, and
declared one-column keys.
```

## Parameters

**min_confidence** (`double`, default `0.5`)

Minimum score a candidate row must reach before it is included in the output.

- Must be in the closed interval `[0.0, 1.0]`.
- Lower thresholds admit weaker suggestions.
- Higher thresholds filter the result down to only the strongest structural
  matches.
- Values outside `[0.0, 1.0]` throw immediately.

## Returns

`DataFrame`
    One row per surviving candidate relationship.

    The output frame has these columns:

    - `name`: a stable suggestion label in the shape `left_right_feature`
    - `left`, `right`: the participating table names
    - `left_key`, `right_key`: the shared feature name on each side
    - `cardinality`: the inferred cardinality from the two uniqueness flags
    - `left_unique`, `right_unique`: whether each side is unique by that feature
    - `left_declared`, `right_declared`: whether each side declares that feature
      as a one-column primary or candidate key
    - `confidence`: the computed score
    - `basis`: a compact explanation string showing which evidence contributed

## Examples

Example 1. Default-threshold deduction returns every candidate at or above `0.5`.

The fixture below is intentionally small but slightly noisy: `loan` and `trans`
share both `account_id` and `amount`, so `deduce()` emits two separate suggestions
for that pair. This is an important behavior to understand. The result is not
"one row per table pair"; it is "one row per plausible shared feature".

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

auto deduced = nx.deduce();

std::cout << deduced.ncases() << " " << deduced.nfeatures() << "\n";
for (std::int64_t i = 0; i < deduced.ncases(); ++i) {
  auto idx = static_cast<std::size_t>(i);
  std::cout << deduced.features("name").as_note().at(idx) << " "
            << deduced.features("left").as_note().at(idx) << " "
            << deduced.features("right").as_note().at(idx) << " "
            << deduced.features("left_key").as_note().at(idx) << " "
            << deduced.features("confidence").as_series().to_vec<double>()[idx] << " "
            << deduced.features("basis").as_note().at(idx) << "\n";
}
```

### Output

```text
4 12
loan_account_account_id loan account account_id 0.875 shared_name+left_unique+right_unique+right_declared
loan_trans_account_id loan trans account_id 0.55 shared_name+left_unique
loan_trans_amount loan trans amount 0.75 shared_name+left_unique+right_unique
account_trans_account_id account trans account_id 0.675 shared_name+left_unique+left_declared
```

Example 2. Raising `min_confidence` filters out weaker structural guesses.

At `0.9`, the same fixture yields no candidates at all. That does not mean the
workspace is unjoinable; it only means none of the suggestions reached that
stricter score cutoff.

```cpp
std::cout << nx.deduce(0.9).ncases();
```

### Output

```text
0
```

Example 3. Threshold validation is strict.

```cpp
try {
  (void)nx.deduce(-0.1);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  (void)nx.deduce(1.1);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::deduce: min_confidence must be in [0, 1]
Nexus::deduce: min_confidence must be in [0, 1]
```

## Notes

- `deduce()` is non-mutating and purely advisory.
- One table pair can produce multiple rows when several shared feature names look
  structurally plausible.
- The `name` column is feature-specific, so those multi-row pair suggestions stay
  distinguishable.
- Only the default call path `deduce()` / `deduce(0.5)` is cached in the current
  implementation. Non-default thresholds recompute.
- Existing declared relationships do not suppress suggestions. `deduce()` is about
  plausible structure, not "only relationships not already present".

## See Also

- [`Nexus::scan`](scan.md): inspect declared relationships and their computed
  validation status.
- [`Nexus::profile`](profile.md): inspect per-table declared-key health.
- [`Nexus::summary`](summary.md): get the compact table inventory without any
  relationship inference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_all.html">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
