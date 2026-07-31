# Registry::clear_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::clear_() -> Registry&
```

## What It Does

Removes all slots in place.

## Parameters

`None`

## Returns

`Registry&`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.clear_();
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear.html">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="cloc.html">
    <span class="label">Next</span>
    <span class="title">cloc</span>
  </a>
</div>
