# Thread::to_note_weekday

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::to_note_weekday() -> Thread
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Extract the `to_note_weekday` component from each temporal value in the current thread.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.to_note_weekday();
```

### Output

```text
[1, 2, 3]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
