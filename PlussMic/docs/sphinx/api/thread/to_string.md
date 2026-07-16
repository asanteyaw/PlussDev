# Thread::to_string

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::to_string(const display::DisplayOptions& options = {}) -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.to_string();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_note_weekday.html">
    <span class="label">Previous</span>
    <span class="title">to_note_weekday</span>
  </a>
  <a class="next-link" href="top.html">
    <span class="label">Next</span>
    <span class="title">top</span>
  </a>
</div>
