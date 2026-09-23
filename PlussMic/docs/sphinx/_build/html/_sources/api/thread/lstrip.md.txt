<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 822470f34bda -->

# Thread::lstrip

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::lstrip() const -> Thread
Thread::lstrip(std::string_view chars) const -> Thread
```

## What It Does

The left-only half of [`strip()`](strip.md): trims characters from the **start** of
a `Note` thread's cells, leaving the trailing end untouched. Same two policies as
`strip()` (whitespace by default, character-set membership with `chars`) — see that
page for the full explanation of why the `chars` overload is set-membership, not
literal substring matching.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's left edge only.

## Returns

`Thread`
    A new `Note`-kind thread, same length as `*this`, with each cell's leading text
    trimmed.

## Examples

Example 1. Strip a leading marker character, leaving the trailing end untouched.

```cpp
using namespace pluss::table;

auto ids = Thread(pluss::make_note({"#203930", "##7788", "4455##"}));
auto out = ids.lstrip("#");
std::cout << out;
```

### Output

```text
0  203930
1  7788
2  4455##

ptype: Note
```

Row `2`'s trailing `##` is untouched — only leading `#` characters are peeled off.

## Notes

- See [`strip()`](strip.md) for the full explanation of the default-whitespace vs.
  `chars`-set-membership policies, and the non-`Note` throw
  (`"Thread::lstrip is only valid for Note payload"` /
  `"Thread::lstrip(chars) is only valid for Note payload"`).

## See Also

- [`Thread::rstrip`](rstrip.md): the mirror image, trimming the trailing end only.
- [`Thread::strip`](strip.md): both ends at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
