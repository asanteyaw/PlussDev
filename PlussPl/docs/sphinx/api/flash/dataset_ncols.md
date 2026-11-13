<!-- pluss-hand-authored -->

# flash::Dataset::ncols

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
std::int64_t flash::Dataset::ncols() const
```

## What It Does

Returns the logical feature count across all stored families in the dataset.

Unlike `nrows()`, `ncols()` does not care whether the dataset is ragged. It counts
features, not cases. If `feature_order` or `feature_kinds` are present, their sizes
must agree with the stored feature count.

## Returns

`std::int64_t`
    Number of logical features across numeric, text, and temporal families.

## Examples

Example 1. A mixed dataset counts every stored feature, not just numeric ones.

```cpp
using namespace pluss;
using namespace pluss::table;

auto frame = DataFrame({
    {"city", make_note({"Accra", "Tema", "Kumasi"})},
    {"score", make_series(std::vector<double>{91.5, 84.0, 88.0})},
    {"stamp", make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-02T00:00:00Z",
        "2026-01-03T00:00:00Z",
    })},
});

auto data = pluss::pl::flash::Dataset::from_table(frame);
std::cout << "ncols=" << data.ncols() << "\n";
```

Output:

```text
ncols=3
```

Example 2. Ragged datasets still have a well-defined feature count.

```cpp
using namespace pluss;

pluss::pl::flash::Dataset data;
data.feature_order = make_note({"time", "value", "label"});
data.feature_kinds = make_note({"temporal", "numeric", "text"});
data.temporal_names = make_note({"time"});
data.temporal_values.push_back(make_timeline({
    "2026-01-01T00:00:00Z",
    "2026-01-02T00:00:00Z",
}));
data.temporal_validity.push_back(std::nullopt);
data.numeric_names = make_note({"value"});
data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0, 30.0}));
data.numeric_validity.push_back(std::nullopt);
data.text_names = make_note({"label"});
data.text_values.push_back(make_note({"alpha", "beta"}));
data.text_validity.push_back(std::nullopt);

std::cout << "ncols=" << data.ncols() << "\n";
```

Output:

```text
ncols=3
```

## See Also

- [`flash::Dataset::nrows`](dataset_nrows.md)
- [`flash::Dataset::size`](dataset_size.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dataset_nrows.html">
    <span class="label">Previous</span>
    <span class="title">Dataset<br>nrows</span>
  </a>
  <a class="next-link" href="dataset_size.html">
    <span class="label">Next</span>
    <span class="title">Dataset<br>size</span>
  </a>
</div>
