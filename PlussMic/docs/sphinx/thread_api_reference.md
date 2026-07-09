# Thread API Reference

# Practical Thread API Reference

## API Reference Style

This page documents the full public `Thread` surface with direct typed-column examples.

### `abs`

#### Signature

```cpp
Thread::abs() -> Thread
```

#### Notes

Returns a numeric thread whose values are the absolute values of the current numeric payload.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.abs();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `all`

#### Signature

```cpp
Thread::all(skipna: bool = true) -> ThreadValue
```

#### Notes



#### Inputs

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.all();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `any`

#### Signature

```cpp
Thread::any(skipna: bool = true) -> ThreadValue
```

#### Notes



#### Inputs

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.any();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `as_note`

#### Signature

```cpp
Thread::as_note() -> const pluss::Note&
```

#### Notes

Interprets or converts the current payload as a note/text thread.

#### Inputs

`None`

#### Output

`const pluss::Note&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.as_note();
```

#### Expected output

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
Thread::as_series() -> const pluss::Series&
```

#### Notes

Interprets or converts the current payload as a numeric series thread.

#### Inputs

`None`

#### Output

`const pluss::Series&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.as_series();
```

#### Expected output

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
Thread::as_timeindex() -> const pluss::TimeIndex&
```

#### Notes

Interprets or converts the current payload as a time-index thread.

#### Inputs

`None`

#### Output

`const pluss::TimeIndex&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeindex();
```

#### Expected output

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
Thread::as_timeline() -> const pluss::Timeline&
```

#### Notes

Interprets or converts the current payload as a timeline thread.

#### Inputs

`None`

#### Output

`const pluss::Timeline&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.as_timeline();
```

#### Expected output

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
Thread::astype(dtype: pluss::ScalarType) -> Thread
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`dtype` : `pluss::ScalarType`
    Target scalar dtype used by this conversion overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.astype(pluss::ScalarType::Float64);
```

#### Expected output

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
Thread::bfill() -> Thread
```

#### Notes

Backward-fills one thread by pulling the next valid value upward within the same payload.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0, 0.0}));
thread = thread.with_validity_mask(pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool));

auto out = thread.bfill();
```

#### Expected output

```text
[10.0, 30.0, 30.0, null]
```

#### Practical Behavior

- `bfill()` works within one column.
- Each null entry pulls the next valid value below it.
- Trailing null runs remain null because there is no later value to use.
- Use this when only one feature should be filled instead of the whole frame.


### `bounded`

#### Signature

```cpp
Thread::bounded(bounds: const WindowBoundsFn&, statistic: WindowStatistic = WindowStatistic::Mean, options: const WindowOptions& = {}) -> Thread
```

#### Notes



#### Inputs

`bounds` : `const WindowBoundsFn&`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`statistic` : `WindowStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`options` : `const WindowOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.bounded();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `btw`

#### Signature

```cpp
Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`a` : `scalar`
    Lower/upper boundary used by this overload.

`b` : `scalar`
    Lower/upper boundary used by this overload.

`include_left` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`include_right` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.btw(50.0, 100.0, true);
```

#### Expected output

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
Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`a` : `scalar`
    Lower/upper boundary used by this overload.

`b` : `scalar`
    Lower/upper boundary used by this overload.

`inclusive` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.btw(50.0, 100.0, true);
```

#### Expected output

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
Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`lo` : `const Thread&`
    Lower/upper boundary used by this overload.

`hi` : `const Thread&`
    Lower/upper boundary used by this overload.

`include_left` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`include_right` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread`
- `Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.btw(lo, hi, true);
```

#### Expected output

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
Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`lo` : `const Thread&`
    Lower/upper boundary used by this overload.

`hi` : `const Thread&`
    Lower/upper boundary used by this overload.

`inclusive` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread`
- `Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.btw(lo, hi, true);
```

#### Expected output

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
Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`lo` : `std::string_view`
    Lower/upper boundary used by this overload.

`hi` : `std::string_view`
    Lower/upper boundary used by this overload.

`include_left` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`include_right` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread`
- `Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

#### Expected output

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
Thread::btw(lo: std::string_view, hi: std::string_view, inclusive: bool = true) -> Thread
```

#### Notes

Builds a boolean mask telling you whether each row falls between lower and upper bounds.

#### Inputs

`lo` : `std::string_view`
    Lower/upper boundary used by this overload.

`hi` : `std::string_view`
    Lower/upper boundary used by this overload.

