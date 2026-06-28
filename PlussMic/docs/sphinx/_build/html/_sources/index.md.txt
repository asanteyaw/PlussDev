# PlussMic User Documentation

PlussMic is the data-analysis layer in the Pluss stack. It gives you three related models:

- `table`: `DataFrame`, `Thread`, `Atlas`
- `kindle`: `Stream`, `Catalog`, `Archive`
- `bundle`: `Slot`, `Registry`, `Aisle`

This manual is organized the same way users usually approach the library:

- start with `DataFrame` for table-style analysis
- use `Thread` when you want typed column operations outside a frame
- use `Catalog` and `Archive` when streams/case-aligned data are a better fit
- use `Registry` and `Aisle` when you want heterogeneous record bundles

## Where To Start

If you are trying to answer "how do I do the table/dataframe thing in PlussMic?", read
these in order:

1. [DataFrame Guide](dataframe.md)
2. [Part V: DataFrame Workflows](manual/part05_dataframe_workflows.md)
3. [DataFrame API Reference](dataframe_api_reference.md)
4. [Thread API Reference](thread_api_reference.md)

Use the guides for task-oriented examples and use the API reference for exact
signatures, overloads, and parameter behavior.

```{toctree}
:maxdepth: 2

manual
quickstart
demo_catalog
dataframe
time_series_accessor_guide
dataframe_display
dataframe_api_reference
column_ref_api_reference
dataframe_plot_api_reference
dataframe_sql_accessor_api_reference
dataframe_sql_prepared_api_reference
thread_atlas
thread_api_reference
thread_value_api_reference
atlas_api_reference
kindle
stream_api_reference
catalog_api_reference
archive_api_reference
bundle
slot_api_reference
registry_api_reference
aisle_api_reference
build_outputs
```
