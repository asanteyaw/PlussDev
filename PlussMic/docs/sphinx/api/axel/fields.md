<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b6af95492a76 -->

# Axel::fields

[Back to Axel API Reference](../../axel_api_reference)

## Signatures

```cpp
Axel Axel::fields(std::span<const std::string_view> keys) const
Axel Axel::fields(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
Axel Axel::fields(const NameRange& keys) const
```

## What It Does

Projects every registry in the axel down to the requested field set.

This is the bundle-stack equivalent of `Registry::fields(keys)`: selection happens first, and any later `cast()`, `nt()`, `tl()`, `fillnull()`, `distinct()`, or similar operation runs against the projected registries.

## Parameters

**keys**
    Field names to keep in every registry.

## Returns

`Axel`
    A new axel whose registries contain only the requested fields.

## Examples

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Tamale")})},
});

auto ax = pluss::bundle::Axel({r0, r1});
auto out = ax.fields({"city"}).nt().upper();

std::cout << out.registry(0).nfields() << " "
          << std::get<pluss::Note>(out.registry(0).fields("city").at(0)).str();
```

### Output

```text
1 ACCRA
```

## Notes

- Every registry must contain the requested fields or the call throws.
- `fields(keys)` is projection, not a hidden field-accessor wrapper.
- Use root `cast()`, `nt()`, and `tl()` when you want the operation over the entire current axel.

## See Also

- [`Registry::fields`](../registry/fields.md): the per-registry equivalent.
- [`Axel::fieldaxis`](fieldaxis.md): project by pattern or slot kind instead of explicit names.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fieldaxis.html">
    <span class="label">Previous</span>
    <span class="title">fieldaxis</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
