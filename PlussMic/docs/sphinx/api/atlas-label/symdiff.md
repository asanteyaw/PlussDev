# AtlasLabelAccessor::symdiff

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::symdiff(const Thread& other) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto q1 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"2024Q1", "2024Q2"})));
auto labels = atlas.label();

auto out = labels.symdiff(df.features("load"));
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="unite.html">
    <span class="label">Next</span>
    <span class="title">unite</span>
  </a>
</div>
