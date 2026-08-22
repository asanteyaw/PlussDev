<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15ed8116178f -->

# Registry::clear_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::clear_()
```

## What It Does

The in-place counterpart to [`clear()`](clear.md) — removes every field from
this registry directly. The registry remains **defined** afterward, just empty.

## Parameters

`None`

## Returns

`Registry&`
    A reference to this registry, now holding zero fields.

## Examples

Example 1. Clear a registry in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});

rg.clear_();
std::cout << std::boolalpha << rg.defined() << " nfields=" << rg.nfields();
```

### Output

```text
true nfields=0
```

## Notes

- Same defined-but-empty result as [`clear()`](clear.md) — see that page's
  important callout.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::clear`](clear.md): the non-mutating counterpart.
- [`Registry::prune_`](prune_.md): removes only zero-length fields, rather than
  every field unconditionally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear.html">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="cloc.html">
    <span class="label">Next</span>
    <span class="title">cloc</span>
  </a>
</div>
