<!-- pluss-hand-authored -->

# SlateNoteAccessor::indicate

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::indicate(std::string_view delimiter = "|",
                             std::string prefix = "") const -> DataFrame
```

## What It Does

Splits every cell of the bound slate on `delimiter`, collects every distinct
non-empty token seen anywhere in the slate, and produces one `Bool` column per
distinct token ("level") — the classic one-hot / multi-label indicator ("dummy
variable") encoding. A direct forward to
[`Slate::indicate()`](../slate/indicate.md). Requires the bound slate to be
`Note`-kind. Column `<prefix><level>` is `true` for a row if `level` is one of that
row's tokens, `false` otherwise.

Levels are collected into a `std::set<std::string>`, so the output columns always
appear in **sorted (alphabetical) order** — not first-seen order.

```{danger} indicate() never attaches a validity mask — invalid rows are just all-false, not NULL
A row that was invalid on input doesn't show `NULL` in the indicator columns — it
shows `false` in every one of them, exactly the same as a row that's genuinely valid
but happens to match none of the levels. There is no way to tell "this row is
invalid" apart from "this row matched nothing" by looking at `indicate()`'s output
alone. See Example 1.
```

## Parameters

**delimiter** (`std::string_view`, default `"|"`)

The literal text to split each cell on before collecting tokens. Must not be empty.

**prefix** (`std::string`, default `""`)

Text prepended to each level to form its column name — with the default empty
prefix, a column is named exactly after the level text itself.

## Returns

`DataFrame`
    A new frame with one `Bool` feature per distinct non-empty token seen in the
    slate, sorted alphabetically by token text, named `<prefix><level>`.

## Examples

Example 1. The default `"|"` delimiter, and the all-false-for-invalid-rows behavior
from the danger box above.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_note({"red|blue", "blue|green", "red", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << tagged;

auto out1 = tagged.nt().indicate();
std::cout << out1;
```

### Output (before)

```text
0  red|blue
1  blue|green
2  red
3  NULL

ptype: Note
```

### Output

```text
==========================
     | blue   green  red  
--------------------------
0    | true   false  true 
1    | true   true   false
2    | false  false  true 
3    | false  false  false
--------------------------
[4 cs x 3 feat]
```

Three distinct tokens appear across the valid rows — `"blue"`, `"green"`, `"red"` —
in alphabetical column order. Row 3 was invalid on input; notice it shows `false` in
every column, not `NULL`.

Example 2. A custom delimiter and prefix.

```cpp
auto langs = Slate(pluss::make_note({"python, rust", "python, go", "just-one-word"}));
auto out2 = langs.nt().indicate(", ", "know_");
std::cout << out2;
```

### Output

```text
==================================================
     | know_go  know_just-one-word  know_python  know_rust
--------------------------------------------------
0    | false    false               true         true     
1    | true     false               true         false    
2    | false    true                false        false    
--------------------------------------------------
[3 cs x 4 feat]
```

Example 3. An empty delimiter throws immediately.

```cpp
try {
  auto bad = tagged.nt().indicate("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::indicate: delimiter cannot be empty
```

Example 4. A non-`Note` slate throws — the same message
[`Slate::indicate()`](../slate/indicate.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().indicate();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::indicate: expected Note payload
```

## Notes

- `SlateNoteAccessor::indicate()` is a one-line forward to
  [`Slate::indicate()`](../slate/indicate.md) — identical behavior and throws.
- Output columns are sorted alphabetically by token text, not by first-appearance
  order.
- No validity mask is attached to the result at all — invalid input rows show
  `false` everywhere, not `NULL`. See the danger box above.
- An empty `delimiter` throws `"Slate::indicate: delimiter cannot be empty"`.

## See Also

- [`Slate::indicate`](../slate/indicate.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::split`](split.md): the same underlying delimiter-split
  mechanic, producing positional columns instead of one-hot level columns.
- [`SlateNoteAccessor::has`](has.md): test for a single specific substring instead
  of enumerating every distinct delimited token.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
