# Thread::encode

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::encode(std::string_view encoding = "base64") -> Thread
```

Encode each note value using one of the built-in text encodings.

`encode(...)` is the text-column surface for turning raw note values into encoded representations such as Base64, hexadecimal text, or URL-safe escaped text.

## Parameters

**encoding**

Encoding algorithm to apply.

- `base64`: Base64 text representation.
- `hex`: hexadecimal text representation.
- `url`: percent-escaped URL-style representation.

## Returns

`Thread`
    Note-backed thread containing the encoded text.

## See Also

- <a href="extract.html"><code>Thread::extract</code></a>: structured regex extraction from note values instead of encoding them.
- <a href="chars.html"><code>Thread::chars</code></a>: slice or transform text at the character level before encoding.

## Notes

`encode(...)` is note-only. Call `as_note()` or select a note-backed feature first when the current thread is not already textual.

Supported encodings are exactly `base64`, `hex`, and `url`. Any other encoding name throws.

## Examples

Example 1. Encode note values as Base64 text.

```cpp
using namespace pluss::table;

auto urls = Thread(pluss::make_note({"A B", "C&D", "email@example.com"}));

auto out = urls.encode("base64");
```

### Output

```text
0  QSBC
1  QyZE
2  ZW1haWxAZXhhbXBsZS5jb~
dtype: Note
```

Example 2. Encode note values as hexadecimal text.

```cpp
using namespace pluss::table;

auto urls = Thread(pluss::make_note({"A B", "C&D", "email@example.com"}));

auto out = urls.encode("hex");
```

### Output

```text
0  412042
1  432644
2  656d61696c406578616d7~
dtype: Note
```

Example 3. Encode note values with URL escaping.

```cpp
using namespace pluss::table;

auto urls = Thread(pluss::make_note({"A B", "C&D", "email@example.com"}));

auto out = urls.encode("url");
```

### Output

```text
0  A%20B
1  C%26D
2  email%40example.com
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>
