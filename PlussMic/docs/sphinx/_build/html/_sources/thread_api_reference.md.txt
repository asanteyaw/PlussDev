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

Returns a numeric thread whose values are the absolute values of the current numeric payload.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.abs();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `all`

#### Signature

```cpp
pluss::Series all(self, bool skipna = true) const
```

#### Parameters

`skipna` : `bool`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Applies `all` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.all();
```

### `any`

#### Signature

```cpp
pluss::Series any(self, bool skipna = true) const
```

#### Parameters

`skipna` : `bool`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Applies `any` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.any();
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

Interprets or converts the current payload as a note/text thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.as_note();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


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

Interprets or converts the current payload as a numeric series thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.as_series();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


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

Interprets or converts the current payload as a time-index thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeindex();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


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

Interprets or converts the current payload as a timeline thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeline();
```

#### Output

```text
["2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


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

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


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

Backward-fills one thread by pulling the next valid value upward within the same payload.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0, 0.0}));
thread = thread.with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool));

auto out = thread.bfill();
```

#### Output

```text
[10.0, 30.0, 30.0, null]
```

#### Practical Behavior

- `bfill()` works within one column.
- Each null entry pulls the next valid value below it.
- Trailing null runs remain null because there is no later value to use.
- Use this when only one feature should be filled instead of the whole frame.


### `btw`

#### Signature

```cpp
Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const
```

#### Parameters

`lo` : `const Thread&`
    See the signature for the supported argument form.

`hi` : `const Thread&`
    See the signature for the supported argument form.

`include_left` : `bool`
    See the signature for the supported argument form.

`include_right` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool inclusive = false) const`
- `Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.btw(lo, hi, true);
```

#### Output

```text
[true, true, false]
```

#### Boundary Rules

- This overload is row-wise: each row is compared against its own lower and upper bound.
- `lo` and `hi` must be defined and must match the size of the current thread.
- Use this when the allowable range changes per row instead of using one scalar interval for the whole column.


### `btw`

#### Signature

```cpp
Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const
```

#### Parameters

`lo` : `const Thread&`
    See the signature for the supported argument form.

`hi` : `const Thread&`
    See the signature for the supported argument form.

`inclusive` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool inclusive = false) const`
- `Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.btw(lo, hi, true);
```

#### Output

```text
[true, true, false]
```

#### Boundary Rules

- This overload is row-wise: each row is compared against its own lower and upper bound.
- `lo` and `hi` must be defined and must match the size of the current thread.
- Use this when the allowable range changes per row instead of using one scalar interval for the whole column.


### `btw`

#### Signature

```cpp
Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const
```

#### Parameters

`a` : `scalar`
    See the signature for the supported argument form.

`b` : `scalar`
    See the signature for the supported argument form.

`include_left` : `bool`
    See the signature for the supported argument form.

`include_right` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool inclusive = false) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.btw(50.0, 100.0, true);
```

#### Output

```text
[false, true, true, false]
```

#### Boundary Rules

- This overload is for numeric `Series` payloads only.
- If `a > b`, the implementation swaps the bounds for you.
- When bounds are swapped, the left/right inclusivity flags are swapped too, so the semantics stay consistent.
- Use `inclusive=true` when both ends should be closed, or the four-argument form when each side needs different behavior.

#### Common Pattern

```cpp
auto passing = scores.btw(50.0, 100.0, true);
auto strict_band = scores.btw(50.0, 100.0, false, true);
```


### `btw`

#### Signature

```cpp
Thread btw(self, scalar a, scalar b, bool inclusive = false) const
```

#### Parameters

`a` : `scalar`
    See the signature for the supported argument form.

`b` : `scalar`
    See the signature for the supported argument form.

`inclusive` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.btw(50.0, 100.0, true);
```

#### Output

```text
[false, true, true, false]
```

#### Boundary Rules

- This overload is for numeric `Series` payloads only.
- If `a > b`, the implementation swaps the bounds for you.
- When bounds are swapped, the left/right inclusivity flags are swapped too, so the semantics stay consistent.
- Use `inclusive=true` when both ends should be closed, or the four-argument form when each side needs different behavior.

#### Common Pattern

```cpp
auto passing = scores.btw(50.0, 100.0, true);
auto strict_band = scores.btw(50.0, 100.0, false, true);
```


### `btw`

#### Signature

```cpp
Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const
```

#### Parameters

`lo` : `std::string_view`
    See the signature for the supported argument form.

`hi` : `std::string_view`
    See the signature for the supported argument form.

`include_left` : `bool`
    See the signature for the supported argument form.

`include_right` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool inclusive = false) const`
- `Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

