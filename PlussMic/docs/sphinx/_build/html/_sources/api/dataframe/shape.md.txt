<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76c25b5d84d4 -->

# DataFrame::shape

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::shape() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Returns the frame's dimensions as a `(cases, features)` pair — the row count first, then
the column count. It is the cheapest possible way to answer "how big is this frame"
without printing the frame itself or walking any column data: both numbers come directly
from the frame's existing bookkeeping.

`shape()` is most useful as a quick assertion or log line in the middle of a pipeline —
confirming a `join()` produced the row count you expected, or that a `filter()` actually
narrowed anything — without the cost or noise of a full [`show()`](show.md).

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    The first element is `ncases()` (row count); the second is `nfeatures()` (column
    count). `std::pair` has no built-in stream operator, so print the two numbers
    yourself (see the example) rather than trying to stream the pair directly.

## Examples

Example 1. Read both dimensions with a structured binding.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
});

auto [cases, features] = orders.shape();
std::cout << "cases: " << cases << ", features: " << features << "\n";
```

### Output

```text
cases: 6, features: 4
```

Example 2. Use `shape()` as a cheap post-condition check after a transform.

```cpp
auto filtered = orders.filter(orders.features("units") >= 3);
auto [rows_after, cols_after] = filtered.shape();
assert(cols_after == 4);        // filter() never changes the feature count
std::cout << rows_after << " of " << orders.shape().first << " orders matched\n";
```

`filter()` (like `query()`, `dropnull()`, and every other row-selecting method) only ever
changes `.first` (cases); the feature count in `.second` is invariant across row
operations, which is exactly what makes `shape()` a convenient one-line assertion after
them.

## Notes

- `shape().first == ncases()` and `shape().second == nfeatures()` always — `shape()` is
  a convenience pairing of those two accessors, not a separately computed value.
- Because `shape()` reads existing metadata rather than scanning column data, calling it
  repeatedly (for logging, assertions, or loop conditions) costs essentially nothing.

## See Also

- [`DataFrame::ncases`](ncases.md) / [`DataFrame::nfeatures`](nfeatures.md): the two
  numbers individually.
- [`DataFrame::info`](info.md): shape plus dtypes, null counts, and memory in one report.
- [`DataFrame::features`](features.md): the actual feature names, when the count alone
  isn't enough.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_.html">
    <span class="label">Previous</span>
    <span class="title">set_</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
