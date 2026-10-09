<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0669836f4bbb -->

# Slate::pad

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::pad(std::int64_t width,
            TextAlign align = TextAlign::Left,
            char fill = ' ') const -> Slate
```

## What It Does

Grows every cell of a `Note` (string) slate up to a target character `width` by
adding `fill` characters, positioning the original text according to `align`.
Requires the slate to be `Note`-kind.

```cpp
enum class TextAlign { Left = 0, Right = 1, Center = 2 };
```

- **`TextAlign::Left`** (the default): the original text is left-aligned — fill
  characters are added on the **right**.
- **`TextAlign::Right`**: the original text is right-aligned — fill characters are
  added on the **left**. The classic use is zero-padding numeric-looking codes
  (`"7"` → `"0007"`).
- **`TextAlign::Center`**: fill is split between both sides. When the amount needed is
  odd, the **extra** fill character goes on the right (`left = needed / 2`, integer
  division; `right = needed - left`).

```{note} pad() only ever grows a cell, never shrinks it
If a cell is already at least `width` characters long, `pad()` leaves it completely
unchanged — there's no truncation mode. `width` is measured in Unicode codepoints,
the same unit [`length()`](length.md) counts.
```

## Parameters

**width** (`std::int64_t`)
    The target character width. Must be `>= 0` — `pad(-1)` throws `"Slate::pad:
    width must be >= 0"`.

**align** (`TextAlign`, default `TextAlign::Left`)
    Where the fill characters go: `Left` pads on the right, `Right` pads on the left,
    `Center` splits the fill (extra character on the right for an odd amount).

**fill** (`char`, default `' '`)
    The single character used to pad. Note this is a `char`, not a
    `std::string_view` — multi-byte UTF-8 fill sequences aren't supported.

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, holding the padded text (or
    left unchanged, for cells already at or beyond `width`).

## Examples

Example 1. All three alignments, side by side, using a visible `.` fill character so
the padding itself is visible in the rendered output.

```cpp
using namespace pluss::table;

auto codes = Slate(pluss::make_note({"A", "BB", "CCCC"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << codes;

auto left = codes.pad(6, TextAlign::Left, '.');
auto right = codes.pad(6, TextAlign::Right, '.');
auto center = codes.pad(6, TextAlign::Center, '.');
std::cout << left;
std::cout << right;
std::cout << center;
```

### Output (before)

```text
0  A
1  BB
2  CCCC
3  NULL

ptype: Note
```

### Output (`TextAlign::Left`, the default alignment)

```text
0  A.....
1  BB....
2  CCCC..
3  NULL

ptype: Note
```

### Output (`TextAlign::Right`)

```text
0  .....A
1  ....BB
2  ..CCCC
3  NULL

ptype: Note
```

### Output (`TextAlign::Center`)

```text
0  ..A...
1  ..BB..
2  .CCCC.
3  NULL

ptype: Note
```

`"A"` (1 character) needs 5 fill characters to reach width 6: all 5 on the right
under `Left`, all 5 on the left under `Right`, and split 2-left/3-right under `Center`
(the odd character goes right). `"BB"` needs 4 (split evenly, 2/2, since 4 is even).
`"CCCC"` needs only 2 (split 1/1). Row 3 was invalid on input and stays `NULL` under
every alignment — `pad()` never manufactures text for an invalid cell.

Example 2. The practical, real-world use of `Right` alignment: zero-padding
numeric-looking codes to a fixed width.

```cpp
auto ids = Slate(pluss::make_note({"7", "42", "103"}));
auto zero_padded = ids.pad(4, TextAlign::Right, '0');
std::cout << zero_padded;
```

### Output

```text
0  0007
1  0042
2  0103

ptype: Note
```

`"7"` becomes `"0007"`, `"42"` becomes `"0042"`, and so on — the familiar zero-padded
ID/order-number look, achieved without any string-formatting helper beyond `pad()`
itself.

Example 3. A cell already at or past the target width is left completely unchanged —
`pad()` never truncates.

```cpp
auto unchanged = codes.pad(3, TextAlign::Left, '.');
std::cout << unchanged;
```

### Output

```text
0  A...
1  BB..
2  CCCC
3  NULL

ptype: Note
```

`"CCCC"` is already 4 characters — at or beyond the requested width of 3 — so it
comes back completely untouched, not truncated to `"CCC"`. `"A"` and `"BB"` are still
shorter than 3, so they're padded as usual.

Example 4. A negative `width` throws immediately.

```cpp
try {
  auto bad = codes.pad(-1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::pad: width must be >= 0
```

Example 5. A non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.pad(4);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::pad: expected Note payload
```

## Notes

- Requires the calling slate to be `Note`-kind.
- `pad()` only grows; a cell already at or beyond `width` is returned unchanged,
  never truncated.
- `width` is measured in Unicode codepoints, the same unit [`length()`](length.md)
  counts.
- `fill` is a single `char`, not a string — no multi-byte UTF-8 fill characters.
- `Center`'s extra fill character (when the amount needed is odd) always goes on the
  right.
- A null cell on input stays null on output.
- `width < 0` throws `"Slate::pad: width must be >= 0"`.

## See Also

- [`Slate::chars`](chars.md): the complementary shrink/slice operation.
- [`Slate::length`](length.md): the character-count unit `width` is measured in.
- [`Slate::strip`](strip.md): removes characters instead of adding them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
