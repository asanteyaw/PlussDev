# Thread::bundelize

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bundelize() -> pluss::bundle::Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`pluss::bundle::Slot`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.bundelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html">
    <span class="label">Previous</span>
    <span class="title">bucketize</span>
  </a>
  <a class="next-link" href="casefold.html">
    <span class="label">Next</span>
    <span class="title">casefold</span>
  </a>
</div>
