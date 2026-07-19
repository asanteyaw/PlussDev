# Archive API Reference

This page documents the full public `Archive` surface for depth-stacked catalogs.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `aggregate_over_catalogs`

#### Signature

```cpp
Archive::aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) -> Catalog
```

#### What It Does

Reduces the full catalog stack to one output by applying an aggregation function across the contained catalogs.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.aggregate_over_catalogs([](const std::vector<pluss::kindle::Catalog>& catalogs) { return catalogs.front(); });
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `aggregate_over_catalogs(...)` hands the full catalog vector to your reducer and expects one output object back.
- Use it when the aggregation depends on multiple catalogs together rather than catalog-by-catalog mapping.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#align_depth">
    <span class="label">Next</span>
    <span class="title">align_depth</span>
  </a>
</div>

### `align_depth`

#### Signature

```cpp
Archive::align_depth(const Stream& labels) -> Archive
```

#### What It Does

Reorders catalogs so their depth labels match the supplied label order.

#### Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto labels = pluss::kindle::Stream(pluss::make_note({"baseline", "stress"}));
auto out = archive.align_depth(labels);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

#### Practical Behavior

- `align_depth(labels)` reorders catalogs to match the requested depth label order.
- Use it when archive depth order must match another archive or reporting layout.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#aggregate_over_catalogs">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_catalogs</span>
  </a>
  <a class="next-link" href="#astype">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>

### `astype`

#### Signature

```cpp
Archive::astype(std::initializer_list<std::string_view> keys, pluss::ScalarType dtype) -> Archive
```

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**dtype**
    Target scalar dtype used by this conversion overload.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.astype();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#align_depth">
    <span class="label">Previous</span>
    <span class="title">align_depth</span>
  </a>
  <a class="next-link" href="#bfill">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>

## B

### `bfill`

#### Signature

```cpp
Archive::bfill(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.bfill();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#astype">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="#cast">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>

## C

### `cast`

#### Signature

```cpp
Archive::cast(std::initializer_list<std::string_view> keys, pluss::table::ThreadCastTarget target, const pluss::table::ThreadCastOptions& options = {}) -> Archive
```

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**target**
    Target payload family chosen by this cast overload.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.cast();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#bfill">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="#catalog">
    <span class="label">Next</span>
    <span class="title">catalog</span>
  </a>
</div>

### `catalog`

#### Signature

```cpp
Archive::catalog(std::int64_t i) -> Catalog
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**i**
    Zero-based positional index used by this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.catalog(0);
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#cast">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="#catalogs">
    <span class="label">Next</span>
    <span class="title">catalogs</span>
  </a>
</div>

### `catalogs`

#### Signature

```cpp
Archive::catalogs() -> const std::vector<Catalog>&
```

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Parameters

`None`

#### Returns

`const std::vector<Catalog>&`
    Borrowed collection of catalogs.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.catalogs();
```

##### Output

```text
[Catalog(train), Catalog(test)]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#catalog">
    <span class="label">Previous</span>
    <span class="title">catalog</span>
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
Archive::defined() -> bool
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#catalogs">
    <span class="label">Previous</span>
    <span class="title">catalogs</span>
  </a>
  <a class="next-link" href="#depth">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>

### `depth`

#### Signature

