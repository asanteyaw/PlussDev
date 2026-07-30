# AtlasLabelAccessor::pos

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signatures

```cpp
AtlasLabelAccessor::pos(const char* value) -> Thread

AtlasLabelAccessor::pos(double value) -> Thread

AtlasLabelAccessor::pos(std::string_view value) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

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

auto out = labels.pos({});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto q1 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"2024Q1", "2024Q2"})));
auto labels = atlas.label();

auto out = labels.pos(0.0);
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto q1 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"2024Q1", "2024Q2"})));
auto labels = atlas.label();

auto out = labels.pos("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect.html">
    <span class="label">Previous</span>
    <span class="title">intersect</span>
  </a>
  <a class="next-link" href="range.html">
    <span class="label">Next</span>
    <span class="title">range</span>
  </a>
</div>