`inclusive` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::btw(a: double, b: double, inclusive: bool = false) -> Thread`
- `Thread::btw(a: double, b: double, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: std::string_view, hi: std::string_view, include_left: bool, include_right: bool) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, inclusive: bool = true) -> Thread`
- `Thread::btw(lo: const Thread&, hi: const Thread&, include_left: bool, include_right: bool) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.btw("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

#### Expected output

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
Thread::bucketize(bins: pluss::RangeView<double>, labels: pluss::RangeView<std::int64_t> = {}, right: bool = true, include_lowest: bool = true) -> Thread
```

#### Notes

Places numeric values into interval bins and returns the assigned bin labels as a new thread.

#### Inputs

`bins` : `pluss::RangeView<scalar>`
    Strictly increasing interval edges used to define bucket boundaries.

`labels` : `pluss::RangeView<std::int64_t>`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

`right` : `bool`
    When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.

`include_lowest` : `bool`
    When `true`, the first bucket also includes its left edge.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({12.0, 18.0, 27.0, 35.0, 0.0}));
thread = thread.with_validity_mask(pluss::make_series({true, true, true, true, false}, pluss::ScalarType::Bool));
std::vector<double> bins{0.0, 20.0, 30.0, 40.0};

auto out = thread.bucketize(bins, {}, true, true);
```

#### Expected output

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


### `casefold`

#### Signature

```cpp
Thread::casefold() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.casefold();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `cast`

#### Signature

```cpp
Thread::cast(target: ThreadCastTarget, options: const ThreadCastOptions& = {}) -> Thread
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`target` : `ThreadCastTarget`
    Target payload family chosen by this cast overload.

`options` : `const ThreadCastOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::cast(target: ThreadCastTarget, series_dtype: std::optional<pluss::ScalarType>) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"1.0", "2.0", "3.0"}));

auto out = thread.cast(pluss::table::ThreadCastTarget::Series, pluss::ScalarType::Float64);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


### `cast`

#### Signature

```cpp
Thread::cast(target: ThreadCastTarget, series_dtype: std::optional<pluss::ScalarType>) -> Thread
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`target` : `ThreadCastTarget`
    Target payload family chosen by this cast overload.

`series_dtype` : `std::optional<pluss::ScalarType>`
    Optional scalar dtype used when the cast target is a numeric `Series`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::cast(target: ThreadCastTarget, options: const ThreadCastOptions& = {}) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"1.0", "2.0", "3.0"}));

auto out = thread.cast(pluss::table::ThreadCastTarget::Series, pluss::ScalarType::Float64);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

#### Conversion Rules

- These methods convert or reinterpret the current payload into the requested thread type.
- Use `as_*()` when you want access to the underlying pluss-type surface directly.
- Use `cast(...)` when you need explicit target selection with broader conversion control.
- `astype(...)` is the numeric-series dtype conversion path.


### `chars`

#### Signature

```cpp
Thread::chars(start: std::int64_t, stop: std::optional<std::int64_t> = std::nullopt, step: std::int64_t = 1) -> Thread
```

#### Notes



#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::optional<std::int64_t>`
    Exclusive ending position of the slice.

`step` : `std::int64_t`
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.chars();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clear_display_labels`

#### Signature

```cpp
Thread::clear_display_labels() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_display_labels();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clear_display_name`

#### Signature

```cpp
Thread::clear_display_name() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_display_name();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clear_validity_mask`

#### Signature

```cpp
Thread::clear_validity_mask() -> Thread
```

#### Notes

Returns a thread with the validity mask removed, so all entries are treated as valid again.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_validity_mask();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `clear_validity_mask()` keeps the payload values but drops null-tracking, so every entry becomes valid again.
- Use this when null semantics should be discarded rather than filled.


### `clear_validity_mask_`

#### Signature

```cpp
Thread::clear_validity_mask_() -> Thread&
```

#### Notes

Removes the validity mask in place, so all entries are treated as valid again.

#### Inputs

`None`

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.clear_validity_mask_();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clip`

#### Signature

```cpp
Thread::clip(min_value: double, max_value: double) -> Thread
```

#### Notes

Clamps numeric values to the supplied lower and upper bounds.

#### Inputs

`min_value` : `scalar`
    Lower/upper boundary used by this overload.

`max_value` : `scalar`
    Lower/upper boundary used by this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clip(0.0, 25.0);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `clip(min, max)` clamps numeric series values into the closed interval `[min, max]`.
- The validity mask is preserved.


### `compare`

#### Signature

