# Dispatch: transmit and map_axis

[Back to Core Utilities](../core.md) · [Core Utilities API Reference](../../api_reference.md)

Dispatch applies one callable to many values. `transmit` works over an
ordinary range, a single value, or a pair of ranges. `map_axis` does the same
job along one axis of a `Series`, `Note`, or `Timeline`.

- [`transmit`](../core-transmit.md): every call form, `transmit_into`, and the
  parallel execution option.
- [`map_axis`](../core-map-axis.md): cutting a table into slices along an
  axis, for all three value types.

For a guide with worked programs, see
[Mapping with transmit](../../manual/tils-aux-transmit.md) and
[Mapping along an axis with translate](../../manual/tils-aux-translate.md).

```{toctree}
:maxdepth: 1
:hidden:

../core-transmit
../core-map-axis
```
