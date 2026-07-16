# Atlas API Reference

This page documents the full public `Atlas` surface for frame-stack workflows.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `aggregate_over_frames`

#### Signature

```cpp
Atlas::aggregate_over_frames(const std::function<DataFrame(const std::vector<DataFrame>&)>& fn) -> DataFrame
```

#### What It Does

Reduces the full frame stack to one output by applying an aggregation function across the contained frames.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
10.0
20.0
```

#### Practical Behavior

- `aggregate_over_frames(...)` hands the full frame vector to your reducer and expects one output object back.
- Use it when the aggregation depends on multiple frames together rather than frame-by-frame mapping.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#align_depth">
    <span class="label">Next</span>
    <span class="title">align_depth</span>
  </a>
</div>

### `align_depth`

#### Signature

```cpp
Atlas::align_depth(const Thread& labels) -> Atlas
```

#### What It Does

Reorders frames so their depth labels match the supplied label order.

#### Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

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

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#aggregate_over_frames">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_frames</span>
  </a>
  <a class="next-link" href="#axis0_labels">
    <span class="label">Next</span>
    <span class="title">axis0_labels</span>
  </a>
</div>

### `axis0_labels`

#### Signature

```cpp
Atlas::axis0_labels() -> const std::optional<Thread>&
```

#### What It Does

Returns the current labels used to address fields, slots, rows, or depth items.

#### Parameters

`None`

#### Returns

`const std::optional<Thread>&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
["train", "test"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#align_depth">
    <span class="label">Previous</span>
    <span class="title">align_depth</span>
  </a>
  <a class="next-link" href="#column_names">
    <span class="label">Next</span>
    <span class="title">column_names</span>
  </a>
</div>

## C

### `column_names`

#### Signature

```cpp
Atlas::column_names() -> const std::vector<std::string>&
```

#### What It Does

Returns the current labels used to address fields, slots, rows, or depth items.

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Examples

Example 1. Pass several names together from a standard string container.

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

##### Output

```text
["sales"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#axis0_labels">
    <span class="label">Previous</span>
    <span class="title">axis0_labels</span>
  </a>
  <a class="next-link" href="#defined">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>

## D

### `defined`

#### Signature

```cpp
Atlas::defined() -> bool
```

#### What It Does

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Inspect the `defined` property of the current object.

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

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#column_names">
    <span class="label">Previous</span>
    <span class="title">column_names</span>
  </a>
  <a class="next-link" href="#depth">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>

### `depth`

#### Signature

```cpp
Atlas::depth() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Inspect the `depth` property of the current object.

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

##### Output

```text
2
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="#dloc">
    <span class="label">Next</span>
    <span class="title">dloc</span>
  </a>
</div>

### `dloc`

#### Signature

```cpp
Atlas::dloc(std::int64_t index) -> DataFrame
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**index**
    Zero-based positional index used by this overload.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
30.0
40.0
```

#### Practical Behavior

- `dloc(i)` is the positional depth-access alias for `frame(i)`.
- Use it when you want the depth-axis wording to be explicit.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#depth">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="#empty">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>

## E

### `empty`

#### Signature

```cpp
Atlas::empty() -> bool
```

#### What It Does

Reports whether the current object has no rows, entries, slots, or depth items.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Inspect the `empty` property of the current object.

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

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#dloc">
    <span class="label">Previous</span>
    <span class="title">dloc</span>
  </a>
  <a class="next-link" href="#flow">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>

## F

### `flow`

#### Signature

```cpp
Atlas::flow(const std::function<Atlas(const Atlas&)>& fn) -> Atlas
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#empty">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="#frame">
    <span class="label">Next</span>
    <span class="title">frame</span>
  </a>
</div>

### `frame`

#### Signature

```cpp
Atlas::frame(std::int64_t index) -> DataFrame
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**index**
    Zero-based positional index used by this overload.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
10.0
20.0
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#flow">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="#frames">
    <span class="label">Next</span>
    <span class="title">frames</span>
  </a>
</div>

### `frames`

#### Signature

```cpp
Atlas::frames() -> const std::vector<DataFrame>&
```

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Parameters

`None`

#### Returns

`const std::vector<DataFrame>&`
    Borrowed collection of frames.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
[frame0, frame1]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#frame">
    <span class="label">Previous</span>
    <span class="title">frame</span>
  </a>
  <a class="next-link" href="#head">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

## H

### `head`

#### Signature

```cpp
Atlas::head(std::int64_t n = 5) -> Atlas
```

#### What It Does

Returns the first `n` frames from the atlas depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#frames">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="#kindelize">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>

## K

### `kindelize`

#### Signatures

```cpp
Atlas::kindelize() -> pluss::kindle::Archive

Atlas::kindelize(const pluss::kindle::KindelizePolicy& policy) -> pluss::kindle::Archive
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#loc">
    <span class="label">Next</span>
    <span class="title">loc</span>
  </a>
</div>

## L

### `loc`

#### Signatures

```cpp
Atlas::loc(const pluss::Timeline& label) -> DataFrame

Atlas::loc(std::int64_t label) -> DataFrame

Atlas::loc(std::string_view label) -> DataFrame
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**label**
    Label or labels used by this overload.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
10.0
20.0
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
10.0
20.0
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

```text
sales
10.0
20.0
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#map">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>

## M

### `map`

#### Signature

```cpp
Atlas::map(const std::function<ThreadValue(const ThreadValue&)>& fn) -> Atlas
```

#### What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.map();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#loc">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="#map_frames">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>

### `map_frames`

#### Signature

```cpp
Atlas::map_frames(const std::function<DataFrame(const DataFrame&)>& fn) -> Atlas
```

#### What It Does

Applies the same function to each frame and returns the transformed atlas.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="#ncases">
    <span class="label">Next</span>
    <span class="title">ncases</span>
  </a>
</div>

## N

### `ncases`

#### Signature

```cpp
Atlas::ncases() -> std::int64_t
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map_frames">
    <span class="label">Previous</span>
    <span class="title">map_frames</span>
  </a>
  <a class="next-link" href="#nfeatures">
    <span class="label">Next</span>
    <span class="title">nfeatures</span>
  </a>
</div>

### `nfeatures`

#### Signature

```cpp
Atlas::nfeatures() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.nfeatures();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ncases">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="#plot">
    <span class="label">Next</span>
    <span class="title">plot</span>
  </a>
</div>

## P

### `plot`

#### Signatures

```cpp
Atlas::plot() -> AtlasPlotAccessor

Atlas::plot(const AtlasPlotOptions& options) -> AtlasPlotAccessor
```

#### What It Does

This opens the plotting accessor for the current data object.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`AtlasPlotAccessor`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
Figure(line)
```
Example 2. Pass an explicit options object to control the operation.

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

##### Output

```text
Figure(line)
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nfeatures">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="#relabel_depth">
    <span class="label">Next</span>
    <span class="title">relabel_depth</span>
  </a>
</div>

## R

### `relabel_depth`

#### Signature

```cpp
Atlas::relabel_depth(const Thread& labels) -> Atlas
```

#### What It Does

Replaces the depth-label axis with the supplied labels while keeping frame contents unchanged.

#### Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#plot">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

## S

### `show`

#### Signatures

```cpp
Atlas::show(const display::DisplayOptions& options = {}) -> void

Atlas::show(std::ostream& os, const display::DisplayOptions& options = {}) -> void
```

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**os**
    Destination output stream used instead of the default display or print target.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Pass an explicit options object to control the operation.

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

##### Output

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
Example 2. Write the output to a caller-provided stream.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#relabel_depth">
    <span class="label">Previous</span>
    <span class="title">relabel_depth</span>
  </a>
  <a class="next-link" href="#slice_frames">
    <span class="label">Next</span>
    <span class="title">slice_frames</span>
  </a>
</div>

### `slice_frames`

#### Signature

```cpp
Atlas::slice_frames(std::int64_t start, std::int64_t stop) -> Atlas
```

#### What It Does

Returns a contiguous depth slice `[start, stop)` from the atlas.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="#sort_depth">
    <span class="label">Next</span>
    <span class="title">sort_depth</span>
  </a>
</div>

### `sort_depth`

#### Signature

```cpp
Atlas::sort_depth(bool ascending = true, bool nulls_last = true) -> Atlas
```

#### What It Does

Sorts frames by their current depth labels.

#### Parameters

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**nulls_last**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice_frames">
    <span class="label">Previous</span>
    <span class="title">slice_frames</span>
  </a>
  <a class="next-link" href="#squeeze">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>

### `squeeze`

#### Signature

```cpp
Atlas::squeeze() -> DataFrame
```

#### What It Does

Collapses a depth-1 atlas back into a single `DataFrame`.

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
sales
10.0
20.0
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sort_depth">
    <span class="label">Previous</span>
    <span class="title">sort_depth</span>
  </a>
  <a class="next-link" href="#tabelize">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>

## T

### `tabelize`

#### Signatures

```cpp
Atlas::tabelize(const pluss::kindle::Archive& archive) -> Atlas

Atlas::tabelize(const pluss::kindle::Archive& archive, const pluss::kindle::TabelizePolicy& policy) -> Atlas
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**archive**
    Archive payload supplied to this overload.

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

##### Output

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
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#squeeze">
    <span class="label">Previous</span>
    <span class="title">squeeze</span>
  </a>
  <a class="next-link" href="#tail">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>

### `tail`

#### Signature

```cpp
Atlas::tail(std::int64_t n = 5) -> Atlas
```

#### What It Does

Returns the last `n` frames from the atlas depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Atlas`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tabelize">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="#tl">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>

### `tl`

#### Signatures

```cpp
Atlas::tl(pluss::RangeView<std::string_view> features = {}) -> AtlasTimelineAccessor

Atlas::tl(std::initializer_list<std::string_view> features) -> AtlasTimelineAccessor

Atlas::tl(std::string_view feature) -> AtlasTimelineAccessor
```

#### What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**feature**
    Name of the source feature used by the operation.

#### Returns

`AtlasTimelineAccessor`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.tl();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.tl();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Work with one named feature.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.tl();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#to_string">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>

### `to_string`

#### Signature

```cpp
Atlas::to_string(const display::DisplayOptions& options = {}) -> std::string
```

#### What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Pass an explicit options object to control the operation.

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

##### Output

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
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tl">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="#ts">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>

### `ts`

#### Signatures

```cpp
Atlas::ts(pluss::RangeView<std::string_view> features = {}) -> AtlasTsAccessor

Atlas::ts(std::initializer_list<std::string_view> features) -> AtlasTsAccessor

Atlas::ts(std::string_view feature) -> AtlasTsAccessor
```

#### What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**feature**
    Name of the source feature used by the operation.

#### Returns

`AtlasTsAccessor`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.ts();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.ts();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Work with one named feature.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.ts();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_string">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="#unique">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>

## U

### `unique`

#### Signature

```cpp
Atlas::unique() -> bool
```

#### What It Does

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Call the simplest overload.

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

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ts">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
</div>
