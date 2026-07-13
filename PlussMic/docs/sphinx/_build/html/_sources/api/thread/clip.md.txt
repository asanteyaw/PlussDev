# Thread::clip

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clip(double min_value, double max_value) -> Thread
```

Clamp numeric values into a closed interval.

## Parameters

**min_value**

Lower bound of the closed interval.

**max_value**

Upper bound of the closed interval.

## Returns

`Thread`
    Numeric thread with every value clipped into `[min_value, max_value]`.

## Examples

Example 1. Clip a numeric series into a bounded grade or score range.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));

auto out = scores.clip(15.0, 35.0);
```

### Output

```text
0  15
1  20
2  30
3  35
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_display_name.html">
    <span class="label">Previous</span>
    <span class="title">clear_display_name</span>
  </a>
  <a class="next-link" href="compare.html">
    <span class="label">Next</span>
    <span class="title">compare</span>
  </a>
</div>
