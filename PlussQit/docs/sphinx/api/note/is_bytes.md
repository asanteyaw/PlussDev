<!-- pluss-hand-authored -->
# Note::is_bytes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_bytes() const noexcept
```

## Test Whether The `Note` Is In Bytes Mode

`is_bytes()` reports whether the complete `Note` has the `NoteDType::Bytes`
dtype. Dtype belongs to the `Note` as a whole, so this is a scalar metadata
query rather than an elementwise predicate. It returns `true` for raw-byte
storage and `false` for Text, Bool, Int64, Float64, and undefined Notes.

Use it to branch before calling text-only methods (character length, case
mapping, regex) that require a UTF-8-mode `Note`, or to assert that a
pipeline stage produced the encoding you expected.

The query does not inspect or validate cell content. A Bytes Note may happen to
contain valid UTF-8, while a Text Note carries text intent independently of a
particular cell's bytes. Use `is_utf8_valid()` when the question is whether
each stored byte sequence is valid UTF-8.

The call is constant time, performs no allocation, and is safe on an undefined
Note, for which it returns `false`.

## Example 1: A UTF-8-Mode `Note`

```cpp
const auto n = pluss::make_note({"a", "b", "c"});
std::cout << std::boolalpha << n.is_bytes() << '\n';
```

### Output

```text
false
```

## Example 2: A Bytes-Mode `Note`

```cpp
const auto n = pluss::note::bytes({"a", "b"});
std::cout << std::boolalpha << n.is_bytes() << '\n';
```

### Output

```text
true
```

## Example 3: Valid UTF-8 Does Not Change The Dtype

```cpp
const auto raw = pluss::note::bytes({"plain ASCII", "caf\xC3\xA9"});

std::cout << std::boolalpha
          << "bytes: " << raw.is_bytes() << '\n'
          << "valid: ";
const auto valid = raw.is_utf8_valid();
for (std::size_t i = 0; i < valid.size(); ++i)
  std::cout << (i ? " " : "") << static_cast<int>(valid[i]);
std::cout << '\n';
```

### Output

```text
bytes: true
valid: 1 1
```

Both cells decode as UTF-8, but the Note remains Bytes until an explicit
conversion such as `to_utf8_strict()` produces a Text Note.

## Failure Behavior

`is_bytes()` has no failure path. It reads only the dtype tag and does not
dereference cell storage. In particular, `Note::undefined().is_bytes()` is
`false`; use `defined()` separately when absence must be distinguished from a
defined non-Bytes Note.

## See Also

- [`pluss::note::bytes`](../note_factory/bytes.md): build a bytes-mode
  `Note`.
- [`Note::is_utf8_valid`](is_utf8_valid.md): whether the bytes would decode
  cleanly as UTF-8.
- `pluss::coden::Note::to_utf8_lossy`: switch a bytes-mode `Note` to text.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_blank.html">
    <span class="label">Previous</span>
    <span class="title">is_blank</span>
  </a>
  <a class="next-link" href="is_digit.html">
    <span class="label">Next</span>
    <span class="title">is_digit</span>
  </a>
</div>
