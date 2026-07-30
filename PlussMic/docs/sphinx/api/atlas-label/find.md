# AtlasLabelAccessor::find

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::find(const Thread& targets) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**targets**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

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

auto out = labels.find(df.features("load"));
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
