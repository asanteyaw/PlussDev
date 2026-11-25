<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eb0647d52d7e -->

# Registry::sloc

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::sloc(std::int64_t state) const
Registry Registry::sloc(std::int64_t start, std::int64_t stop) const
```

## What It Does

A thin, positional-selection wrapper over [`slice()`](slice.md):

- `sloc(state)` — selects a **single** state across every field, equivalent to
  `slice(state, state + 1)`.
- `sloc(start, stop)` — selects a **range** across every field, equivalent to
  `slice(start, stop)` directly.

Both overloads inherit `slice()`'s per-field, independent-length behavior — see
[`slice()`](slice.md)'s important callout for how this plays out on a ragged
registry.

## Parameters

**state** (`std::int64_t`)

The single positional state to select (single-state overload). Negative values
count from the end of each field.

**start** / **stop** (`std::int64_t`)

The `[start, stop)` positional range to select (range overload). Negative
values count from the end of each field.

## Returns

`Registry`
    A new registry with each field's `Slot` restricted to the selected state or
    range.

## Examples

Example 1. Select a single state by position.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.sloc(1);
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[2]
```

Example 2. Select a range by position — identical to `slice(start, stop)`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

auto out = rg.sloc(0, 2);
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2]
```

## Notes

- `sloc(state)` is literally implemented as `slice(state, state + 1)` — there is no
  behavioral difference beyond call-site convenience for the single-state case.
- Both overloads share `slice()`'s per-field independent clamping and negative-index
  support.

## See Also

- [`Registry::slice`](slice.md): the method both `sloc()` overloads delegate to.
- [`Registry::head`](head.md) / [`Registry::tail`](tail.md): other `slice()`-based
  positional selections.
- [`Registry::take`](take.md): arbitrary (not-necessarily-contiguous) positional
  selection, for when a single state or contiguous range isn't enough.

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
