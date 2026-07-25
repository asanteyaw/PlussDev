<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::casefold

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::casefold() const -> DataFrame
```

## What It Does

Produces a case-insensitive-comparison form of every selected text feature's
cells — conceptually similar to [`lower()`](lower.md), but intended
specifically as a normalization step *before equality comparison*, not as a
display transform. The distinction matters in full Unicode case handling:
true Unicode case folding can differ from a simple lowercase mapping for
certain characters (the canonical example is German `ß`, which case-folds to
`ss` rather than lowercasing to itself) — `casefold()` is the method that's
supposed to get that right, where `lower()` is not.

```{important} This build's actual behavior
`casefold()`'s Unicode-aware folding (handling cases like `ß` -> `ss`) is
only active when this build links the optional Unicode backend (GNU
libunistring). Without it, `casefold()` falls back to being **functionally
identical to [`lower()`](lower.md)** — same output, same ASCII-only
guarantees. On plain ASCII input, `casefold()` and `lower()` always produce
the same result regardless of build configuration, since ASCII case mapping
has no folding-vs-lowering distinction to begin with. Confirm your build's
Unicode support before relying on the two methods actually diverging.
```

Like every method on this accessor, `casefold()` applies to **every feature
the accessor was built with** in one call — see [`strip()`](strip.md) for
the shared mechanic and how null cells pass through unaffected.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with every selected feature's text case-folded.

## Examples

Example 1. Case-fold a mixed-case text feature.

```cpp
using namespace pluss::table;

auto roles_a = DataFrame({
    {"role", Thread(pluss::make_note({"MEMBER", "Guest", "Admin"}))},
});
std::cout << roles_a;

auto folded = roles_a.note().casefold();
std::cout << folded;
```

### Output (before)

```text
=============
     | role  
-------------
0    | MEMBER
1    | Guest 
2    | Admin 
-------------
[3 cs x 1 feat]
```

### Output (after)

```text
=============
     | role  
-------------
0    | member
1    | guest 
2    | admin 
-------------
[3 cs x 1 feat]
```

On this plain-ASCII input, the result is identical to what
`roles_a.note().lower()` would produce.

Example 2. The actual motivating use case: normalize two differently-cased
frames before comparing them, instead of comparing the raw text directly.

```cpp
auto roles_b = DataFrame({
    {"role", Thread(pluss::make_note({"member", "GUEST", "admin"}))},
});

auto raw_eq = roles_a.eq(roles_b);
std::cout << raw_eq;

auto folded_eq = roles_a.note().casefold().eq(roles_b.note().casefold());
std::cout << folded_eq;
```

### Output (raw eq)

```text
============
     | role 
------------
0    | false
1    | false
2    | false
------------
[3 cs x 1 feat]
```

### Output (casefolded eq)

```text
===========
     | role
-----------
0    | true
1    | true
2    | true
-----------
[3 cs x 1 feat]
```

`roles_a` and `roles_b` hold the same three role names, just typed with
different casing conventions (`"MEMBER"` vs. `"member"`, and so on) — a
direct `eq()` on the raw text reports every row as different. Case-folding
both sides first makes the comparison casing-insensitive, correctly reporting
all three rows as equal.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.
- Reach for `casefold()`, not `lower()`, whenever the goal is a
  case-insensitive *comparison* — even though the two currently produce
  identical output on ASCII text in every build, `casefold()` states the
  intent explicitly and is the one that picks up full Unicode folding
  automatically if this build's Unicode backend changes.

## See Also

- [`DataFrameNoteAccessor::lower`](lower.md): the plain lowercase display
  transform this method is easy to confuse with.
- [`DataFrame::eq`](../dataframe/eq.md): the comparison this method is
  usually a normalization step for.
- [`DataFrameNoteAccessor::titlecase`](titlecase.md): a display-oriented case
  transform, not a comparison-oriented one.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
