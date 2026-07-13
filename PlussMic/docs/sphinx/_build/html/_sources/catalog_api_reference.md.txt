# Catalog API Reference

This page documents the full public `Catalog` surface for named stream collections.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `add_text_features`

#### Signature

```cpp
Catalog::add_text_features(std::string_view field) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.add_text_features("text");
```

##### Output

```text
ct.head():
text -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])

out.head():
text            -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])
text_char_len   -> Series([18, 22, 20])
text_word_count -> Series([4, 3, 3])
```

#### Practical Behavior

- `add_text_features(field)` adds `<field>_char_len` and `<field>_word_count` streams.
- The source field is coerced through note/text handling before the text statistics are computed.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#aggregate_text_stats">
    <span class="label">Next</span>
    <span class="title">aggregate_text_stats</span>
  </a>
</div>

### `aggregate_text_stats`

#### Signature

```cpp
Catalog::aggregate_text_stats(std::string_view field, std::string_view stat_type) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

**stat_type**
    Statistic selector string understood by this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.aggregate_text_stats("text", "mean");
```

##### Output

```text
text_mean_char_len -> Series([20.0])
```

#### Practical Behavior

- `aggregate_text_stats(field, stat_type)` collapses one text field into a one-value summary stream.
- Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#add_text_features">
    <span class="label">Previous</span>
    <span class="title">add_text_features</span>
  </a>
  <a class="next-link" href="#apply">
    <span class="label">Next</span>
    <span class="title">apply</span>
  </a>
</div>

### `apply`

#### Signature

```cpp
Catalog::apply(std::string_view field, const std::function<Stream(const Stream&)>& func) -> Catalog
```

#### What It Does

Applies a function to one named stream and returns the updated catalog.

#### Parameters

**field**
    Name of the target stream in the catalog.

