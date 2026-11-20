<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 912efca41ceb -->

# Shelf::as_series

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
const pluss::Series& Shelf::as_series() const
```

## What It Does

Exposes the underlying `pluss::Series` payload directly, by reference — the numeric escape
hatch for when you need the full `Series` surface (native reductions, elementwise
arithmetic, kernels that live below the table layer) rather than the curated subset
`Shelf` re-exposes as its own methods. Requires the shelf to be `Series`-kind; calling it
on a `Note`/`Timeline`/`TimeIndex` shelf throws.

```{important} Returns a reference, not a copy — and does not rewrap into Shelf
`as_series()` hands back `const pluss::Series&`, not a `Shelf`. It does no rewrapping and
performs no computation of its own. If you run `Series`-level operations and want the
result usable by `Shelf`-only methods again afterward, wrap it explicitly:
`Shelf(scores.as_series() + 5.0)`. Forgetting this step is a common way to end up with a
`pluss::Series` where a `Shelf` was expected.
```

## Parameters

`None`

## Returns

`const pluss::Series&`
    A reference to the underlying series payload, valid as long as the `Shelf` itself
    stays alive.

## Examples

Example 1. Drop to `Series` for arithmetic, then wrap the result back into a `Shelf`.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({-2.5, 0.0, 3.5}));

auto out = Shelf(scores.as_series() + 5.0);
std::cout << out;
```

### Output

```text
0  2.5
1  5
2  8.5
ptype: Series
```

Example 2. Calling `as_series()` on a non-`Series` shelf throws.

```cpp
auto names = Shelf(pluss::make_note({"Ada", "Bola"}));
try {
  const auto& bad = names.as_series();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf: payload is not Series
```

## Notes

- Returns a reference into the shelf's own storage — no copy, no allocation.
- Requires `Series`-kind; throws on every other payload kind.
- The result does not automatically become a `Shelf` again — wrap it explicitly with
  `Shelf(...)` when a `Shelf`-only API needs it back.

## See Also

- [`Shelf::as_note`](as_note.md) / [`as_timeline`](as_timeline.md): the equivalent escape hatch for the other three
  payload kinds.
- [`Shelf::astype`](astype.md): stay on the `Shelf` surface for a plain scalar-dtype
  conversion, instead of dropping to `Series` directly.
- [`Shelf::kind`](kind.md): check the payload kind before calling a kind-specific
  accessor like this one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_note.html">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
  <a class="next-link" href="as_timeline.html">
    <span class="label">Next</span>
    <span class="title">as_timeline</span>
  </a>
</div>
