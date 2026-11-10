<!-- pluss-hand-authored -->

# SlateNoteAccessor::pad

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::pad(std::int64_t width,
                        TextAlign align = TextAlign::Left,
                        char fill = ' ') const -> Slate
```

## What It Does

Grows every cell of the bound slate up to a target character `width` by adding
`fill` characters, positioning the original text according to `align` — a direct
forward to [`Slate::pad()`](../slate/pad.md). Requires the bound slate to be
`Note`-kind.

### `TextAlign` values

| Value | Numeric value | Description |
|---|---|---|
| `Left` | `0` | — |
| `Right` | `1` | — |
| `Center` | `2` | — |

- **`TextAlign::Left`** (the default): fill characters are added on the **right**.
- **`TextAlign::Right`**: fill characters are added on the **left** — the classic
  zero-padding-a-code use case (`"7"` → `"0007"`).
- **`TextAlign::Center`**: fill is split between both sides, extra character on
  the right for an odd amount.

`pad()` only ever grows a cell — a cell already at or beyond `width` comes back
unchanged, never truncated.

## Parameters

**width** (`std::int64_t`)

The target character width, measured in Unicode codepoints. Must be `>= 0`.

**align** (`TextAlign`, default `TextAlign::Left`)

Where the fill characters go.

**fill** (`char`, default `' '`)

The single character used to pad. A `char`, not a `std::string_view` — no
multi-byte UTF-8 fill sequences.

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, holding the
    padded (or unchanged) text.

## Examples

Example 1. Zero-padding numeric-looking codes to a fixed width via the accessor.

```cpp
using namespace pluss::table;

auto ids = Slate(pluss::make_note({"7", "42", "103"}));
auto out = ids.nt().pad(4, TextAlign::Right, '0');
std::cout << out;
```

### Output

```text
0  0007
1  0042
2  0103

ptype: Note
```

Example 2. `TextAlign::Center` splits fill between both sides, extra character
on the right.

```cpp
auto codes = Slate(pluss::make_note({"A", "BB", "CCCC"}));
auto out2 = codes.nt().pad(6, TextAlign::Center, '.');
std::cout << out2;
```

### Output

```text
0  ..A...
1  ..BB..
2  .CCCC.

ptype: Note
```

Example 3. A negative `width` throws — the same message
[`Slate::pad()`](../slate/pad.md) produces directly.

```cpp
try {
  auto bad = codes.nt().pad(-1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::pad: width must be >= 0
```

## Notes

- `SlateNoteAccessor::pad()` is a one-line forward to
  [`Slate::pad()`](../slate/pad.md) — identical behavior and throws.
- `pad()` only grows; a cell already at or beyond `width` is returned unchanged,
  never truncated.
- `width` is measured in Unicode codepoints, the same unit
  [`length()`](length.md) counts.
- `Center`'s extra fill character (when the amount needed is odd) always goes on
  the right.
- A null cell on input stays null on output.

## See Also

- [`Slate::pad`](../slate/pad.md): the direct, non-accessor form this method
  forwards to.
- [`SlateNoteAccessor::chars`](chars.md): the complementary shrink/slice
  operation.
- [`SlateNoteAccessor::strip`](strip.md): removes characters instead of adding
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
