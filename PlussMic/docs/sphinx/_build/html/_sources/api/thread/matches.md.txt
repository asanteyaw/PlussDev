# Thread::matches

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::matches(std::string_view pattern) -> Thread
```

## What It Does

Checks whether each note-backed text value matches the requested regular expression.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `matches` to the current thread using explicit `pattern` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.matches("Acc");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="matmul.html">
    <span class="label">Next</span>
    <span class="title">matmul</span>
  </a>
</div>
