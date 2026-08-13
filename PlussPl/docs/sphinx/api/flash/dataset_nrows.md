<!-- pluss-hand-authored -->

# flash::Dataset::nrows

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
std::int64_t flash::Dataset::nrows() const
```

## What It Does

Returns the shared row count of a rectangular flash dataset.

`nrows()` is deliberately strict. If one stored feature family is longer or shorter than
another, the dataset is ragged and `nrows()` throws instead of silently choosing a
minimum or maximum.

## Returns

`std::int64_t`
    Shared row count for rectangular datasets. Returns `0` for an empty dataset.

## Examples

Example 1. A rectangular dataset reports one shared row count.

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

auto data = pluss::pl::flash::Dataset::from_frame(frame);
std::cout << "nrows=" << data.nrows() << "\n";
```

Output:

```text
nrows=3
```

Example 2. Ragged datasets throw.

```cpp
using namespace pluss;

pluss::pl::flash::Dataset data;
data.feature_order = make_note({"time", "value"});
data.feature_kinds = make_note({"temporal", "numeric"});
data.temporal_names = make_note({"time"});
data.temporal_values.push_back(make_timeline({
    "2026-01-01T00:00:00Z",
    "2026-01-02T00:00:00Z",
}));
data.temporal_validity.push_back(std::nullopt);
data.numeric_names = make_note({"value"});
data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0, 30.0}));
data.numeric_validity.push_back(std::nullopt);

try {
  std::cout << data.nrows() << "\n";
} catch (const pluss::Error& err) {
  std::cout << err.what() << "\n";
}
```

Output:

```text
flash::Dataset::nrows: dataset is ragged; use frame({.policy = FramePolicy::PadNull}) to rectangularize
```

## See Also

- [`flash::Dataset::ncols`](dataset_ncols.md)
- [`flash::Dataset::size`](dataset_size.md)
- [`flash::Dataset::frame`](dataset_frame.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dataset_frame.html">
    <span class="label">Previous</span>
    <span class="title">Dataset<br>frame</span>
  </a>
  <a class="next-link" href="dataset_ncols.html">
    <span class="label">Next</span>
    <span class="title">Dataset<br>ncols</span>
  </a>
</div>
