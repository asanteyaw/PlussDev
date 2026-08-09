<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Slot::to_string

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::string Slot::to_string() const
```

## What It Does

Renders the slot as a string. Byte-identical to
[`to_json()`](to_json.md) — `to_string()` simply forwards to it.

## Parameters

`None`

## Returns

`std::string`
    The same JSON rendering [`to_json()`](to_json.md) produces.

## Examples

Example 1. `to_string()` and `to_json()` produce identical output.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
std::cout << std::boolalpha << (slot.to_string() == slot.to_json());
```

### Output

```text
true
```

## Notes

- Unlike [`Registry::to_string()`](../registry/to_string.md), which
  differs from `Registry::to_json()`, `Slot::to_string()` is exactly
  `to_json()` — see [`to_json()`](to_json.md)'s important callout for the
  full explanation.

## See Also

- [`Slot::to_json`](to_json.md): the method this one forwards to, with the
  full output format documented.
- [`Slot::show`](show.md): writes this same string to a caller-provided
  stream instead of returning it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_kindle.html">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
