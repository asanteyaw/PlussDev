# Thread::take

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::take(std::span<const std::int64_t> indices) -> Thread
```

## What It Does

Returns the entries at the requested integer positions.

## Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Pass several integer positions together.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

std::vector<std::int64_t> picks{0, 2};
auto out = thread.take(picks);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="tally.html">
    <span class="label">Next</span>
    <span class="title">tally</span>
  </a>
</div>
