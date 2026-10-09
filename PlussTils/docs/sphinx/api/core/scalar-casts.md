# Scalar casts: the as_ family

[Back to Core Utilities](../core.md) · [Core Utilities API Reference](../../api_reference.md)

Thirteen dtypes each have the same four overloads: tag a `Config`, convert a
`Series`, build a `Series` from a host value, or read a one-element `Series`
back as a host value. Each overload has its own reference page, since the job
differs even though the name is shared.

- [Overview](../core-scalar-cast.md): the shared contract across all thirteen
  dtypes.
- [Config form](../core-scalar-cast/config-form.md)
- [Conversion form](../core-scalar-cast/conversion-form.md)
- [Construction form](../core-scalar-cast/construction-form.md)
- [Extraction form](../core-scalar-cast/extraction-form.md)
- [`as_dtype`](../core-scalar-cast/as-dtype.md): the untyped form, for a dtype
  chosen at runtime.
- [`to_scalar` and `is_scalar_extractable_v`](../core-scalar-cast/to-scalar.md):
  the general form of extraction.

For a guide with worked programs, see
[Scalar casts](../../manual/tils-aux-scalar-casts.md).

```{toctree}
:maxdepth: 1
:hidden:

../core-scalar-cast
../core-scalar-cast/config-form
../core-scalar-cast/conversion-form
../core-scalar-cast/construction-form
../core-scalar-cast/extraction-form
../core-scalar-cast/as-dtype
../core-scalar-cast/to-scalar
```
