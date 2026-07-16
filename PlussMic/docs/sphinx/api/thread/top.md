# Thread::top

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::top(std::int64_t n, bool render = false) -> ThreadView
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`ThreadView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.top(2, true);
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
