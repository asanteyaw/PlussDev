<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Axel::to_string

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
std::string Axel::to_string() const
```

## What It Does

Renders a curated, human-readable JSON-formatted summary of this
`Axel` — depth, label summary (if labeled), and per-frame field
lists with slot sizes. For a depth greater than `6`, the middle frames
are elided with a `"... N registries omitted"` marker, showing only the
first `3` and last `2` frames.

```{important} `to_string()` is a curated summary; `to_json()` is a full, unabridged dump
Unlike `Slot`, where `to_string()`/`to_json()`/`show()` are byte-identical,
`Axel::to_string()` and [`Axel::to_json()`](to_json.md) diverge sharply:
`to_string()` never grows unbounded (it truncates past 6 frames and only
shows field names + slot sizes, not values), while `to_json()` embeds
every frame's full [`Registry::to_json()`](../registry/to_json.md)
output — including every value — with no truncation at all. Use
`to_string()`/`show()` for a quick console-friendly overview; use
`to_json()` when you need the complete, machine-readable structure.
```

## Parameters

`None`

## Returns

`std::string`
    A pretty-printed JSON-formatted summary string.

## Examples

Example 1. A small, unlabeled, two-frame `Axel`.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})}}),
});
std::cout << axel.to_string();
```

### Output

```text
{
  "_type": "Axel",
  "defined": true,
  "depth": 2,
  "labels": false,
  "registries": [
    {
      "index": 0,
      "nfields": 1,
      "fields": [
        "ids"
      ],
      "slot_sizes": {
        "ids": 2
      }
    },
    {
      "index": 1,
      "nfields": 1,
      "fields": [
        "ids"
      ],
      "slot_sizes": {
        "ids": 2
      }
    }
  ]
}
```

Notice `slot_sizes` reports each field's row count, not its values —
`to_string()` never renders actual value content, only shape.

Example 2. An undefined `Axel`.

```cpp
pluss::bundle::Axel undef;
std::cout << undef.to_string();
```

### Output

```text
{ "_type": "Axel", "defined": false }
```

## Notes

- See the important callout above — `to_string()` and
  [`to_json()`](to_json.md) are genuinely different, unlike the
  Slot-level trio (`to_string()`/`to_json()`/`show()` there are all
  identical).
- Past a depth of `6`, only the first `3` and last `2` frames are shown,
  with a `"... N registries omitted"` marker in between — the summary
  stays bounded regardless of how deep the `Axel` is.
- [`show()`](show.md) writes this exact string to an `std::ostream`.

## See Also

- [`Axel::to_json`](to_json.md): the full, unabridged, value-including
  dump.
- [`Axel::show`](show.md): streams this same string to an
  `std::ostream`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_kindle.html">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