#### Output

```text
[true, true, false]
```

#### Boundary Rules

- This overload is for `Timeline` and `TimeIndex` payloads.
- The string bounds are parsed as timeline values.
- Reversed bounds are accepted; the implementation swaps them before comparison.
- This is the range-filter form to use for date/time columns.

#### Common Pattern

```cpp
auto january = ts.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```


### `btw`

#### Signature

```cpp
Thread btw(self, std::string_view lo, std::string_view hi, bool inclusive = true) const
```

#### Parameters

`lo` : `std::string_view`
    See the signature for the supported argument form.

`hi` : `std::string_view`
    See the signature for the supported argument form.

`inclusive` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### See also

- `Thread btw(self, scalar a, scalar b, bool inclusive = false) const`
- `Thread btw(self, scalar a, scalar b, bool include_left, bool include_right) const`
- `Thread btw(self, std::string_view lo, std::string_view hi, bool include_left, bool include_right) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool inclusive = true) const`
- `Thread btw(self, const Thread& lo, const Thread& hi, bool include_left, bool include_right) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

#### Output

```text
[true, true, false]
```

#### Boundary Rules

- This overload is for `Timeline` and `TimeIndex` payloads.
- The string bounds are parsed as timeline values.
- Reversed bounds are accepted; the implementation swaps them before comparison.
- This is the range-filter form to use for date/time columns.

#### Common Pattern

```cpp
auto january = ts.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```


### `bucketize`

#### Signature

```cpp
Thread bucketize(self, pluss::msharp::RangeView::Double bins, pluss::msharp::RangeView::Int labels =
```

#### Parameters

`bins` : `pluss::msharp::RangeView::Double`
    See the signature for the supported argument form.

`labels` : `pluss::msharp::RangeView::Int`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Places numeric values into interval bins and returns the assigned bin labels as a new thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({12.0, 18.0, 27.0, 35.0, 0.0}));
thread = thread.with_validity_mask(pluss::make_series({true, true, true, true, false}, pluss::codem::ScalarType::Bool));
std::vector<double> bins{0.0, 20.0, 30.0, 40.0};

auto out = thread.bucketize(bins, {}, true, true);
```

#### Output

```text
[0, 0, 1, 2, null]
```

#### Practical Behavior

- `bucketize(...)` is only valid for numeric `Series` payloads.
- `bins` must contain at least two edges, must not contain `NaN`, and must be strictly increasing.
- If `labels` is empty, the method uses `0, 1, 2, ...` for the bucket labels.
- If `labels` is provided, its size must be exactly `bins.size() - 1`.
- Values that do not land in any interval remain invalid in the output rather than being forced into a bucket.

#### Boundary Rules

- With `right=true`, intervals are right-closed by default: `(a, b]`.
- With `right=false`, intervals are left-closed and right-open: `[a, b)`.
- `include_lowest=true` makes the first bucket include its left edge even when `right=true`.


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

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


### `clear_display_labels`

#### Signature

```cpp
Thread clear_display_labels(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `clear_display_labels` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_display_labels();
```

### `clear_display_name`

#### Signature

```cpp
Thread clear_display_name(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `clear_display_name` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_display_name();
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

Returns a thread with the validity mask removed, so all entries are treated as valid again.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_validity_mask();
```

#### Practical Behavior

- `clear_validity_mask()` keeps the payload values but drops null-tracking, so every entry becomes valid again.
- Use this when null semantics should be discarded rather than filled.


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

Removes the validity mask in place, so all entries are treated as valid again.

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

Clamps numeric values to the supplied lower and upper bounds.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clip(0.0, 25.0);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `clip(min, max)` clamps numeric series values into the closed interval `[min, max]`.
- The validity mask is preserved.


### `compare`

#### Signature

```cpp
Thread compare(self, const char* other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const char*`
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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


### `compare`

#### Signature

```cpp
Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const pluss::Note&`
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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


### `compare`

#### Signature

```cpp
Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const pluss::Series&`
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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


### `compare`

#### Signature

```cpp
Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const pluss::TimeIndex&`
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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


### `compare`

#### Signature

```cpp
Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `const pluss::Timeline&`
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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


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
- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));
auto out = thread.compare(other, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, false]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


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

- `Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


### `compare`

#### Signature

```cpp
Thread compare(self, std::string_view other, ThreadBinaryCompareOp op) const
```

#### Parameters

`other` : `std::string_view`
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
- `Thread compare(self, const char* other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Series& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Note& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::Timeline& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const`
- `Thread compare(self, const Thread& other, ThreadBinaryCompareOp op) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Output

```text
[false, true, true]
```

#### Comparison Rules

- Scalar/native comparisons broadcast the right-hand value across the whole thread.
- Thread-vs-thread comparisons are row-wise and require compatible sizes.
- Text and temporal comparisons route through the payload-specific kernels rather than forcing everything through numeric conversion.


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

Compares two positions inside the same thread and returns the comparison result.

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

auto out = thread.contains("Acc");
```

#### Output

```text
[true, false, false]
```

#### Practical Behavior

- `contains(needle)` is only valid for note-backed threads.
- It returns a boolean series telling you which text entries contain the requested substring.


### `corr`

#### Signature

```cpp
scalar corr(const Thread& other, ThreadCorrMethod method = ThreadCorrMethod::Pearson) const
```

#### Parameters

`other` : `const Thread&`
    See the signature for the supported argument form.

`method` : `ThreadCorrMethod`
    See the signature for the supported argument form.

#### Returns

`double`
    Value produced by this call.

#### Notes

Applies `corr` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({12.0, 18.0, 29.0}));

