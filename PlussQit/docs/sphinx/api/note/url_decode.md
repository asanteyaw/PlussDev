<!-- pluss-hand-authored -->
# Note::url_decode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::url_decode() const
```

## Decode Percent-Escapes In Every Cell

`url_decode()` is the inverse of [`url_encode`](url_encode.md): it replaces
every `%XX` escape in each cell with the byte it names, and also turns a `+`
into a space. Use it to recover a value taken from a URL path segment, a
query field, or a form body.

### The `+` asymmetry

`url_encode()` never produces `+` — it writes a space as `%20`. But
`url_decode()` still treats `+` as a space, because form-encoded data
(`application/x-www-form-urlencoded`) uses `+` for spaces and it is common to
have to decode both. So the pair is not perfectly symmetric on inputs that
contain a literal `+`: `url_decode()` of `"a+b"` is `"a b"`. If you need a
literal `+` preserved, it must arrive as `%2B`.

Non-escape characters pass through untouched, and a malformed escape (a `%`
not followed by two hex digits) is left as-is rather than raising. Bytes from
`%XX` runs are reassembled directly, so a percent-encoded UTF-8 sequence
decodes back to the original characters. The operation is shape-preserving,
cell-independent, and returns a fresh `Note`.

## Example 1: Percent-Escapes

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a%20b%26c%3Dd", "caf%C3%A9"}).url_decode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a b&c=d", "café"]
```

`%20` → space, `%26` → `&`, `%3D` → `=`, and `%C3%A9` reassembles the two
bytes of `é`.

## Example 2: `+` Decodes To A Space

```cpp
const pluss::Note n = pluss::make_note({"a+b+c"}).url_decode();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a b c"]
```

## Example 3: Round Trip From `url_encode`

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"a b/c?d", "café"}).url_encode().url_decode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a b/c?d", "café"]
```

## Example 4: Clean Text Passes Through

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"already-clean_text.~"}).url_decode())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["already-clean_text.~"]
```

## See Also

- [`Note::url_encode`](url_encode.md): the (near-)inverse.
- [`Note::base64_decode`](base64_decode.md) /
  [`Note::hex_decode`](hex_decode.md): the other transport decoders.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="upper.html">
    <span class="label">Previous</span>
    <span class="title">upper</span>
  </a>
  <a class="next-link" href="url_encode.html">
    <span class="label">Next</span>
    <span class="title">url_encode</span>
  </a>
</div>
