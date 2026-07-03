# Stream API Reference

# Practical Stream API Reference

## API Reference Style

This page documents the full public `Stream` surface for the Kindle model.

### `abs`

#### Signature

```cpp
Stream::abs() -> Stream
```

#### Notes

Returns a numeric stream whose values are the absolute values of the current numeric payload.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.abs();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `as_note`

#### Signature

```cpp
Stream::as_note() -> const pluss::Note&
```

#### Notes

Interprets or converts the current payload as a note/text stream.

#### Inputs

`None`

#### Output

`const pluss::Note&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = stream.as_note();
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `as_series`

#### Signature

```cpp
Stream::as_series() -> const pluss::Series&
```

#### Notes

Interprets or converts the current payload as a numeric series stream.

#### Inputs

`None`

#### Output

`const pluss::Series&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.as_series();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `as_timeindex`

#### Signature

```cpp
Stream::as_timeindex() -> const pluss::TimeIndex&
```

#### Notes

Interprets or converts the current payload as a time-index stream.

#### Inputs

`None`

#### Output

`const pluss::TimeIndex&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}));

auto out = stream.as_timeindex();
```

#### Expected output

```text
Timeline([...])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `as_timeline`

#### Signature

```cpp
Stream::as_timeline() -> const pluss::Timeline&
```

#### Notes

Interprets or converts the current payload as a timeline stream.

#### Inputs

`None`

#### Output

`const pluss::Timeline&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z", "2024-01-01T10:05:00Z"}));

auto out = stream.as_timeline();
```

#### Expected output

```text
Timeline([...])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `astype`

#### Signature

```cpp
Stream::astype(dtype: pluss::ScalarType) -> Stream
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`dtype` : `pluss::ScalarType`
    Target scalar dtype used by this conversion overload.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.astype(pluss::ScalarType::Float64);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `bfill`

#### Signature

```cpp
Stream::bfill() -> Stream
```

#### Notes

Backward-fills one stream while preserving its case labels and stream-level structure.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0, 0.0}));
stream = stream.with_validity_mask(pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool));

auto out = stream.bfill();
```

#### Expected output

```text
Series([10.0, 30.0, 30.0, null])
```

#### Practical Behavior

- `bfill()` applies thread-level backward fill while preserving the stream case labels.
- Use this when you want the fill behavior of `Thread::bfill()` but you are already operating in Kindle/Stream workflows.


### `cases`

#### Signature

```cpp
Stream::cases() -> const std::optional<pluss::table::Thread>&
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`None`

#### Output

`const std::optional<pluss::table::Thread>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.cases();
```

#### Expected output

```text
["cs0", "cs1", "cs2"]
```

### `cast`

#### Signature

```cpp
Stream::cast(target: pluss::table::ThreadCastTarget, series_dtype: std::optional<pluss::ScalarType> = std::nullopt) -> Stream
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`target` : `pluss::table::ThreadCastTarget`
    Target payload family chosen by this cast overload.

`series_dtype` : `std::optional<pluss::ScalarType>`
    Optional scalar dtype used when the cast target is a numeric `Series`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"1.0", "2.0", "3.0"}));

auto out = stream.cast(pluss::table::ThreadCastTarget::Series, pluss::ScalarType::Float64);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

#### Conversion Rules

- These methods convert the underlying payload while keeping stream-level structure such as cases and metadata in view.
- `astype(...)` is the direct numeric-series dtype path.


### `clear_cases`

#### Signature

```cpp
Stream::clear_cases() -> Stream
```

#### Notes

Returns a stream with its case labels removed.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.clear_cases();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `clear_cases_`

#### Signature

```cpp
Stream::clear_cases_() -> Stream&
```

#### Notes

Removes the stream case labels in place.

#### Inputs

`None`

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.clear_cases_();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clear_validity_mask`

#### Signature

```cpp
Stream::clear_validity_mask() -> Stream
```

#### Notes

Returns a stream with the validity mask removed, so all entries are treated as valid again.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.clear_validity_mask();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `clear_validity_mask_`

#### Signature

```cpp
Stream::clear_validity_mask_() -> Stream&
```

#### Notes

Removes the validity mask in place, so all entries are treated as valid again.

#### Inputs

`None`

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.clear_validity_mask_();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clip`

