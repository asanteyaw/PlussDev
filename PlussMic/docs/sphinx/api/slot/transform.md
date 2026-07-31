# Slot::transform

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::transform(const std::function<Value(const Value&)>& fn) -> Slot
```

## What It Does

Applies a value-wise transform to each slot entry and returns the updated slot.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="type_counts.html">
    <span class="label">Next</span>
    <span class="title">type_counts</span>
  </a>
</div>
