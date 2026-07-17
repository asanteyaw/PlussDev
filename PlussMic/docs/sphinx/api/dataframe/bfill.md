# DataFrame::bfill

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::bfill(const FillOptions& options) -> DataFrame

DataFrame::bfill(std::int64_t limit = -1) -> DataFrame
```

Backward-fill missing values by pulling the next observed value upward within each selected feature.

`DataFrame::bfill(...)` is the frame-level missing-data repair helper for backward propagation. Use it when later observations should repair earlier gaps, such as when importing sparse gradebook columns, time-aligned metadata, or partially scanned score sheets.

## Parameters

**limit**

Maximum number of consecutive missing values to fill within each run for the simple overload.

- `-1`: no limit. Fill every missing position that has a later observed value in the same feature.
- Positive integer: fill at most that many missing positions per consecutive run.

Use the default when you want full propagation. Use a positive limit when long missing stretches should remain partly untouched.

**options**

Structured backward-fill policy.

The current `FillOptions` fields are:

- `features`: the feature subset to fill. Leave it empty to target every compatible feature.
- `limit`: maximum number of missing positions to fill per consecutive run. `-1` means unlimited fill.

This overload is the better choice when you want to target only one or two columns instead of the entire frame.

## Returns

`DataFrame`
    A new frame with backward-filled values in the targeted features.

    Untargeted features are preserved unchanged.

## See Also

- <a href="ffill.html"><code>DataFrame::ffill</code></a>: propagate the last observed value forward instead of using the next one below.
- <a href="fillnull.html"><code>DataFrame::fillnull</code></a>: write an explicit replacement value instead of borrowing a neighboring observation.
- <a href="../thread/bfill.html"><code>Thread::bfill</code></a>: backward-fill one column directly outside a frame.

## Notes

Backward fill runs feature by feature. Values never jump across feature boundaries.

Trailing missing runs stay missing because there is no later observed value available to borrow.

Use the `FillOptions` overload when only selected features should be filled or when the fill should stop after a limited run length.

## Examples

Example 1. Backward-fill every compatible feature with no run-length limit.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"score", pluss::make_series({72.0, std::numeric_limits<double>::quiet_NaN(), 81.0,
                                  std::numeric_limits<double>::quiet_NaN(), 95.0})},
    {"cost", pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), 40.0,
                                 std::numeric_limits<double>::quiet_NaN()})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Sunyani"})},
}).title("missing");

auto out = missing.bfill();
```

### Output

```text
===========================
     | score  cost  city
---------------------------
0    | 72     10    Accra
1    | 81     40    Tema
2    | 81     40    Kumasi
3    | 95     40    Tamale
4    | 95     NA    Sunyani
---------------------------
[5 cs x 3 feat]
```

Example 2. Backward-fill only one selected feature and stop after one value per missing run.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"score", pluss::make_series({72.0, std::numeric_limits<double>::quiet_NaN(), 81.0,
                                  std::numeric_limits<double>::quiet_NaN(), 95.0})},
    {"cost", pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), 40.0,
                                 std::numeric_limits<double>::quiet_NaN()})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Sunyani"})},
}).title("missing");

DataFrame::FillOptions options;
options.features = {"score"};
options.limit = 1;

auto out = missing.bfill(options);
```

### Output

```text
===========================
     | score  cost  city
---------------------------
0    | 72     10    Accra
1    | 81     nan   Tema
2    | 81     nan   Kumasi
3    | 95     40    Tamale
4    | 95     nan   Sunyani
---------------------------
[5 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="bottom.html">
    <span class="label">Next</span>
    <span class="title">bottom</span>
  </a>
</div>
