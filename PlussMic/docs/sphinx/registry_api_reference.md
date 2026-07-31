# Registry API Reference

This page documents the full public `Registry` surface for named bundle slots.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Accessors

`Registry` exposes nested accessors, each with its own method pages and sidebar:

- `fields(keys)` -> [`RegistryFieldAccessor`](registry_field_accessor_api_reference.md)
- `fieldaxis()` -> [`RegistryFieldAxisAccessor`](registry_fieldaxis_accessor_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

api/registry/apply
api/registry/apply_
api/registry/apply_all
api/registry/as_kindle
api/registry/begin
api/registry/clear
api/registry/clear_
api/registry/cloc
api/registry/combine
api/registry/concat_values
api/registry/concat_values_
api/registry/defined
api/registry/distinct
api/registry/drop
api/registry/drop_empty
api/registry/drop_empty_
api/registry/dropnull
api/registry/empty
api/registry/end
api/registry/ensure_stream_compatible
api/registry/fieldaxis
api/registry/fields
api/registry/fillnull
api/registry/filter_values
api/registry/filter_values_
api/registry/freq
api/registry/from_kindle
api/registry/get
api/registry/has
api/registry/has_all
api/registry/has_any
api/registry/head
api/registry/invalid
api/registry/is_stream_compatible
api/registry/isnull
api/registry/kindelize
api/registry/load
api/registry/load_csv
api/registry/load_excel
api/registry/load_json
api/registry/load_parquet
api/registry/load_txt
api/registry/map
api/registry/map_
api/registry/max
api/registry/mean
api/registry/min
api/registry/ndistinct
api/registry/nfields
api/registry/notnull
api/registry/push
api/registry/push_
api/registry/push_all
api/registry/push_all_
api/registry/rename
api/registry/rename_
api/registry/sample
api/registry/save_as
api/registry/save_csv
api/registry/save_excel
api/registry/save_json
api/registry/save_parquet
api/registry/save_txt
api/registry/select
api/registry/set
api/registry/set_
api/registry/shape
api/registry/show
api/registry/size
api/registry/sizes
api/registry/slice
api/registry/sum
api/registry/tail
api/registry/take
api/registry/take_values
api/registry/take_values_
api/registry/to_json
api/registry/to_kindle
api/registry/to_string
api/registry/transform_values
api/registry/transform_values_
api/registry/type_profiles
api/registry/unique
```
