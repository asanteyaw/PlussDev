# PlussMic User Documentation

PlussMic is the data-analysis layer in the Pluss stack. Its public surface is organized around three models:

- `table`: `DataFrame`, `Thread`, `Atlas`
- `kindle`: `Stream`, `Catalog`, `Archive`
- `bundle`: `Slot`, `Registry`, `Aisle`

Use the guides when you want workflow-oriented explanations. Use the API reference when you want exhaustive method signatures, parameter semantics, overload behavior, and concrete examples with verified output directly underneath the code.

## Recommended Start Order

If you are learning the table workflow first, read these in order:

1. [Quickstart](quickstart.md)
2. [DataFrame Guide](dataframe.md)
3. [Manual: DataFrame Workflows](manual/part05_dataframe_workflows.md)
4. [API Reference](api_reference.md)

## API Reference

The API reference is navigable directly from this landing page.

Start with the stack branch that matches the object you are holding in code:

```{toctree}
:maxdepth: 1

api_reference
```

### PlussMic

- [PlussMic API Reference](api/plussmic_root.md)
- [table](api/plussmic_table.md): `DataFrame`, `Thread`, `Atlas`
- [kindle](api/plussmic_kindle.md): `Stream`, `Catalog`, `Archive`
- [bundle](api/plussmic_bundle.md): `Slot`, `Registry`, `Aisle`
- [Plotting](api/plussmic_plotting.md)

### PlussQit

- [PlussQit API Reference](api/plussqit_root.md)
- [Series](api/plussqit_series.md)
- [Note](api/plussqit_note.md)
- [Timeline](api/plussqit_timeline.md)
- [TimeIndex](api/plussqit_timeindex.md)

### PlussTils

- [PlussTils API Reference](api/plusstils_root.md)
- [Pathing](api/plusstils_pathing.md)
- [PlussView](api/plusstils_view.md)
- [Aux Utilities](api/plusstils_aux.md)

```{toctree}
:hidden:
:maxdepth: 2

quickstart
manual
dataframe
kindle
bundle
demo_catalog
build_outputs
```
