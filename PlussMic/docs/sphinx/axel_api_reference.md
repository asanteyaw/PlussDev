# Axel API Reference

This page documents the full public `Axel` surface for depth-stacked registries.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Axel` exposes:

- `fields(keys)` -> `Axel` projection
- `fieldaxis()` -> [`AxelFieldAxisAccessor`](axel_fieldaxis_accessor_api_reference.md)
- `cast()`, `nt()`, `tl()` -> nested accessors over the current axel

```{toctree}
:hidden:
:maxdepth: 1

axel_fieldaxis_accessor_api_reference
axel_cast_accessor_api_reference
axel_note_accessor_api_reference
axel_timeline_accessor_api_reference
api/axel/cast
api/axel/concat_depth
api/axel/defined
api/axel/depth
api/axel/distinct
api/axel/drop
api/axel/dropinf
api/axel/dropinvalid
api/axel/dropnan
api/axel/dropneginf
api/axel/dropnull
api/axel/dropposinf
api/axel/empty
api/axel/fieldaxis
api/axel/fields
api/axel/fillinf
api/axel/fillinvalid
api/axel/fillnan
api/axel/fillneginf
api/axel/fillnull
api/axel/fillposinf
api/axel/freq
api/axel/has
api/axel/head
api/axel/invalid
api/axel/isinf
api/axel/isnan
api/axel/isneginf
api/axel/isnull
api/axel/isposinf
api/axel/kindelize
api/axel/labels
api/axel/map
api/axel/ndistinct
api/axel/notinf
api/axel/notnan
api/axel/notnull
api/axel/nt
api/axel/registries
api/axel/registry
api/axel/rename
api/axel/show
api/axel/size
api/axel/slice
api/axel/tabelize
api/axel/tail
api/axel/tl
api/axel/to_json
api/axel/to_string
api/axel/unique
```
