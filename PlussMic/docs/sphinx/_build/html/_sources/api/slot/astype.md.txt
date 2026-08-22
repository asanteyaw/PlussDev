<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e4ecefa27d7c -->

# Slot::astype

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::astype(pluss::codem::ScalarType dtype) const
```

## What It Does

Casts every value to `Series`-kind with scalar dtype `dtype`. A narrow
shorthand for `cast(ThreadCastOptions{.series_dtype = dtype}).series()`.

## Parameters

**dtype** (`pluss::codem::ScalarType`)
    The target numeric scalar dtype (e.g. `Int64`, `Float64`, `Bool`).

## Returns

`Slot`
    A new slot, every value cast to `Series`-kind with dtype `dtype`.

## Examples

Example 1. Cast an `int64` `Series` slot to `float64`.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)});
auto out = slot.astype(pluss::codem::ScalarType::Float64);
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Series[float64]",
    "Series[float64]"
  ]
}
```

Example 2. A single-value slot converts the same way a multi-value one
does.

```cpp
auto single = pluss::scribble::Slot({pluss::make_series(1)});
auto out = single.astype(pluss::codem::ScalarType::Float64);
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    "Series[float64]"
  ]
}
```

## Notes

- Always targets `Series`-kind — for `Note`/`Timeline`/`TimeIndex` targets,
  use [`cast()`](cast.md)'s full accessor instead.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::cast`](cast.md): the general entry point `astype()` is a
  narrower shorthand for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../slot_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Slot API</span>
  </a>
  <a class="next-link" href="at.html">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>
