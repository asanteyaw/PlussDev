<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3b9517d24c2 -->

# Registry::take

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::take(std::span<const std::int64_t> indices) const
Registry Registry::take(const std::vector<std::int64_t>& indices) const
Registry Registry::take(std::string_view key, std::span<const std::int64_t> indices) const
Registry Registry::take(std::string_view key, const std::vector<std::int64_t>& indices) const
```

## What It Does

`take()` has **two roles**, split by argument count:

- **No-key overloads** — `take(indices)` applies the same positional index list
  to **every** field independently, applying
  [`Slot::take(indices)`](../slot/take.md) to each. Unlike
  [`slice()`](slice.md)/[`head()`](head.md)/[`tail()`](tail.md), it is not
  restricted to a contiguous range — indices may repeat, skip around, or reorder
  arbitrarily.
- **Keyed overloads** — `take(key, indices)` scopes the same positional
  selection to a **single named field**, leaving every other field untouched.

```{important} Out-of-range indices throw — no clamping, no negative-index wraparound
Both roles perform no clamping and no Python-style negative-index support. Each
index is looked up via `Slot::at()`, which throws
`"Slot::at: index out of bounds"` for any index `< 0` or `>= size()` for that
field. On a ragged registry, an index valid for one field can throw for a shorter
one — and the keyed overload only checks the one field you name.
```

## Parameters

**indices** (`std::span<const std::int64_t>` / `const std::vector<std::int64_t>&`)
    The positions to select. May repeat or appear out of order.

**key** (`std::string_view`, keyed overloads only)
    The single field to scope the selection to. Throws if it doesn't exist.

## Returns

`Registry`
    The no-key overloads return a new registry with every field's `Slot`
    rebuilt from `indices`. The keyed overloads return a new registry with only
    the named field's `Slot` rebuilt; every other field is unchanged.

## Examples

Example 1. Reorder and repeat positions across every field.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

std::vector<std::int64_t> picks{3, 0, 0};
auto out = rg.take(picks);
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
[4, 1, 1]
```

Example 2. An out-of-range index throws — no clamping.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
});

std::vector<std::int64_t> picks{99};
try {
  auto out = rg.take(picks);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slot::at: index out of bounds
```

Example 3. The keyed overload reorders one field's `Slot`, leaving the rest of
the registry — including that field's own length elsewhere in a ragged
registry — untouched.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{2, 0};
auto out = rg.take("id", picks);
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
[3, 1] city size=3
```

## Notes

- No clamping and no negative-index support in either role — contrast with
  [`slice()`](slice.md)'s important callout, which clamps and supports negative
  indices.
- The keyed overload throws `"Registry::take: unknown key '<name>'"` for a
  missing field.
- [`sample()`](sample.md) is built on top of the no-key `take(indices)`, with an
  internally generated shuffled index list.
- Never mutates the receiver — use [`take_()`](take_.md) for the in-place forms.

## See Also

- [`Registry::take_`](take_.md): the in-place counterparts.
- [`Registry::slice`](slice.md): the clamping, negative-index-aware, contiguous-range
  alternative to the no-key overloads.
- [`Registry::sample`](sample.md): random sampling, implemented via the no-key
  `take(indices)`.
- [`Registry::concat`](concat.md): a related field-scoped operation, appending
  rather than reordering.
- [`Catalog::take`](../catalog/take.md): the equivalent whole-catalog operation
  on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="take_.html">
    <span class="label">Next</span>
    <span class="title">take_</span>
  </a>
</div>
