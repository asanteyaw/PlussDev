# Thread::starts

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::starts(std::string_view prefix) -> Thread
```

## What It Does

Checks whether each note-backed text value starts with the requested prefix.

## Parameters

**prefix**
    Text prefix inserted ahead of the existing label or value.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `starts` to the current thread using explicit `prefix` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.starts("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
