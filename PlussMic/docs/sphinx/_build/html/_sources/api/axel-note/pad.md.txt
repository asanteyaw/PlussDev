<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2ed748794820 -->

# AxelNoteAccessor::pad

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::pad(std::int64_t width,
                           pluss::table::TextAlign align = pluss::table::TextAlign::Left,
                           char fill = ' ') const
```

## What It Does

Pads every bound field's text out to at least `width` characters, in
every registry frame, using `fill` and the alignment given by `align`.
Returns a new axel with those fields replaced. Applies
[`RegistryNoteAccessor::pad`](../registry-note/pad.md) to every frame
via [`Axel::map`](../axel/map.md).

```cpp
enum class TextAlign : std::uint8_t {
  Left = 0,
  Right = 1,
  Center = 2,
};
```

`align` describes where the *original text* ends up, not where the
padding goes: `Left` keeps the text flush left and adds padding on the
right; `Right` keeps the text flush right and adds padding on the left;
`Center` splits the padding between both sides.

## Parameters

**width** (`std::int64_t`)
    The minimum total length of the result. Values already at or
    beyond this length are returned unchanged.

**align** (`pluss::table::TextAlign`, defaults to `pluss::table::TextAlign::Left`)
    Where the original text sits relative to the added padding.

**fill** (`char`, defaults to `' '`)
    The character used to pad.

## Returns

`Axel`
    A new axel with every bound field's text padded to `width`, in
    every frame; every unbound field unchanged.

## Examples

Example 1. Default call: left-align, pad with spaces on the right.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("7")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"code"}).nt().pad(4);
std::cout << "'" << std::get<pluss::Note>(out.registry(0).fields("code").at(0)).at(0) << "'";
```

### Output

```text
'7   '
```

Example 2. Right-align with a `'0'` fill — a common way to zero-pad
numeric codes stored as text.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("7")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"code"}).nt().pad(4, pluss::table::TextAlign::Right, '0');
std::cout << std::get<pluss::Note>(out.registry(0).fields("code").at(0)).at(0);
```

### Output

```text
0007
```

Example 3. Center alignment splits the padding across both sides.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("7")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"code"}).nt().pad(5, pluss::table::TextAlign::Center);
std::cout << "'" << std::get<pluss::Note>(out.registry(0).fields("code").at(0)).at(0) << "'";
```

### Output

```text
'  7  '
```

## Notes

- Values already at or beyond `width` characters pass through unchanged —
  this is a *minimum*-width pad, not a fixed-width truncate.
- `align` describes where the original text lands, not where the
  padding goes — see the description above.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::strip`](strip.md) / [`lstrip()`](lstrip.md) /
  [`rstrip()`](rstrip.md): the inverse operation family, removing
  characters from the edges instead of adding them.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
