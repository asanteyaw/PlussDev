<!-- pluss-hand-authored -->

# Registry::type_profiles

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> Registry::type_profiles() const
```

## What It Does

Returns each field's value-kind tally, keyed by field name — for every field, a
nested map from kind name (`"Series"`, `"Note"`, `"Timeline"`, `"TimeIndex"`) to how
many values of that kind the field holds. This is the same per-field tally
[`to_string()`](to_string.md)'s `"types"` section prints, exposed as data you can
use directly rather than parsed back out of a rendered string.

```{important} A stream-compatible field has exactly one kind in its tally
A field where [`is_stream_compatible()`](is_stream_compatible.md) would report
`true` for the whole registry has a single-entry inner map (one kind, count equal
to the field's `size()`). A genuinely mixed field's inner map has more than one
entry — that's what makes it not stream-compatible. See Example 1's `mixed` field.
```

## Parameters

`None`

## Returns

`std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>`
    A map from field name to a map from value-kind name to count.

## Examples

Example 1. A registry with one uniform field and one genuinely mixed field, read
back sorted for a deterministic printout.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

auto profiles = rg.type_profiles();
std::vector<std::string> fields;
for (const auto& [field, tally] : profiles) fields.push_back(field);
std::sort(fields.begin(), fields.end());

for (const auto& field : fields) {
  std::cout << field << ": {";
  std::vector<std::pair<std::string, std::int64_t>> items(profiles[field].begin(), profiles[field].end());
  std::sort(items.begin(), items.end());
  for (const auto& [kind, count] : items) std::cout << kind << ":" << count << " ";
  std::cout << "}\n";
}
```

### Output

```text
id: {Series:3 }
mixed: {Note:1 Series:2 }
```

## Notes

- Unordered at both levels — iterate a sorted copy (as in the example) if you need
  deterministic output.
- A field's inner-map entry count directly determines whether that field is
  stream-compatible — see the important callout above.
- This is the same per-field data [`to_string()`](to_string.md) renders under
  `"types"` — use `type_profiles()` when you need the counts programmatically
  rather than as a printed string.

## See Also

- [`Registry::to_string`](to_string.md): renders this same information as a JSON
  summary string.
- [`Registry::is_stream_compatible`](is_stream_compatible.md): the boolean question
  each field's tally-entry-count answers.
- [`Slot::type_counts`](../slot/type_counts.md): the per-field tally this method
  gathers across every field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform_values_.html">
    <span class="label">Previous</span>
    <span class="title">transform_values_</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
