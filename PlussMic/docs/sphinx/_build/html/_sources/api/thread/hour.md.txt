# Thread::hour

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::hour() -> Thread
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Extract the `hour` component from each temporal value in the current thread.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.hour();
```

### Output

```text
[1, 2, 3]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
