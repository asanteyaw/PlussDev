# Stream::reduce_groups

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::reduce_groups(const std::vector<std::int64_t>& source, std::int64_t shift, const std::vector<std::int64_t>& offsets, GroupReduce op, bool unbiased = true) -> Stream
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**source**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**shift**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**offsets**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

**unbiased**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`Stream`
    Value produced by this call.

## Examples

Example 1. Pass several integer positions together.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.reduce_groups();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="scribbelize.html">
    <span class="label">Next</span>
    <span class="title">scribbelize</span>
  </a>
</div>
