<!-- pluss-hand-authored -->
# NoteTexAccessor::balance_braces

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::balance_braces() const
```

## Repair Structural Brace Counts Cell By Cell

`balance_braces()` makes the unescaped curly braces in every Text cell
structurally balanced. It is useful at a controlled cleanup boundary: for
example, after assembling generated fragments or before passing extracted
source to a later TeX-aware stage that requires balanced groups.

The scan maintains a nesting depth for each cell. An unescaped `{` is copied
and increases that depth. An unescaped `}` is copied only when it closes an
open group; an unmatched closing brace is discarded. If the scan ends with
open groups, one `}` per remaining level is appended at the end. Backslash
escapes are copied as two-byte units, so `\{` and `\}` are literal glyphs and
do not participate in the count.

That rule is deterministic and idempotent, but deliberately conservative. It
cannot infer where a missing closing brace was intended to occur; it places
all required closers at the cell boundary. It does not parse comments,
validate command arity, match environments, or decide whether an unmatched
closing brace should have had an opening brace inserted earlier. Use it to
repair brace *structure*, not to certify that a document expresses the
author's intended TeX.

The result owns fresh Text storage and preserves the receiver's rank, shape,
cell order, and encoding. Empty cells remain empty. Undefined propagates as
undefined. Bytes, Bool, Int64, and Float64 Notes reject rather than being
decoded or formatted implicitly. Runtime is linear in the total number of
input bytes, with one pass and output proportional to input plus appended
closers.

## Example 1: Missing Opens, Missing Closes, Escapes, And Valid Input

```cpp
const auto source = pluss::make_note(
    {"{a{b}", "a}}b", R"(\{literal\})", "already {balanced}"});
std::cout << source.tex().balance_braces() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["{a{b}}", "ab", "\\{literal\\}", "already {balanced}"]
```

The first cell receives one trailing closer. The second loses both unmatched
closers, while the escaped braces in the third cell remain literal text.

## Example 2: Repaired Text Is A Fixed Point

```cpp
const auto repaired = source.tex().balance_braces();
const bool idempotent =
    (repaired.tex().balance_braces() == repaired).all().bool_item();
std::cout << std::boolalpha << "idempotent=" << idempotent
          << " undefined="
          << pluss::Note::undefined().tex().balance_braces().defined()
          << '\n';
```

### Output

```text
idempotent=true undefined=false
```

## Example 3: Native Integer Cells Are Not Coerced To Text

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().balance_braces();
} catch (const pluss::Error&) {
  std::cout << "braces Int64: rejected\n";
}
```

### Output

```text
braces Int64: rejected
```

## See Also

- [`balance_math_delims`](balance_math_delims.md): repair math delimiter counts.
- `strip_cmds`: remove command syntax instead of attempting repair.
- [`to_text`](to_text.md): project TeX source into normalized readable prose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="args.html"><span class="label">Previous</span><span class="title">args</span></a>
  <a class="next-link" href="balance_math_delims.html"><span class="label">Next</span><span class="title">balance_math_delims</span></a>
</div>
