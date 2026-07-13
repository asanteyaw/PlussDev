# Thread::tl

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::tl() -> ThreadTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`ThreadTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tl();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
  <a class="next-link" href="to_note_weekday.html">
    <span class="label">Next</span>
    <span class="title">to_note_weekday</span>
  </a>
</div>
