<!-- pluss-hand-authored -->

# Registry::cloc

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::cloc(std::int64_t row) const
Registry Registry::cloc(std::int64_t start, std::int64_t stop) const
```

## What It Does

A thin, positional-selection wrapper over [`slice()`](slice.md):

- `cloc(row)` — selects a **single** row across every field, equivalent to
  `slice(row, row + 1)`.
- `cloc(start, stop)` — selects a **range** across every field, equivalent to
  `slice(start, stop)` directly.

Both overloads inherit `slice()`'s per-field, independent-length behavior — see
[`slice()`](slice.md)'s important callout for how this plays out on a ragged
registry.

## Parameters

**row** (`std::int64_t`)
    The single positional row to select (single-row overload). Negative values
    count from the end of each field.

**start** / **stop** (`std::int64_t`)
    The `[start, stop)` positional range to select (range overload). Negative
    values count from the end of each field.

## Returns

`Registry`
    A new registry with each field's `Slot` restricted to the selected row or
    range.

## Examples

Example 1. Select a single row by position.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.cloc(1);
auto ids = out.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[2]
```

Example 2. Select a range by position — identical to `slice(start, stop)`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.cloc(0, 2);
auto ids = out.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2]
```

## Notes

- `cloc(row)` is literally implemented as `slice(row, row + 1)` — there is no
  behavioral difference beyond call-site convenience for the single-row case.
- Both overloads share `slice()`'s per-field independent clamping and negative-index
  support.

## See Also

- [`Registry::slice`](slice.md): the method both `cloc()` overloads delegate to.
- [`Registry::head`](head.md) / [`Registry::tail`](tail.md): other `slice()`-based
  positional selections.
- [`Registry::take`](take.md): arbitrary (not-necessarily-contiguous) positional
  selection, for when a single row or contiguous range isn't enough.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_.html">
    <span class="label">Previous</span>
    <span class="title">clear_</span>
  </a>
  <a class="next-link" href="combine.html">
    <span class="label">Next</span>
    <span class="title">combine</span>
  </a>
</div>
