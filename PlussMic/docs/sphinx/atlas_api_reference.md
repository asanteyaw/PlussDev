# Atlas API Reference

# Practical Atlas API Reference

## API Reference Style

This page documents the full public `Atlas` surface for frame-stack workflows.

### `aggregate_over_frames`

#### Signature

```cpp
DataFrame aggregate_over_frames(self, const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) const
```

#### Parameters

`fn` : `const std::function<DataFrame(const std::vector<DataFrame>&)>&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reduces the full frame stack to one output by applying an aggregation function across the contained frames.

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

#### Output

```text
sales
10.0
20.0
```

### `align_depth`

#### Signature

```cpp
Atlas align_depth(self, const Thread& labels) const
```

#### Parameters

`labels` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Reorders frames so their depth labels match the supplied label order.

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

#### Output

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

### `axis0_labels`

#### Signature

```cpp
const std::optional<Thread>& axis0_labels(self) const
```

#### Parameters

`None`

#### Returns

`const std::optional<Thread>&`
    Value produced by this call.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

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

#### Output

```text
["train", "test"]
```

### `column_names`

#### Signature

```cpp
const std::vector<std::string>& column_names(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

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

#### Output

```text
["sales"]
```

### `defined`

#### Signature

```cpp
bool defined(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

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

#### Output

```text
true
```

### `depth`

#### Signature

```cpp
std::int64_t depth(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Returns a size/count summary for the current object without changing it.

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

#### Output

```text
2
```

### `empty`

#### Signature

```cpp
bool empty(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

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

#### Output

```text
false
```

### `frame`

#### Signature

```cpp
DataFrame frame(self, std::int64_t index) const
```

#### Parameters

`index` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

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

#### Output

```text
sales
10.0
20.0
```

### `frames`

#### Signature

```cpp
const std::vector<DataFrame>& frames(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<DataFrame>&`
    Borrowed collection of frames.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

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

#### Output

```text
[frame0, frame1]
```

### `head`

#### Signature

```cpp
Atlas head(self, std::int64_t n = 5) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Returns the first `n` frames from the atlas depth axis.

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

#### Output

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

### `iloc`

#### Signature

```cpp
DataFrame iloc(self, std::int64_t index) const
```

#### Parameters

`index` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.iloc(0);
```

#### Output

```text
sales
10.0
20.0
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Archive kindelize(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `pluss::kindle::Archive kindelize(self, const pluss::kindle::KindelizePolicy& policy) const`

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

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Archive kindelize(self, const pluss::kindle::KindelizePolicy& policy) const
```

#### Parameters

`policy` : `const pluss::kindle::KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `pluss::kindle::Archive kindelize(self) const`

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

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `loc`

#### Signature

```cpp
DataFrame loc(self, const pluss::Timeline& label) const
```

#### Parameters

`label` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame loc(self, std::string_view label) const`
- `DataFrame loc(self, std::int64_t label) const`

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

#### Output

```text
sales
10.0
20.0
```

### `loc`

#### Signature

```cpp
DataFrame loc(self, std::int64_t label) const
```

#### Parameters

`label` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame loc(self, std::string_view label) const`
- `DataFrame loc(self, const pluss::Timeline& label) const`

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

#### Output

```text
sales
10.0
20.0
```

### `loc`

#### Signature

```cpp
DataFrame loc(self, std::string_view label) const
```

#### Parameters

`label` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame loc(self, std::int64_t label) const`
- `DataFrame loc(self, const pluss::Timeline& label) const`

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

#### Output

```text
sales
10.0
20.0
```

### `map_frames`

#### Signature

```cpp
Atlas map_frames(self, const std::function<DataFrame(const DataFrame&)>& fn) const
```

#### Parameters

`fn` : `const std::function<DataFrame(const DataFrame&)>&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Applies the same function to each frame and returns the transformed atlas.

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

#### Output

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

### `ncases`

#### Signature

```cpp
std::int64_t ncases(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Applies `ncases` to the current `Atlas` object. Use the example and output together to see which part of the object changes and which structure is preserved.

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

### `nfeats`

#### Signature

```cpp
std::int64_t nfeats(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Applies `nfeats` to the current `Atlas` object. Use the example and output together to see which part of the object changes and which structure is preserved.

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

### `plot`

#### Signature

```cpp
AtlasPlotAccessor plot(self) const
```

#### Parameters

`None`

#### Returns

`AtlasPlotAccessor`
    Value produced by this call.

#### Notes

This opens the plotting accessor for the current data object.

#### See also

- `AtlasPlotAccessor plot(self, const AtlasPlotOptions& options) const`

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

#### Output

```text
Figure(line)
```

### `plot`

#### Signature

```cpp
AtlasPlotAccessor plot(self, const AtlasPlotOptions& options) const
```

#### Parameters

`options` : `const AtlasPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`AtlasPlotAccessor`
    Value produced by this call.

#### Notes

This opens the plotting accessor for the current data object.

#### See also

- `AtlasPlotAccessor plot(self) const`

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

#### Output

```text
Figure(line)
```

### `relabel_depth`

#### Signature

```cpp
Atlas relabel_depth(self, const Thread& labels) const
```

#### Parameters

`labels` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Replaces the depth-label axis with the supplied labels while keeping frame contents unchanged.

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

#### Output

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
void show(self, const display::DisplayOptions& options =
```

#### Parameters

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void show(self, std::ostream& os, const display::DisplayOptions& options =`

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

#### Output

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
void show(self, std::ostream& os, const display::DisplayOptions& options =
```

#### Parameters

`os` : `std::ostream&`
    See the signature for the supported argument form.

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void show(self, const display::DisplayOptions& options =`

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

#### Output

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
Atlas slice_frames(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Returns a contiguous depth slice `[start, stop)` from the atlas.

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

#### Output

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
Atlas sort_depth(self, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Sorts frames by their current depth labels.

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

#### Output

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
DataFrame squeeze(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Collapses a depth-1 atlas back into a single `DataFrame`.

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

#### Output

```text
sales
10.0
20.0
```

### `tabelize`

#### Signature

```cpp
static Atlas tabelize(const pluss::kindle::Archive& archive)
```

#### Parameters

`archive` : `const pluss::kindle::Archive&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `static Atlas tabelize(const pluss::kindle::Archive& archive, const pluss::kindle::TabelizePolicy& policy)`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

#### Output

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
static Atlas tabelize(const pluss::kindle::Archive& archive, const pluss::kindle::TabelizePolicy& policy)
```

#### Parameters

`archive` : `const pluss::kindle::Archive&`
    See the signature for the supported argument form.

`policy` : `const pluss::kindle::TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `static Atlas tabelize(const pluss::kindle::Archive& archive)`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

#### Output

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
Atlas tail(self, std::int64_t n = 5) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Returns the last `n` frames from the atlas depth axis.

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

#### Output

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
std::string to_string(self, const display::DisplayOptions& options =
```

#### Parameters

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

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

#### Output

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
bool unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

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

#### Output

```text
true
```