**func**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.apply("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `apply(...)` rewrites one named stream; `apply_all(...)` rewrites every stream.
- The transform must return a defined stream with the same length as the input stream.
- Use the underscore form when you want the mutation in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#aggregate_text_stats">
    <span class="label">Previous</span>
    <span class="title">aggregate_text_stats</span>
  </a>
  <a class="next-link" href="#apply_">
    <span class="label">Next</span>
    <span class="title">apply_</span>
  </a>
</div>

### `apply_`

#### Signature

```cpp
Catalog::apply_(std::string_view field, const std::function<Stream(const Stream&)>& func) -> Catalog&
```

#### What It Does

Applies a function to one named stream in place.

#### Parameters

**field**
    Name of the target stream in the catalog.

**func**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.apply_("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="#apply_all">
    <span class="label">Next</span>
    <span class="title">apply_all</span>
  </a>
</div>

### `apply_all`

#### Signature

```cpp
Catalog::apply_all(const std::function<Stream(const Stream&)>& fn) -> Catalog
```

#### What It Does

Applies the same function to every stream and returns the updated catalog.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.apply_all([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `apply(...)` rewrites one named stream; `apply_all(...)` rewrites every stream.
- The transform must return a defined stream with the same length as the input stream.
- Use the underscore form when you want the mutation in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_">
    <span class="label">Previous</span>
    <span class="title">apply_</span>
  </a>
  <a class="next-link" href="#apply_all_">
    <span class="label">Next</span>
    <span class="title">apply_all_</span>
  </a>
</div>

### `apply_all_`

#### Signature

```cpp
Catalog::apply_all_(const std::function<Stream(const Stream&)>& fn) -> Catalog&
```

#### What It Does

Applies the same function to every stream in place.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.apply_all_([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_all">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="#astype">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>

### `astype`

#### Signature

```cpp
Catalog::astype(std::initializer_list<std::string_view> keys, pluss::ScalarType dtype) -> Catalog
```

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**dtype**
    Target scalar dtype used by this conversion overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.astype({"metric"}, pluss::ScalarType::Float64);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- `astype(keys, dtype)` is the catalog-level numeric-series conversion helper.
- It delegates to stream/thread conversion on the selected keys only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_all_">
    <span class="label">Previous</span>
    <span class="title">apply_all_</span>
  </a>
  <a class="next-link" href="#at">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>

### `at`

#### Signature

```cpp
Catalog::at(std::string_view key) -> Stream
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.at("metric");
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `at(key)` fetches one named stream from the catalog.
- Use it when you want stream access without routing through table conversion first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#astype">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="#begin">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>

## B

### `begin`

#### Signature

```cpp
Catalog::begin() -> const_iterator
```

#### What It Does

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : ct) {
  (void)key;
}
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#at">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="#bfill">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>

### `bfill`

#### Signature

```cpp
Catalog::bfill(std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "Final Points Grade"_k = pluss::kindle::Stream(pluss::make_series({71.0, std::numeric_limits<double>::quiet_NaN(), 88.0, std::numeric_limits<double>::quiet_NaN()})),
    "Midterm 3"_k = pluss::kindle::Stream(pluss::make_series({60.0, std::numeric_limits<double>::quiet_NaN(), 75.0, 91.0})),
    "Name"_k = pluss::kindle::Stream(pluss::make_note({"A", "B", "C", "D"})),
});

auto out = ct.bfill({"Final Points Grade", "Midterm 3"});
```

##### Output

```text
ct.head():
Final Points Grade -> Series([71.0, null, 88.0, null])
Midterm 3          -> Series([60.0, null, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])

out.head():
Final Points Grade -> Series([71.0, 88.0, 88.0, null])
Midterm 3          -> Series([60.0, 75.0, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])
```

#### Practical Behavior

- With no keys supplied, `bfill()` is applied to every stream in the catalog.
- If you pass keys, only those streams are backward-filled; all other streams are left unchanged.
- Unknown keys throw immediately instead of silently skipping.

#### Common Pattern

```cpp
auto cleaned = catalog.bfill({"Final Points Grade", "Midterm 3"});
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#begin">
    <span class="label">Previous</span>
    <span class="title">begin</span>
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
Catalog::cast(std::initializer_list<std::string_view> keys, pluss::table::ThreadCastTarget target, const pluss::table::ThreadCastOptions& options = {}) -> Catalog
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

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cast();
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
  <a class="next-link" href="#cloc">
    <span class="label">Next</span>
    <span class="title">cloc</span>
  </a>
</div>

### `cloc`

#### Signatures

```cpp
Catalog::cloc(std::int64_t row) -> Catalog

Catalog::cloc(std::int64_t start, std::int64_t stop) -> Catalog
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**row**
    Zero-based row position used by this overload.

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cloc();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cloc();
```

##### Output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices aligned row positions across every stream in the catalog.
- Use it when you want row-wise positional access while keeping stream alignment intact.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#cast">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="#compute_similarity">
    <span class="label">Next</span>
    <span class="title">compute_similarity</span>
  </a>
</div>

### `compute_similarity`

#### Signature

```cpp
Catalog::compute_similarity(std::string_view field1, std::string_view field2, std::string_view method = "cosine") -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field1**
    First named field used by this overload.

**field2**
    Second named field used by this overload.

**method**
    Method selector string understood by this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.compute_similarity("text", "text", "cosine");
```

##### Output

```text
text__text_similarity -> Series([1.0, 1.0, 1.0])
```

#### Practical Behavior

- `compute_similarity(field1, field2, "cosine")` computes row-wise text similarity and appends the result as a numeric stream.
- At the moment the supported similarity method is cosine.
- Both fields must have the same length.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#cloc">
    <span class="label">Previous</span>
    <span class="title">cloc</span>
  </a>
  <a class="next-link" href="#count">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>

### `count`

#### Signature

```cpp
Catalog::count() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Compute `count` and inspect the resulting summary value or summary object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.count();
```

##### Output

```text
3
```

#### Practical Behavior

- `count()` returns the catalog row count after alignment through `tabelize()`.
- This is the quick “how many aligned rows do I have?” answer.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#compute_similarity">
    <span class="label">Previous</span>
    <span class="title">compute_similarity</span>
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
Catalog::defined() -> bool
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#count">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="#display">
    <span class="label">Next</span>
    <span class="title">display</span>
  </a>
</div>

### `display`

#### Signature

```cpp
Catalog::display() -> void
```

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.display();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `display()` renders a human-readable catalog view for inspection.
- Use it for inspection and debugging rather than transformation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="#drop">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>

### `drop`

#### Signatures

```cpp
Catalog::drop(const std::vector<std::string>& field_names) -> Catalog

Catalog::drop(std::initializer_list<std::string_view> keys) -> Catalog

Catalog::drop(std::string_view field_name) -> Catalog
```

#### What It Does

Removes the named streams and keeps the remaining catalog structure intact.

#### Parameters

**field_names**
    Field names supplied together for this overload.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**field_name**
    Field name used to select the target text, stream, or grouping column.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"city"};
auto out = ct.drop(names);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop({"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop("city");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `drop(...)` removes named streams, not rows.
- The remaining streams keep their alignment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#display">
    <span class="label">Previous</span>
    <span class="title">display</span>
  </a>
  <a class="next-link" href="#dropna">
    <span class="label">Next</span>
    <span class="title">dropna</span>
  </a>
</div>

### `dropna`

#### Signature

```cpp
Catalog::dropna(std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto metric = pluss::kindle::Stream(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0}));
auto ct = pluss::kindle::Catalog({
    "metric"_k = metric,
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.dropna({"metric"});
```

##### Output

```text
ct.head():
metric -> Series([10.0, null, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
metric -> Series([10.0, 30.0])
city   -> Note(["Accra", "Kumasi"])
```

#### Practical Behavior

- `dropna(keys)` applies stream-level `dropna()` to the selected keys.
- With no keys supplied, every stream is processed.
- Unknown keys throw immediately.

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
Catalog::empty() -> bool
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.empty();
```

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#dropna">
    <span class="label">Previous</span>
    <span class="title">dropna</span>
  </a>
  <a class="next-link" href="#end">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>

### `end`

#### Signature

```cpp
Catalog::end() -> const_iterator
```

#### What It Does

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : ct) {
  (void)key;
}
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
  <a class="next-link" href="#extract_keywords">
    <span class="label">Next</span>
    <span class="title">extract_keywords</span>
  </a>
</div>

### `extract_keywords`

#### Signature

```cpp
Catalog::extract_keywords(std::string_view field, int top_n = 10) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.extract_keywords("text", 5);
```

##### Output

```text
keyword -> Note(["clear", "downtown", "heavy", "issued", "on"])
count   -> Series([1, 1, 1, 1, 1])
```

#### Practical Behavior

- `extract_keywords(field, top_n)` tokenizes the text field, counts tokens, sorts by frequency descending then alphabetically, and returns a small keyword/count catalog.
- Use it for quick corpus-level keyword previews rather than row-wise text features.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#end">
    <span class="label">Previous</span>
    <span class="title">end</span>
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
Catalog::ffill(std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.ffill();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#extract_keywords">
    <span class="label">Previous</span>
    <span class="title">extract_keywords</span>
  </a>
  <a class="next-link" href="#fillna">
    <span class="label">Next</span>
    <span class="title">fillna</span>
  </a>
</div>

### `fillna`

#### Signatures

```cpp
Catalog::fillna(const char* value, std::initializer_list<std::string_view> keys = {}) -> Catalog

Catalog::fillna(const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) -> Catalog

Catalog::fillna(const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) -> Catalog

Catalog::fillna(double value, std::initializer_list<std::string_view> keys = {}) -> Catalog

Catalog::fillna(std::string_view value, std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 3. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 4. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 5. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ffill">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="#filter">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>

### `filter`

#### Signature

```cpp
Catalog::filter(const pluss::Series& condition) -> Catalog
```

#### What It Does

Keeps only the rows where the supplied mask stream is true.

#### Parameters

**condition**
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.filter(mask);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#fillna">
    <span class="label">Previous</span>
    <span class="title">fillna</span>
  </a>
  <a class="next-link" href="#filter_by_text">
    <span class="label">Next</span>
    <span class="title">filter_by_text</span>
  </a>
</div>

### `filter_by_text`

#### Signature

```cpp
Catalog::filter_by_text(std::string_view field, std::string_view pattern) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

**pattern**
    Pattern string used for matching or extraction.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.filter_by_text("text", "rain");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#filter">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="#from_json">
    <span class="label">Next</span>
    <span class="title">from_json</span>
  </a>
</div>

### `from_json`

#### Signature

```cpp
Catalog::from_json(std::string_view json_text) -> Catalog
```

#### What It Does

Builds a catalog from JSON input using the PlussMic JSON loader route.

#### Parameters

**json_text**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto out = pluss::kindle::Catalog::from_json(R"([{"id":1,"city":"Accra"},{"id":2,"city":"Tema"}])");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#filter_by_text">
    <span class="label">Previous</span>
    <span class="title">filter_by_text</span>
  </a>
  <a class="next-link" href="#get">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>

## G

### `get`

#### Signature

```cpp
Catalog::get(std::string_view key) -> std::optional<Stream>
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.get("metric");
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#from_json">
    <span class="label">Previous</span>
    <span class="title">from_json</span>
  </a>
  <a class="next-link" href="#get_stream">
    <span class="label">Next</span>
    <span class="title">get_stream</span>
  </a>
</div>

### `get_stream`

#### Signature

```cpp
Catalog::get_stream(std::string_view key) -> std::optional<Stream>
```

#### What It Does

Returns one named stream from the catalog.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.get_stream("metric");
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#get">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="#group_by">
    <span class="label">Next</span>
    <span class="title">group_by</span>
  </a>
</div>

### `group_by`

#### Signature

```cpp
Catalog::group_by(std::string_view key_field) -> std::unordered_map<std::string, Catalog>
```

#### What It Does

Splits the catalog into sub-catalogs keyed by the distinct values in the grouping stream.

#### Parameters

**key_field**
    Field name that acts as the key for matching, grouping, or lookup.

#### Returns

`std::unordered_map<std::string, Catalog>`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.group_by("city");
```

##### Output

```text
{"Accra": Catalog(...), "Tema": Catalog(...)}
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#get_stream">
    <span class="label">Previous</span>
    <span class="title">get_stream</span>
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
Catalog::has(std::string_view key) -> bool
```

#### What It Does

Checks whether a named stream exists.

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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.has("metric");
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#group_by">
    <span class="label">Previous</span>
    <span class="title">group_by</span>
  </a>
  <a class="next-link" href="#has_stream">
    <span class="label">Next</span>
    <span class="title">has_stream</span>
  </a>
</div>

### `has_stream`

#### Signature

```cpp
Catalog::has_stream(std::string_view key) -> bool
```

#### What It Does

Checks whether a named stream exists.

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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.has_stream("metric");
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="#head">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

### `head`

#### Signature

```cpp
Catalog::head(std::int64_t n) -> Catalog
```

#### What It Does

Returns the first `n` rows across every stream in the catalog.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.head(2);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has_stream">
    <span class="label">Previous</span>
    <span class="title">has_stream</span>
  </a>
  <a class="next-link" href="#info">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>

## I

### `info`

#### Signature

```cpp
Catalog::info() -> void
```

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.info();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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
Catalog::isnull(std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.isnull();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#info">
    <span class="label">Previous</span>
    <span class="title">info</span>
  </a>
  <a class="next-link" href="#keys">
    <span class="label">Next</span>
    <span class="title">keys</span>
  </a>
</div>

## K

### `keys`

#### Signature

```cpp
Catalog::keys() -> const std::vector<std::string>&
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.keys();
```

##### Output

```text
["id", "metric", "city"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#isnull">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="#kindelize">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>

### `kindelize`

#### Signature

```cpp
Catalog::kindelize(const pluss::table::DataFrame& frame, const KindelizePolicy& policy = {}) -> Catalog
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**frame**
    Frame payload supplied to this overload.

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = pluss::kindle::Catalog::kindelize(df);
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#keys">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="#load">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>

## L

### `load`

#### Signatures

```cpp
Catalog::load(std::string_view source_type, std::string_view filepath) -> Catalog

Catalog::load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**source_type**
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#load_csv">
    <span class="label">Next</span>
    <span class="title">load_csv</span>
  </a>
</div>

### `load_csv`

#### Signatures

```cpp
Catalog::load_csv(std::string_view filepath) -> Catalog

Catalog::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass a typed reader-options object to control file loading details.

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv", options);
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="#load_csv_with_key">
    <span class="label">Next</span>
    <span class="title">load_csv_with_key</span>
  </a>
</div>

### `load_csv_with_key`

#### Signature

```cpp
Catalog::load_csv_with_key(std::string_view filepath, std::string_view key_field, const pluss::loader::ReadCsvOptions& options = {}) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**key_field**
    Field name that acts as the key for matching, grouping, or lookup.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass a typed reader-options object to control file loading details.

```cpp
auto out = pluss::kindle::Catalog::load_csv_with_key("path/to/data.csv", "id");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_csv">
    <span class="label">Previous</span>
    <span class="title">load_csv</span>
  </a>
  <a class="next-link" href="#load_excel">
    <span class="label">Next</span>
    <span class="title">load_excel</span>
  </a>
</div>

### `load_excel`

#### Signatures

```cpp
Catalog::load_excel(std::string_view filepath) -> Catalog

Catalog::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass a typed reader-options object to control file loading details.

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx", options);
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_csv_with_key">
    <span class="label">Previous</span>
    <span class="title">load_csv_with_key</span>
  </a>
  <a class="next-link" href="#load_json">
    <span class="label">Next</span>
    <span class="title">load_json</span>
  </a>
</div>

### `load_json`

#### Signatures

```cpp
Catalog::load_json(std::string_view filepath) -> Catalog

Catalog::load_json(std::string_view filepath, const pluss::loader::Config& config) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json", {"nrows", 16});
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_excel">
    <span class="label">Previous</span>
    <span class="title">load_excel</span>
  </a>
  <a class="next-link" href="#load_parquet">
    <span class="label">Next</span>
    <span class="title">load_parquet</span>
  </a>
</div>

### `load_parquet`

#### Signatures

```cpp
Catalog::load_parquet(std::string_view filepath) -> Catalog

Catalog::load_parquet(std::string_view filepath, const pluss::loader::Config& config) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet", {"nrows", 16});
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_json">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="#load_txt">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>

### `load_txt`

#### Signatures

```cpp
Catalog::load_txt(std::string_view filepath) -> Catalog

Catalog::load_txt(std::string_view filepath, const pluss::loader::Config& config) -> Catalog
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt");
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt", {"nrows", 16});
```

##### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_parquet">
    <span class="label">Previous</span>
    <span class="title">load_parquet</span>
  </a>
  <a class="next-link" href="#masked_select">
    <span class="label">Next</span>
    <span class="title">masked_select</span>
  </a>
</div>

## M

### `masked_select`

#### Signature

```cpp
Catalog::masked_select(const pluss::Series& mask) -> Catalog
```

#### What It Does

Keeps only the rows where the supplied mask stream is true.

#### Parameters

**mask**
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.masked_select(mask);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_txt">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="#max">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>

### `max`

#### Signature

```cpp
Catalog::max(std::string_view field) -> Stream
```

#### What It Does

Computes the maximum value of one named numeric stream.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Compute `max` and inspect the resulting summary value or summary object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.max("metric");
```

##### Output

```text
Series([20.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#masked_select">
    <span class="label">Previous</span>
    <span class="title">masked_select</span>
  </a>
  <a class="next-link" href="#mean">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>

### `mean`

#### Signature

```cpp
Catalog::mean(std::string_view field) -> Stream
```

#### What It Does

Computes the mean value of one named numeric stream.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Compute `mean` and inspect the resulting summary value or summary object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.mean("metric");
```

##### Output

```text
Series([20.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#max">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="#merge">
    <span class="label">Next</span>
    <span class="title">merge</span>
  </a>
</div>

### `merge`

#### Signature

```cpp
Catalog::merge(const Catalog& other, std::string_view on, std::string_view how = "inner") -> Catalog
```

#### What It Does

Combines two catalogs by matching rows on a key stream.

#### Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**on**
    Field name used as the match or join key for this overload.

**how**
    Join mode for `DataFrame::unify(...)`.

    Supported values:
    - `common`: Keep only rows whose key values match on both sides.
    - `keep_left`: Preserve left keys and attach right-hand matches where they exist.
    - `keep_right`: Preserve right keys and attach left-hand matches where they exist.
    - `unite`: Keep the stable union of keys from both frames.
    - `cross`: Create the cartesian product; key feature arguments must be empty.
    - `left_only`: Keep left rows whose keys do not occur on the right.
    - `right_only`: Keep right rows whose keys do not occur on the left.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "score"_k = pluss::kindle::Stream(pluss::make_series({0.9, 0.7, 0.8}))});
auto out = ct.merge(right, "id", "inner");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#mean">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="#min">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>

### `min`

#### Signature

```cpp
Catalog::min(std::string_view field) -> Stream
```

#### What It Does

Computes the minimum value of one named numeric stream.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Compute `min` and inspect the resulting summary value or summary object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.min("metric");
```

##### Output

```text
Series([20.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#merge">
    <span class="label">Previous</span>
    <span class="title">merge</span>
  </a>
  <a class="next-link" href="#nfields">
    <span class="label">Next</span>
    <span class="title">nfields</span>
  </a>
</div>

## N

### `nfields`

#### Signature

```cpp
Catalog::nfields() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nfields();
```

##### Output

```text
3
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#min">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="#note">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>

### `note`

#### Signature

```cpp
Catalog::note() -> Catalog
```

#### What It Does

Returns one named stream interpreted as a note/text stream.

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.note();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nfields">
    <span class="label">Previous</span>
    <span class="title">nfields</span>
  </a>
  <a class="next-link" href="#notnull">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>

### `notnull`

#### Signature

```cpp
Catalog::notnull(std::initializer_list<std::string_view> keys = {}) -> Catalog
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.notnull();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#note">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="#nstreams">
    <span class="label">Next</span>
    <span class="title">nstreams</span>
  </a>
</div>

### `nstreams`

#### Signature

```cpp
Catalog::nstreams() -> std::int64_t
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nstreams();
```

##### Output

```text
3
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#notnull">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="#query">
    <span class="label">Next</span>
    <span class="title">query</span>
  </a>
</div>

## Q

### `query`

#### Signature

```cpp
Catalog::query(const QueryFn& fn) -> Catalog
```

#### What It Does

Keeps only the rows selected by the supplied row-wise query function or expression.

#### Parameters

**fn**
    Callable used to transform, map, or derive values in this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.query([](const pluss::kindle::Catalog& c) { return c.stream("metric").as_series().gt(15.0); });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nstreams">
    <span class="label">Previous</span>
    <span class="title">nstreams</span>
  </a>
  <a class="next-link" href="#rename">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>

## R

### `rename`

#### Signatures

```cpp
Catalog::rename(const std::unordered_map<std::string, std::string>& mapping) -> Catalog

Catalog::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) -> Catalog

Catalog::rename(std::string_view from, std::string_view to) -> Catalog
```

#### What It Does

Renames one or more streams without changing their values.

#### Parameters

**mapping**
    Name-to-name mapping used by this overload.

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::unordered_map<std::string, std::string> renames{{"metric", "distance_km"}};
auto out = ct.rename(renames);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.rename({{"metric", "distance_km"}});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.rename("metric", "distance_km");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#query">
    <span class="label">Previous</span>
    <span class="title">query</span>
  </a>
  <a class="next-link" href="#rename_">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>

### `rename_`

#### Signature

```cpp
Catalog::rename_(std::string_view from, std::string_view to) -> Catalog&
```

#### What It Does

Renames one or more streams in place without changing their values.

#### Parameters

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.rename_("metric", "distance_km");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#rename">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="#sample">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>

## S

### `sample`

#### Signature

```cpp
Catalog::sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) -> Catalog
```

#### What It Does

Returns a random sample of rows across all streams.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

**random_state**
    Optional seed used to make sampling or shuffling reproducible.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sample(2, 7);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#rename_">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="#save_as">
    <span class="label">Next</span>
    <span class="title">save_as</span>
  </a>
</div>

### `save_as`

#### Signature

```cpp
Catalog::save_as(std::string_view target_type, std::string_view filename, const pluss::loader::Config& config = {}) -> void
```

#### What It Does

Writes the catalog using the explicitly supplied output format selector.

#### Parameters

**target_type**
    Format or target selector string used by this overload.

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_as("json", "path/to/output.json");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sample">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="#save_csv">
    <span class="label">Next</span>
    <span class="title">save_csv</span>
  </a>
</div>

### `save_csv`

#### Signatures

```cpp
Catalog::save_csv(std::string_view filename) -> void

Catalog::save_csv(std::string_view filename, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the catalog to CSV after aligning its streams into a table-like representation.

#### Parameters

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_csv("path/to/output.csv");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_csv("path/to/output.csv");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_as">
    <span class="label">Previous</span>
    <span class="title">save_as</span>
  </a>
  <a class="next-link" href="#save_excel">
    <span class="label">Next</span>
    <span class="title">save_excel</span>
  </a>
</div>

### `save_excel`

#### Signatures

```cpp
Catalog::save_excel(std::string_view filename) -> void

Catalog::save_excel(std::string_view filename, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.

#### Parameters

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_excel("path/to/output.xlsx");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_excel("path/to/output.xlsx");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_csv">
    <span class="label">Previous</span>
    <span class="title">save_csv</span>
  </a>
  <a class="next-link" href="#save_json">
    <span class="label">Next</span>
    <span class="title">save_json</span>
  </a>
</div>

### `save_json`

#### Signatures

```cpp
Catalog::save_json(std::string_view filename) -> void

Catalog::save_json(std::string_view filename, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the catalog to JSON for interchange or later reload.

#### Parameters

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_json("path/to/output.json");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_json("path/to/output.json");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_excel">
    <span class="label">Previous</span>
    <span class="title">save_excel</span>
  </a>
  <a class="next-link" href="#save_parquet">
    <span class="label">Next</span>
    <span class="title">save_parquet</span>
  </a>
</div>

### `save_parquet`

#### Signatures

```cpp
Catalog::save_parquet(std::string_view filename) -> void

Catalog::save_parquet(std::string_view filename, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the catalog to Parquet for compact typed storage.

#### Parameters

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_parquet("path/to/output.parquet");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_parquet("path/to/output.parquet");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_json">
    <span class="label">Previous</span>
    <span class="title">save_json</span>
  </a>
  <a class="next-link" href="#save_txt">
    <span class="label">Next</span>
    <span class="title">save_txt</span>
  </a>
</div>

### `save_txt`

#### Signatures

```cpp
Catalog::save_txt(std::string_view filename) -> void

Catalog::save_txt(std::string_view filename, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes a text rendering of the catalog for inspection or logging.

#### Parameters

**filename**
    Filename or path-like string used by this overload.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_txt("path/to/output.txt");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_txt("path/to/output.txt");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_parquet">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="#select">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>

### `select`

#### Signatures

```cpp
Catalog::select(const std::vector<std::string>& field_names) -> Catalog

Catalog::select(std::initializer_list<std::string_view> field_names) -> Catalog
```

#### What It Does

Keeps only the named streams and preserves their shared row alignment.

#### Parameters

**field_names**
    Field names supplied together for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"id", "metric"};
auto out = ct.select(names);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.select({"id", "metric"});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_txt">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="#series">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>

### `series`

#### Signature

```cpp
Catalog::series() -> Catalog
```

#### What It Does

Returns one named stream interpreted as a numeric series stream.

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.series();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#select">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="#set">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>

### `set`

#### Signature

```cpp
Catalog::set(std::string_view key, const Stream& value) -> Catalog
```

#### What It Does

Adds or replaces one named stream and returns the updated catalog.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#series">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="#set_">
    <span class="label">Next</span>
    <span class="title">set_</span>
  </a>
</div>

### `set_`

#### Signature

```cpp
Catalog::set_(std::string_view key, const Stream& value) -> Catalog&
```

#### What It Does

Adds or replaces one named stream in place.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="#set_stream">
    <span class="label">Next</span>
    <span class="title">set_stream</span>
  </a>
</div>

### `set_stream`

#### Signature

```cpp
Catalog::set_stream(std::string_view key, const Stream& value) -> Catalog
```

#### What It Does

Adds or replaces one named stream and returns the updated catalog.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_">
    <span class="label">Previous</span>
    <span class="title">set_</span>
  </a>
  <a class="next-link" href="#set_stream_">
    <span class="label">Next</span>
    <span class="title">set_stream_</span>
  </a>
</div>

### `set_stream_`

#### Signature

```cpp
Catalog::set_stream_(std::string_view key, const Stream& value) -> Catalog&
```

#### What It Does

Adds or replaces one named stream in place.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_stream">
    <span class="label">Previous</span>
    <span class="title">set_stream</span>
  </a>
  <a class="next-link" href="#set_streams">
    <span class="label">Next</span>
    <span class="title">set_streams</span>
  </a>
</div>

### `set_streams`

#### Signatures

```cpp
Catalog::set_streams(std::initializer_list<NamedStream> items) -> Catalog

Catalog::set_streams(std::initializer_list<std::pair<std::string_view, Stream>> items) -> Catalog
```

#### What It Does

Adds or replaces several named streams and returns the updated catalog.

#### Parameters

**items**
    Inline collection passed directly at the call site.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto out = ct.set_streams({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_streams({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_stream_">
    <span class="label">Previous</span>
    <span class="title">set_stream_</span>
  </a>
  <a class="next-link" href="#set_streams_">
    <span class="label">Next</span>
    <span class="title">set_streams_</span>
  </a>
</div>

### `set_streams_`

#### Signatures

```cpp
Catalog::set_streams_(std::initializer_list<NamedStream> items) -> Catalog&

Catalog::set_streams_(std::initializer_list<std::pair<std::string_view, Stream>> items) -> Catalog&
```

#### What It Does

Adds or replaces several named streams in place.

#### Parameters

**items**
    Inline collection passed directly at the call site.

#### Returns

`Catalog&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
ct.set_streams_({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_streams_({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_streams">
    <span class="label">Previous</span>
    <span class="title">set_streams</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

### `show`

#### Signature

```cpp
Catalog::show(std::ostream& os) -> void
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.show();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_streams_">
    <span class="label">Previous</span>
    <span class="title">set_streams_</span>
  </a>
  <a class="next-link" href="#slice">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>

### `slice`

#### Signature

```cpp
Catalog::slice(std::int64_t start, std::int64_t stop) -> Catalog
```

#### What It Does

Returns a contiguous row slice `[start, stop)` across every stream.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.slice(0, 2);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="#sort">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>

### `sort`

#### Signature

```cpp
Catalog::sort(std::string_view by_field, bool ascending = true) -> Catalog
```

#### What It Does

Reorders rows using one stream as the sort key, while keeping the other streams aligned to the same row order.

#### Parameters

**by_field**
    Field name that controls the grouping or sorting basis of the operation.

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sort("metric", false);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="#sort_by">
    <span class="label">Next</span>
    <span class="title">sort_by</span>
  </a>
</div>

### `sort_by`

#### Signature

```cpp
Catalog::sort_by(const std::vector<std::string>& fields, const std::vector<bool>& ascending = {}) -> Catalog
```

#### What It Does

Reorders rows using multiple sort keys, while keeping all streams aligned.

#### Parameters

**fields**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**ascending**
    Sort-order switch. `true` means ascending order and `false` means descending order.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sort_by({"city", "id"}, {true, false});
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sort">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="#stream">
    <span class="label">Next</span>
    <span class="title">stream</span>
  </a>
</div>

### `stream`

#### Signature

```cpp
Catalog::stream(std::string_view key) -> Stream
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.stream("metric");
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sort_by">
    <span class="label">Previous</span>
    <span class="title">sort_by</span>
  </a>
  <a class="next-link" href="#streams">
    <span class="label">Next</span>
    <span class="title">streams</span>
  </a>
</div>

### `streams`

#### Signature

```cpp
Catalog::streams() -> std::vector<Stream>
```

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Parameters

`None`

#### Returns

`std::vector<Stream>`
    Materialized collection of stream objects.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.streams();
```

##### Output

```text
[Stream(id), Stream(metric), Stream(city)]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#stream">
    <span class="label">Previous</span>
    <span class="title">stream</span>
  </a>
  <a class="next-link" href="#sum">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>

### `sum`

#### Signature

```cpp
Catalog::sum(std::string_view field) -> Stream
```

#### What It Does

Computes the sum of one named numeric stream.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Examples

Example 1. Compute `sum` and inspect the resulting summary value or summary object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sum("metric");
```

##### Output

```text
Series([20.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#streams">
    <span class="label">Previous</span>
    <span class="title">streams</span>
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
Catalog::tabelize(const TabelizePolicy& policy = {}) -> pluss::table::DataFrame
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

#### Returns

`pluss::table::DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = ct.tabelize(policy);
```

##### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sum">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="#tail">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>

### `tail`

#### Signature

```cpp
Catalog::tail(std::int64_t n) -> Catalog
```

#### What It Does

Returns the last `n` rows across every stream in the catalog.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.tail(2);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tabelize">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="#take">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>

### `take`

#### Signature

```cpp
Catalog::take(std::span<const std::int64_t> indices) -> Catalog
```

#### What It Does

Returns the rows at the requested integer positions across every stream.

#### Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Pass several integer positions together.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::int64_t> picks{0, 2};
auto out = ct.take(picks);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#text_preprocess">
    <span class="label">Next</span>
    <span class="title">text_preprocess</span>
  </a>
</div>

### `text_preprocess`

#### Signature

```cpp
Catalog::text_preprocess(std::string_view field) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.text_preprocess("text");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#take">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="#tfidf">
    <span class="label">Next</span>
    <span class="title">tfidf</span>
  </a>
</div>

### `tfidf`

#### Signature

```cpp
Catalog::tfidf(std::string_view field) -> Catalog
```

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.tfidf("text");
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#text_preprocess">
    <span class="label">Previous</span>
    <span class="title">text_preprocess</span>
  </a>
  <a class="next-link" href="#timeindex">
    <span class="label">Next</span>
    <span class="title">timeindex</span>
  </a>
</div>

### `timeindex`

#### Signature

```cpp
Catalog::timeindex() -> Catalog
```

#### What It Does

Returns one named stream interpreted as a time-index stream.

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.timeindex();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tfidf">
    <span class="label">Previous</span>
    <span class="title">tfidf</span>
  </a>
  <a class="next-link" href="#timeline">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>

### `timeline`

#### Signature

```cpp
Catalog::timeline() -> Catalog
```

#### What It Does

Returns one named stream interpreted as a timeline stream.

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.timeline();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#timeindex">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
  <a class="next-link" href="#to_json">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>

### `to_json`

#### Signature

```cpp
Catalog::to_json() -> std::string
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.to_json();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#timeline">
    <span class="label">Previous</span>
    <span class="title">timeline</span>
  </a>
  <a class="next-link" href="#to_string">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>

### `to_string`

#### Signature

```cpp
Catalog::to_string() -> std::string
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.to_string();
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_json">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="#to_table">
    <span class="label">Next</span>
    <span class="title">to_table</span>
  </a>
</div>

### `to_table`

#### Signature

```cpp
Catalog::to_table(const TabelizePolicy& policy = {}, bool render = true) -> pluss::table::DataFrame
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`pluss::table::DataFrame`
    Value produced by this call.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = ct.to_table(policy, true);
```

##### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_string">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="#transform">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>

### `transform`

#### Signatures

```cpp
Catalog::transform(const std::function<Stream(const Stream&)>& fn) -> Catalog

Catalog::transform(std::string_view field, const std::function<Stream(const Stream&)>& fn) -> Catalog
```

#### What It Does

Rewrites one stream, several streams, or all streams by applying a user function.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**field**
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.transform([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
Example 2. Apply a user function and inspect the transformed result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.transform("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_table">
    <span class="label">Previous</span>
    <span class="title">to_table</span>
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
Catalog::unique() -> bool
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

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.unique();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#transform">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="#where">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>

## W

### `where`

#### Signature

```cpp
Catalog::where(const pluss::Series& condition) -> Catalog
```

#### What It Does

Keeps only the rows where the supplied mask stream is true.

#### Parameters

**condition**
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Returns

`Catalog`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.where(mask);
```

##### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#unique">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
</div>