auto out = thread.corr(other, pluss::table::ThreadCorrMethod::Pearson);
```

#### Output

```text
0.9819805
```

#### Practical Behavior

- `corr(other, method)` requires both threads to be numeric series of the same size.
- Invalid, null, or non-finite pairs are removed before the statistic is computed.
- If too few valid paired values remain, the result is `NaN`.


### `count`

#### Signature

```cpp
pluss::Series count(self, bool count_true = true) const
```

#### Parameters

`count_true` : `bool`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.count(true);
```

#### Output

```text
[3]
```

#### Practical Behavior

- For numeric/text/temporal threads, `count(true)` returns the number of valid entries.
- For boolean series, the underlying series count kernel is used.
- `count(false)` is only meaningful on boolean series payloads.


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

#### Practical Behavior

- `day()` is valid only for timeline/timeindex payloads.
- The result preserves the validity mask while projecting the day component.


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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.defined();
```

#### Output

```text
true
```

### `describe`

#### Signature

```cpp
ThreadView describe(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`ThreadView`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.describe(false);
```

#### Output

```text
count  3
mean   20.0
std    10.0
min    10.0
25%    15.0
50%    20.0
75%    25.0
max    30.0
```

#### Practical Behavior

- Numeric series are summarized through the table-style numeric describe path.
- Note payloads produce count/unique/top/freq style summaries.
- Other payload kinds are not supported by `describe()`.


### `display_labels`

#### Signature

```cpp
Thread display_labels(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `display_labels` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_labels(pluss::table::Thread(pluss::make_note({"a", "b", "c"})));

auto out = thread.display_labels();
```

#### Output

```text
["a", "b", "c"]
```

#### Practical Behavior

- These accessors expose display metadata attached to the thread.
- They do not change the payload values or validity mask.


### `display_name`

#### Signature

```cpp
std::string display_name(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Applies `display_name` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_name("sales");

auto out = thread.display_name();
```

#### Output

```text
sales
```

#### Practical Behavior

- These accessors expose display metadata attached to the thread.
- They do not change the payload values or validity mask.


### `distinct`

#### Signature

```cpp
Thread distinct(self, bool dropna = true) const
```

#### Parameters

`dropna` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `distinct` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Accra"}));

auto out = thread.distinct(true);
```

#### Output

```text
["Accra", "Tema"]
```

#### Practical Behavior

- `distinct(dropna)` keeps the first occurrence of each distinct value in encounter order.
- With `dropna=false`, at most one null entry is retained.


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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0}));
thread = thread.with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));

auto out = thread.dropna();
```

#### Output

```text
[10.0, 30.0]
```

#### Practical Behavior

- `dropna()` removes invalid entries from the thread and compacts the result.
- Use this when null rows should disappear rather than be filled.


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

Returns the scalar dtype name used by the current numeric payload.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.dtype_name();
```

#### Output

```text
Float64
```

#### Practical Behavior

- `dtype_name()` reports the scalar dtype used by a numeric series payload.
- Use it when debugging type-dependent behavior or conversion steps.


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

Applies the elementwise exponential to a numeric thread.

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

Keeps only the entries where the supplied boolean mask is true.

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

### `has_display_labels`

#### Signature

```cpp
bool has_display_labels(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Applies `has_display_labels` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_display_labels();
```

### `has_display_name`

#### Signature

```cpp
bool has_display_name(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Applies `has_display_name` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_display_name();
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

Reports whether the thread currently carries a validity mask.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_validity_mask();
```

### `head`

#### Signature

