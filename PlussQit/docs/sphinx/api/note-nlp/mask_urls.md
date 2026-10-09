<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_urls

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_urls() const
```

## Remove Destinations While Preserving Sentence Punctuation

`mask_urls()` replaces HTTP, HTTPS, and `www.` URL-shaped spans with
`[URL]`. Scheme matching is case-insensitive. Paths, fragments, and query
strings remain part of the same match, so a complete destination becomes one
placeholder rather than several numeric or punctuation fragments.

The recognized starts are `http://`, `https://`, and `www.`. Bare domains and
other schemes such as `ftp://` are intentionally outside this method. A URL
must end in a URL-like byte; terminal prose punctuation such as `.`, `,`, `!`,
closing `)`, `]`, or a quote remains in the sentence. Characters such as `?`,
`#`, `&`, `=`, `/`, `_`, `%`, `~`, `+`, and `-` may legitimately end a URL.

This is deterministic text masking, not network validation. The method does
not resolve hosts or check whether a resource exists. Use
[`remove_urls`](remove_urls.md) when the span and its category marker
should both disappear.

The receiver must be Text. The result is a fresh packed Text Note with the same
shape, and the receiver is unchanged. Undefined propagates, empty shapes stay
defined and empty, and `[URL]` is idempotent. Cells without an HTTP or `www.`
prefix take a fast no-match path.

## Example 1: Scheme, Case, Path, Query, And Fragment

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"HTTPS://example.com/a?q=1", "www.x.io/path#part"})
        .nlp().mask_urls()) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["[URL]", "[URL]"]
```

## Example 2: Sentence Punctuation And Unsupported Schemes

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"(https://x.io/a), www.x.io! not ftp://x.io"})
        .nlp().mask_urls()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["([URL]), [URL]! not ftp://x.io"]
```

## Example 3: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_urls();
std::cout << "urls empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "urls undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_urls().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"https://x.io"},
      std::array<std::int64_t, 1>{1}).nlp().mask_urls();
} catch (const pluss::Error&) { std::cout << "urls Bytes: rejected\n"; }
```

### Output

```text
urls empty dim=1 numel=0
urls undefined=false
urls Bytes: rejected
```

## See Also

- [`remove_urls`](remove_urls.md): delete recognized URLs instead of preserving `[URL]`.
- [`mask_pii`](mask_pii.md): mask URLs before mentions and broad numbers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_times.html"><span class="label">Previous</span><span class="title">mask_times</span></a>
  <a class="next-link" href="match_tokens.html"><span class="label">Next</span><span class="title">match_tokens</span></a>
</div>
