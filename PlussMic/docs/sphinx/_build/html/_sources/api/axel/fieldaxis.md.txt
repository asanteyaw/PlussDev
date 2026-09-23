<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc7f6b57c74c -->

# Axel::fieldaxis

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
AxelFieldAxisAccessor Axel::fieldaxis() const
```

## What It Does

Returns an [`AxelFieldAxisAccessor`](../../axel_fieldaxis_accessor_api_reference)
bound to this `Axel`, for selecting fields by name pattern or `Slot`
kind — broadcast across every registry frame in the depth stack — rather
than by exact name (which is what [`fields()`](fields.md) does).

## Parameters

`None`

## Returns

`AxelFieldAxisAccessor`
    An accessor exposing `like()`, `match()`, `prefix()`, `suffix()`,
    `kind()`, and the kind-shorthand methods (`series()`, `note()`,
    `timeline()`, `timeindex()`, `mixed()`, `nulls()`, `empty()`) — see
    the [`AxelFieldAxisAccessor` reference](../../axel_fieldaxis_accessor_api_reference)
    for the full method list.

## Examples

Example 1. Keep only `Series`-kind fields in every frame.

```cpp
auto r0 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::scribble::Axel({r0, r1});

auto out = axel.fieldaxis().series();
std::cout << out.registry(0).nfields();
```

### Output

```text
1
```

Only `"ids"` (`Series`-kind) survives in each frame; `"city"`
(`Note`-kind) is dropped from every frame.

## Notes

- Every call on the returned accessor is applied independently to each
  frame — a field kept in one frame because it matches the pattern or
  kind there is not forced to exist in every other frame; frames can end
  up with different resulting field sets if their schemas differ.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`AxelFieldAxisAccessor` API Reference](../../axel_fieldaxis_accessor_api_reference):
  every method available on the returned accessor.
- [`Axel::fields`](fields.md): select by exact field name instead of
  pattern or kind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="fields.html">
    <span class="label">Next</span>
    <span class="title">fields</span>
  </a>
</div>