#### Signature

```cpp
Stream::clip(min_value: double, max_value: double) -> Stream
```

#### Notes

Clamps numeric stream values to the supplied lower and upper bounds.

#### Inputs

`min_value` : `scalar`
    Lower/upper boundary used by this overload.

`max_value` : `scalar`
    Lower/upper boundary used by this overload.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.clip(0.0, 25.0);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `defined`

#### Signature

```cpp
Stream::defined() -> bool
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
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.defined();
```

#### Expected output

```text
true
```

### `drop_meta`

#### Signature

```cpp
Stream::drop_meta(key: std::string_view) -> Stream
```

#### Notes

Returns a stream with one metadata key removed.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
stream = stream.with_meta("unit", "km");

auto out = stream.drop_meta("unit");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- These methods remove one metadata key only.
- Payload values, cases, and validity remain unchanged.


### `drop_meta_`

#### Signature

```cpp
Stream::drop_meta_(key: std::string_view) -> Stream&
```

#### Notes

Removes one metadata key in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
stream = stream.with_meta("unit", "km");

stream.drop_meta_("unit");
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- These methods remove one metadata key only.
- Payload values, cases, and validity remain unchanged.


### `dropna`

#### Signature

```cpp
Stream::dropna() -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0}));
stream = stream.with_validity_mask(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));

auto out = stream.dropna();
```

#### Expected output

```text
Series([10.0, 30.0])
```

#### Practical Behavior

- `dropna()` removes invalid entries from the stream while preserving stream semantics for the remaining entries.
- Use it when rows with null payload values should be removed entirely.


### `dtype_name`

#### Signature

```cpp
Stream::dtype_name() -> std::string
```

#### Notes

Returns the scalar dtype name used by the current numeric payload.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.dtype_name();
```

#### Expected output

```text
Float64
```

### `exp`

#### Signature

```cpp
Stream::exp() -> Stream
```

#### Notes

Applies the elementwise exponential to a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.exp();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `ffill`

#### Signature

```cpp
Stream::ffill() -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.ffill();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `fillna`

#### Signature

```cpp
Stream::fillna(value: const char*) -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::fillna(value: double) -> Stream`
- `Stream::fillna(value: std::string_view) -> Stream`
- `Stream::fillna(value: const pluss::Timeline&) -> Stream`
- `Stream::fillna(value: const pluss::TimeIndex&) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = stream.fillna("Unknown");
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Stream::fillna(value: const pluss::TimeIndex&) -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::TimeIndex&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::fillna(value: double) -> Stream`
- `Stream::fillna(value: std::string_view) -> Stream`
- `Stream::fillna(value: const char*) -> Stream`
- `Stream::fillna(value: const pluss::Timeline&) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}));

auto out = stream.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Stream::fillna(value: const pluss::Timeline&) -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::Timeline&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::fillna(value: double) -> Stream`
- `Stream::fillna(value: std::string_view) -> Stream`
- `Stream::fillna(value: const char*) -> Stream`
- `Stream::fillna(value: const pluss::TimeIndex&) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z", "2024-01-01T10:05:00Z"}));

auto out = stream.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Stream::fillna(value: double) -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::fillna(value: std::string_view) -> Stream`
- `Stream::fillna(value: const char*) -> Stream`
- `Stream::fillna(value: const pluss::Timeline&) -> Stream`
- `Stream::fillna(value: const pluss::TimeIndex&) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.fillna(0.0);
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Stream::fillna(value: std::string_view) -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::fillna(value: double) -> Stream`
- `Stream::fillna(value: const char*) -> Stream`
- `Stream::fillna(value: const pluss::Timeline&) -> Stream`
- `Stream::fillna(value: const pluss::TimeIndex&) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = stream.fillna("Unknown");
```

#### Expected output

```text
Note(["Accra", "Tema", "Kumasi"])
```

### `filter`

#### Signature

```cpp
Stream::filter(bool_mask: const pluss::Series&) -> Stream
```

#### Notes

Keeps only the entries where the supplied boolean mask is true.

#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = stream.filter(mask);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `from_thread`

#### Signature

```cpp
Stream::from_thread(thread: const pluss::table::Thread&, cases: std::optional<pluss::table::Thread> = std::nullopt) -> Stream
```

#### Notes

