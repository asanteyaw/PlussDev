# Registry::push_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::push_(std::string_view key, const Slot::Value& value) -> Registry&
```

## What It Does

Appends one new value to a named slot in place.

## Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

## Returns

`Registry&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_("ids", pluss::make_series(4));
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
  <a class="prev-link" href="push.html">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="push_all.html">
    <span class="label">Next</span>
    <span class="title">push_all</span>
  </a>
</div>
