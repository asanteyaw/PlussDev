<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e1bbf4ebe786 -->

# DataFrameNoteAccessor::pad

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::pad(std::int64_t width,
                            TextAlign align = TextAlign::Left,
                            char fill = ' ') const -> DataFrame
```

## What It Does

Grows every selected text feature's cells up to a target character `width`
by adding `fill` characters, positioning the original text according to
`align`:

```cpp
enum class TextAlign { Left = 0, Right = 1, Center = 2 };
```

- **`TextAlign::Left`** (the default): the original text is left-aligned —
  fill characters are added on the **right**.
- **`TextAlign::Right`**: the original text is right-aligned — fill
  characters are added on the **left**. The classic use is zero-padding
  numeric-looking codes (`"7"` → `"0007"`).
- **`TextAlign::Center`**: fill is split between both sides. When the
  amount needed is odd, the **extra** fill character goes on the right
  (`left = needed / 2`, integer division; `right = needed - left`).

```{note} pad() only ever grows a cell, never shrinks it
If a cell is already at least `width` characters long, `pad()` leaves it
completely unchanged — there's no truncation mode. `width` is measured in
Unicode codepoints, the same unit [`length()`](length.md) counts (see that
page's caveat about codepoints vs. raw UTF-8 storage bytes).
```

```{tip} Padding is invisible in a rendered table when fill is a space
The default fill character is a space, and the plain-text table renderer
used throughout these docs *also* right-pads cells to align columns — so a
space-padded cell and the renderer's own column-alignment padding look
identical side by side. The examples below use a visible fill character
(`.`) for the alignment demonstrations so the padding itself is visible in
the rendered table; the practical zero-padding example (Example 2) uses
`'0'`, which is visible for the same reason.
```

## Parameters

**width** (`std::int64_t`)
    The target character width. Must be `>= 0` — `pad(-1)` throws
    `"Thread::pad: width must be >= 0"`.

**align** (`TextAlign`, default `TextAlign::Left`)
    Where the fill characters go: `Left` pads on the right, `Right` pads
    on the left, `Center` splits the fill (extra character on the right
    for an odd amount).

**fill** (`char`, default `' '`)
    The single character used to pad. Note this is a `char`, not a
    `std::string_view` — multi-byte UTF-8 fill sequences aren't supported.

## Returns

`DataFrame`
    A new frame with every selected feature's text padded to `width`
    characters (or left unchanged, for cells already at or beyond that
    width).

## Examples

Example 1. All three alignments, side by side, using a visible `.` fill
character so the padding itself is visible in the rendered table.

```cpp
using namespace pluss::table;

auto codes = DataFrame({
    {"code", Thread(pluss::make_note({"A", "BB", "CCCC"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << codes;

auto left = codes.nt().pad(6, TextAlign::Left, '.');
auto right = codes.nt().pad(6, TextAlign::Right, '.');
auto center = codes.nt().pad(6, TextAlign::Center, '.');
```

### Output (before)

```text
===========
     | code
-----------
0    | A   
1    | BB  
2    | CCCC
3    | NULL
-----------
[4 cs x 1 feat]
```

### Output (`TextAlign::Left`, the default alignment)

```text
=============
     | code  
-------------
0    | A.....
1    | BB....
2    | CCCC..
3    | NULL  
-------------
[4 cs x 1 feat]
```

### Output (`TextAlign::Right`)

```text
=============
     | code  
-------------
0    | .....A
1    | ....BB
2    | ..CCCC
3    | NULL  
-------------
[4 cs x 1 feat]
```

### Output (`TextAlign::Center`)

```text
=============
     | code  
-------------
0    | ..A...
1    | ..BB..
2    | .CCCC.
3    | NULL  
-------------
[4 cs x 1 feat]
```

`"A"` (1 character) needs 5 fill characters to reach width 6: all 5 on the
right under `Left`, all 5 on the left under `Right`, and split 2-left/3-right
under `Center` (the odd character goes right). `"BB"` needs 4 (split
evenly, 2/2, since 4 is even). `"CCCC"` needs only 2 (split 1/1). Row 3
was invalid on input and stays `NULL` under every alignment — `pad()`
never manufactures text for an invalid cell.

Example 2. The practical, real-world use of `Right` alignment: zero-padding
numeric-looking codes to a fixed width.

```cpp
auto ids = DataFrame({
    {"id", Thread(pluss::make_note({"7", "42", "103"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << ids;

auto zero_padded = ids.nt().pad(4, TextAlign::Right, '0');
std::cout << zero_padded;
```

### Output (before)

```text
===========
     | id  
-----------
0    | 7   
1    | 42  
2    | 103 
3    | NULL
-----------
[4 cs x 1 feat]
```

### Output

```text
===========
     | id  
-----------
0    | 0007
1    | 0042
2    | 0103
3    | NULL
-----------
[4 cs x 1 feat]
```

`"7"` becomes `"0007"`, `"42"` becomes `"0042"`, and so on — the familiar
zero-padded ID/order-number look, achieved without any string-formatting
helper beyond `pad()` itself.

Example 3. A cell already at or past the target width is left completely
unchanged — `pad()` never truncates.

```cpp
auto unchanged = codes.nt().pad(3, TextAlign::Left, '.');
std::cout << unchanged;
```

### Output

```text
===========
     | code
-----------
0    | A...
1    | BB..
2    | CCCC
3    | NULL
-----------
[4 cs x 1 feat]
```

`"CCCC"` is already 4 characters — at or beyond the requested width of 3 —
so it comes back completely untouched, not truncated to `"CCC"`. `"A"` and
`"BB"` are still shorter than 3, so they're padded as usual.

Example 4. A negative `width` throws immediately.

```cpp
try {
  auto bad = codes.nt().pad(-1);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::pad: width must be >= 0
```

## Notes

- `pad()` only grows; a cell already at or beyond `width` is returned
  unchanged, never truncated.
- `width` is measured in Unicode codepoints, the same unit
  [`length()`](length.md) counts.
- `fill` is a single `char`, not a string — no multi-byte UTF-8 fill
  characters.
- `Center`'s extra fill character (when the amount needed is odd) always
  goes on the right.
- A space fill (the default) is visually indistinguishable from the
  rendered table's own column-alignment padding — see the tip above.
- A null cell on input stays null on output.
- `width < 0` throws `"Thread::pad: width must be >= 0"`.

## See Also

- [`DataFrameNoteAccessor::chars`](chars.md): the complementary
  shrink/slice operation.
- [`DataFrameNoteAccessor::length`](length.md): the character-count unit
  `width` is measured in.
- [`DataFrameNoteAccessor::strip`](strip.md): removes characters instead
  of adding them.

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