```cpp
Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const char*`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const pluss::Note&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const pluss::Series&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const pluss::TimeIndex&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const pluss::Timeline&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));
auto out = thread.compare(other, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare(other: std::string_view, op: ThreadBinaryCompareOp) -> Thread
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `std::string_view`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryCompareOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::compare(other: double, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const char*, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Series&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Note&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::Timeline&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const pluss::TimeIndex&, op: ThreadBinaryCompareOp) -> Thread`
- `Thread::compare(other: const Thread&, op: ThreadBinaryCompareOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare(15.0, pluss::table::ThreadBinaryCompareOp::Gt);
```

#### Expected output

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
Thread::compare_rows(lhs_row: std::int64_t, rhs_row: std::int64_t, nulls_last: bool = true) -> int
```

#### Notes

Compares two positions inside the same thread and returns the comparison result.

#### Inputs

`lhs_row` : `std::int64_t`
    Zero-based left-hand row position used in a row-wise comparison.

`rhs_row` : `std::int64_t`
    Zero-based right-hand row position used in a row-wise comparison.

`nulls_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`int`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.compare_rows(0, 1, true);
```

#### Expected output

```text
1
```

### `contains`

#### Signature

```cpp
Thread::contains(needle: std::string_view) -> pluss::Series
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`needle` : `std::string_view`
    Substring or token to search for.

#### Output

`pluss::Series`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.contains("Acc");
```

#### Expected output

```text
[true, false, false]
```

#### Practical Behavior

- `contains(needle)` is only valid for note-backed threads.
- It returns a boolean series telling you which text entries contain the requested substring.


### `corr`

#### Signature

```cpp
Thread::corr(other: const Thread&, method: ThreadCorrMethod = ThreadCorrMethod::Pearson) -> double
```

#### Notes



#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`method` : `ThreadCorrMethod`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`double`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({12.0, 18.0, 29.0}));

auto out = thread.corr(other, pluss::table::ThreadCorrMethod::Pearson);
```

#### Expected output

```text
0.9819805
```

#### Practical Behavior

- `corr(other, method)` requires both threads to be numeric series of the same size.
- Invalid, null, or non-finite pairs are removed before the statistic is computed.
- If too few valid paired values remain, the result is `NaN`.


### `corr_with`

#### Signature

```cpp
Thread::corr_with(other: const Thread&, method: ThreadCorrMethod = ThreadCorrMethod::Pearson, min_count: std::int64_t = 1) -> ThreadValue
```

#### Notes



#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`method` : `ThreadCorrMethod`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.corr_with();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `count`

#### Signature

```cpp
Thread::count(count_true: bool = true) -> ThreadValue
```

#### Notes

Returns a size/count summary for the current object without changing it.

#### Inputs

`count_true` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.count(true);
```

#### Expected output

```text
[3]
```

#### Practical Behavior

- For numeric/text/temporal threads, `count(true)` returns the number of valid entries.
- For boolean series, the underlying series count kernel is used.
- `count(false)` is only meaningful on boolean series payloads.


### `covar`

#### Signature

```cpp
Thread::covar(other: const Thread&, options: const StatisticOptions& = {}) -> ThreadValue
```

#### Notes



#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.covar();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `day`

#### Signature

```cpp
Thread::day() -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.day();
```

#### Expected output

```text
[1, 2, 3]
```

#### Practical Behavior

- `day()` is valid only for timeline/timeindex payloads.
- The result preserves the validity mask while projecting the day component.


### `defined`

#### Signature

```cpp
Thread::defined() -> bool
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.defined();
```

#### Expected output

```text
true
```

### `describe`

#### Signature

```cpp
Thread::describe(render: bool = false) -> ThreadView
```

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Inputs

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`ThreadView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.describe(false);
```

#### Expected output

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
Thread::display_labels() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_labels(pluss::table::Thread(pluss::make_note({"a", "b", "c"})));

auto out = thread.display_labels();
```

#### Expected output

```text
["a", "b", "c"]
```

#### Practical Behavior

- These accessors expose display metadata attached to the thread.
- They do not change the payload values or validity mask.


### `display_name`

#### Signature

```cpp
Thread::display_name() -> std::string
```

#### Notes



#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})).with_display_name("sales");

auto out = thread.display_name();
```

#### Expected output

```text
sales
```

#### Practical Behavior

- These accessors expose display metadata attached to the thread.
- They do not change the payload values or validity mask.


### `distinct`

#### Signature

```cpp
Thread::distinct(dropna: bool = true) -> Thread
```

#### Notes



#### Inputs

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Accra"}));

auto out = thread.distinct(true);
```

#### Expected output

```text
["Accra", "Tema"]
```

#### Practical Behavior

- `distinct(dropna)` keeps the first occurrence of each distinct value in encounter order.
- With `dropna=false`, at most one null entry is retained.


### `dropna`

#### Signature

```cpp
Thread::dropna() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 0.0, 30.0}));
thread = thread.with_validity_mask(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));

auto out = thread.dropna();
```

#### Expected output

```text
[10.0, 30.0]
```

#### Practical Behavior

- `dropna()` removes invalid entries from the thread and compacts the result.
- Use this when null rows should disappear rather than be filled.


### `dtype_name`

#### Signature

```cpp
Thread::dtype_name() -> std::string
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.dtype_name();
```

#### Expected output

```text
Float64
```

#### Practical Behavior

- `dtype_name()` reports the scalar dtype used by a numeric series payload.
- Use it when debugging type-dependent behavior or conversion steps.


### `duplicated`

#### Signature

```cpp
Thread::duplicated(keep_first: bool = true, keep_last: bool = false) -> Thread
```

#### Notes



#### Inputs

`keep_first` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`keep_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.duplicated();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `encode`

#### Signature

```cpp
Thread::encode(encoding: std::string_view = "base64") -> Thread
```

#### Notes



#### Inputs

