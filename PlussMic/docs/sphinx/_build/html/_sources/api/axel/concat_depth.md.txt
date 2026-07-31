# Axel::concat_depth

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::concat_depth(const Axel& other) -> Axel
```

## What It Does

Appends the registries from another axel onto the current depth axis.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

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

auto out = axel.concat_depth(axel);
```

### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
depth2 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth3 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

## Practical Behavior

- `concat_depth(other)` appends the other axel’s registries onto the current depth axis.
- If both axels have labels, the labels are concatenated too.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_kindle.html">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
