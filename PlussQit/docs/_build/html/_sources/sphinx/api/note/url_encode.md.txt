<!-- pluss-hand-authored -->
# Note::url_encode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::url_encode() const
```

## Percent-Encode Every Cell For Use In A URL

`url_encode()` returns a new `Note`, same shape as the input, with each cell
percent-encoded so it can be dropped into a URL path segment or query value
without changing the URL's structure. Use it when building request URLs,
`?key=value` pairs, or redirect targets from text that may contain spaces,
`&`, `=`, `/`, `?`, `#`, or non-ASCII characters.

### The rule

The **unreserved** set of RFC 3986 is left as-is:

```
A-Z   a-z   0-9   -   _   .   ~
```

Every other byte is written as `%` followed by its two uppercase hex digits.
This means:

- A space becomes `%20` (not `+` — this is component encoding, not
  `application/x-www-form-urlencoded`).
- The sub-delimiters and gen-delimiters `& = / ? # : @ [ ]` and so on are all
  encoded, so an encoded value cannot break out of its path segment or query
  field.
- Non-ASCII text is encoded **per UTF-8 byte**: `é` (bytes `C3 A9`) becomes
  `%C3%A9`, and `€` (bytes `E2 82 AC`) becomes `%E2%82%AC`.

`url_decode()` is the exact inverse.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Encoded cells are longer wherever a byte had to be escaped; an empty
cell stays empty.

## Example 1: Reserved Characters And Spaces

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a b&c=d", "path/to/x?y#z"}).url_encode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a%20b%26c%3Dd", "path%2Fto%2Fx%3Fy%23z"]
```

Space → `%20`, `&` → `%26`, `=` → `%3D`, `/` → `%2F`, `?` → `%3F`,
`#` → `%23`.

## Example 2: Unreserved Characters Pass Through

```cpp
const pluss::Note n = pluss::make_note({"A-Za-z0-9-_.~"}).url_encode();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["A-Za-z0-9-_.~"]
```

## Example 3: Non-ASCII Is Encoded Per UTF-8 Byte

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"café", "€"}).url_encode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["caf%C3%A9", "%E2%82%AC"]
```

## Example 4: Round-Trips Through `url_decode`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a b/c?d", "café"}).url_encode().url_decode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a b/c?d", "café"]
```

## See Also

- `pluss::coden::Note::url_decode`: the inverse.
- [`Note::base64_encode`](base64_encode.md): the other "make it ASCII-safe"
  encoding, for transfer rather than URL syntax.
- [`Note::tex_escape`](tex_escape.md): escaping for a different consumer.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="url_decode.html">
    <span class="label">Previous</span>
    <span class="title">url_decode</span>
  </a>
  <a class="next-link" href="vector.html">
    <span class="label">Next</span>
    <span class="title">vector</span>
  </a>
</div>
