# Atlas API Reference

# Practical Atlas API Reference

## API Reference Style

This page documents the full public `Atlas` surface for frame-stack workflows.

### `aggregate_over_frames`

#### Signature

```cpp
Atlas::aggregate_over_frames(fn: const std::function<DataFrame(const std::vector<DataFrame>&)>&) -> DataFrame
```

#### Notes

Reduces the full frame stack to one output by applying an aggregation function across the contained frames.

#### Inputs

`fn` : `const std::function<DataFrame(const std::vector<DataFrame>&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.aggregate_over_frames([](const std::vector<pluss::table::DataFrame>& frames) { return frames.front(); });
```

#### Expected output

```text
sales
10.0
20.0
```

#### Practical Behavior

- `aggregate_over_frames(...)` hands the full frame vector to your reducer and expects one output object back.
- Use it when the aggregation depends on multiple frames together rather than frame-by-frame mapping.


### `align_depth`

#### Signature

```cpp
Atlas::align_depth(labels: const Thread&) -> Atlas
```

#### Notes

Reorders frames so their depth labels match the supplied label order.

#### Inputs

`labels` : `const Thread&`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labels = pluss::table::Thread(pluss::make_note({"baseline", "stress"}));
auto out = atlas.align_depth(labels);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

#### Practical Behavior

- `align_depth(labels)` reorders frames to match the requested depth label order.
- Use it when two atlases should share the same depth layout before comparison or combination.


### `axis0_labels`

#### Signature

```cpp
Atlas::axis0_labels() -> const std::optional<Thread>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::optional<Thread>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.axis0_labels();
```

#### Expected output

```text
["train", "test"]
```

### `column_names`

#### Signature

```cpp
Atlas::column_names() -> const std::vector<std::string>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::vector<std::string>&`
    Borrowed list of names.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.column_names();
```

#### Expected output

```text
["sales"]
```

### `defined`

#### Signature

```cpp
Atlas::defined() -> bool
```

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.defined();
```

#### Expected output

```text
true
```

### `depth`

#### Signature

```cpp
Atlas::depth() -> std::int64_t
```

#### Notes

Returns a size/count summary for the current object without changing it.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.depth();
```

#### Expected output

```text
2
```

### `dloc`

#### Signature

```cpp
Atlas::dloc(index: std::int64_t) -> DataFrame
```

#### Notes



#### Inputs

`index` : `std::int64_t`
    Zero-based positional index used by this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.dloc(1);
```

#### Expected output

```text
sales
30.0
40.0
```

#### Practical Behavior

- `dloc(i)` is the positional depth-access alias for `frame(i)`.
- Use it when you want the depth-axis wording to be explicit.


### `empty`

#### Signature

