<!-- pluss-hand-authored -->

# Axel::cast

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
AxelCastAccessor Axel::cast() const
AxelCastAccessor Axel::cast(const pluss::table::ThreadCastOptions& options) const
```

## What It Does

Returns an [`AxelCastAccessor`](../../axel_cast_accessor_api_reference)
bound to this `Axel`, for reinterpreting every field's `Slot` values as
a specific target kind (`Series`, `Note`, `Timeline`, or `TimeIndex`)
across the whole depth stack.

```{important} Scope with `fields()` first, not with `cast()`'s own parameters
`cast()` itself takes no field names — it always acts on *every* field
of every frame. To cast only one field, chain
`axel.fields({"..."}).cast()` instead, exactly like
[`Axel::nt`](nt.md) and [`Axel::tl`](tl.md).
```

## Parameters

**options** (`const pluss::table::ThreadCastOptions&`, optional)
    Cast behavior options (e.g. parse formats) forwarded to each
    frame's underlying `Registry::cast(options)` call. Omit for
    defaults.

## Returns

`AxelCastAccessor`
    An accessor exposing `series()`, `note()`, `timeline()`, and
    `timeindex()` — see the
    [`AxelCastAccessor` reference](../../axel_cast_accessor_api_reference).

## Examples

Example 1. Cast the `"ids"` field of every frame to `Note` (text).

```cpp
auto r0 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})}});
auto r1 = pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})}});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.fields({"ids"}).cast().note();
std::cout << std::get<pluss::Note>(out.registry(0).fields("ids").at(0)).at(0);
```

### Output

```text
1
```

## Notes

- `cast()` with no field scope applies to every field of every frame —
  scope it first with [`fields()`](fields.md) to affect only specific
  fields, the same idiom used for [`nt()`](nt.md) and [`tl()`](tl.md).
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`AxelCastAccessor` API Reference](../../axel_cast_accessor_api_reference):
  every method available on the returned accessor.
- [`Axel::fields`](fields.md): narrow to a specific field before
  casting it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_kindle.html">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
  </a>
  <a class="next-link" href="concat_depth.html">
    <span class="label">Next</span>
    <span class="title">concat_depth</span>
  </a>
</div>
