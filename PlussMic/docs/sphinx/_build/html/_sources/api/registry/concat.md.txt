<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72a3e0c03c2b -->

# Registry::concat

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::concat(std::string_view key, const Slot& other) const
```

## What It Does

Returns a new registry with `other`'s values appended to one named field's
`Slot` — every other field is left untouched. Built on
[`Slot::concat()`](../slot/concat.md).

## Parameters

**key** (`std::string_view`)
    The field to append to. Throws if it doesn't exist.

**other** (`const Slot&`)
    The values to append. Must be defined.

## Returns

`Registry`
    A new registry with `other`'s values appended to the named field's `Slot`;
    every other field unchanged.

## Examples

Example 1. Append two more values onto `id`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::scribble::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = rg.concat("id", extra);

auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "] city size=" << out.fields("city").size();
```

### Output

```text
[1, 2, 3, 4, 5] city size=3
```

## Notes

- Scoped to exactly one field — appending to `id` doesn't touch `city`'s
  length, so a previously-aligned registry can come out ragged.
- Throws `"Registry::concat: unknown key '<name>'"` for a missing field, and
  `"Registry::concat: other slot undefined"` if `other` isn't defined.
- Never mutates the receiver — use [`concat_()`](concat_.md) for the in-place
  form.

## See Also

- [`Registry::concat_`](concat_.md): the in-place counterpart.
- [`Registry::push`](push.md): append a single value, rather than an entire
  `Slot`'s worth.
- [`Registry::combine`](combine.md): merges whole *registries* by field name,
  rather than appending values within one field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="combine.html">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="concat_.html">
    <span class="label">Next</span>
    <span class="title">concat_</span>
  </a>
</div>
