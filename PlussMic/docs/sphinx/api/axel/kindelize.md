<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: abb09beec00e -->

# Axel::kindelize

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
pluss::kindle::Archive Axel::kindelize() const
```

## What It Does

Converts this `Axel` to its `kindle::Archive` representation. A
backward-compatible alias for [`to_kindle()`](to_kindle.md) — the body
is a single `return to_kindle();`, kept for older call sites.

## Parameters

`None`

## Returns

`pluss::kindle::Archive`
    The same result [`to_kindle()`](to_kindle.md) would produce.

## Examples

Example 1. `kindelize()` agrees with [`to_kindle()`](to_kindle.md) and
[`as_kindle()`](as_kindle.md) on depth.

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

- For new code, prefer [`to_kindle()`](to_kindle.md) or
  [`as_kindle()`](as_kindle.md) — `kindelize()` exists purely for
  backward compatibility with older call sites.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::to_kindle`](to_kindle.md): the canonical name for this
  operation.
- [`Axel::as_kindle`](as_kindle.md): the semantic-switch-flavored
  alias.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="labels.html">
    <span class="label">Next</span>
    <span class="title">labels</span>
  </a>
</div>
