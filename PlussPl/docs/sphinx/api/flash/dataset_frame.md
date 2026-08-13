<!-- pluss-hand-authored -->

# flash::Dataset::frame

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
pluss::table::DataFrame flash::Dataset::frame(
    const ToFrameOptions& options = {}) const
```

## What It Does

Rebuilds a rectangular PlussMic `DataFrame` from a flash `Dataset`.

This is the `flash::Dataset -> DataFrame` bridge. It takes the grouped numeric, text,
and temporal families stored in the dataset and reconstructs a tabular frame suitable
for the higher-level PlussMic ecosystem.

The method respects the dataset metadata:

- `feature_order` controls the output column order
- `feature_kinds` tells the bridge which family each feature belongs to
- `*_validity` masks become missing cells in the reconstructed frame

If `feature_order` and `feature_kinds` are not present, the bridge falls back to family
order: numeric first, then text, then temporal.

## Parameters

**options**
    Controls ragged handling and the output title.

    `ToFrameOptions` contains:

    - `policy`

      Chooses how ragged dataset families are handled.

      `FramePolicy::Strict`
      : Every feature must have the same length. If any feature length differs, the
        method throws immediately.

      `FramePolicy::PadNull`
      : Shorter features are extended to the maximum feature length and the padded rows
        are marked invalid. Numeric pads become missing numeric cells, text pads become
        missing text cells, and temporal pads become missing temporal cells.

    - `title`

      Optional output title for the reconstructed `DataFrame`. Leave it empty when you
      want the returned frame to keep the default unnamed state.

## Returns

`pluss::table::DataFrame`
    A rectangular frame rebuilt from the flash dataset.

## Notes

- This method belongs to the flash/DataFrame interop layer. In normal user code, use a
  unified Pluss include such as `<pluss/pluss.h>` so the PlussMic bridge definitions
  are present.
- Temporal features reconstruct as `Timeline`. The dataset does not preserve enough
  information to reconstitute a distinct `TimeIndex` surface automatically.
- `FramePolicy::PadNull` is opt-in. The default remains strict so shape mismatches are
  not silently hidden.
- When `feature_order` is present, it wins over family storage order.

## Examples

Example 1. Rebuild a rectangular frame and set an explicit output title.

```cpp
using namespace pluss;
using namespace pluss::table;

auto grades = DataFrame({
    {"city", make_note({"Accra", "Tema", "Kumasi"})},
    {"score", make_series(std::vector<double>{91.5, 84.0, 88.0})},
    {"stamp", make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-02T00:00:00Z",
        "2026-01-03T00:00:00Z",
    })},
}).title("grades");

auto data = pluss::pl::flash::Dataset::from_frame(grades);
auto rebuilt = data.frame({.title = "rebuilt"});
std::cout << rebuilt << "\n";
```

Output:

```text
================================
     | city    score  stamp     
--------------------------------
0    | Accra   91.5   2026-01-01
1    | Tema    84     2026-01-02
2    | Kumasi  88     2026-01-03
--------------------------------
[3 cs x 3 feat]
title=rebuilt
```

Example 2. Use `FramePolicy::PadNull` to rectangularize a ragged dataset and preserve
missingness in the padded rows.

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

auto padded = data.frame({.policy = pluss::pl::flash::FramePolicy::PadNull, .title = "padded"});
std::cout << padded << "\n";
```

Output:

```text
===========================================
     | time                    value  label
-------------------------------------------
0    | 2026-01-01 00:00:00.0~  10     alpha
1    | 2026-01-02 00:00:00.0~  20     beta 
2    | NULL                    30     NULL 
-------------------------------------------
[3 cs x 3 feat]
time_valid=1,1,0
label_valid=1,1,0
```

Example 3. `feature_order` controls the output column order even though numeric and text
families are stored separately.

```cpp
using namespace pluss;

pluss::pl::flash::Dataset data;
data.feature_order = make_note({"label", "value"});
data.feature_kinds = make_note({"text", "numeric"});
data.numeric_names = make_note({"value"});
data.numeric_values.push_back(make_series(std::vector<double>{10.0, 20.0}));
data.numeric_validity.push_back(std::nullopt);
data.text_names = make_note({"label"});
data.text_values.push_back(make_note({"alpha", "beta"}));
data.text_validity.push_back(std::nullopt);

auto rebuilt = data.frame();
std::cout << rebuilt << "\n";
```

Output:

```text
===================
     | label  value
-------------------
0    | alpha  10   
1    | beta   20   
-------------------
[2 cs x 2 feat]
features=label,value
```

Example 4. The default `FramePolicy::Strict` rejects ragged datasets immediately.

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
  (void)data.frame();
} catch (const pluss::Error& err) {
  std::cout << err.what() << "\n";
}
```

Output:

```text
error=flash::Dataset::frame: dataset is ragged; use FramePolicy::PadNull to rectangularize
```

## See Also

- [`flash::Dataset::from_frame`](dataset_from_frame.md): exports a `DataFrame` into a
  flash dataset.
- [`flash::Dataset::nrows`](dataset_nrows.md): reports the shared row count for
  rectangular datasets.
- [`flash::Dataset::size`](dataset_size.md): returns `{nrows, ncols}` for rectangular
  datasets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dataset_from_frame.html">
    <span class="label">Previous</span>
    <span class="title">Dataset<br>from_frame</span>
  </a>
  <a class="next-link" href="dataset_nrows.html">
    <span class="label">Next</span>
    <span class="title">Dataset<br>nrows</span>
  </a>
</div>
- [`flash::load_dataset`](load_dataset.md): loads a persisted flash dataset directly.
