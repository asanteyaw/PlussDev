<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6cddd140ebd1 -->

# Thread::strip

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::strip() const -> Thread
Thread::strip(std::string_view chars) const -> Thread
```

## What It Does

Trims characters from both ends of a `Note` (string) thread's cells. Requires the
thread to be `Note`-kind. Two distinct trimming policies:

- **No-argument overload**: trims the classic ASCII whitespace set — space, tab,
  newline, carriage return, form feed, vertical tab — from both ends of each cell.
- **`chars` overload**: trims by **character-set membership**, not by matching
  `chars` as a literal prefix/suffix substring. Every leading character that appears
  anywhere in `chars` is removed, one at a time, until a character not in the set is
  reached; the same happens independently from the trailing end — the same
  convention Python's `str.strip(chars)` uses.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's edges. Order doesn't matter.

## Returns

`Thread`
    A new `Note`-kind thread, same length as `*this`, with each cell's text trimmed.

## Examples

Example 1. The default overload trims ASCII whitespace from both ends.

```cpp
using namespace pluss::table;

auto skus = Thread(pluss::make_note({"  SKU-1001\n", "\tSKU-1002", "SKU-1003   "}));
auto out = skus.strip();
std::cout << out;
```

### Output

```text
0  SKU-1001
1  SKU-1002
2  SKU-1003

ptype: Note
```

Example 2. The `chars` overload strips by character-set membership, not by matching
a literal substring — proven here by stripping mismatched bracket characters that
never appear together as the literal text `"[]()"`.

```cpp
auto codes = Thread(pluss::make_note({"[[[SKU-1001]]]", "((SKU-1002))", "SKU-1003"}));
auto out2 = codes.strip("[]()");
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
character still gets stripped: `[`, `]`, `(`, and `)` are each individually members
of the `chars` set, so `strip("[]()")` peels off `[[[` from the front and `]]]`
from the back, one character at a time, stopping the instant it reaches `S` or `1`.

Example 3. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.strip();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::strip is only valid for Note payload
```

## Notes

- The `chars` overload's set-membership semantics apply to [`lstrip()`](lstrip.md)
  and [`rstrip()`](rstrip.md) identically — one-sided versions of everything shown
  here.
- A null cell on input stays null on output.
- See [`DataFrameNoteAccessor::strip`](../dataframe-note/strip.md) for the
  multi-feature counterpart.

## See Also

- [`Thread::lstrip`](lstrip.md) / [`Thread::rstrip`](rstrip.md): the one-sided
  equivalents.
- [`Thread::pad`](pad.md): the inverse operation — add characters instead of
  removing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
