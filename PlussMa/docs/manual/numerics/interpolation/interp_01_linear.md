<!-- generated from interp_01_linear.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Interpolation 1: Linear Interpolation

A table gives a function at a few points; interpolation estimates it between them. Pluss provides piecewise-linear interpolation: join the table points by straight segments and read the curve at the query points. After this chapter you can read a table at one or many points, choose what happens outside it, interpolate several curves at once, differentiate the result, and judge the accuracy.

Every program is complete and its output is the output of that program.

## The call

```cpp
Series interp::linear(const Series& coordinate, const Series& values, const Series& query,
                      interp::EdgeMode edge = interp::EdgeMode::Clamp);
```

| Argument | Shape and rules |
|---|---|
| `coordinate` | one-dimensional, at least two points, **strictly increasing** (spacing need not be equal) |
| `values` | `[..., n]` with `n = coordinate.size`; any leading axes are a batch of curves sharing the coordinate; floating or complex |
| `query` | a scalar or a one-dimensional vector of `m` points; the same floating dtype as `coordinate` |
| result | the shape of `values` with its last axis replaced by `m` (or removed for a scalar query) |

All inputs must be finite and on the same platform; violations throw `std::invalid_argument`.

`EdgeMode` decides what a query outside `[coordinate[0], coordinate[n-1]]` returns:

| `EdgeMode` | Outside the table |
|---|---|
| `Clamp` (default) | the nearest end value |
| `Linear` | the line through the first (or last) two points, continued |
| `Error` | throws `std::out_of_range` |

## Programs

- [Reading a table](interp_01_linear/reading_a_table.md)
- [Queries outside the table](interp_01_linear/queries_outside_the_table.md)
- [Batches, complex values and gradients](interp_01_linear/batches_complex_values_and_gradients.md)
- [Accuracy](interp_01_linear/accuracy.md)

## Checklist

1. `coordinate` strictly increasing; the same dtype as the query.
2. Pick `EdgeMode` deliberately: `Error` while developing.
3. Interpolate batches of curves in one call.
4. Expect second-order accuracy; halve the spacing for four times the accuracy.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

interp_01_linear/reading_a_table
interp_01_linear/queries_outside_the_table
interp_01_linear/batches_complex_values_and_gradients
interp_01_linear/accuracy
```
<!-- apiref:toctree:end -->
