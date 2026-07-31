# Axel::apply_slot

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) -> Axel
```

## What It Does

Applies a slot transform to one named key inside every registry and returns the updated axel.

## Parameters

**key**
    Name of the target slot or stream.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.apply_slot("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

## Practical Behavior

- `apply_slot(key, fn)` walks every registry in the axel and rewrites that slot only where it exists.
- Registries that do not have the requested key are left unchanged.



<div class="pluss-inline-prev-next">
  <a class="next-link" href="as_kindle.html">
    <span class="label">Next</span>
    <span class="title">as_kindle</span>
  </a>
</div>
