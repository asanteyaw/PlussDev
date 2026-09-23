<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1d063783034 -->

# Axel::to_json

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
std::string Axel::to_json() const
```

## What It Does

Renders a full, unabridged JSON-formatted dump of this `Axel` — every
registry frame's complete [`Registry::to_json()`](../registry/to_json.md)
output, embedded verbatim, with no truncation and no value elision.

```{important} `to_json()` is the full dump; `to_string()` is a bounded summary
See the important callout on [`to_string()`](to_string.md) — the two
methods diverge sharply for `Axel`. `to_json()` never truncates
regardless of depth, and it includes every field's actual values (via
each frame's own `Registry::to_json()`), not just shapes.
```

## Parameters

`None`

## Returns

`std::string`
    A pretty-printed JSON-formatted string with every frame's full
    content.

## Examples

Example 1. A small, unlabeled, two-frame `Axel` — every value is
present in the output.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})}}),
});
std::cout << axel.to_json();
```

### Output

```text
{
  "_type": "Axel",
  "defined": true,
  "depth": 1,
  "labels": false,
  "registries": [
{
  "_type": "Registry",
  "defined": true,
  "nfields": 1,
  "fields": {
    "ids": {
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Series[int64]",
    "Series[int64]"
  ]
}
  }
}
  ]
}
```

Each frame's `Registry::to_json()` output is embedded exactly as that
method would produce it on its own — note the embedded block's own
indentation doesn't nest further under `"registries"`, since it's
spliced in verbatim rather than re-indented.

Example 2. An undefined `Axel`.

```cpp
pluss::scribble::Axel undef;
std::cout << undef.to_json();
```

### Output

```text
{ "_type": "Axel", "defined": false }
```

## Notes

- See the important callout above — unlike `to_string()`, `to_json()`
  never truncates, no matter how many frames the `Axel` has.
- Each embedded registry block is exactly what
  [`Registry::to_json()`](../registry/to_json.md) would produce called
  directly on that frame — spliced in without adjusting its
  indentation to the surrounding structure.
- This is genuinely different output from [`to_string()`](to_string.md) —
  an important asymmetry to remember, since `Slot`'s equivalent trio of
  methods are all identical to each other.

## See Also

- [`Axel::to_string`](to_string.md): the bounded, value-eliding summary
  form.
- [`Registry::to_json`](../registry/to_json.md): the per-frame method
  embedded here.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
