# DataFrame::matrix

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::matrix() -> MatrixValues
```

## What It Does

Packs same-kind features into matrix-shaped pluss blocks so lower-level numeric, text, or temporal kernels can consume them efficiently.

## Parameters

`None`

## Returns

`MatrixValues`
    Grouped matrix blocks split by payload family (`Series`, `Note`, `Timeline`, and `TimeIndex`).

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto blocks = df.matrix();
auto out = fmt::format(
    "series blocks: {}\nnote blocks: {}\ntimeline blocks: {}\ntimeindex blocks: {}",
    blocks.series_span().size(),
    blocks.note_span().size(),
    blocks.timeline_span().size(),
    blocks.timeindex_span().size());
```

### Output

```text
series blocks: 1
note blocks: 1
timeline blocks: 0
timeindex blocks: 0
```

## Practical Behavior

- `matrix()` does not coerce every feature into one mixed block.
- Instead, it packs features by payload family so numeric, note, timeline, and timeindex data stay type-stable.
- Use `as_series()`, `as_note()`, `as_timeline()`, or `as_timeindex()` only when the frame contributes exactly one block of that family.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
