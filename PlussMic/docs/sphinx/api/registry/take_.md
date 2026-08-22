<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bdfdeda86a2a -->

# Registry::take_

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry& Registry::take_(std::string_view key, std::span<const std::int64_t> indices)
Registry& Registry::take_(std::string_view key, const std::vector<std::int64_t>& indices)
```

## What It Does

The in-place counterpart to [`take()`](take.md)'s **keyed** overloads — reorders
or selects positions within one named field's `Slot` directly, leaving every
other field untouched. There is no in-place form of the no-key,
every-field `take(indices)`; `take_()` always requires a `key`.

```{important} Out-of-range indices throw — no clamping, no negative-index wraparound
Same as `take()`'s keyed overloads: each index is looked up via `Slot::at()`,
which throws `"Slot::at: index out of bounds"` for any index `< 0` or
`>= size()`.
```

## Parameters

**key** (`std::string_view`)
    The field to reorder. Throws if it doesn't exist.

**indices** (`std::span<const std::int64_t>` / `const std::vector<std::int64_t>&`)
    The positions to select, applied to just this field. May repeat or appear
    out of order.

## Returns

`Registry&`
    A reference to this registry, now holding the reordered field.

## Examples

Example 1. Select a single position from `id`, in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

std::vector<std::int64_t> picks{1};
rg.take_("id", picks);
auto ids = rg.fields("id");
std::cout << "[" << std::get<pluss::Series>(ids.at(0)).to_vec<std::int64_t>()[0] << "]";
```

### Output

```text
[2]
```

## Notes

- Requires a `key` — there is no in-place equivalent of the no-key,
  every-field `take(indices)` overload.
- Same throw conditions as `take()`'s keyed overloads.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::take`](take.md): the non-mutating counterpart, including the
  no-key, every-field overloads this method doesn't have.
- [`Registry::concat_`](concat_.md): a related field-scoped, in-place operation,
  appending rather than reordering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
