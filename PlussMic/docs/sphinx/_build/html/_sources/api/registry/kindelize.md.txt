<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 80d47afd3413 -->

# Registry::kindelize

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
pluss::kindle::Catalog Registry::kindelize() const
```

## What It Does

A pure alias for [`to_kindle()`](to_kindle.md), kept for older call sites — the
comment in `registry.h` marks it as a "backward-compatible alias." Prefer
[`to_kindle()`](to_kindle.md) or [`as_kindle()`](as_kindle.md) in new code.

## Parameters

`None`

## Returns

`pluss::kindle::Catalog`
    A new catalog with one stream per field — identical to what `to_kindle()`
    would return.

## Examples

Example 1. `kindelize()` produces the same result as `to_kindle()`.

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

- Identical to `to_kindle()` in every respect, including throwing on a
  non-stream-compatible field.
- Kept for backward compatibility — prefer `to_kindle()` or `as_kindle()` for
  new code.

## See Also

- [`Registry::to_kindle`](to_kindle.md): the method this is a pure alias for.
- [`Registry::as_kindle`](as_kindle.md): another pure alias for the same
  method.

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
