<!-- pluss-hand-authored -->

# SlateNoteAccessor::casefold

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
Slate SlateNoteAccessor::casefold() const
```

## What It Does

Produces a case-insensitive-comparison form of the bound slate's text — a direct
forward to [`Slate::casefold()`](../slate/casefold.md), the identical operation
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

Requires the bound slate to be `Note`-kind — calling it on a numeric or other kind
of slate throws.

## Parameters

`None`

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, holding the
    case-folded text.

## Examples

Example 1. Case-fold mixed-case text via the accessor.

```cpp
using namespace pluss::table;

auto roles = Slate(pluss::make_note({"MEMBER", "Guest", "Admin"}));
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

Example 2. Calling it on a non-`Note` slate throws — the same message
[`Slate::casefold()`](../slate/casefold.md) produces directly, since the accessor
is a pure forward.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().casefold();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::casefold: expected Note payload
```

## Notes

- `SlateNoteAccessor::casefold()` is a one-line forward to
  [`Slate::casefold()`](../slate/casefold.md) — identical behavior, same throw
  message, whether reached via `.nt().casefold()` or `.casefold()` directly.
- A null cell on input stays null on output.
- Reach for `casefold()`, not `lower()`, whenever the goal is a case-insensitive
  *comparison* — even though the two produce identical output on ASCII text in every
  build, `casefold()` states the intent explicitly and picks up full Unicode folding
  automatically if this build's Unicode support changes.

## See Also

- [`Slate::casefold`](../slate/casefold.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::lower`](lower.md): the plain lowercase display transform this
  method is easy to confuse with.
- [`SlateNoteAccessor::titlecase`](titlecase.md): a display-oriented case transform,
  not a comparison-oriented one.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
