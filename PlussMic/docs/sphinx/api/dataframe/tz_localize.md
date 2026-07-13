# DataFrame::tz_localize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::tz_localize(pluss::RangeView<std::string_view> features, std::string_view tz_name, int fold = 0, std::string_view nonexistent = "raise") -> DataFrame
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**tz_name**
    Timezone name to attach to naive temporal values.

**fold**
    Fold flag used when localizing ambiguous wall-clock times around a DST fall-back transition.

**nonexistent**
    Policy for nonexistent local times, such as `"raise"` or `"shift_forward"`.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.tz_localize({"ts"}, "UTC", 0, "shift_forward");
```

### Output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz_convert.html">
    <span class="label">Previous</span>
    <span class="title">tz_convert</span>
  </a>
  <a class="next-link" href="unify.html">
    <span class="label">Next</span>
    <span class="title">unify</span>
  </a>
</div>
