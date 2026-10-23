<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e8678436b11 -->

# DataFrameLabelAccessor::depth

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::depth() const -> std::int64_t
```

## What It Does

Returns the **lexsort depth** of the case labels: how many of the leading
levels, taken together in their current order, are already sorted ascending
(nulls last). For an unstructured frame, this is `1` if the flat case-label
slate itself is sorted ascending, else `0`. For a structured frame, `depth()`
checks longer and longer level prefixes — level 0 alone, then levels 0–1
together, then 0–2, and so on — stopping at the first prefix that isn't
jointly sorted.

## Returns

`std::int64_t`
    The number of leading levels that are jointly sorted ascending; `0` if even
    the first level isn't sorted (or the frame is unstructured and not sorted).

## Examples

Example 1. A 2-level frame where both levels are sorted, once nested
correctly — full depth.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.labels().set({"region", "city"}, true, "/");

auto out = grouped.labels().depth();
std::cout << out;
```

### Output

```text
2
```

`region` alone is sorted (`"east"` then `"west"`); and within each `region`
group, `city` is also sorted (`"Boston"` < `"NYC"`; `"LA"` < `"SF"`) — both
levels jointly sorted, so `depth()` is `2`, the maximum possible for a 2-level
frame.

Example 2. The same region grouping, but with `city` out of order inside the
first group — depth drops to `1`.

```cpp
auto readings2 = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"NYC", "Boston", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped2 = readings2.labels().set({"region", "city"}, true, "/");

auto out2 = grouped2.labels().depth();
std::cout << out2;
```

### Output

```text
1
```

`region` alone is still sorted, so the 1-level prefix passes. But within the
first `region` group, `"NYC"` comes before `"Boston"` — jointly, `(region,
city)` is not sorted — so `depth()` stops at `1` rather than reaching `2`.

## Notes

- `depth()` checks the levels **in their current order** (as returned by
  [`names()`](names.md)); [`order()`](order.md) or [`swap()`](swap.md) can
  change which prefixes are checked, and therefore the result.
- Every level comparison uses `nulls_last = true`; there's no parameter to
  change this.
- For an unstructured frame, `depth()` is equivalent to
  `monotonic(true, false, true) ? 1 : 0`.

## See Also

- [`DataFrameLabelAccessor::monotonic`](monotonic.md): the underlying single-axis
  sortedness check, generalized here across nested levels.
- [`DataFrameLabelAccessor::sort`](sort.md): sorting by all levels (or an
  explicit subset) to make a frame fully lexsorted.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="codes.html">
    <span class="label">Previous</span>
    <span class="title">codes</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
