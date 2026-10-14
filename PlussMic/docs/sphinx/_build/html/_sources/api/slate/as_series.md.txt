<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 912efca41ceb -->

# Slate::as_series

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::as_series() const -> const pluss::Series&
```

## What It Does

Exposes the underlying `pluss::Series` payload directly, by reference — the numeric escape
hatch for when you need the full `Series` surface (native reductions, elementwise
arithmetic, kernels that live below the table layer) rather than the curated subset
`Slate` re-exposes as its own methods. Requires the slate to be `Series`-kind; calling it
on a `Note`/`Timeline`/`TimeIndex` slate throws.

```{important} Returns a reference, not a copy — and does not rewrap into Slate
`as_series()` hands back `const pluss::Series&`, not a `Slate`. It does no rewrapping and
performs no computation of its own. If you run `Series`-level operations and want the
result usable by `Slate`-only methods again afterward, wrap it explicitly:
`Slate(scores.as_series() + 5.0)`. Forgetting this step is a common way to end up with a
`pluss::Series` where a `Slate` was expected.
```

## Parameters

`None`

## Returns

`const pluss::Series&`
    A reference to the underlying series payload, valid as long as the `Slate` itself
    stays alive.

## Examples

Example 1. Drop to `Series` for arithmetic, then wrap the result back into a `Slate`.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({-2.5, 0.0, 3.5}));

auto out = Slate(scores.as_series() + 5.0);
std::cout << out;
```

### Output

```text
0  2.5
1  5
2  8.5
ptype: Series
```

Example 2. Calling `as_series()` on a non-`Series` slate throws.

```cpp
auto names = Slate(pluss::make_note({"Ada", "Bola"}));
try {
  const auto& bad = names.as_series();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate: payload is not Series
```

## Notes

- Returns a reference into the slate's own storage — no copy, no allocation.
- Requires `Series`-kind; throws on every other payload kind.
- The result does not automatically become a `Slate` again — wrap it explicitly with
  `Slate(...)` when a `Slate`-only API needs it back.

## See Also

- [`Slate::as_note`](as_note.md) / [`as_timeline`](as_timeline.md): the equivalent escape hatch for the other three
  payload kinds.
- [`Slate::astype`](astype.md): stay on the `Slate` surface for a plain scalar-dtype
  conversion, instead of dropping to `Series` directly.
- [`Slate::kind`](kind.md): check the payload kind before calling a kind-specific
  accessor like this one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_note.html">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
</div>