Builds a Kindle stream from a table thread.

#### Inputs

`thread` : `const pluss::table::Thread&`
    Thread payload supplied to this overload.

`cases` : `std::optional<pluss::table::Thread>`
    Case-label thread to attach to the current object.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = pluss::kindle::Stream::from_thread(thread);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `has_cases`

#### Signature

```cpp
Stream::has_cases() -> bool
```

#### Notes

Reports whether the stream currently carries explicit case labels.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.has_cases();
```

#### Expected output

```text
true
```

### `has_meta`

#### Signature

```cpp
Stream::has_meta(key: std::string_view) -> bool
```

#### Notes

Reports whether a metadata key exists on the stream.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream = stream.with_meta("unit", "km");
auto out = stream.has_meta("unit");
```

#### Expected output

```text
true
```

### `has_validity_mask`

#### Signature

```cpp
Stream::has_validity_mask() -> bool
```

#### Notes

Reports whether the stream currently carries a validity mask.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.has_validity_mask();
```

#### Expected output

```text
true
```

### `head`

#### Signature

```cpp
Stream::head(n: std::int64_t) -> Stream
```

#### Notes

Returns the first `n` entries of the stream.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.head();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `isnull`

#### Signature

```cpp
Stream::isnull() -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.isnull();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `kind`

#### Signature

```cpp
Stream::kind() -> StreamKind
```

#### Notes

Returns which payload kind this stream currently stores, such as Series, Note, Timeline, or TimeIndex.

#### Inputs

`None`

#### Output

`StreamKind`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.kind();
```

#### Expected output

```text
Series
```

### `log`

#### Signature

```cpp
Stream::log() -> Stream
```

#### Notes

Applies the elementwise natural logarithm to a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.log();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `max`

#### Signature

```cpp
Stream::max() -> Stream
```

#### Notes

Returns the maximum value of a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.max();
```

#### Expected output

```text
Series([20.0])
```

### `mean`

#### Signature

```cpp
Stream::mean() -> Stream
```

#### Notes

Returns the mean value of a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.mean();
```

#### Expected output

```text
Series([20.0])
```

### `meta`

#### Signature

```cpp
Stream::meta() -> const std::unordered_map<std::string, std::string>&
```

#### Notes

Returns the value stored under one metadata key.

#### Inputs

`None`

#### Output

`const std::unordered_map<std::string, std::string>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::meta(key: std::string_view) -> std::optional<std::string>`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream = stream.with_meta("unit", "km");
auto out = stream.meta("unit");
```

#### Expected output

```text
km
```

### `meta`

#### Signature

```cpp
Stream::meta(key: std::string_view) -> std::optional<std::string>
```

#### Notes

Returns the value stored under one metadata key.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`std::optional<std::string>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::meta() -> const std::unordered_map<std::string, std::string>&`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream = stream.with_meta("unit", "km");
auto out = stream.meta("unit");
```

#### Expected output

```text
km
```

### `min`

#### Signature

```cpp
Stream::min() -> Stream
```

#### Notes

Returns the minimum value of a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.min();
```

#### Expected output

```text
Series([20.0])
```

### `notnull`

#### Signature

```cpp
Stream::notnull() -> Stream
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.notnull();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `show`

#### Signature

```cpp
Stream::show(os: std::ostream&) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.show(std::cout);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `size`

#### Signature

```cpp
Stream::size() -> std::int64_t
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
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.size();
```

#### Expected output

```text
3
```

### `slice`

#### Signature

```cpp
Stream::slice(start: std::int64_t, stop: std::int64_t) -> Stream
```

#### Notes

Returns a contiguous slice `[start, stop)` of the stream.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.slice(0, 2);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `std`

#### Signature

```cpp
Stream::std(unbiased: bool = true) -> Stream
```

#### Notes

Returns the standard deviation of a numeric stream.

#### Inputs

`unbiased` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.std(true);
```

#### Expected output

```text
Series([20.0])
```

### `sum`

#### Signature

```cpp
Stream::sum() -> Stream
```

#### Notes

Returns the sum of a numeric stream.

#### Inputs

`None`

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.sum();
```

#### Expected output

```text
Series([20.0])
```

### `tail`

#### Signature

```cpp
Stream::tail(n: std::int64_t) -> Stream
```