`encoding` : `std::string_view`
    Text encoding used when reading or writing text-based files.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.encode();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ends`

#### Signature

```cpp
Thread::ends(suffix: std::string_view) -> Thread
```

#### Notes



#### Inputs

`suffix` : `std::string_view`
    Text suffix appended after the existing label or value.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ends();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `exp`

#### Signature

```cpp
Thread::exp() -> Thread
```

#### Notes

Applies the elementwise exponential to a numeric thread.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.exp();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `extract`

#### Signature

```cpp
Thread::extract(pattern: std::string_view, names: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes



#### Inputs

`pattern` : `std::string_view`
    Pattern string used for matching or extraction.

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.extract();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ffill`

#### Signature

```cpp
Thread::ffill() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ffill();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread::fillna(value: const char*) -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna(value: double) -> Thread`
- `Thread::fillna(value: std::string_view) -> Thread`
- `Thread::fillna(value: const pluss::Timeline&) -> Thread`
- `Thread::fillna(value: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread::fillna(value: const pluss::TimeIndex&) -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::TimeIndex&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna(value: double) -> Thread`
- `Thread::fillna(value: std::string_view) -> Thread`
- `Thread::fillna(value: const char*) -> Thread`
- `Thread::fillna(value: const pluss::Timeline&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

auto out = thread.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread::fillna(value: const pluss::Timeline&) -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::Timeline&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna(value: double) -> Thread`
- `Thread::fillna(value: std::string_view) -> Thread`
- `Thread::fillna(value: const char*) -> Thread`
- `Thread::fillna(value: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread::fillna(value: double) -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna(value: std::string_view) -> Thread`
- `Thread::fillna(value: const char*) -> Thread`
- `Thread::fillna(value: const pluss::Timeline&) -> Thread`
- `Thread::fillna(value: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fillna(0.0);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna`

#### Signature

```cpp
Thread::fillna(value: std::string_view) -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna(value: double) -> Thread`
- `Thread::fillna(value: const char*) -> Thread`
- `Thread::fillna(value: const pluss::Timeline&) -> Thread`
- `Thread::fillna(value: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread::fillna_(value: const char*) -> Thread&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna_(value: double) -> Thread&`
- `Thread::fillna_(value: std::string_view) -> Thread&`
- `Thread::fillna_(value: const pluss::Timeline&) -> Thread&`
- `Thread::fillna_(value: const pluss::TimeIndex&) -> Thread&`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

thread.fillna_("Unknown");
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread::fillna_(value: const pluss::TimeIndex&) -> Thread&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::TimeIndex&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna_(value: double) -> Thread&`
- `Thread::fillna_(value: std::string_view) -> Thread&`
- `Thread::fillna_(value: const char*) -> Thread&`
- `Thread::fillna_(value: const pluss::Timeline&) -> Thread&`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

thread.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread::fillna_(value: const pluss::Timeline&) -> Thread&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::Timeline&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna_(value: double) -> Thread&`
- `Thread::fillna_(value: std::string_view) -> Thread&`
- `Thread::fillna_(value: const char*) -> Thread&`
- `Thread::fillna_(value: const pluss::TimeIndex&) -> Thread&`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

thread.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread::fillna_(value: double) -> Thread&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna_(value: std::string_view) -> Thread&`
- `Thread::fillna_(value: const char*) -> Thread&`
- `Thread::fillna_(value: const pluss::Timeline&) -> Thread&`
- `Thread::fillna_(value: const pluss::TimeIndex&) -> Thread&`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.fillna_(0.0);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `fillna_`

#### Signature

```cpp
Thread::fillna_(value: std::string_view) -> Thread&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::fillna_(value: double) -> Thread&`
- `Thread::fillna_(value: const char*) -> Thread&`
- `Thread::fillna_(value: const pluss::Timeline&) -> Thread&`
- `Thread::fillna_(value: const pluss::TimeIndex&) -> Thread&`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

thread.fillna_("Unknown");
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `filter`

#### Signature

```cpp
Thread::filter(bool_mask: const pluss::Series&) -> Thread
```

#### Notes

Keeps only the entries where the supplied boolean mask is true.

#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = thread.filter(mask);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `flow`

#### Signature

```cpp
Thread::flow(fn: const std::function<Thread(const Thread&)>&) -> Thread
```

#### Notes



#### Inputs

`fn` : `const std::function<Thread(const Thread&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.flow();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `fold`

#### Signature

```cpp
Thread::fold(fn: const std::function<Thread(const Thread&)>&, shape: ResultShape = ResultShape::Reduce) -> Thread
```

#### Notes



#### Inputs

`fn` : `const std::function<Thread(const Thread&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

`shape` : `ResultShape`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fold();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `freq`

#### Signature

```cpp
Thread::freq(top_n: std::int64_t = 0, dropna: bool = true) -> Thread
```

#### Notes



#### Inputs

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.freq();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `fullmatch`

#### Signature

```cpp
Thread::fullmatch(pattern: std::string_view) -> Thread
```

#### Notes



#### Inputs

`pattern` : `std::string_view`
    Pattern string used for matching or extraction.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fullmatch();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `has_display_labels`

#### Signature

```cpp
Thread::has_display_labels() -> bool
```

#### Notes



#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_display_labels();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `has_display_name`

#### Signature

```cpp
Thread::has_display_name() -> bool
```

#### Notes



#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_display_name();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `has_validity_mask`

#### Signature

```cpp
Thread::has_validity_mask() -> bool
```

#### Notes

Reports whether the thread currently carries a validity mask.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_validity_mask();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `head`

#### Signature

```cpp
Thread::head(n: std::int64_t = 5, render: bool = false) -> ThreadView
```

#### Notes

Returns the first `n` entries of the thread.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`ThreadView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.head(2);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `hour`

#### Signature

```cpp
Thread::hour() -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.hour();
```

#### Expected output

```text
[1, 2, 3]
```

### `indicate`

#### Signature

```cpp
Thread::indicate(delimiter: std::string_view = "|", prefix: std::string = "") -> DataFrame
```

#### Notes



#### Inputs

`delimiter` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`prefix` : `std::string`
    Text prefix inserted ahead of the existing label or value.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.indicate();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `info`

#### Signature

```cpp
Thread::info() -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`None`

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::info(os: std::ostream&) -> void`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `info`

#### Signature

```cpp
Thread::info(os: std::ostream&) -> void
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

#### See also

- `Thread::info() -> void`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `info_string`

#### Signature

```cpp
Thread::info_string() -> std::string
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info_string();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `is_valid`

#### Signature

```cpp
Thread::is_valid(row: std::int64_t) -> bool
```

#### Notes

Checks whether the value at one position is marked valid by the thread validity mask.

#### Inputs

`row` : `std::int64_t`
    Zero-based row position used by this overload.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.is_valid(0);
```

#### Expected output

```text
true
```

### `isfinite`

#### Signature

```cpp
Thread::isfinite() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isfinite();
```

#### Expected output

```text
[true, false, true]
```

### `isnan`

#### Signature

```cpp
Thread::isnan() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnan();
```

#### Expected output

```text
[true, false, true]
```

### `isnull`

#### Signature

```cpp
Thread::isnull() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnull();
```

#### Expected output

```text
[true, false, true]
```

### `kind`

#### Signature

```cpp
Thread::kind() -> ThreadKind
```

#### Notes

Returns which payload kind this thread currently stores, such as Series, Note, Timeline, or TimeIndex.

#### Inputs

`None`

#### Output

`ThreadKind`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.kind();
```

#### Expected output

```text
Series
```

### `kurtosis`

#### Signature

```cpp
Thread::kurtosis(min_count: std::int64_t = 4) -> ThreadValue
```

#### Notes



#### Inputs

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.kurtosis();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `length`

#### Signature

```cpp
Thread::length() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.length();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `log`

#### Signature

```cpp
Thread::log() -> Thread
```

#### Notes

Applies the elementwise natural logarithm to a numeric thread.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.log();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `lower`

#### Signature

```cpp
Thread::lower() -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lower();
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `lstrip`

#### Signature

```cpp
Thread::lstrip() -> Thread
```

#### Notes

Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::lstrip(chars: std::string_view) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lstrip();
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `lstrip`

#### Signature

```cpp
Thread::lstrip(chars: std::string_view) -> Thread
```

#### Notes

Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.

#### Inputs

`chars` : `std::string_view`
    Characters to trim from the edge of each text value.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::lstrip() -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.lstrip("#");
```

#### Expected output

```text
["203930", "7788", "4455##"]
```

### `map`

#### Signature

```cpp
Thread::map(fn: const std::function<ThreadValue(const ThreadValue&)>&) -> Thread
```

#### Notes

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Inputs

`fn` : `const std::function<ThreadValue(const ThreadValue&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.map();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `matches`

#### Signature

```cpp
Thread::matches(pattern: std::string_view) -> Thread
```

#### Notes



#### Inputs

`pattern` : `std::string_view`
    Pattern string used for matching or extraction.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.matches();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `matmul`

#### Signature

```cpp
Thread::matmul(other: const Thread&) -> ThreadValue
```

#### Notes



#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.matmul();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `max`

#### Signature

```cpp
Thread::max() -> ThreadValue
```

#### Notes

Returns the maximum value of a numeric thread.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.max();
```

#### Expected output

```text
20.0
```

### `mean`

#### Signature

```cpp
Thread::mean() -> ThreadValue
```

#### Notes

Returns the mean value of a numeric thread.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.mean();
```

#### Expected output

```text
20.0
```

### `median`

#### Signature

```cpp
Thread::median() -> ThreadValue
```

#### Notes



#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.median();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `min`

#### Signature

```cpp
Thread::min() -> ThreadValue
```

#### Notes

Returns the minimum value of a numeric thread.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.min();
```

#### Expected output

```text
20.0
```

### `mode`

#### Signature

```cpp
Thread::mode(drop_nulls: bool = true) -> Thread
```

#### Notes



#### Inputs

`drop_nulls` : `bool`
    When `true`, missing values do not participate while the most-frequent value is selected.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.mode();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `month`

#### Signature

```cpp
Thread::month() -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.month();
```

#### Expected output

```text
[1, 2, 3]
```

### `ndistinct`

#### Signature

```cpp
Thread::ndistinct(dropna: bool = true) -> ThreadValue
```

#### Notes



#### Inputs

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ndistinct();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `normalize`

#### Signature

```cpp
Thread::normalize(form: UnicodeForm = UnicodeForm::NFC) -> Thread
```

#### Notes



#### Inputs

`form` : `UnicodeForm`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.normalize();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `note`

#### Signature

```cpp
Thread::note() -> ThreadNoteAccessor
```

#### Notes

Returns the current payload as a note/text object.

#### Inputs

`None`

#### Output

`ThreadNoteAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.note();
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `notnull`

#### Signature

```cpp
Thread::notnull() -> Thread
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.notnull();
```

#### Expected output

```text
[true, false, true]
```

### `numeric_binary`

#### Signature

```cpp
Thread::numeric_binary(other: const Thread&, op: ThreadBinaryMathOp) -> Thread
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryMathOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::numeric_binary(other: double, op: ThreadBinaryMathOp, reverse: bool = false) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

#### Expected output

```text
[20.0, 30.0, 40.0]
```

### `numeric_binary`

#### Signature

```cpp
Thread::numeric_binary(other: double, op: ThreadBinaryMathOp, reverse: bool = false) -> Thread
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`op` : `ThreadBinaryMathOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`reverse` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::numeric_binary(other: const Thread&, op: ThreadBinaryMathOp) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

#### Expected output

```text
[20.0, 30.0, 40.0]
```

### `pad`

#### Signature

```cpp
Thread::pad(width: std::int64_t, align: TextAlign = TextAlign::Left, fill: char = ' ') -> Thread
```

#### Notes



#### Inputs

`width` : `std::int64_t`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`align` : `TextAlign`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`fill` : `char`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.pad();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `partition`

#### Signature

```cpp
Thread::partition(separator: std::string_view, prefix: std::string = "part") -> DataFrame
```

#### Notes



#### Inputs

`separator` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`prefix` : `std::string`
    Text prefix inserted ahead of the existing label or value.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.partition();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `percentiles`

#### Signature

```cpp
Thread::percentiles(points: pluss::RangeView<double> = {0.25, 0.5, 0.75}, min_count: std::int64_t = 1) -> Thread
```

#### Notes



#### Inputs

`points` : `pluss::RangeView<scalar>`
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.percentiles();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const Thread&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `pluss::null_t`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const Thread&, value: const char*) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const Thread&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const Thread&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const Thread&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const Thread&, value: double) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const Thread&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const Thread&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `pluss::null_t`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `put`

#### Signature

```cpp
Thread::put(bool_mask: const Thread&, value: std::string_view) -> Thread
```

#### Notes



#### Inputs

`bool_mask` : `const Thread&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::put(bool_mask: const Thread&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: double) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const Thread&, value: pluss::null_t) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const Thread&) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: double) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: std::string_view) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: const char*) -> Thread`
- `Thread::put(bool_mask: const pluss::Series&, value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `rank`

#### Signature

```cpp
Thread::rank(ties: RankTieRule = RankTieRule::Average, nulls: RankNullRule = RankNullRule::Keep, ascending: bool = true, percentage: bool = false) -> Thread
```

#### Notes



#### Inputs

`ties` : `RankTieRule`
    Tie policy for equal values: `Average`, `Lowest`, `Highest`, `First`, or `Dense`.

`nulls` : `RankNullRule`
    Null ranking policy: `Keep` leaves null ranks missing, `Top` ranks them first, and `Bottom` ranks them last.

`ascending` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`percentage` : `bool`
    When `true`, divide each rank by the number of ranked observations.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rank();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Note&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Note&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Note&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Series&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Series&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Series&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::TimeIndex&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::TimeIndex&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::TimeIndex&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Timeline&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Timeline&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `const pluss::Timeline&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: double, to: double) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `scalar`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `scalar`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: double, to: pluss::null_t) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `scalar`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Note&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Series&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::TimeIndex&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `const pluss::Timeline&`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: pluss::null_t, to: double) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `pluss::null_t`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `scalar`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: std::string_view, to: std::string_view) -> Thread`
- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `replace`

#### Signature

```cpp
Thread::replace(from: std::string_view, to: std::string_view) -> Thread
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`from` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::replace(from: double, to: double) -> Thread`
- `Thread::replace(from: double, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: double) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Series&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Series&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Note&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Note&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::Timeline&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::Timeline&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&) -> Thread`
- `Thread::replace(from: const pluss::TimeIndex&, to: pluss::null_t) -> Thread`
- `Thread::replace(from: pluss::null_t, to: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `rstrip`

#### Signature

```cpp
Thread::rstrip() -> Thread
```

#### Notes

Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::rstrip(chars: std::string_view) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.rstrip();
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `rstrip`

#### Signature

```cpp
Thread::rstrip(chars: std::string_view) -> Thread
```

#### Notes

Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.

#### Inputs

`chars` : `std::string_view`
    Characters to trim from the edge of each text value.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::rstrip() -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.rstrip("#");
```

#### Expected output

```text
["#203930", "##7788", "4455"]
```

### `scan`

#### Signature

```cpp
Thread::scan(statistic: ScanStatistic, skip_nulls: bool = true) -> Thread
```

#### Notes



#### Inputs

`statistic` : `ScanStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`skip_nulls` : `bool`
    When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.scan();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `se`

#### Signature

```cpp
Thread::se(target: ErrorTarget = ErrorTarget::Mean, options: const StatisticOptions& = {}) -> ThreadValue
```

#### Notes



#### Inputs

`target` : `ErrorTarget`
    Target payload family chosen by this cast overload.

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.se();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `const char*`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `const pluss::TimeIndex&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `const pluss::Timeline&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `const Thread&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `scalar`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `pluss::null_t`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`choices` : `pluss::RangeView<Thread>`
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

`default_value` : `std::string_view`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `const char*`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `const pluss::TimeIndex&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `const pluss::Timeline&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `const Thread&`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `scalar`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `pluss::null_t`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### Signature

```cpp
Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: std::string_view) -> Thread
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`conditions` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`choices` : `std::initializer_list<Thread>`
    Inline collection passed directly at the call site.

