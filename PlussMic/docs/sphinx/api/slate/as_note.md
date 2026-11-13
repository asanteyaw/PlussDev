<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 00f8fe5cc6f1 -->

# Slate::as_note

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
const pluss::Note& Slate::as_note() const
```

## What It Does

Exposes the underlying `pluss::Note` payload directly, by reference — the text escape
hatch for when you need `Note`'s own method surface directly rather than the subset
`Slate` re-exposes. Requires the slate to be `Note`-kind; every other payload kind
throws.

Same reference/no-rewrap contract as [`as_series()`](as_series.md) — see that page's
important callout for the full explanation.

## Parameters

`None`

## Returns

`const pluss::Note&`
    A reference to the underlying note payload, valid as long as the `Slate` itself
    stays alive.

## Examples

Example 1. Drop to `Note` for chained text cleanup, then wrap the result back into a
`Slate`.

```cpp
using namespace pluss::table;

auto names = Slate(pluss::make_note({" Ada ", "BOLA", " chen "}));

auto out = Slate(names.as_note().strip().lower());
std::cout << out;
```

### Output

```text
0  ada
1  bola
2  chen
ptype: Note
```

Example 2. Calling `as_note()` on a non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0}));
try {
  const auto& bad = numbers.as_note();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate: payload is not Note
```

## Notes

- Returns a reference into the slate's own storage — no copy, no allocation.
- Requires `Note`-kind; throws on every other payload kind.
- The result does not automatically become a `Slate` again — wrap it explicitly with
  `Slate(...)` when a `Slate`-only API needs it back.

## See Also

- [`Slate::as_series`](as_series.md): the same escape hatch for numeric payloads,
  including the full reference/no-rewrap explanation.
- [`Slate::as_timeline`](as_timeline.md): the
  equivalent accessors for the two temporal payload kinds.
- [`Slate::nt`](nt.md): the `Slate`-surface text accessor — prefer this in most
  application code; reach for `as_note()` only when you need a `Note`-only method `nt()`
  doesn't re-expose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arrange.html">
    <span class="label">Previous</span>
    <span class="title">arrange</span>
  </a>
  <a class="next-link" href="as_series.html">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>
