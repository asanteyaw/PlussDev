<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c55dd7929cac -->

# Registry::combine

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::combine(const Registry& other, std::string suffix = "_other") const
```

## What It Does

Merges all of `other`'s fields into a copy of this registry and returns the
result. Fields unique to either side pass through unchanged. When a field name
exists in **both** registries, this registry's field wins the plain name, and
`other`'s field is added under `name + suffix` — if that suffixed name *also*
collides (with either registry's fields, including ones already renamed by a
prior collision in the same call), a numeric counter (`_2`, `_3`, ...) is appended
until a unique name is found.

## Parameters

**other** (`const Registry&`)
    The registry whose fields to merge in. Must be defined.

**suffix** (`std::string`, optional, defaults to `"_other"`)
    The suffix appended to `other`'s field name on a collision.

## Returns

`Registry`
    A new registry containing every field from both registries, positioned
    this-registry-first then `other`-second, with collisions resolved via the
    suffix rule above.

## Examples

Example 1. No overlapping field names — a straightforward union.

```cpp
auto left = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1)})}});
auto right = pluss::bundle::Registry({{"city", pluss::bundle::Slot({pluss::make_note("Accra")})}});

auto out = left.combine(right);
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id city
```

Example 2. A colliding field name gets the default `"_other"` suffix.

```cpp
auto left = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1)})}});
auto right = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(99)})}});

auto out = left.combine(right);
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id id_other
```

Example 3. Pass an explicit suffix instead of the default.

```cpp
auto left = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1)})}});
auto right = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(99)})}});

auto out = left.combine(right, "_right");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id id_right
```

Example 4. If the suffixed name is *also* already taken, a numeric counter is
appended until the name is unique.

```cpp
auto left = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
    {"id_other", pluss::bundle::Slot({pluss::make_series(2)})},
});
auto right = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(99)})}});

auto out = left.combine(right);
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id id_other id_other_2
```

## Notes

- This registry's field wins the plain name on a collision; `other`'s field is
  always the one that gets renamed.
- Field order in the result is this-registry's fields (in their original order),
  followed by `other`'s fields (in their original order, suffixed where needed).
- Neither input registry is mutated — `combine()` always returns a new `Registry`.
- Both registries must be defined — throws `"Registry: undefined handle"` /
  `"Registry::combine: other registry undefined"` otherwise.

## See Also

- [`Registry::rename`](rename.md): rename a field explicitly, if you'd rather
  control collision naming yourself before combining.
- [`Registry::fields`](fields.md) / [`Registry::drop`](drop.md): narrow a
  registry's fields before combining, to avoid unwanted collisions entirely.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cloc.html">
    <span class="label">Previous</span>
    <span class="title">cloc</span>
  </a>
  <a class="next-link" href="concat.html">
    <span class="label">Next</span>
    <span class="title">concat</span>
  </a>
</div>
