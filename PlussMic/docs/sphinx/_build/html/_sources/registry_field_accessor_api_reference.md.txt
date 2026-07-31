# RegistryFieldAccessor API Reference

This page documents the public multi-key field accessor returned by `Registry::fields(keys)`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`RegistryFieldAccessor` exposes nested accessors, each with its own method pages and sidebar:

- `fields(keys).cast()` -> [`RegistryCastAccessor`](registry_cast_accessor_api_reference.md)
- `fields(keys).nt()` -> [`RegistryNoteAccessor`](registry_note_accessor_api_reference.md)
- `fields(keys).tl()` -> [`RegistryTimelineAccessor`](registry_timeline_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

api/registry-field/astype
api/registry-field/cast
api/registry-field/distinct
api/registry-field/dropnull
api/registry-field/fillnull
api/registry-field/freq
api/registry-field/invalid
api/registry-field/isnull
api/registry-field/ndistinct
api/registry-field/notnull
api/registry-field/nt
api/registry-field/tl
```
