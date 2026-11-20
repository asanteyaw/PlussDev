<!-- pluss-hand-authored -->

# ShelfNoteAccessor::casefold

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signature

```cpp
Shelf ShelfNoteAccessor::casefold() const
```

## What It Does

Produces a case-insensitive-comparison form of the bound shelf's text — a direct
forward to [`Shelf::casefold()`](../shelf/casefold.md), the identical operation
reached without going through the accessor. Conceptually similar to
[`lower()`](lower.md), but intended specifically as a normalization step **before
equality comparison**, not as a display transform. True Unicode case folding can
differ from a simple lowercase mapping for certain characters (the canonical example
is German `ß`, which case-folds to `ss` rather than lowercasing to itself).

```{important} This build's actual behavior
`casefold()`'s Unicode-aware folding is only active when this build links the
optional Unicode backend (GNU libunistring). Without it, `casefold()` falls back to
being functionally identical to [`lower()`](lower.md). On plain ASCII input, the two
always produce the same result regardless of build configuration.
```

Requires the bound shelf to be `Note`-kind — calling it on a numeric or other kind
of shelf throws.

## Parameters

`None`

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, holding the
    case-folded text.

## Examples

Example 1. Case-fold mixed-case text via the accessor.

```cpp
using namespace pluss::table;

auto roles = Shelf(pluss::make_note({"MEMBER", "Guest", "Admin"}));
auto out = roles.nt().casefold();
std::cout << out;
```

### Output

```text
0  member
1  guest
2  admin

ptype: Note
```

On this plain-ASCII input, identical to what `roles.nt().lower()` would produce.

Example 2. Calling it on a non-`Note` shelf throws — the same message
[`Shelf::casefold()`](../shelf/casefold.md) produces directly, since the accessor
is a pure forward.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().casefold();
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::casefold: expected Note payload
```

## Notes

- `ShelfNoteAccessor::casefold()` is a one-line forward to
  [`Shelf::casefold()`](../shelf/casefold.md) — identical behavior, same throw
  message, whether reached via `.nt().casefold()` or `.casefold()` directly.
- A null cell on input stays null on output.
- Reach for `casefold()`, not `lower()`, whenever the goal is a case-insensitive
  *comparison* — even though the two produce identical output on ASCII text in every
  build, `casefold()` states the intent explicitly and picks up full Unicode folding
  automatically if this build's Unicode support changes.

## See Also

- [`Shelf::casefold`](../shelf/casefold.md): the direct, non-accessor form this
  method forwards to.
- [`ShelfNoteAccessor::lower`](lower.md): the plain lowercase display transform this
  method is easy to confuse with.
- [`ShelfNoteAccessor::titlecase`](titlecase.md): a display-oriented case transform,
  not a comparison-oriented one.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
