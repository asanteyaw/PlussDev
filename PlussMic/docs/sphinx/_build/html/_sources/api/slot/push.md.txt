# Slot::push

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::push(const Value& value) -> Slot
```

## What It Does

Returns a new slot with one additional value appended.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.push(pluss::make_note("tail"));
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="push_.html">
    <span class="label">Next</span>
    <span class="title">push_</span>
  </a>
</div>
