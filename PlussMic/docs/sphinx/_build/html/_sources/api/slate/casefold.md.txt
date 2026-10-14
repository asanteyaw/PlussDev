<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1f2d32f1f5c2 -->

# Slate::casefold

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::casefold() const -> Slate
```

## What It Does

Produces a case-insensitive-comparison form of a `Note` (string) slate's text —
conceptually similar to [`lower()`](lower.md), but intended specifically as a
normalization step **before equality comparison**, not as a display transform. True
Unicode case folding can differ from a simple lowercase mapping for certain
characters (the canonical example is German `ß`, which case-folds to `ss` rather
than lowercasing to itself).

```{important} This build's actual behavior
`casefold()`'s Unicode-aware folding is only active when this build links the
optional Unicode backend (GNU libunistring). Without it, `casefold()` falls back to
being functionally identical to [`lower()`](lower.md). On plain ASCII input, the two
always produce the same result regardless of build configuration.
```

Requires the slate to be `Note`-kind — calling it on a numeric or other kind of
slate throws.

## Parameters

`None`

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, holding the case-folded text.

## Examples

Example 1. Case-fold mixed-case text.

```cpp
using namespace pluss::table;

auto roles = Slate(pluss::make_note({"MEMBER", "Guest", "Admin"}));
auto out = roles.casefold();
std::cout << out;
```

### Output

```text
0  member
1  guest
2  admin

ptype: Note
```

On this plain-ASCII input, identical to what [`roles.lower()`](lower.md) would
produce.

Example 2. Calling it on a non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.casefold();
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

- A null cell on input stays null on output.
- Reach for `casefold()`, not `lower()`, whenever the goal is a case-insensitive
  *comparison* — even though the two produce identical output on ASCII text in every
  build, `casefold()` states the intent explicitly and picks up full Unicode folding
  automatically if this build's Unicode support changes.
- See [`DataFrameNoteAccessor::casefold`](../dataframe-note/casefold.md) for the
  multi-feature counterpart and a worked comparison example.

## See Also

- [`Slate::lower`](lower.md): the plain lowercase display transform this method is
  easy to confuse with.
- [`Slate::eq`](eq.md): the comparison this method is usually a normalization step
  for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html">
    <span class="label">Previous</span>
    <span class="title">bucketize</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
