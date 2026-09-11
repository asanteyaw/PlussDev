<!-- pluss-hand-authored -->
# Note::collapse_punct

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::collapse_punct() const
```

## Reduce Every Run Of Punctuation To Its First Character

`collapse_punct()` returns a new `Note`, same shape as the input, with every
run of consecutive ASCII punctuation characters in each cell reduced to just
its **first** character. Use it to tame expressive punctuation —
`"wait..."`, `"really?!?!"`, `"a--b"` — before tokenization or display.

The run is defined by the ASCII punctuation *class*, not by a single
repeated mark, so a mixed run collapses too: `"?!?!"` becomes `"?"` (the
first punctuation character, then the rest of the adjacent punctuation is
dropped). Letters, digits, whitespace, and non-ASCII characters are kept
exactly and break a run.

`collapse_punct` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"wait...", "really?!?!", "a--b__c"}).collapse_punct())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["wait.", "really?", "a-b_c"]
```

`"..."` → `"."`; the mixed run `"?!?!"` → `"?"`; `"--"` → `"-"` and `"__"` →
`"_"` because the letters between them break the runs.

## See Also

- [`Note::remove_punctuation`](remove_punctuation.md): delete punctuation
  entirely instead of thinning it.
- [`Note::collapse_repeats`](collapse_repeats.md): collapse runs of any
  identical character, not just punctuation.
- [`Note::collapse_whitespace`](collapse_whitespace.md): the whitespace
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_newlines.html">
    <span class="label">Previous</span>
    <span class="title">collapse_newlines</span>
  </a>
  <a class="next-link" href="collapse_repeats.html">
    <span class="label">Next</span>
    <span class="title">collapse_repeats</span>
  </a>
</div>
