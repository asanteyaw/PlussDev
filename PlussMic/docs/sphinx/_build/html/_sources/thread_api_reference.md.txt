# Thread API Reference

# Practical Thread API Reference

## API Reference Style

This page documents the full public `Thread` surface with direct typed-column examples.

### `abs`

#### Signature

```cpp
Thread abs(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.abs();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `as_note`

#### Signature

```cpp
const pluss::Note& as_note(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Note&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.as_note();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `as_series`

#### Signature

```cpp
const pluss::Series& as_series(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Series&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.as_series();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `as_timeindex`

#### Signature

```cpp
const pluss::TimeIndex& as_timeindex(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::TimeIndex&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeindex();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

### `as_timeline`

#### Signature

```cpp
const pluss::Timeline& as_timeline(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Timeline&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeline();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

### `astype`

#### Signature

```cpp
Thread astype(self, pluss::codem::ScalarType dtype) const
```

#### Parameters

`dtype` : `pluss::codem::ScalarType`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.astype(pluss::codem::ScalarType::Float64);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `bfill`

#### Signature

```cpp
Thread bfill(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.bfill();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `cast`

#### Signature

```cpp
Thread cast(self, ThreadCastTarget target, std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const
```

#### Parameters

`target` : `ThreadCastTarget`
    See the signature for the supported argument form.

`series_dtype` : `std::optional<pluss::codem::ScalarType>`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"1.0", "2.0", "3.0"}));

auto out = thread.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `clear_validity_mask`

#### Signature

```cpp
Thread clear_validity_mask(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_validity_mask();
```

### `clear_validity_mask_`

#### Signature

```cpp
Thread& clear_validity_mask_(self)
```

#### Parameters

