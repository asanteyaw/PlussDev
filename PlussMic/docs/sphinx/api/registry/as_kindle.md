<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2c0222ae3185 -->

# Registry::as_kindle

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
pluss::kindle::Catalog Registry::as_kindle() const
```

## What It Does

A pure alias for [`to_kindle()`](to_kindle.md) — converts the registry into a
`pluss::kindle::Catalog`. `as_kindle()` exists for call sites that read
naturally as "treat this as a kindle-model object"; `to_kindle()` exists for the
more literal "convert to kindle" framing. There is no behavioral difference,
including the same stream-compatibility requirement.

## Parameters

`None`

## Returns

`pluss::kindle::Catalog`
    A new catalog with one stream per field — identical to what `to_kindle()`
    would return.

## Examples

Example 1. `as_kindle()` and `to_kindle()` produce equivalent catalogs.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

std::cout << rg.as_kindle().nkeys();
```

### Output

```text
1
```

## Notes

- Identical to `to_kindle()` in every respect, including throwing on a
  non-stream-compatible field.

## See Also

- [`Registry::to_kindle`](to_kindle.md): the method this is a pure alias for.
- [`Registry::kindelize`](kindelize.md): another pure alias for the same
  method.
- [`Registry::from_kindle`](from_kindle.md): the inverse conversion.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>
