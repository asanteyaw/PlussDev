<!-- pluss-hand-authored -->

# flash::Dataset

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
flash::Dataset
```

## What It Is

`flash::Dataset` is flash's grouped, name-aware container for mixed plotting features.

Unlike a single numeric matrix, `Dataset` can preserve three different feature families
side by side:

- numeric payloads
- text payloads
- temporal payloads

That makes it the common hand-off type across:

- persisted bundled datasets from [`flash::load_dataset`](load_dataset.md)
- DataFrame export via [`flash::Dataset::from_table`](dataset_from_table.md)
- dataset-aware plot overloads such as [`flash::pairplot`](pairplot.md) and
  [`flash::jointplot`](jointplot.md)

## Stored Attributes

- `feature_order`
  Optional logical feature order across every stored family. When present, this is the
  public feature order flash should preserve.
- `feature_kinds`
  Optional per-feature kind labels. Typical values are `"numeric"`, `"text"`, and
  `"temporal"`.
- `numeric_names`, `numeric_values`, `numeric_validity`
  The numeric family: names, payloads, and optional validity masks.
- `text_names`, `text_values`, `text_validity`
  The text family: names, payloads, and optional validity masks.
- `temporal_names`, `temporal_values`, `temporal_validity`
  The temporal family: names, payloads, and optional validity masks.

## Named Accessors

- `numeric(name)`
  Returns one numeric feature by name when it exists.
- `text(name)`
  Returns one text feature by name when it exists.
- `temporal(name)`
  Returns one temporal feature by name when it exists.
- `numeric_validity_mask(name)`, `text_validity_mask(name)`, `temporal_validity_mask(name)`
  Return the stored validity mask for one named feature when a mask exists.
- `continuous(name)`
  Returns a numeric plotting view of a numeric or temporal feature. Temporal values are
  converted into numeric axis coordinates; invalid rows become `NaN`.

## Dimension Surface

- [`flash::Dataset::nrows`](dataset_nrows.md)
  Shared row count for rectangular datasets. Ragged datasets throw.
- [`flash::Dataset::ncols`](dataset_ncols.md)
  Logical feature count across all stored families.
- [`flash::Dataset::size`](dataset_size.md)
  Returns `{nrows, ncols}` for rectangular datasets.

## Bridge Surface

- [`flash::Dataset::from_table`](dataset_from_table.md)
  Export a PlussMic `DataFrame` into flash's grouped dataset form.
- [`flash::Dataset::frame`](dataset_frame.md)
  Rebuild a rectangular PlussMic `DataFrame` from a flash dataset.

## Examples

Example 1. Load one bundled dataset and inspect the grouped family counts.

```cpp
using namespace pluss::pl;

auto tips = flash::load_dataset("tips");
if (tips) {
  std::cout << "ncols=" << tips->ncols() << "\n";
  std::cout << "numeric families=" << tips->numeric_values.size() << "\n";
  std::cout << "text families=" << tips->text_values.size() << "\n";
  std::cout << "temporal families=" << tips->temporal_values.size() << "\n";
}
```

Output:

```text
ncols=9
numeric families=5
text families=4
temporal families=0
```

Example 2. The bundled `tips` dataset keeps its numeric and text columns in separate families.

```cpp
const auto tips = *flash::load_dataset("tips");
std::cout << "numeric columns " << tips.numeric_names->numel() << ", text columns " << tips.text_names->numel() << ", rows " << tips.nrows() << "\n";
```

Output:

```text
numeric columns 5, text columns 4, rows 240
```

Example 3. Lookup is by family: a text column is not found as numeric, and a missing name is an empty result.

```cpp
const auto tips = *flash::load_dataset("tips");
std::cout << "numeric(\"tip\") found: " << tips.numeric("tip").has_value() << ", numeric(\"day\") found: " << tips.numeric("day").has_value()
          << ", text(\"day\") found: " << tips.text("day").has_value() << ", numeric(\"nothing\") found: " << tips.numeric("nothing").has_value() << "\n";
```

Output:

```text
numeric("tip") found: 1, numeric("day") found: 0, text("day") found: 1, numeric("nothing") found: 0
```

Example 4. Wrapping a table of your own.

```cpp
auto frame = pluss::table::DataFrame({{"a", pluss::make_series({1.0, 2.0})}, {"b", pluss::make_series({3.0, 4.0})}});
const auto wrapped = flash::Dataset::from_table(frame);
std::cout << "wrapped table: " << wrapped.nrows() << " rows, " << wrapped.ncols() << " columns, numeric names " << wrapped.numeric_names->numel() << "\n";
```

Output:

```text
wrapped table: 2 rows, 2 columns, numeric names 2
```

## Notes

- `Dataset` is allowed to be ragged. That is why `nrows()` and `size()` are strict
  rather than silently padding.
- The temporal family stores `Timeline`, not `TimeIndex`.
- `Dataset` is intentionally higher-level than a raw numeric matrix but lower-level than
  PlussMic `DataFrame`.

## See Also

- [`flash::load_dataset`](load_dataset.md)
- [`flash::Dataset::from_table`](dataset_from_table.md)
- [`flash::Dataset::frame`](dataset_frame.md)
- [`flash::pairplot`](pairplot.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_dataset.html">
    <span class="label">Previous</span>
    <span class="title">Bundled Datasets<br>load_dataset</span>
  </a>
  <a class="next-link" href="dataset_from_table.html">
    <span class="label">Next</span>
    <span class="title">Dataset<br>from_table</span>
  </a>
</div>