`default_value` : `std::string_view`
    Fallback value written anywhere the input is missing or unmatched.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: std::string_view) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: pluss::RangeView<Thread>, choices: pluss::RangeView<Thread>, default_value: pluss::null_t) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const Thread&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: double) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const char*) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::Timeline&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: const pluss::TimeIndex&) -> Thread`
- `Thread::select(conditions: std::initializer_list<Thread>, choices: std::initializer_list<Thread>, default_value: pluss::null_t) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `shares_payload_with`

#### Signature

```cpp
Thread::shares_payload_with(other: const Thread&) -> bool
```

#### Notes



#### Inputs

`other` : `const Thread&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shares_payload_with();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `shift`

#### Signature

```cpp
Thread::shift(periods: std::int64_t = 1) -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`periods` : `std::int64_t`
    Number of row or time steps to shift forward or backward. Positive values usually move data down or later; negative values move it up or earlier.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shift();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `show`

#### Signature

```cpp
Thread::show(options: const display::DisplayOptions& = {}) -> void
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

- `Thread::show(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `show`

#### Signature

```cpp
Thread::show(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void
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

- `Thread::show(options: const display::DisplayOptions& = {}) -> void`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `size`

#### Signature

```cpp
Thread::size() -> std::int64_t
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.size();
```

#### Expected output

```text
3
```

### `skew`

#### Signature

```cpp
Thread::skew(min_count: std::int64_t = 3) -> ThreadValue
```

#### Notes



#### Inputs

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.skew();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `slice`

#### Signature

```cpp
Thread::slice(start: std::int64_t, stop: std::int64_t) -> Thread
```

#### Notes

Returns a contiguous slice `[start, stop)` of the thread.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.slice(0, 2);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `sort_values`

#### Signature

```cpp
Thread::sort_values(ascending: bool = true, nulls_last: bool = true) -> Thread
```

#### Notes



#### Inputs

`ascending` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`nulls_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sort_values();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `split`

#### Signature

```cpp
Thread::split(delimiter: std::string_view, limit: std::int64_t = -1, prefix: std::string = "part") -> DataFrame
```

#### Notes



#### Inputs

`delimiter` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`limit` : `std::int64_t`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`prefix` : `std::string`
    Text prefix inserted ahead of the existing label or value.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.split();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `starts`

#### Signature

```cpp
Thread::starts(prefix: std::string_view) -> Thread
```

#### Notes



#### Inputs

`prefix` : `std::string_view`
    Text prefix inserted ahead of the existing label or value.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.starts();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `std`

#### Signature

```cpp
Thread::std(options: const StatisticOptions&) -> ThreadValue
```

#### Notes

Returns the standard deviation of a numeric thread.

#### Inputs

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::std(unbiased: bool = true) -> ThreadValue`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

#### Expected output

```text
20.0
```

### `std`

#### Signature

```cpp
Thread::std(unbiased: bool = true) -> ThreadValue
```

#### Notes

Returns the standard deviation of a numeric thread.

#### Inputs

`unbiased` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::std(options: const StatisticOptions&) -> ThreadValue`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

#### Expected output

```text
20.0
```

### `strip`

#### Signature

```cpp
Thread::strip() -> Thread
```

#### Notes

Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::strip(chars: std::string_view) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip();
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `strip`

#### Signature

```cpp
Thread::strip(chars: std::string_view) -> Thread
```

#### Notes

Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.

#### Inputs

`chars` : `std::string_view`
    Characters to trim from the edge of each text value.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::strip() -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip("# ");
```

#### Expected output

```text
["Accra", "Tema", "Kumasi"]
```

### `sum`

#### Signature

```cpp
Thread::sum() -> ThreadValue
```

#### Notes

Returns the sum of a numeric thread.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sum();
```

#### Expected output

```text
20.0
```

### `tail`

#### Signature

```cpp
Thread::tail(n: std::int64_t = 5, render: bool = false) -> ThreadView
```

#### Notes

Returns the last `n` entries of the thread.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`ThreadView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tail(2);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `take`

#### Signature

```cpp
Thread::take(indices: std::span<const std::int64_t>) -> Thread
```

#### Notes

Returns the entries at the requested integer positions.

#### Inputs

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

std::vector<std::int64_t> picks{0, 2};
auto out = thread.take(picks);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `tally`

#### Signature

```cpp
Thread::tally(top_n: std::int64_t = 0, dropna: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Inputs

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tally();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `titlecase`

#### Signature

```cpp
Thread::titlecase() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.titlecase();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `tl`

#### Signature

```cpp
Thread::tl() -> ThreadTimelineAccessor
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`ThreadTimelineAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tl();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `to_note_weekday`

#### Signature

```cpp
Thread::to_note_weekday() -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.to_note_weekday();
```

#### Expected output

```text
[1, 2, 3]
```

### `to_string`

#### Signature

```cpp
Thread::to_string(options: const display::DisplayOptions& = {}) -> std::string
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.to_string();
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `transform`

#### Signature

```cpp
Thread::transform(fn: const std::function<Thread(const Thread&)>&, shape: ResultShape = ResultShape::Reduce) -> Thread
```

#### Notes

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Inputs

`fn` : `const std::function<Thread(const Thread&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

`shape` : `ResultShape`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::transform(fn: const std::function<ThreadValue(const ThreadValue&)>&) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.transform();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `transform`

#### Signature

```cpp
Thread::transform(fn: const std::function<ThreadValue(const ThreadValue&)>&) -> Thread
```

#### Notes

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Inputs

`fn` : `const std::function<ThreadValue(const ThreadValue&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::transform(fn: const std::function<Thread(const Thread&)>&, shape: ResultShape = ResultShape::Reduce) -> Thread`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.transform();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ts`

#### Signature

```cpp
Thread::ts() -> TimeSeriesAccessor
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`TimeSeriesAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ts();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `unique`

#### Signature

```cpp
Thread::unique() -> bool
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.unique();
```

#### Expected output

```text
true
```

### `upper`

#### Signature

```cpp
Thread::upper() -> Thread
```

#### Notes



#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.upper();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `validity_mask`

#### Signature

```cpp
Thread::validity_mask() -> const std::optional<pluss::Series>&
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
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.validity_mask();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `value`

#### Signature

```cpp
Thread::value() -> ThreadValue
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.value();
```

#### Expected output

```text
10.0
```

### `var`

#### Signature

```cpp
Thread::var(options: const StatisticOptions&) -> ThreadValue
```

#### Notes

Returns the variance of a numeric thread.

#### Inputs

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::var(unbiased: bool = true) -> ThreadValue`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.var(true);
```

#### Expected output

```text
20.0
```

### `var`

#### Signature

```cpp
Thread::var(unbiased: bool = true) -> ThreadValue
```

#### Notes

Returns the variance of a numeric thread.

#### Inputs

`unbiased` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Thread::var(options: const StatisticOptions&) -> ThreadValue`

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.var(true);
```

#### Expected output

```text
20.0
```

### `view`

#### Signature

```cpp
Thread::view(start: std::int64_t, stop: std::int64_t, render: bool = false) -> ThreadView
```

#### Notes

Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`ThreadView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.view();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `weigh`

#### Signature

```cpp
Thread::weigh(weights: pluss::RangeView<double>, statistic: WindowStatistic = WindowStatistic::Mean, min_count: std::int64_t = 1) -> Thread
```

#### Notes



#### Inputs

`weights` : `pluss::RangeView<scalar>`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`statistic` : `WindowStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.weigh();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `window`

#### Signature

```cpp
Thread::window(window: std::int64_t, op: ThreadWindowOp, min_periods: std::int64_t = 1) -> Thread
```

#### Notes

Computes a window-based transformation such as a rolling mean over the thread values.

#### Inputs

`window` : `std::int64_t`
    Positive fixed case count in each moving window.

`op` : `ThreadWindowOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`min_periods` : `std::int64_t`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

### `with_display_labels`

#### Signature

```cpp
Thread::with_display_labels(labels: const Thread&) -> Thread
```

#### Notes



#### Inputs

`labels` : `const Thread&`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_display_labels();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_display_name`

#### Signature

```cpp
Thread::with_display_name(name: std::string) -> Thread
```

#### Notes



#### Inputs

`name` : `std::string`
    Name used to identify the target object, field, feature, or slot for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_display_name();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_validity_mask`

#### Signature

```cpp
Thread::with_validity_mask(validity_mask: const pluss::Series&) -> Thread
```

#### Notes

Returns a new thread with the supplied validity mask attached.

#### Inputs

`validity_mask` : `const pluss::Series&`
    Boolean validity mask attached to the payload. `true` marks observed entries and `false` marks missing entries.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_validity_mask(mask);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `with_validity_mask_`

#### Signature

```cpp
Thread::with_validity_mask_(validity_mask: const pluss::Series&) -> Thread&
```

#### Notes

Attaches the supplied validity mask in place.

#### Inputs

`validity_mask` : `const pluss::Series&`
    Boolean validity mask attached to the payload. `true` marks observed entries and `false` marks missing entries.

#### Output

`Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.with_validity_mask_(mask);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `year`

#### Signature

```cpp
Thread::year() -> Thread
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.year();
```

#### Expected output

```text
[2024, 2024, 2024]
```