```cpp
ThreadView head(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`ThreadView`
    Value produced by this call.

#### Notes

Returns the first `n` entries of the thread.

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

### `info`

#### Signature

```cpp
void info(self) const
```

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void info(self, std::ostream& os) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info();
```

### `info`

#### Signature

```cpp
void info(self, std::ostream& os) const
```

#### Parameters

`os` : `std::ostream&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void info(self) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info();
```

### `info_string`

#### Signature

```cpp
std::string info_string(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info_string();
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

Checks whether the value at one position is marked valid by the thread validity mask.

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
Thread isfinite(self) const
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

auto out = thread.isfinite();
```

#### Output

```text
[true, false, true]
```

### `isnan`

#### Signature

```cpp
Thread isnan(self) const
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

auto out = thread.isnan();
```

#### Output

```text
[true, false, true]
```

### `isnull`

#### Signature

```cpp
Thread isnull(self) const
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

Returns which payload kind this thread currently stores, such as Series, Note, Timeline, or TimeIndex.

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

Applies the elementwise natural logarithm to a numeric thread.

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

### `lstrip`

#### Signature

```cpp
Thread lstrip(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.

#### See also

- `Thread lstrip(self, std::string_view chars) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lstrip();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `lstrip`

#### Signature

```cpp
Thread lstrip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.

#### See also

- `Thread lstrip(self) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.lstrip("#");
```

#### Output

```text
["203930", "7788", "4455##"]
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

Returns the maximum value of a numeric thread.

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

Returns the mean value of a numeric thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.mean();
```

#### Output

```text
20.0
```

### `median`

#### Signature

```cpp
pluss::Series median(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Applies `median` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.median();
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

Returns the minimum value of a numeric thread.

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

### `notnull`

#### Signature

```cpp
Thread notnull(self) const
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

### `rstrip`

#### Signature

```cpp
Thread rstrip(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.

#### See also

- `Thread rstrip(self, std::string_view chars) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.rstrip();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `rstrip`

#### Signature

```cpp
Thread rstrip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.

#### See also

- `Thread rstrip(self) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.rstrip("#");
```

#### Output

```text
["#203930", "##7788", "4455"]
```

### `shift`

#### Signature

```cpp
Thread shift(self, std::int64_t periods = 1) const
```

#### Parameters

`periods` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `shift` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shift();
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

Returns a size/count summary for the current object without changing it.

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
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Returns a contiguous slice `[start, stop)` of the thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.slice(0, 2);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `sort_values`

#### Signature

```cpp
Thread sort_values(self, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `sort_values` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sort_values();
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

Returns the standard deviation of a numeric thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

#### Output

```text
20.0
```

### `strip`

#### Signature

```cpp
Thread strip(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.

#### See also

- `Thread strip(self, std::string_view chars) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip();
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
```

### `strip`

#### Signature

```cpp
Thread strip(self, std::string_view chars) const
```

#### Parameters

`chars` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.

#### See also

- `Thread strip(self) const`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip("# ");
```

#### Output

```text
["Accra", "Tema", "Kumasi"]
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

Returns the sum of a numeric thread.

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
ThreadView tail(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`ThreadView`
    Value produced by this call.

#### Notes

Returns the last `n` entries of the thread.

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

Returns the entries at the requested integer positions.

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

### `tally`

#### Signature

```cpp
DataFrameView tally(self, std::int64_t top_n = 0, bool dropna = true, bool render = true) const
```

#### Parameters

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrameView`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tally();
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

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.to_string();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `ts`

#### Signature

```cpp
TimeSeriesAccessor ts(self) const
```

#### Parameters

`None`

#### Returns

`TimeSeriesAccessor`
    Value produced by this call.

#### Notes

Applies `ts` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ts();
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

Returns the current validity mask as a boolean series.

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

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.value();
```

#### Output

```text
10.0
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

Returns the variance of a numeric thread.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.var(true);
```

#### Output

```text
20.0
```

### `view`

#### Signature

```cpp
ThreadView view(self, std::int64_t start, std::int64_t stop, bool render = true) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`ThreadView`
    Value produced by this call.

#### Notes

Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.view();
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

Computes a window-based transformation such as a rolling mean over the thread values.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `with_display_labels`

#### Signature

```cpp
Thread with_display_labels(self, const Thread& labels) const
```

#### Parameters

`labels` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `with_display_labels` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_display_labels();
```

### `with_display_name`

#### Signature

```cpp
Thread with_display_name(self, std::string name) const
```

#### Parameters

`name` : `std::string`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Applies `with_display_name` to the current `Thread` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_display_name();
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

Returns a new thread with the supplied validity mask attached.

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

Attaches the supplied validity mask in place.

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