#### Notes

Returns the last `n` entries of the stream.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.tail();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `take`

#### Signature

```cpp
Stream::take(indices: std::span<const std::int64_t>) -> Stream
```

#### Notes

Returns the entries at the requested integer positions.

#### Inputs

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::vector<std::int64_t> picks{0, 2};
auto out = stream.take(picks);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `to_json`

#### Signature

```cpp
Stream::to_json() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.to_json();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `to_string`

#### Signature

```cpp
Stream::to_string() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.to_string();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `to_thread`

#### Signature

```cpp
Stream::to_thread() -> pluss::table::Thread
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::table::Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::to_thread(start: std::int64_t, stop: std::int64_t) -> pluss::table::Thread`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.to_thread();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `to_thread`

#### Signature

```cpp
Stream::to_thread(start: std::int64_t, stop: std::int64_t) -> pluss::table::Thread
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`pluss::table::Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::to_thread() -> pluss::table::Thread`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.to_thread(0, 2);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `unique`

#### Signature

```cpp
Stream::unique() -> bool
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
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.unique();
```

#### Expected output

```text
true
```

### `validity_mask`

#### Signature

```cpp
Stream::validity_mask() -> const std::optional<pluss::Series>&
```

#### Notes

Returns the current validity mask as a boolean series.

#### Inputs

`None`

#### Output

`const std::optional<pluss::Series>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.validity_mask();
```

#### Expected output

```text
[true, false, true]
```

### `var`

#### Signature

```cpp
Stream::var(unbiased: bool = true) -> Stream
```

#### Notes

Returns the variance of a numeric stream.

#### Inputs

`unbiased` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.var(true);
```

#### Expected output

```text
Series([20.0])
```

### `with_cases`

#### Signature

```cpp
Stream::with_cases(cases: const pluss::table::Thread&) -> Stream
```

#### Notes

Returns a new stream with the supplied case labels attached.

#### Inputs

`cases` : `const pluss::table::Thread&`
    Case-label thread to attach to the current object.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.with_cases(cases);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `with_cases_`

#### Signature

```cpp
Stream::with_cases_(cases: const pluss::table::Thread&) -> Stream&
```

#### Notes

Attaches the supplied case labels in place.

#### Inputs

`cases` : `const pluss::table::Thread&`
    Case-label thread to attach to the current object.

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.with_cases_(cases);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_meta`

#### Signature

```cpp
Stream::with_meta(items: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Stream
```

#### Notes

Returns a new stream with one metadata key/value attached.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    Inline collection passed directly at the call site.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::with_meta(key: std::string, value: std::string) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.with_meta("unit", "km");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `with_meta`

#### Signature

```cpp
Stream::with_meta(key: std::string, value: std::string) -> Stream
```

#### Notes

Returns a new stream with one metadata key/value attached.

#### Inputs

`key` : `std::string`
    Name of the target slot or stream.

`value` : `std::string`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::with_meta(items: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Stream`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.with_meta("unit", "km");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `with_meta_`

#### Signature

```cpp
Stream::with_meta_(items: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Stream&
```

#### Notes

Adds or updates one metadata key/value in place.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    Inline collection passed directly at the call site.

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::with_meta_(key: std::string, value: std::string) -> Stream&`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.with_meta_("unit", "km");
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_meta_`

#### Signature

```cpp
Stream::with_meta_(key: std::string, value: std::string) -> Stream&
```

#### Notes

Adds or updates one metadata key/value in place.

#### Inputs

`key` : `std::string`
    Name of the target slot or stream.

`value` : `std::string`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Stream::with_meta_(items: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Stream&`

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.with_meta_("unit", "km");
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_validity_mask`

#### Signature

```cpp
Stream::with_validity_mask(mask: const pluss::Series&) -> Stream
```

#### Notes

Returns a new stream with the supplied validity mask attached.

#### Inputs

`mask` : `const pluss::Series&`
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.with_validity_mask(mask);
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `with_validity_mask_`

#### Signature

```cpp
Stream::with_validity_mask_(mask: const pluss::Series&) -> Stream&
```

#### Notes

Attaches the supplied validity mask in place.

#### Inputs

`mask` : `const pluss::Series&`
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Output

`Stream&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.with_validity_mask_(mask);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```
