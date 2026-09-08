<!-- pluss-hand-authored -->
# Note::nlp_remove_urls

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::nlp_remove_urls() const
```

## Delete Web URLs From Each Cell

`nlp_remove_urls()` removes web links from every text cell and tidies the
whitespace left behind. Use it as a text-preprocessing step when links are
noise for the downstream task — building a bag-of-words model, computing
readability, de-duplicating near-identical messages, or preparing prompt
context where the URL adds tokens but no signal.

A span is treated as a URL when it starts with `http://`, `https://`, or
`www.` (case-insensitive) and runs until the next whitespace or one of
`<`, `>`, `"`, `'`, `)`, `]`. That covers the common cases — schemes and
`www.` hosts with paths and query strings — but deliberately **not**
bare domains without a scheme or `www.` prefix (`example.com` on its own is
left alone), and not email addresses (use
[`nlp_mask_pii`](nlp_mask_pii.md) for those).

After the links are cut, the surrounding whitespace is collapsed to single
spaces and the cell is trimmed at both ends, so removing a URL from the
middle of a sentence does not leave a double space or a dangling gap.

Contrast [`nlp_mask_pii`](nlp_mask_pii.md), whose `[URL]` category
**replaces** each link with a placeholder rather than deleting it: use
`nlp_mask_pii` when the fact that a link was there matters, and
`nlp_remove_urls` when it does not. The operation is shape-preserving,
cell-independent, and returns a fresh `Note`; a cell with no link is copied
through unchanged.

## Example 1: Links In The Middle Of Prose

```cpp
std::cout << pluss::note::print(
    pluss::make_note(
        {"see https://pluss.dev/docs and www.example.com now", "no link here"})
        .nlp_remove_urls())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["see and now", "no link here"]
```

Both the `https://` link and the `www.` host are removed, and the resulting
run of spaces is collapsed. The second cell has no link and is unchanged.

## Example 2: A Scalar With A Query String

```cpp
const pluss::Note n =
    pluss::make_note("visit http://a.b/c?q=1 today").nlp_remove_urls();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"visit today"
```

The path and query string are part of the URL span and go with it.

## Example 3: Non-URL Text Is Left Alone

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"emails a@b.io are kept"}).nlp_remove_urls())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["emails a@b.io are kept"]
```

An email address has no `http`/`www.` prefix, so `nlp_remove_urls` does not
touch it.

## See Also

- [`Note::nlp_mask_pii`](nlp_mask_pii.md): replace URLs (and other
  identifying spans) with named placeholders instead of deleting them.
- [`Note::remove_all`](remove_all.md): delete a known literal substring.
- [`Note::regex_findall`](regex_findall.md): pull the URLs out for
  inspection or an allow-list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nlp_mask_pii.html">
    <span class="label">Previous</span>
    <span class="title">nlp_mask_pii</span>
  </a>
  <a class="next-link" href="quote.html">
    <span class="label">Next</span>
    <span class="title">quote</span>
  </a>
</div>
