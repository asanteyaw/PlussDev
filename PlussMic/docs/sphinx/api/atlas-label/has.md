# AtlasLabelAccessor::has

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::has() -> bool
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto q1 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"2024Q1", "2024Q2"})));
auto labels = atlas.label();

auto out = labels.has();
std::cout << out;
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find.html">
    <span class="label">Previous</span>
    <span class="title">find</span>
  </a>
  <a class="next-link" href="intersect.html">
    <span class="label">Next</span>
    <span class="title">intersect</span>
  </a>
</div>
