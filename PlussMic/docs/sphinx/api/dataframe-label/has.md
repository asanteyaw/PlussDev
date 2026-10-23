<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11250034a70b -->

# DataFrameLabelAccessor::has

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::has() const -> bool
DataFrameLabelAccessor::has(double value) const -> Slate
DataFrameLabelAccessor::has(std::string_view value) const -> Slate
DataFrameLabelAccessor::has(const char* value) const -> Slate
DataFrameLabelAccessor::has(const Slate& values) const -> Slate
```

## What It Does

Two unrelated questions share this name:

- **`has()`** (no arguments) answers a structural question: does this frame
  have **structured** (named, multi-level-capable) case labels at all — i.e.,
  were they built via a route like [`set(feature_name)`](set.md) rather than
  left as the default integer index or a raw axis from
  [`set(const Slate&)`](set.md)?
- **`has(value)`** / **`has(values)`** answer an elementwise membership
  question: for each row, does its case label equal `value` (single-value
  overloads), or match *any* of `values` (the `Slate` overload, an "is one
  of" test)?

## Parameters

**value** (`double` / `std::string_view` / `const char*`)

A single value to test each case label against, elementwise.

**values** (`const Slate&`)

A collection of values; each row is `true` if its case label matches *any* of
them.

## Returns

`bool`
    (No-argument overload) Whether the frame's case labels are structured.

`Slate`
    (Value-taking overloads) A boolean slate, one entry per row, displayed
    against the case labels, `display_name` `"has"`.

## Examples

Example 1. Check structure before and after promoting a feature to case
labels.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});

std::cout << orders.labels().has();

auto labeled = orders.labels().set("region");
std::cout << labeled.labels().has();
```

### Output

```text
false
true
```

`orders` still has the default integer case index — no structure yet.
`labeled` was built by promoting `"region"` via `set()`, which always creates a
one-level `case_axis_`, so `has()` becomes `true`.

Example 2. Elementwise membership against a single value, and against several.

```cpp
auto eq_east = labeled.labels().has("east");
std::cout << eq_east;

auto in_set = labeled.labels().has(Slate(pluss::make_note({"west", "north"})));
std::cout << in_set;
```

### Output (has("east"))

```text
 east  true
 west  false
 east  true
north  false
Name: has, ptype: Series
```

### Output (has([west, north]))

```text
 east  false
 west  true
 east  false
north  true
Name: has, ptype: Series
```

Example 3. Elementwise membership on a numeric (`Series`) case-label axis,
using the `double` overload.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}},
    Slate(pluss::make_series({10.0, 20.0, 10.0, 30.0})));
std::cout << readings;

auto eq_ten = readings.labels().has(10.0);
std::cout << eq_ten;
```

### Output (readings)

```text
============
     | sales
------------
10   | 100  
20   | 200  
10   | 150  
30   | 90   
------------
[4 cs x 1 feat]
```

### Output (has(10.0))

```text
10  true
20  false
10  true
30  false
Name: has, ptype: Series
```

Same mechanics as `has("east")` in Example 2, just matched by numeric equality
instead of string equality.

## Notes

- The no-argument overload is purely structural — it says nothing about
  whether any particular value is present.
- `has(values)` is an elementwise "is one of" test across the whole `values`
  collection, not a single membership question about the frame as a whole.

## See Also

- [`DataFrameLabelAccessor::set`](set.md): the operation that turns `has()`
  from `false` to `true`.
- [`DataFrameLabelAccessor::pos`](pos.md),
  [`DataFrameLabelAccessor::find`](find.md): locate *where* a value occurs,
  rather than just whether it does.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="intersect.html">
    <span class="label">Next</span>
    <span class="title">intersect</span>
  </a>
</div>
