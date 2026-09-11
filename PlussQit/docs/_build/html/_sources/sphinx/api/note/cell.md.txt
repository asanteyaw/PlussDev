<!-- pluss-hand-authored -->
# Note::cell

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TextRef Note::cell(std::int64_t linear) const
```

## Retain One Cell Without Copying Its Bytes

`cell(linear)` selects the same flat row-major position as [`at()`](at.md),
but returns a `pluss::TextRef` that co-owns the Note's packed text arena. The
handle remains valid after the source Note and temporary views are destroyed.
Use it when a cell must cross a scope, be stored in another object, or be
returned without paying for an immediate string copy.

Creating the handle is constant-time: it copies an intrusive arena pointer and
an offset-length pair, not the text. Because the arena is shared, retaining one
small cell can keep the source Note's complete packed text allocation alive.
Use `ref.str()` when independent ownership of only that cell is preferable to
retaining a large arena.

`TextRef` exposes the exact byte slice through `view()`, `data()`, `size()`,
and `size_bytes()`. It iterates over bytes, converts implicitly to
`std::string_view`, compares lexically, and supports stream insertion. `str()`
is the only one of those operations that copies. An empty cell returns a
defined, empty `TextRef`; that is different from a default-constructed,
undefined `TextRef`.

`linear` must be in `[0, numel())`; negative indexing is not supported.
Undefined and zero-element Notes reject.

## Example 1: Keep A Cell Past Its Note

```cpp
pluss::TextRef kept;
{
  const auto temporary = pluss::note::make({"alpha", "beta"});
  kept = temporary.cell(1);
}

std::cout << std::boolalpha
          << "defined=" << kept.defined()
          << " bytes=" << kept.size_bytes()
          << " text=" << kept << '\n';
```

### Output

```text
defined=true bytes=4 text=beta
```

## Example 2: Borrow From The Handle Or Materialize A Copy

```cpp
const auto ref = pluss::note::make("delta").cell(0);
const std::string_view borrowed = ref;
const std::string copied = ref.str();

std::cout << "view=" << borrowed
          << " copy=" << copied
          << " first=" << ref[0] << '\n';
```

### Output

```text
view=delta copy=delta first=d
```

The view is tied to `ref`; the string is independent.

## Example 3: Iterate Stored Bytes

```cpp
const auto ref = pluss::note::make("ABC").cell(0);
for (const unsigned char byte : ref) {
  std::cout << static_cast<int>(byte) << ' ';
}
std::cout << '\n';
```

### Output

```text
65 66 67 
```

Iteration is byte-oriented, not code-point- or grapheme-oriented.

## Example 4: Empty Text Is Still A Defined Cell

```cpp
const auto empty = pluss::note::make("").cell(0);
const pluss::TextRef absent;

std::cout << std::boolalpha
          << "empty: defined=" << empty.defined()
          << " empty=" << empty.empty() << '\n'
          << "default: defined=" << absent.defined()
          << " empty=" << absent.empty() << '\n';
```

### Output

```text
empty: defined=true empty=true
default: defined=false empty=true
```

## Example 5: Raw Bytes Retain Their Full Length

```cpp
const std::array<std::byte, 3> raw{
    std::byte{'A'}, std::byte{0}, std::byte{'B'}};
const auto ref = pluss::note::bytes(raw).cell(0);
std::cout << "bytes=" << ref.size_bytes()
          << " middle=" << static_cast<int>(
                 static_cast<unsigned char>(ref[1])) << '\n';
```

### Output

```text
bytes=3 middle=0
```

## Example 6: Invalid Flat Positions Reject

```cpp
const auto values = pluss::note::make({"a", "b"});
for (const std::int64_t index : {-1, 2}) {
  try { (void)values.cell(index); }
  catch (const pluss::Error&) {
    std::cout << "cell(" << index << "): rejected\n";
  }
}
```

### Output

```text
cell(-1): rejected
cell(2): rejected
```

## See Also

- [`at`](at.md): faster borrowed-view semantics for scope-local access.
- [`cell_at`](cell_at.md): retain a cell addressed by coordinates.
- [`owned`](owned.md): retain the only cell of a one-element Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html"><span class="label">Previous</span><span class="title">casefold</span></a>
  <a class="next-link" href="cell_at.html"><span class="label">Next</span><span class="title">cell_at</span></a>
</div>
