# Registry::concat_values

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::concat_values(std::string_view key, const Slot& other) -> Registry
```

## What It Does

Appends extra values onto one named slot and returns the updated registry.

## Parameters

**key**
    Name of the target slot or stream.

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = rg.concat_values("ids", extra);
```

### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3), Series(4), Series(5)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

## Practical Behavior

- `concat_values(key, other)` appends the values from another slot onto one existing slot.
- The target key must exist and the appended slot must be defined.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="combine.html">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="concat_values_.html">
    <span class="label">Next</span>
    <span class="title">concat_values_</span>
  </a>
</div>