```cpp
Atlas::empty() -> bool
```

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.empty();
```

#### Expected output

```text
false
```

### `flow`

#### Signature

```cpp
Atlas::flow(fn: const std::function<Atlas(const Atlas&)>&) -> Atlas
```

#### Notes



#### Inputs

`fn` : `const std::function<Atlas(const Atlas&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.flow();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `frame`

#### Signature

```cpp
Atlas::frame(index: std::int64_t) -> DataFrame
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`index` : `std::int64_t`
    Zero-based positional index used by this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.frame(0);
```

#### Expected output

```text
sales
10.0
20.0
```

### `frames`

#### Signature

```cpp
Atlas::frames() -> const std::vector<DataFrame>&
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`None`

#### Output

`const std::vector<DataFrame>&`
    Borrowed collection of frames.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.frames();
```

#### Expected output

```text
[frame0, frame1]
```

### `head`

#### Signature

```cpp
Atlas::head(n: std::int64_t = 5) -> Atlas
```

#### Notes

Returns the first `n` frames from the atlas depth axis.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.head(1);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `kindelize`

#### Signature

```cpp
Atlas::kindelize() -> pluss::kindle::Archive
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Archive`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::kindelize(policy: const pluss::kindle::KindelizePolicy&) -> pluss::kindle::Archive`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.kindelize();
```

#### Expected output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `kindelize`

#### Signature

```cpp
Atlas::kindelize(policy: const pluss::kindle::KindelizePolicy&) -> pluss::kindle::Archive
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`policy` : `const pluss::kindle::KindelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`pluss::kindle::Archive`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::kindelize() -> pluss::kindle::Archive`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.kindelize();
```

#### Expected output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `loc`

#### Signature

```cpp
Atlas::loc(label: const pluss::Timeline&) -> DataFrame
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`label` : `const pluss::Timeline&`
    Label or labels used by this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::loc(label: std::string_view) -> DataFrame`
- `Atlas::loc(label: std::int64_t) -> DataFrame`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

#### Expected output

```text
sales
10.0
20.0
```

### `loc`

#### Signature

```cpp
Atlas::loc(label: std::int64_t) -> DataFrame
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`label` : `std::int64_t`
    Integer label or bucket code used by this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::loc(label: std::string_view) -> DataFrame`
- `Atlas::loc(label: const pluss::Timeline&) -> DataFrame`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

#### Expected output

```text
sales
10.0
20.0
```

### `loc`

#### Signature

```cpp
Atlas::loc(label: std::string_view) -> DataFrame
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`label` : `std::string_view`
    Label or labels used by this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::loc(label: std::int64_t) -> DataFrame`
- `Atlas::loc(label: const pluss::Timeline&) -> DataFrame`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

#### Expected output

```text
sales
10.0
20.0
```

### `map_elements`

#### Signature

```cpp
Atlas::map_elements(fn: const std::function<ThreadValue(const ThreadValue&)>&) -> Atlas
```

#### Notes



#### Inputs

`fn` : `const std::function<ThreadValue(const ThreadValue&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map_elements();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `map_frames`

#### Signature

```cpp
Atlas::map_frames(fn: const std::function<DataFrame(const DataFrame&)>&) -> Atlas
```

#### Notes

Applies the same function to each frame and returns the transformed atlas.

#### Inputs

`fn` : `const std::function<DataFrame(const DataFrame&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map_frames([](const pluss::table::DataFrame& frame) { return frame.head(1, false); });
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `moving`

#### Signature

```cpp
Atlas::moving(window: std::int64_t, statistic: WindowStatistic = WindowStatistic::Mean, options: const WindowOptions& = {}) -> Atlas
```

#### Notes



#### Inputs

`window` : `std::int64_t`
    Positive fixed case count in each moving window.

`statistic` : `WindowStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`options` : `const WindowOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.moving();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ncases`

#### Signature

```cpp
Atlas::ncases() -> std::int64_t
```

#### Notes



#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.ncases();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `nfeats`

#### Signature

```cpp
Atlas::nfeats() -> std::int64_t
```

#### Notes



#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.nfeats();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `plot`

#### Signature

```cpp
Atlas::plot() -> AtlasPlotAccessor
```

#### Notes

This opens the plotting accessor for the current data object.

#### Inputs

`None`

#### Output

`AtlasPlotAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::plot(options: const AtlasPlotOptions&) -> AtlasPlotAccessor`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto fig = atlas.plot(0).line("sales");
```

#### Expected output

```text
Figure(line)
```

### `plot`

#### Signature

```cpp
Atlas::plot(options: const AtlasPlotOptions&) -> AtlasPlotAccessor
```

#### Notes

This opens the plotting accessor for the current data object.

#### Inputs

`options` : `const AtlasPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`AtlasPlotAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::plot() -> AtlasPlotAccessor`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto fig = atlas.plot(0).line("sales");
```

#### Expected output

```text
Figure(line)
```

### `relabel_depth`

#### Signature

```cpp
Atlas::relabel_depth(labels: const Thread&) -> Atlas
```

#### Notes

Replaces the depth-label axis with the supplied labels while keeping frame contents unchanged.

#### Inputs

`labels` : `const Thread&`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labels = pluss::table::Thread(pluss::make_note({"baseline", "stress"}));
auto out = atlas.relabel_depth(labels);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `show`

#### Signature

```cpp
Atlas::show(options: const display::DisplayOptions& = {}) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::show(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

atlas.show();
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `show`

#### Signature

```cpp
Atlas::show(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::show(options: const display::DisplayOptions& = {}) -> void`

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

atlas.show();
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `slice_frames`

#### Signature

```cpp
Atlas::slice_frames(start: std::int64_t, stop: std::int64_t) -> Atlas
```

#### Notes

Returns a contiguous depth slice `[start, stop)` from the atlas.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.slice_frames(0, 1);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `sort_depth`

#### Signature

```cpp
Atlas::sort_depth(ascending: bool = true, nulls_last: bool = true) -> Atlas
```

#### Notes

Sorts frames by their current depth labels.

#### Inputs

`ascending` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`nulls_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"test", "train"})));
auto out = labeled.sort_depth(true, true);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `squeeze`

#### Signature

```cpp
Atlas::squeeze() -> DataFrame
```

#### Notes

Collapses a depth-1 atlas back into a single `DataFrame`.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.squeeze();
```

#### Expected output

```text
sales
10.0
20.0
```

### `tabelize`

#### Signature

```cpp
Atlas::tabelize(archive: const pluss::kindle::Archive&) -> Atlas
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`archive` : `const pluss::kindle::Archive&`
    Archive payload supplied to this overload.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::tabelize(archive: const pluss::kindle::Archive&, policy: const pluss::kindle::TabelizePolicy&) -> Atlas`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `tabelize`

#### Signature

```cpp
Atlas::tabelize(archive: const pluss::kindle::Archive&, policy: const pluss::kindle::TabelizePolicy&) -> Atlas
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`archive` : `const pluss::kindle::Archive&`
    Archive payload supplied to this overload.

`policy` : `const pluss::kindle::TabelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Atlas::tabelize(archive: const pluss::kindle::Archive&) -> Atlas`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `tail`

#### Signature

```cpp
Atlas::tail(n: std::int64_t = 5) -> Atlas
```

#### Notes

Returns the last `n` frames from the atlas depth axis.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.tail(1);
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `to_string`

#### Signature

```cpp
Atlas::to_string(options: const display::DisplayOptions& = {}) -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.to_string();
```

#### Expected output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `unique`

#### Signature

```cpp
Atlas::unique() -> bool
```

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.unique();
```

#### Expected output

```text
true
```
