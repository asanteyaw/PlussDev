<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Registry::to_string

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::string Registry::to_string() const
```

## What It Does

Renders the registry as a JSON **summary** string — one entry per field, each
showing its `size` and a tally of value kinds (`types`). This is the format
[`operator<<`](../../registry_api_reference)/[`show()`](show.md) also use; all
three are identical. It is a coarser summary than [`to_json()`](to_json.md),
which lists every individual value's type instead of just a per-kind count.

## Parameters

`None`

## Returns

`std::string`
    A JSON summary of the registry's fields.

## Examples

Example 1. A registry with a uniform-kind field and a mixed-kind field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello")})},
});

std::cout << rg.to_string();
```

### Output

```text
{
  "_type": "Registry",
  "defined": true,
  "nfields": 2,
  "fields": {
    "id": {
      "size": 2,
      "types": {
        "Series": 2
      }
    },
    "mixed": {
      "size": 2,
      "types": {
        "Note": 1,
        "Series": 1
      }
    }
  }
}
```

## Notes

- Identical output to `operator<<` and [`show()`](show.md).
- A field's `types` tally having more than one entry is exactly what makes that
  field not stream-compatible — see [`Registry::to_kindle`](to_kindle.md).
- On an undefined registry, returns `{ "_type": "Registry", "defined": false }`
  rather than throwing.

## See Also

- [`Registry::to_json`](to_json.md): the finer-grained, per-value counterpart.
- [`Registry::show`](show.md): writes this same string to a caller-provided
  stream.
- [`Registry::profiles`](profiles.md): the same per-field type tally, as data
  rather than a rendered string.
- [`Catalog::to_string`](../catalog/to_string.md): the equivalent method on the
  kindle model (a box-table format, not JSON).

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
