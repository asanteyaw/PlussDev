<!-- pluss-hand-authored -->
# Note::swapcase

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::swapcase() const
```

## Invert The Case Of Every Letter

`swapcase()` returns a new `Note`, same shape as the input, with each ASCII
letter's case flipped: lowercase becomes uppercase and vice versa.
Non-letters are untouched. It is a niche transform — useful for toggling a
display, generating a quick contrast variant, or as a reversible obfuscation
step.

The transform is **ASCII-only**. A non-ASCII letter (`é`, `Α`, `Д`) is passed
through unchanged, so `swapcase()` does not fully invert the case of
non-Latin or accented text. It changes case only, is shape-preserving, and
returns a fresh `Note`.

`swapcase()` is its own inverse for ASCII text: applying it twice returns the
original.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Hello World", "ALL caps", "café MODE"}).swapcase())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["hELLO wORLD", "all CAPS", "CAFé mode"]
```

Every ASCII letter flips; the `é` in `"café MODE"` stays lowercase because it
is outside ASCII.

## Example 2: Applying It Twice Restores The Original

```cpp
const pluss::Note n =
    pluss::make_note({"MiXeD CaSe"}).swapcase().swapcase();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["MiXeD CaSe"]
```

## Example 3: A Scalar

```cpp
pluss::note::print(std::cout, pluss::make_note("aBcD").swapcase()) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"AbCd"
```

## See Also

- [`Note::upper`](upper.md) / [`Note::lower`](lower.md): move all letters one
  direction, with full Unicode support.
- [`Note::title`](title.md) / [`Note::capitalize`](capitalize.md): partial
  re-casing rather than inversion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="tex_escape.html">
    <span class="label">Next</span>
    <span class="title">tex_escape</span>
  </a>
</div>
