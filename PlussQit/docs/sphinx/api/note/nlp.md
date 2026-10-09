<!-- pluss-hand-authored -->
# Note::nlp

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::NlpAccessor Note::nlp() const noexcept
```

## Enter The Linguistic Operation Surface

`nlp()` binds a lightweight accessor to the current Note. The accessor groups
linguistic analysis, corpus cleanup, token extraction, phonetic transforms,
and language-oriented comparison without crowding the general Note surface
with prefixed method names.

The call does not analyze text, allocate storage, copy the Note, or change its
dtype. It returns a temporary proxy containing one pointer to the receiver;
the selected operation performs the work. This keeps `Note` itself a
one-pointer intrusive handle while allowing fluent pipelines:

```cpp
const auto cleaned = text.nlp().remove_html();
const auto words = cleaned.nlp().tokens();
```

The accessor is valid for the full expression in which it is created. Store
the Note, not the accessor, when work must outlive that expression. Each
accessor method documents its own accepted dtype, shape behavior, options,
and result type.

[Open the Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Example: Cleanup Then Tokenization

```cpp
const auto source = pluss::make_note({"<b>Ship</b> #release today"});
const auto plain = source.nlp().remove_html().nlp().remove_hashtags();
const auto words = plain.nlp().tokens();

std::cout << pluss::note::print(plain) << '\n';
std::cout << pluss::note::print(words.tokens()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Ship today"]
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Ship", "today"]
```

The first call strips element syntax but keeps `Ship`; the second removes the
tag token, and `tokens()` then represents the two remaining words as the Text
member of a closed `TokenBatch`. Use
[`words.tokens()`](../tokenbatch/tokens.md) for the flat values and
[`words.offsets()`](../tokenbatch/offsets.md) for the cumulative row boundaries;
neither is a root `Note` method.

## See Also

- [Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)
- [TokenBatch API Reference](../../tokenbatch_api_reference)
- [`split`](split.md): delimiter-based structural splitting outside linguistic analysis.
- [`regex`](regex.md): explicit pattern operations outside linguistic analysis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="narrow.html"><span class="label">Previous</span><span class="title">narrow</span></a>
  <a class="next-link" href="normalize_dashes.html"><span class="label">Next</span><span class="title">normalize_dashes</span></a>
</div>
