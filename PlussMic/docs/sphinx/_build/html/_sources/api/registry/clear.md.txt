<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86660893f49c -->

# Registry::clear

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::clear() const
```

## What It Does

Returns a new, **defined** registry with zero fields — the explicit-empty
construction seen in [`defined()`](defined.md)'s important callout, produced by
a method call rather than a constructor.

```{important} A cleared registry is defined, unlike a default-constructed one
`rg.clear()` returns a registry for which `defined()` is `true` and `empty()` is
`true` — the same "defined but empty" state as an explicit
`Registry(std::vector<std::pair<std::string, Slot>>{})`. This is different from
a default-constructed `Registry{}`, which is **undefined**. See
[`Registry::defined`](defined.md).
```

## Parameters

`None`

## Returns

`Registry`
    A new, defined registry with zero fields.

## Examples

Example 1. Clearing a registry produces a defined-but-empty result.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});

auto cleared = rg.clear();
std::cout << std::boolalpha << cleared.defined() << " " << cleared.empty() << " nfields=" << cleared.nfields();
```

### Output

```text
true true nfields=0
```

## Notes

- See the important callout above for the defined-vs-undefined distinction.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.
- Never mutates the receiver — use [`clear_()`](clear_.md) for the in-place
  form.

## See Also

- [`Registry::clear_`](clear_.md): the in-place counterpart.
- [`Registry::defined`](defined.md): the defined-vs-undefined distinction this
  method's result illustrates.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="clear_.html">
    <span class="label">Next</span>
    <span class="title">clear_</span>
  </a>
</div>
