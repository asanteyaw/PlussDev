# AxelFieldAccessor API Reference

This page documents the public multi-key field accessor returned by `Axel::fields(keys)`, applied frame-by-frame across the depth stack.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`AxelFieldAccessor` exposes nested accessors, each with its own method pages and sidebar:

- `fields(keys).cast()` -> [`AxelCastAccessor`](axel_cast_accessor_api_reference.md)
- `fields(keys).nt()` -> [`AxelNoteAccessor`](axel_note_accessor_api_reference.md)
- `fields(keys).tl()` -> [`AxelTimelineAccessor`](axel_timeline_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

api/axel-field/astype
api/axel-field/cast
api/axel-field/distinct
api/axel-field/dropnull
api/axel-field/fillnull
api/axel-field/freq
api/axel-field/invalid
api/axel-field/isnull
api/axel-field/ndistinct
api/axel-field/notnull
api/axel-field/nt
api/axel-field/tl
```
