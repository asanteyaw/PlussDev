<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60f7519b3236 -->

# Registry::end

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::const_iterator Registry::end() const
```

## What It Does

Returns the past-the-end iterator matching [`begin()`](begin.md) — together they
make `Registry` usable in a range-`for` loop and with standard algorithms,
iterating field names.

## Parameters

`None`

## Returns

`Registry::const_iterator`
    The past-the-end iterator; equal to `begin()` for an empty registry.

## Examples

Example 1. Manual iteration using `begin()`/`end()` directly.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra")})},
});

for (auto it = rg.begin(); it != rg.end(); ++it) std::cout << *it << " ";
```

### Output

```text
id city
```

## Notes

- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one (via the underlying `keys()` accessor).

## See Also

- [`Registry::begin`](begin.md): the matching begin iterator, including the
  important callout about what gets iterated (field names, not `Slot`s).
- [`Catalog::end`](../catalog/end.md): the equivalent iterator on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="fieldaxis.html">
    <span class="label">Next</span>
    <span class="title">fieldaxis</span>
  </a>
</div>
