<!-- pluss-hand-authored -->
# Note::is_text

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_text() const noexcept
```

## Distinguish Validated Text From Raw Bytes

`is_text()` reports whether the receiver stores validated UTF-8 text lanes.
It is deliberately distinct from [`is_bytes`](is_bytes.md): Bytes may contain
arbitrary octets, including embedded zero bytes and invalid UTF-8, while Text
participates in character-, grapheme-, case-, and language-aware operations.

The method checks the storage tag only. It does not rescan UTF-8, inspect every
cell, or claim that a Bytes payload could not be decoded. Use
`to_utf8_strict()` or `to_utf8_lossy()` when conversion is intended. Most text
transformations preserve Text dtype, whereas predicates and measurements
intentionally return Bool or Int64 Notes.

The query is constant-time, allocation-free, and `noexcept`. It is independent
of rank and cell contents. Undefined returns `false`, so test `defined()` as
well when absence must be reported separately.

## Example 1: Text And Bytes Are Different Contracts

```cpp
const auto text = pluss::note::make("alpha");
const auto bytes = pluss::note::bytes(std::string_view{"A\0B", 3});
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "text=" << text.is_text()
          << " bytes=" << bytes.is_text()
          << " absent=" << absent.is_text() << '\n';
```

### Output

```text
text=true bytes=false absent=false
```

The embedded zero is retained by Bytes; it is not treated as a C-string
terminator.

## Example 2: A Text Transformation Preserves Dtype

```cpp
const auto lowered = pluss::note::make("Alpha").lower();
std::cout << std::boolalpha << "lowered=" << lowered.is_text()
          << " value=" << lowered.scalar() << '\n';
```

### Output

```text
lowered=true value=alpha
```

## Example 3: A Predicate Changes The Result Dtype

```cpp
const auto mask = pluss::note::make({"alpha", "beta"}).contains("a");
std::cout << std::boolalpha
          << "text=" << mask.is_text()
          << " bool=" << mask.is_bool() << '\n';
```

### Output

```text
text=false bool=true
```

## See Also

- [`Note::is_bytes`](is_bytes.md): recognize arbitrary byte storage.
- [`Note::is_utf8_valid`](is_utf8_valid.md): validate byte cells without
  changing their dtype.
- [`Note::dtype`](dtype.md): retrieve the complete storage tag.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_scalar.html"><span class="label">Previous</span><span class="title">is_scalar</span></a>
  <a class="next-link" href="is_title.html"><span class="label">Next</span><span class="title">is_title</span></a>
</div>
