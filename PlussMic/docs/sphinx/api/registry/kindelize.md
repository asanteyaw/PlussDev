<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 80d47afd3413 -->

# Registry::kindelize

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
pluss::kindle::Catalog Registry::kindelize() const
```

## What It Does

Converts the registry into a `pluss::kindle::Catalog`, one stream per field, in
the registry's field order.

## Parameters

`None`

## Returns

`pluss::kindle::Catalog`
    A new catalog with one stream per field.

## Examples

Example 1. `kindelize()` produces one catalog key per registry field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

std::cout << rg.kindelize().nkeys();
```

### Output

```text
1
```

## Notes

- Every field must be stream-compatible; mixed-kind slots throw.
- Requires a defined registry.

## See Also

- [`Catalog::bundelize`](../catalog/bundelize.md): the reverse bridge from
  Kindle back into the bundle model.
- [`Registry::save_csv`](save_csv.md): one of the export methods that routes
  through this conversion internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_.html">
    <span class="label">Previous</span>
    <span class="title">keep_</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
