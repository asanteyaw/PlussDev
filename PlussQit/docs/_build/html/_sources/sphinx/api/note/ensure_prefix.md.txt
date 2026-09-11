<!-- pluss-hand-authored -->
# Note::ensure_prefix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::ensure_prefix(std::string_view prefix) const
```

## Guarantee Each Cell Begins With A Prefix

`ensure_prefix(prefix)` returns a new `Note`, same shape as the input, in
which every cell is guaranteed to start with the literal string `prefix`. A
cell that already begins with the full `prefix` is left unchanged; any other
cell has `prefix` prepended. It is the idempotent way to normalize leading
markup — a scheme (`https://`), a mount point (`/api/`), a namespace, a
bullet.

The check is a **literal, exact prefix match**: only when the cell's first
`prefix.size()` bytes equal `prefix` is it considered already-prefixed. A
cell that starts with just *part* of `prefix` still gets the whole `prefix`
prepended (see Example 2), which can produce a repeated fragment — pass the
complete token you expect. `ensure_prefix` never removes anything; it is not
the inverse of [`remove_prefix`](remove_prefix.md).

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Add A Leading Slash Where Missing

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"path", "/path", "p/ath"}).ensure_prefix("/"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["/path", "/path", "/p/ath"]
```

`"/path"` already starts with `/` and is untouched; the other two get one
prepended.

## Example 2: The Whole Prefix Must Already Be Present

```cpp
const pluss::Note n =
    pluss::make_note({"htt", "http://x"}).ensure_prefix("http://");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["http://htt", "http://x"]
```

`"htt"` is only a partial prefix, so the full `"http://"` is prepended,
giving `"http://htt"`.

## Example 3: A Scalar

```cpp
pluss::note::print(std::cout, pluss::make_note("users").ensure_prefix("/api/"))
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"/api/users"
```

## See Also

- [`Note::ensure_suffix`](ensure_suffix.md): the trailing-end counterpart.
- [`Note::remove_prefix`](remove_prefix.md): strip a prefix if present.
- [`Note::starts_with`](starts_with.md): test for a prefix without adding
  one.
- [`Note::surround`](surround.md): add both a prefix and a suffix
  unconditionally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends_with.html">
    <span class="label">Previous</span>
    <span class="title">ends_with</span>
  </a>
  <a class="next-link" href="ensure_suffix.html">
    <span class="label">Next</span>
    <span class="title">ensure_suffix</span>
  </a>
</div>
