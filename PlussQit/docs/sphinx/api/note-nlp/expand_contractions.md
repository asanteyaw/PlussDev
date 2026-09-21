<!-- pluss-hand-authored -->
# NoteNlpAccessor::expand_contractions

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::expand_contractions() const
```

## Make Selected English Contractions Explicit

`expand_contractions()` rewrites common straight-apostrophe English
contractions as separate words. It is useful before word counting, rule-based
matching, or tokenization when `can't` should contribute `can` and `not`
instead of one surface token.

The fixed vocabulary includes `can't`, `won't`, `I'm`, `you're`, `we're`,
`they're`, `it's`, `that's`, `there's`, `what's`, `who's`, `let's`, `I've`,
`we've`, `they've`, `I'll`, and `you'll`. In addition, an ASCII word ending in
`n't` uses the general rule `stem + " not"`, so `didn't`, `isn't`, and
`shouldn't` are covered without separate dictionary entries.

Matching is case-insensitive and occurs on complete ASCII letter/apostrophe
tokens. Casing is transferred according to the source form:

| Source form | Result policy | Example |
|---|---|---|
| lowercase | lowercase expansion | `can't` -> `can not` |
| initial capital | first expansion letter capitalized | `Won't` -> `Will not` |
| all capitals | complete expansion capitalized | `CAN'T` -> `CAN NOT` |

The method is intentionally lexical rather than grammatical. It does not infer
whether `'s` means *is* or *has*, and it does not expand arbitrary possessives
or apostrophe-bearing identifiers. A curly apostrophe is not silently treated
as a straight apostrophe; call `normalize_quotes()` or `clean()` first when
input uses typographic punctuation.

No cleanup is bundled into this operation. Surrounding spaces, tabs, line
breaks, punctuation, and unaffected words are copied byte-for-byte. That makes
the method safe to insert into a pipeline without unexpectedly reflowing text.
The result is a fresh, shape-preserving Text Note, and a second application is
idempotent because the expanded forms contain no contraction token.

Empty Text Notes remain empty, undefined propagates, and non-Text dtypes are
rejected.

## Example 1: Dictionary And General `n't` Forms

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"I can't; You're ready, but they didn't."})
        .nlp().expand_contractions()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["I can not; You are ready, but they did not."]
```

`can't` uses a special expansion because its stem is not obtained by merely
removing `n't`; `didn't` exercises the general suffix rule.

## Example 2: Casing And Token Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"CAN'T Won't I'M we're shell'code"})
        .nlp().expand_contractions()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["CAN NOT Will not I AM we are shell'code"]
```

`shell'code` is one apostrophe-bearing token but not a known contraction, so it
is preserved.

## Example 3: Whitespace Is Content Here

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  it's\tstill\nthere  "})
        .nlp().expand_contractions()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  it is\tstill\nthere  "]
```

## Example 4: A Shape-Preserving Batch

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{" can't ", "12", "“yes”", "plain"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().expand_contractions()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  [" can not ", "12"], 
  ["“yes”", "plain"]
]
```

For empty, undefined, and dtype behavior, the family-wide contract described
on [`clean`](clean.md) applies identically. Bytes, Bool, Int64, and
Float64 Notes are not interpreted as prose.

## See Also

- [`clean`](clean.md): normalize punctuation and whitespace before expansion.
- [`normalize_quotes`](../note/normalize_quotes.md): convert curly apostrophes without applying the rest of the cleaning pipeline.
- [`split_whitespace`](../note/split_whitespace.md): produce word-like fields after expansion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="emails.html"><span class="label">Previous</span><span class="title">emails</span></a>
  <a class="next-link" href="formality.html"><span class="label">Next</span><span class="title">formality</span></a>
</div>
