# DataFrame::timeline_features

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::timeline_features() -> DataFrame
```

## What It Does

Returns only the timeline-backed features.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"start", pluss::table::Thread(pluss::make_timeline({
        "2020-01-01T00:00:00Z",
        "2020-01-02T08:00:00Z",
    }))},
    {"end", pluss::table::Thread(pluss::make_timeindex({
        "2020-01-01T01:30:00Z",
        "2020-01-02T09:00:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
});

auto out = df.timeline_features();
```

### Output

```text
start                 end
2020-01-01T00:00:00Z  2020-01-01T01:30:00Z
2020-01-02T08:00:00Z  2020-01-02T09:00:00Z
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tally.html">
    <span class="label">Previous</span>
    <span class="title">tally</span>
  </a>
  <a class="next-link" href="title.html">
    <span class="label">Next</span>
    <span class="title">title</span>
  </a>
</div>
