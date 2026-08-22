<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 322aeb9672fd -->

# AxelNoteAccessor::normalize

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const
```

## What It Does

Applies Unicode normalization to every bound field's text, in every
registry frame, returning a new axel with those fields replaced.
Applies
[`RegistryNoteAccessor::normalize`](../registry-note/normalize.md) to
every frame via [`Axel::map`](../axel/map.md).

```cpp
enum class UnicodeForm : std::uint8_t {
  NFC = 0,
  NFD = 1,
  NFKC = 2,
  NFKD = 3,
};
```

Normalization matters when the same visual character can be represented
by different underlying byte sequences (for example, a precomposed
accented letter versus the base letter plus a combining accent mark) —
two values that look identical on screen can compare as unequal or hash
differently until they're normalized to the same form.

## Parameters

**form** (`pluss::table::UnicodeForm`, defaults to `pluss::table::UnicodeForm::NFC`)
    Which of the four standard Unicode normalization forms to apply:
    `NFC` (canonical composition, the default), `NFD` (canonical
    decomposition), `NFKC` (compatibility composition), or `NFKD`
    (compatibility decomposition).

## Returns

`Axel`
    A new axel with every bound field's text normalized, in every
    frame; every unbound field unchanged.

## Examples

Example 1. Default call normalizes to `NFC`.

```cpp
auto r0 = pluss::scribble::Registry({{"txt", pluss::scribble::Slot({pluss::make_note("cafe")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"txt"}).nt().normalize();
std::cout << std::get<pluss::Note>(out.registry(0).fields("txt").at(0)).at(0);
```

### Output

```text
cafe
```

Example 2. Passing a form explicitly.

```cpp
auto r0 = pluss::scribble::Registry({{"txt", pluss::scribble::Slot({pluss::make_note("cafe")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"txt"}).nt().normalize(pluss::table::UnicodeForm::NFD);
std::cout << std::get<pluss::Note>(out.registry(0).fields("txt").at(0)).at(0);
```

### Output

```text
cafe
```

Plain ASCII text like `"cafe"` has no composed/decomposed distinction
to make, so every form round-trips it unchanged — the visible
difference shows up on text containing combining marks or compatibility
characters.

## Notes

- Defaults to `NFC`, the most common form for storage and comparison.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::casefold`](casefold.md): another transform aimed
  at making text comparable, applied to case rather than composition.
- [`AxelNoteAccessor::encode`](encode.md): the other byte/text-level
  transform on this accessor.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
