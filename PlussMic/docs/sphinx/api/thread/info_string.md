# Thread::info_string

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::info_string() -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

`None`

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info_string();
```

### Output

```text
sales
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info.html">
    <span class="label">Previous</span>
    <span class="title">info</span>
  </a>
  <a class="next-link" href="interp.html">
    <span class="label">Next</span>
    <span class="title">interp</span>
  </a>
</div>
