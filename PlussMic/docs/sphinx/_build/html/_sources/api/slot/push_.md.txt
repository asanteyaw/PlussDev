# Slot::push_

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::push_(const Value& value) -> Slot&
```

## What It Does

Appends one additional value to the slot in place.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

## Returns

`Slot&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.push_(pluss::make_note("tail"));
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="push.html">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