```cpp
Archive::depth() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.depth();
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
  <a class="next-link" href="#depth_labels">
    <span class="label">Next</span>
    <span class="title">depth_labels</span>
  </a>
</div>

### `depth_labels`

#### Signature

```cpp
Archive::depth_labels() -> const std::optional<Stream>&
```

#### What It Does

Returns the current labels used to address fields, slots, rows, or depth items.

#### Parameters

`None`

#### Returns

`const std::optional<Stream>&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.depth_labels();
```

##### Output

```text
["train", "test"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#depth">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="#distinct">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>

### `distinct`

#### Signatures

```cpp
Archive::distinct(DistinctOptions options = {}) -> Archive

Archive::distinct(std::initializer_list<std::string_view> keys, DistinctOptions options = {}) -> Archive
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.distinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.distinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#depth_labels">
    <span class="label">Previous</span>
    <span class="title">depth_labels</span>
  </a>
  <a class="next-link" href="#dloc">
    <span class="label">Next</span>
    <span class="title">dloc</span>
  </a>
</div>

### `dloc`

#### Signature

```cpp
Archive::dloc(std::int64_t i) -> Catalog
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**i**
    Zero-based positional index used by this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.dloc(1);
```

##### Output

```text
metric -> Series([30.0, 40.0])
```

#### Practical Behavior

- `dloc(i)` is the positional depth-access alias for `catalog(i)`.
- Use it when you want the depth-axis wording to be explicit.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#distinct">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="#drop">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>

### `drop`

#### Signatures

```cpp
Archive::drop(const std::vector<std::string>& keys) -> Archive

Archive::drop(std::initializer_list<std::string_view> keys) -> Archive
```

#### What It Does

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.drop();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.drop();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#dloc">
    <span class="label">Previous</span>
    <span class="title">dloc</span>
  </a>
  <a class="next-link" href="#dropnull">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>

### `dropnull`

#### Signature

```cpp
Archive::dropnull(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.dropnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#drop">
    <span class="label">Previous</span>
    <span class="title">drop</span>
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
Archive::empty() -> bool
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.empty();
```

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#dropnull">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="#ffill">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>

## F

### `ffill`

#### Signature

```cpp
Archive::ffill(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.ffill();
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
  <a class="next-link" href="#fillnull">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>

### `fillnull`

#### Signatures

```cpp
Archive::fillnull(const char* value, std::initializer_list<std::string_view> keys = {}) -> Archive

Archive::fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) -> Archive

Archive::fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) -> Archive

Archive::fillnull(double value, std::initializer_list<std::string_view> keys = {}) -> Archive

Archive::fillnull(std::string_view value, std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 4. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 5. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ffill">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="#freq">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>

### `freq`

#### Signatures

```cpp
Archive::freq(FreqOptions options = {}) -> Archive

Archive::freq(std::initializer_list<std::string_view> keys, FreqOptions options = {}) -> Archive
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.freq();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.freq();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#fillnull">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="#has">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>

## H

### `has`

#### Signature

```cpp
Archive::has(std::string_view key) -> bool
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.has();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#freq">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="#head">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

### `head`

#### Signature

```cpp
Archive::head(std::int64_t n = 5) -> Archive
```

#### What It Does

Returns the first `n` catalogs from the archive depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.head(1);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="#invalid">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>

## I

### `invalid`

#### Signature

```cpp
Archive::invalid(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.invalid();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#isnull">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>

### `isnull`

#### Signature

```cpp
Archive::isnull(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.isnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#invalid">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="#kindelize">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>

## K

### `kindelize`

#### Signature

```cpp
Archive::kindelize(const pluss::table::Atlas& atlas, const KindelizePolicy& policy = {}) -> Archive
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**atlas**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`Archive`
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

auto out = pluss::kindle::Archive::kindelize(atlas);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#isnull">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
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
Archive::loc(const pluss::Timeline& label) -> Catalog

Archive::loc(std::int64_t label) -> Catalog

Archive::loc(std::string_view label) -> Catalog
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**label**
    Label or labels used by this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.loc("test");
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.loc("test");
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.loc("test");
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#map_catalogs">
    <span class="label">Next</span>
    <span class="title">map_catalogs</span>
  </a>
</div>

## M

### `map_catalogs`

#### Signature

```cpp
Archive::map_catalogs(const std::function<Catalog(const Catalog&)>& fn) -> Archive
```

#### What It Does

Applies the same function to each catalog and returns the transformed archive.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.map_catalogs([](const pluss::kindle::Catalog& catalog) { return catalog.head(1); });
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#loc">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="#maxkeys">
    <span class="label">Next</span>
    <span class="title">maxkeys</span>
  </a>
</div>

### `maxkeys`

#### Signature

```cpp
Archive::maxkeys() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.maxkeys();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map_catalogs">
    <span class="label">Previous</span>
    <span class="title">map_catalogs</span>
  </a>
  <a class="next-link" href="#maxrecords">
    <span class="label">Next</span>
    <span class="title">maxrecords</span>
  </a>
</div>

### `maxrecords`

#### Signature

```cpp
Archive::maxrecords() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.maxrecords();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#maxkeys">
    <span class="label">Previous</span>
    <span class="title">maxkeys</span>
  </a>
  <a class="next-link" href="#minkeys">
    <span class="label">Next</span>
    <span class="title">minkeys</span>
  </a>
</div>

### `minkeys`

#### Signature

```cpp
Archive::minkeys() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.minkeys();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#maxrecords">
    <span class="label">Previous</span>
    <span class="title">maxrecords</span>
  </a>
  <a class="next-link" href="#minrecords">
    <span class="label">Next</span>
    <span class="title">minrecords</span>
  </a>
</div>

### `minrecords`

#### Signature

```cpp
Archive::minrecords() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.minrecords();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#minkeys">
    <span class="label">Previous</span>
    <span class="title">minkeys</span>
  </a>
  <a class="next-link" href="#ndistinct">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>

## N

### `ndistinct`

#### Signatures

```cpp
Archive::ndistinct(DistinctOptions options = {}) -> Archive

Archive::ndistinct(std::initializer_list<std::string_view> keys, DistinctOptions options = {}) -> Archive
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.ndistinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.ndistinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#minrecords">
    <span class="label">Previous</span>
    <span class="title">minrecords</span>
  </a>
  <a class="next-link" href="#nkeys">
    <span class="label">Next</span>
    <span class="title">nkeys</span>
  </a>
</div>

### `nkeys`

#### Signature

```cpp
Archive::nkeys() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.nkeys();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ndistinct">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="#notnull">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>

### `notnull`

#### Signature

```cpp
Archive::notnull(std::initializer_list<std::string_view> keys = {}) -> Archive
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.notnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nkeys">
    <span class="label">Previous</span>
    <span class="title">nkeys</span>
  </a>
  <a class="next-link" href="#nrecords">
    <span class="label">Next</span>
    <span class="title">nrecords</span>
  </a>
</div>

### `nrecords`

#### Signature

```cpp
Archive::nrecords() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.nrecords();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#notnull">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
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
Archive::relabel_depth(const Stream& labels) -> Archive
```

#### What It Does

Replaces the depth-label axis with the supplied labels while keeping catalog contents unchanged.

#### Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto labels = pluss::kindle::Stream(pluss::make_note({"baseline", "stress"}));
auto out = archive.relabel_depth(labels);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nrecords">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="#rename">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>

### `rename`

#### Signatures

```cpp
Archive::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) -> Archive

Archive::rename(std::string_view from, std::string_view to) -> Archive
```

#### What It Does

Changes names or labels only. The underlying payload values, row order, and aligned structure stay the same.

#### Parameters

**mapping**
    Inline collection passed directly at the call site.

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.rename();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.rename();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#relabel_depth">
    <span class="label">Previous</span>
    <span class="title">relabel_depth</span>
  </a>
  <a class="next-link" href="#select">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>

## S

### `select`

#### Signatures

```cpp
Archive::select(const std::vector<std::string>& keys) -> Archive

Archive::select(std::initializer_list<std::string_view> keys) -> Archive
```

#### What It Does

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.select();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.select();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#rename">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="#shape">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

### `shape`

#### Signature

```cpp
Archive::shape() -> std::tuple<std::int64_t, std::int64_t, std::int64_t>
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::tuple<std::int64_t, std::int64_t, std::int64_t>`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.shape();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#select">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

### `show`

#### Signature

```cpp
Archive::show(std::ostream& os) -> void
```

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Parameters

**os**
    Destination output stream used instead of the default display or print target.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Write the output to a caller-provided stream.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

archive.show(std::cout);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#shape">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="#size">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>

### `size`

#### Signature

```cpp
Archive::size() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Inspect the `size` property of the current object.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.size();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="#slice_depth">
    <span class="label">Next</span>
    <span class="title">slice_depth</span>
  </a>
</div>

### `slice_depth`

#### Signature

```cpp
Archive::slice_depth(std::int64_t start, std::int64_t stop) -> Archive
```

#### What It Does

Returns a contiguous depth slice `[start, stop)` from the archive.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.slice_depth(0, 1);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#size">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="#sort_depth">
    <span class="label">Next</span>
    <span class="title">sort_depth</span>
  </a>
</div>

### `sort_depth`

#### Signature

```cpp
Archive::sort_depth(bool ascending = true, bool nulls_last = true) -> Archive
```

#### What It Does

Sorts catalogs by their current depth labels.

#### Parameters

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**nulls_last**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.sort_depth(true, true);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice_depth">
    <span class="label">Previous</span>
    <span class="title">slice_depth</span>
  </a>
  <a class="next-link" href="#tabelize">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>

## T

### `tabelize`

#### Signature

```cpp
Archive::tabelize(const TabelizePolicy& policy = {}) -> pluss::table::Atlas
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`pluss::table::Atlas`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = archive.tabelize(policy);
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
  <a class="prev-link" href="#sort_depth">
    <span class="label">Previous</span>
    <span class="title">sort_depth</span>
  </a>
  <a class="next-link" href="#tail">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>

### `tail`

#### Signature

```cpp
Archive::tail(std::int64_t n = 5) -> Archive
```

#### What It Does

Returns the last `n` catalogs from the archive depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Archive`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.tail(1);
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tabelize">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="#to_string">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>

### `to_string`

#### Signature

```cpp
Archive::to_string() -> std::string
```

#### What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.to_string();
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#to_table">
    <span class="label">Next</span>
    <span class="title">to_table</span>
  </a>
</div>

### `to_table`

#### Signature

```cpp
Archive::to_table(const TabelizePolicy& policy = {}, bool render = false) -> pluss::table::Atlas
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`pluss::table::Atlas`
    Value produced by this call.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = archive.to_table(policy, true);
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
Archive::unique() -> bool
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.unique();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_table">
    <span class="label">Previous</span>
    <span class="title">to_table</span>
  </a>
</div>
