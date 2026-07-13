# Thread::year

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::year() -> Thread
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Extract the `year` component from each temporal value in the current thread.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.year();
```

### Output

```text
[2024, 2024, 2024]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_display_name.html">
    <span class="label">Previous</span>
    <span class="title">with_display_name</span>
  </a>
</div>