`None`

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.clear_validity_mask_();
```

### `clip`

#### Signature

```cpp
Thread clip(self, scalar min_value, scalar max_value) const
```

#### Parameters

`min_value` : `scalar`
    See the signature for the supported argument form.

`max_value` : `scalar`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clip(0.0, 25.0);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `compare`

#### Signature

```cpp
Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const Thread&`
    See the signature for the supported argument form.

`op` : `ThreadBinaryCompareOp`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `Thread compare(self, scalar other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));
auto out = thread.compare(other, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
1
```

### `compare`

#### Signature

```cpp
Thread compare(self, scalar other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

`op` : `ThreadBinaryCompareOp`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
1
```

### `compare_rows`

#### Signature

```cpp
int compare_rows(self, std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const
```

#### Parameters

`lhs_row` : `std::int64_t`
    See the signature for the supported argument form.

`rhs_row` : `std::int64_t`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`int`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare_rows(0, 1, true);
```

#### Output

```text
1
```

### `contains`

#### Signature

```cpp
pluss::Series contains(self, std::string_view needle) const
```

#### Parameters

`needle` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.contains("Accra");
```

#### Output

```text
[true, false, true]
```

### `day`

#### Signature

```cpp
Thread day(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.day();
```

#### Output

```text
[1, 2, 3]
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

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.defined();
```

#### Output

```text
true
```

### `dropna`

#### Signature

```cpp
Thread dropna(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.dropna();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `dtype_name`

#### Signature

```cpp
std::string dtype_name(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.dtype_name();
```

#### Output

```text
Float64
```

### `exp`

#### Signature

```cpp
Thread exp(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.exp();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `ffill`

#### Signature

```cpp
Thread ffill(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ffill();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread fillna(self, const char* value) const
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread fillna(self, scalar value) const`
- `Thread fillna(self, std::string_view value) const`
- `Thread fillna(self, const pluss::Timeline& value) const`
- `Thread fillna(self, const pluss::TimeIndex& value) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread fillna(self, const pluss::TimeIndex& value) const
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread fillna(self, scalar value) const`
- `Thread fillna(self, std::string_view value) const`
- `Thread fillna(self, const char* value) const`
- `Thread fillna(self, const pluss::Timeline& value) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

auto out = thread.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread fillna(self, const pluss::Timeline& value) const
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread fillna(self, scalar value) const`
- `Thread fillna(self, std::string_view value) const`
- `Thread fillna(self, const char* value) const`
- `Thread fillna(self, const pluss::TimeIndex& value) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread fillna(self, scalar value) const
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread fillna(self, std::string_view value) const`
- `Thread fillna(self, const char* value) const`
- `Thread fillna(self, const pluss::Timeline& value) const`
- `Thread fillna(self, const pluss::TimeIndex& value) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fillna(0.0);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread fillna(self, std::string_view value) const
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread fillna(self, scalar value) const`
- `Thread fillna(self, const char* value) const`
- `Thread fillna(self, const pluss::Timeline& value) const`
- `Thread fillna(self, const pluss::TimeIndex& value) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread& fillna_(self, const char* value)
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread& fillna_(self, scalar value)`
- `Thread& fillna_(self, std::string_view value)`
- `Thread& fillna_(self, const pluss::Timeline& value)`
- `Thread& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

thread.fillna_("Unknown");
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread& fillna_(self, const pluss::TimeIndex& value)
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread& fillna_(self, scalar value)`
- `Thread& fillna_(self, std::string_view value)`
- `Thread& fillna_(self, const char* value)`
- `Thread& fillna_(self, const pluss::Timeline& value)`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

thread.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread& fillna_(self, const pluss::Timeline& value)
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread& fillna_(self, scalar value)`
- `Thread& fillna_(self, std::string_view value)`
- `Thread& fillna_(self, const char* value)`
- `Thread& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

thread.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread& fillna_(self, scalar value)
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread& fillna_(self, std::string_view value)`
- `Thread& fillna_(self, const char* value)`
- `Thread& fillna_(self, const pluss::Timeline& value)`
- `Thread& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.fillna_(0.0);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread& fillna_(self, std::string_view value)
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Thread& fillna_(self, scalar value)`
- `Thread& fillna_(self, const char* value)`
- `Thread& fillna_(self, const pluss::Timeline& value)`
- `Thread& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

thread.fillna_("Unknown");
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `filter`

#### Signature

```cpp
Thread filter(self, const pluss::Series& bool_mask) const
```

#### Parameters

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = thread.filter(mask);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `has_validity_mask`

#### Signature

```cpp
bool has_validity_mask(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_validity_mask();
```

### `head`

#### Signature

```cpp
Thread head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.head(2);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `hour`

#### Signature

```cpp
Thread hour(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.hour();
```

#### Output

```text
[1, 2, 3]
```

### `is_valid`

#### Signature

```cpp
bool is_valid(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.is_valid(0);
```

#### Output

```text
true
```

### `isfinite`

#### Signature

```cpp
pluss::Series isfinite(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isfinite();
```

#### Output

```text
[true, false, true]
```

### `isnan`

#### Signature

```cpp
pluss::Series isnan(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnan();
```

#### Output

```text
[true, false, true]
```

### `isnull`

#### Signature

```cpp
pluss::Series isnull(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnull();
```

#### Output

```text
[true, false, true]
```

### `kind`

#### Signature

```cpp
ThreadKind kind(self) const
```

#### Parameters

`None`

#### Returns

`ThreadKind`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.kind();
```

#### Output

```text
Series
```

### `log`

#### Signature

```cpp
Thread log(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.log();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `lower`

#### Signature

```cpp
Thread lower(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lower();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `max`

#### Signature

```cpp
pluss::Series max(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.max();
```

#### Output

```text
20.0
```

### `mean`

#### Signature

```cpp
pluss::Series mean(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.mean();
```

#### Output

```text
20.0
```

### `min`

#### Signature

```cpp
pluss::Series min(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.min();
```

#### Output

```text
20.0
```

### `month`

#### Signature

```cpp
Thread month(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.month();
```

#### Output

```text
[1, 2, 3]
```

### `note`

#### Signature

```cpp
const pluss::Note& note(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Note&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.note();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `notnull`

#### Signature

```cpp
pluss::Series notnull(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.notnull();
```

#### Output

```text
[true, false, true]
```

### `numeric_binary`

#### Signature

```cpp
Thread numeric_binary(self, const Thread& other, ThreadBinaryMathOp op) const
```

#### Parameters

`other` : `const Thread&`
    See the signature for the supported argument form.

`op` : `ThreadBinaryMathOp`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `Thread numeric_binary(self, scalar other, ThreadBinaryMathOp op, bool reverse = false) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

#### Output

```text
[20.0, 30.0, 40.0]
```

### `numeric_binary`

#### Signature

```cpp
Thread numeric_binary(self, scalar other, ThreadBinaryMathOp op, bool reverse = false) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

`op` : `ThreadBinaryMathOp`
    See the signature for the supported argument form.

`reverse` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `Thread numeric_binary(self, const Thread& other, ThreadBinaryMathOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

#### Output

```text
[20.0, 30.0, 40.0]
```

### `replace`

#### Signature

```cpp
Thread replace(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `series`

#### Signature

```cpp
const pluss::Series& series(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Series&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.series();
```

#### Output

```text
[10.0, 20.0, 30.0]
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

#### Output

```text
[10.0, 20.0, 30.0]
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `size`

#### Signature

```cpp
std::int64_t size(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.size();
```

#### Output

```text
3
```

### `slice`

#### Signature

```cpp
Thread slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.slice(0, 2);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `std`

#### Signature

```cpp
pluss::Series std(self, bool unbiased = true) const
```

#### Parameters

`unbiased` : `bool`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

#### Output

```text
20.0
```

### `sum`

#### Signature

```cpp
pluss::Series sum(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sum();
```

#### Output

```text
20.0
```

### `tail`

#### Signature

```cpp
Thread tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tail(2);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `take`

#### Signature

```cpp
Thread take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

std::vector<std::int64_t> picks{0, 2};
auto out = thread.take(picks);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `timeindex`

#### Signature

```cpp
const pluss::TimeIndex& timeindex(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::TimeIndex&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.timeindex();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

### `timeline`

#### Signature

```cpp
const pluss::Timeline& timeline(self) const
```

#### Parameters

`None`

#### Returns

`const pluss::Timeline&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.timeline();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

### `to_note_weekday`

#### Signature

```cpp
Thread to_note_weekday(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.to_note_weekday();
```

#### Output

```text
[1, 2, 3]
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

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.to_string();
```

#### Output

```text
[10.0, 20.0, 30.0]
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

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.unique();
```

#### Output

```text
true
```

### `validity_mask`

#### Signature

```cpp
const std::optional<pluss::Series>& validity_mask(self) const
```

#### Parameters

`None`

#### Returns

`const std::optional<pluss::Series>&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.validity_mask();
```

### `value`

#### Signature

```cpp
ThreadValue value(self) const
```

#### Parameters

`None`

#### Returns

`ThreadValue`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.value();
```

#### Output

```text
10.0
```

### `values`

#### Signature

```cpp
const Thread& values(self) const
```

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.values();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `var`

#### Signature

```cpp
pluss::Series var(self, bool unbiased = true) const
```

#### Parameters

`unbiased` : `bool`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.var(true);
```

#### Output

```text
20.0
```

### `window`

#### Signature

```cpp
Thread window(self, std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) const
```

#### Parameters

`window` : `std::int64_t`
    See the signature for the supported argument form.

`op` : `ThreadWindowOp`
    See the signature for the supported argument form.

`min_periods` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This call is part of the public `Thread` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `with_validity_mask`

#### Signature

```cpp
Thread with_validity_mask(self, const pluss::Series& validity_mask) const
```

#### Parameters

`validity_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_validity_mask(mask);
```

### `with_validity_mask_`

#### Signature

```cpp
Thread& with_validity_mask_(self, const pluss::Series& validity_mask)
```

#### Parameters

`validity_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Thread&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.with_validity_mask_(mask);
```

### `year`

#### Signature

```cpp
Thread year(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.year();
```

#### Output

```text
[2024, 2024, 2024]
```
