<!-- pluss-hand-authored -->

# Axel::as_kindle

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
pluss::kindle::Archive Axel::as_kindle() const
```

## What It Does

Converts this `Axel` to its `kindle::Archive` representation. A
semantic alias for [`to_kindle()`](to_kindle.md) — the body is a
single `return to_kindle();`, byte-identical behavior under a name that
reads better at an explicit model-switch call site.

## Parameters

`None`

## Returns

`pluss::kindle::Archive`
    The same result [`to_kindle()`](to_kindle.md) would produce.

## Examples

Example 1. `as_kindle()`, [`to_kindle()`](to_kindle.md), and
[`kindelize()`](kindelize.md) all agree on depth.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
});
auto a1 = axel.to_kindle();
auto a2 = axel.as_kindle();
auto a3 = axel.kindelize();
std::cout << a1.depth() << " " << a2.depth() << " " << a3.depth();
```

### Output

```text
2 2 2
```

## Notes

- `as_kindle()`, [`to_kindle()`](to_kindle.md), and
  [`kindelize()`](kindelize.md) are three names for the exact same
  operation.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::to_kindle`](to_kindle.md): the canonical name for this
  operation.
- [`Axel::from_kindle`](from_kindle.md): the reverse conversion.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
