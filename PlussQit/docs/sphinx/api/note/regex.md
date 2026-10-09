<!-- pluss-hand-authored -->
# Note::regex

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::RegexAccessor Note::regex() const noexcept
```

## Enter The Pattern Operation Surface

`regex()` binds a lightweight regular-expression accessor to the current
Note. It separates pattern interpretation from general literal text methods:
`contains()` and `replace()` treat their arguments literally, while
`regex().search()` and `regex().replace()` compile a pattern.

Obtaining the accessor does not compile a pattern, inspect text, allocate
storage, or enlarge `Note`. The temporary proxy contains one pointer to the
receiver; the terminal method performs the operation. Keep the receiver alive
for the complete accessor call. Store the resulting Note or `TokenBatch`, not
the borrowed accessor.

[Open the Note Regex Accessor API Reference](../../note_regex_accessor_api_reference)

## Example: Select, Rewrite, Then Extract

```cpp
const auto source = pluss::make_note({"A1,b22,,C333", "none"});
const auto has_number = source.regex().search(R"(\d+)");
const auto redacted = source.regex().replace(R"(\d+)", "#");
const auto numbers = source.regex().findall(R"(\d+)");

std::cout << pluss::note::print(has_number) << '\n';
std::cout << pluss::note::print(redacted) << '\n';
std::cout << pluss::note::print(numbers.tokens()) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Bool)
[true, false]
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A#,b#,,C#", "none"]
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["1", "22", "333"]
```

The same source can feed several independent operations. Each terminal call
compiles its own pattern; the accessor itself is only the public namespace.

## See Also

- [Note Regex Accessor API Reference](../../note_regex_accessor_api_reference)
- [`contains`](contains.md): literal substring testing without pattern syntax.
- [`replace`](replace.md): literal substitution without pattern syntax.
- [`nlp`](nlp.md): language-oriented analysis and cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reals.html"><span class="label">Previous</span><span class="title">reals</span></a>
  <a class="next-link" href="remove.html"><span class="label">Next</span><span class="title">remove</span></a>
</div>
