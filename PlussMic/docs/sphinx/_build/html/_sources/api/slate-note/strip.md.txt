<!-- pluss-hand-authored -->

# SlateNoteAccessor::strip

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signatures

```cpp
SlateNoteAccessor::strip() const -> Slate
SlateNoteAccessor::strip(std::string_view chars) const -> Slate
```

## What It Does

Trims characters from both ends of the bound slate's cells — a direct forward
to [`Slate::strip()`](../slate/strip.md). Requires the bound slate to be
`Note`-kind. Two distinct trimming policies:

- **No-argument overload**: trims the classic ASCII whitespace set (space, tab,
  newline, carriage return, form feed, vertical tab) from both ends.
- **`chars` overload**: trims by **character-set membership**, not by matching
  `chars` as a literal prefix/suffix substring — every leading/trailing
  character that appears anywhere in `chars` is removed, one at a time, until a
  character not in the set is reached.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's edges. Order doesn't matter.

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, with each cell's
    text trimmed.

## Examples

Example 1. The default overload trims ASCII whitespace from both ends.

```cpp
using namespace pluss::table;

auto skus = Slate(pluss::make_note({"  SKU-1001\n", "\tSKU-1002", "SKU-1003   "}));
auto out = skus.nt().strip();
std::cout << out;
```

### Output

```text
0  SKU-1001
1  SKU-1002
2  SKU-1003

ptype: Note
```

Example 2. The `chars` overload strips by character-set membership, not by
matching a literal substring.

```cpp
auto codes = Slate(pluss::make_note({"[[[SKU-1001]]]", "((SKU-1002))", "SKU-1003"}));
auto out2 = codes.nt().strip("[]()");
std::cout << out2;
```

### Output

```text
0  SKU-1001
1  SKU-1002
2  SKU-1003

ptype: Note
```

`"[[[SKU-1001]]]"` has no `"[]()"` substring anywhere in it, yet every edge
character still gets stripped, one character at a time, stopping the instant a
character outside the `chars` set is reached.

Example 3. A non-`Note` slate throws — the same message
[`Slate::strip()`](../slate/strip.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().strip();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::strip is only valid for Note payload
```

## Notes

- `SlateNoteAccessor::strip()` is a one-line forward to
  [`Slate::strip()`](../slate/strip.md) — identical behavior and throws.
- The `chars` overload's set-membership semantics apply to
  [`lstrip()`](lstrip.md) and [`rstrip()`](rstrip.md) identically — one-sided
  versions of everything shown here.
- A null cell on input stays null on output.

## See Also

- [`Slate::strip`](../slate/strip.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::lstrip`](lstrip.md) /
  [`SlateNoteAccessor::rstrip`](rstrip.md): the one-sided equivalents.
- [`SlateNoteAccessor::pad`](pad.md): the inverse operation — add characters
  instead of removing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
