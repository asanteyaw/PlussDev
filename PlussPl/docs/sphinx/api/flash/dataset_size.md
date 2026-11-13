<!-- pluss-hand-authored -->

# flash::Dataset::size

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
std::pair<std::int64_t, std::int64_t> flash::Dataset::size() const
```

## What It Does

Returns the rectangular dataset shape as `{nrows, ncols}`.

This is a shape-style method, not a flat element count. It intentionally mirrors the
row/column meaning users expect from table-like surfaces.

Because it delegates to `nrows()`, ragged datasets throw instead of being silently
coerced.

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `{nrows, ncols}` for rectangular datasets.

## Examples

Example 1. Rectangular datasets return `{rows, cols}`.

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
const auto [rows, cols] = data.size();
std::cout << "size=(" << rows << ", " << cols << ")\n";
```

Output:

```text
size=(3, 3)
```

Example 2. Ragged datasets throw for the same reason `nrows()` does.

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
  const auto [rows, cols] = data.size();
  std::cout << rows << "," << cols << "\n";
} catch (const pluss::Error& err) {
  std::cout << err.message() << "\n";
}
```

Output:

```text
flash::Dataset::nrows: dataset is ragged; use frame({.policy = FramePolicy::PadNull}) to rectangularize
```

## See Also

- [`flash::Dataset::nrows`](dataset_nrows.md)
- [`flash::Dataset::ncols`](dataset_ncols.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dataset_ncols.html">
    <span class="label">Previous</span>
    <span class="title">Dataset<br>ncols</span>
  </a>
  <a class="next-link" href="../../flash_api_reference.html">
    <span class="label">Next</span>
    <span class="title">flash API Reference</span>
  </a>
</div>
