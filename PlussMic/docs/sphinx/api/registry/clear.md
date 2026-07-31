# Registry::clear

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::clear() -> Registry
```

## What It Does

Removes all slots and returns an empty registry.

## Parameters

`None`

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.clear();
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
<empty rg>
```

## Practical Behavior

- `clear()` returns an empty registry with no slots.
- Use `clear_()` when you want the same effect in place.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="begin.html">
    <span class="label">Previous</span>
    <span class="title">begin</span>
  </a>
  <a class="next-link" href="clear_.html">
    <span class="label">Next</span>
    <span class="title">clear_</span>
  </a>
</div>
