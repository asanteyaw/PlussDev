# Stream::bundelize

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::bundelize() -> pluss::bundle::Slot
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
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.bundelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
