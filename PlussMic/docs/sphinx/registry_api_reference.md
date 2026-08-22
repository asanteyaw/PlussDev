# Registry API Reference

This page documents the full public `Registry` surface for named scribble slots.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Registry` exposes:

- `fields(keys)` -> `Registry` projection
- `fieldaxis()` -> [`RegistryFieldAxisAccessor`](registry_fieldaxis_accessor_api_reference.md)
- `cast()`, `nt()`, `tl()` -> nested accessors over the current registry

```{toctree}
:hidden:
:maxdepth: 1

registry_fieldaxis_accessor_api_reference
registry_cast_accessor_api_reference
registry_note_accessor_api_reference
registry_timeline_accessor_api_reference
api/registry/begin
api/registry/cast
api/registry/clear
api/registry/clear_
api/registry/cloc
api/registry/combine
api/registry/concat
api/registry/concat_
api/registry/defined
api/registry/distinct
api/registry/drop
api/registry/dropnull
api/registry/empty
api/registry/end
api/registry/fieldaxis
api/registry/fields
api/registry/fillnull
api/registry/freq
api/registry/get
api/registry/has
api/registry/has_all
api/registry/has_any
api/registry/head
api/registry/invalid
api/registry/isnull
api/registry/keep
api/registry/keep_
api/registry/kindelize
api/registry/load
api/registry/load_csv
api/registry/load_excel
api/registry/load_json
api/registry/load_parquet
api/registry/load_txt
api/registry/map
api/registry/map_
api/registry/maxstate
api/registry/minstate
api/registry/morph
api/registry/morph_
api/registry/ndistinct
api/registry/nfields
api/registry/notnull
api/registry/nstate
api/registry/nt
api/registry/profiles
api/registry/prune
api/registry/prune_
api/registry/push
api/registry/push_
api/registry/rename
api/registry/rename_
api/registry/sample
api/registry/save_as
api/registry/save_csv
api/registry/save_excel
api/registry/save_json
api/registry/save_parquet
api/registry/save_txt
api/registry/set
api/registry/set_
api/registry/shape
api/registry/show
api/registry/size
api/registry/sizes
api/registry/slice
api/registry/tabelize
api/registry/tail
api/registry/take
api/registry/take_
api/registry/tl
api/registry/to_json
api/registry/to_string
api/registry/unique
```
