<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc556c9952ed -->

# DataFrameNoteAccessor::strip

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameNoteAccessor::strip() const
DataFrame DataFrameNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from both ends of every selected text feature, cell by cell,
returning a new frame. There are two distinct trimming policies depending on
which overload you call:

- **No-argument overload**: trims the classic ASCII whitespace set — space,
  tab (`\t`), newline (`\n`), carriage return (`\r`), form feed (`\f`), and
  vertical tab (`\v`) — from both the start and end of each cell.
- **`chars` overload**: trims by *character-set membership*, not by matching
  `chars` as a literal prefix/suffix substring. Every leading character that
  appears anywhere in `chars` is removed, one at a time, until a character not
  in the set is reached; the same happens independently from the trailing
  end. This is the same convention Python's `str.strip(chars)` uses — `chars`
  is a bag of characters to peel off, not a string to match.

Every method on `DataFrameNoteAccessor` — `strip()` included — applies to
**every feature the accessor was built with**, not just one. When reached the
ordinary way, through [`DataFrame::nt()`](../dataframe/nt.md), that means
every feature on the (already all-`Note`) frame the accessor was constructed
from. A cell that is already null on the underlying `Slate` stays null;
`strip()` never turns a missing cell into an empty string or vice versa.

```{tip} A second way to reach the accessor: construct it directly
`DataFrameNoteAccessor`'s constructor — `DataFrameNoteAccessor(DataFrame
frame, std::vector<std::string> features)` — is public. Besides the usual
`frame.features({"city"}).nt()` route (which requires the *entire* frame
to already be `Note`-kind), you can construct the accessor directly with an
explicit feature list against a frame that still has other, non-text columns
mixed in — see Example 3. Every other accessor method inherits this same
alternate construction path; it isn't repeated on every method's page.
```

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's edges. Order doesn't matter,
and duplicates have no effect — `"ab"` and `"ba"` strip identically.

## Returns

`DataFrame`
    A new frame with every selected feature's text trimmed; every other
    feature (when reached via direct construction against a mixed frame) is
    left completely unchanged.

## Examples

Example 1. The default overload trims ASCII whitespace from both ends of
*every* selected feature in one call, leaving an already-null cell null.

```cpp
using namespace pluss::table;

auto catalog = DataFrame({
    {"sku",  Slate(
        pluss::make_note({"  SKU-1001\n", "\tSKU-1002", "SKU-1003   ", "SKU-1004"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
    {"name", Slate(pluss::make_note({
        " Wireless Mouse ", "USB-C Cable\t", "  Mechanical Keyboard", "Clearance Bin"}))},
}, Slate(pluss::make_note({"p1", "p2", "p3", "p4"})));

auto out = catalog.nt().strip();
std::cout << out;
```

### Output

```text
====================================
     | sku       name               
------------------------------------
p1   | SKU-1001  Wireless Mouse     
p2   | SKU-1002  USB-C Cable        
p3   | SKU-1003  Mechanical Keyboard
p4   | NULL      Clearance Bin      
------------------------------------
[4 cs x 2 feat]
```

`catalog` is already entirely `Note`-kind (`sku` and `name`), so `catalog.nt()`
works directly with no `.features()` narrowing needed. Both columns are
stripped in the same call: `sku`'s leading spaces, a trailing newline, a
leading tab, and trailing spaces are all removed, and `name`'s leading/trailing
spaces and trailing tab go too. `p4`'s `sku` cell was constructed as invalid
(paired with `false` in an explicit validity mask) — it stays `NULL` in the
output rather than being coerced into an empty string. `p4`'s `name`
(`"Clearance Bin"`) had no edge whitespace to begin with, so it passes through
unchanged, same as any other no-op case.

Example 2. The `chars` overload strips by character-set membership, not by
matching a literal substring — proven here by stripping mismatched bracket
characters that never appear together as the literal text `"[]()"`.

```cpp
auto codes = DataFrame({
    {"code", Slate(pluss::make_note({"[[[SKU-1001]]]", "((SKU-1002))", "SKU-1003"}))},
});
std::cout << codes;

auto stripped = codes.nt().strip("[]()");
std::cout << stripped;
```

### Output (before)

```text
=====================
     | code          
---------------------
0    | [[[SKU-1001]]]
1    | ((SKU-1002))  
2    | SKU-1003      
---------------------
[3 cs x 1 feat]
```

### Output (after)

```text
===============
     | code    
---------------
0    | SKU-1001
1    | SKU-1002
2    | SKU-1003
---------------
[3 cs x 1 feat]
```

`"[[[SKU-1001]]]"` has no `"[]()"` substring anywhere in it, yet every edge
character still gets stripped: `[`, `]`, `(`, and `)` are each individually
members of the `chars` set, so `strip("[]()")` peels off `[[[` from the front
and `]]]` from the back, one character at a time, stopping the instant it
reaches `S` or `1`. Row `2` (`"SKU-1003"`) has no bracket characters on either
edge at all, so it's returned unchanged.

Example 3. Construct the accessor directly against a mixed frame, selecting
just one text feature — bypassing `nt()`'s "every column must already be
`Note`" requirement entirely — then see the same direct construction throw
when the named feature doesn't exist.

```cpp
auto mixed = DataFrame({
    {"id",   Slate(pluss::make_series({1, 2, 3}))},
    {"city", Slate(pluss::make_note({"  Accra", "Tema ", " Kumasi "}))},
});

auto direct = DataFrameNoteAccessor(mixed, {"city"});
auto out2 = direct.strip();
std::cout << out2;
```

### Output

```text
===================
     | id    city  
-------------------
0    | 1     Accra 
1    | 2     Tema  
2    | 3     Kumasi
-------------------
[3 cs x 2 feat]
```

```cpp
try {
  auto bad = DataFrameNoteAccessor(mixed, {"price"});
  auto out3 = bad.strip();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::nt().strip: unknown feature 'price'
```

`mixed.nt()` directly would have thrown immediately, since `mixed` still has
a `Series` feature (`id`). Constructing `DataFrameNoteAccessor(mixed,
{"city"})` explicitly sidesteps that check: only `city` is touched, `id`
passes through completely untouched, and the frame never needs to be narrowed
with [`features()`](../dataframe/features.md) first. Naming a feature that
doesn't exist on the frame at all (`"price"`) throws once an actual operation
is called on it, not at construction time — the constructor itself performs no
validation.

## Notes

- The `chars` overload's "set of characters" semantics apply to `lstrip()` and
  `rstrip()` identically — see [`lstrip`](lstrip.md) and [`rstrip`](rstrip.md)
  for the one-sided versions of everything shown here.
- Every accessor method — not just `strip()` — applies to every feature the
  accessor was built with, whether that's "every `Note` feature on the frame"
  (via `.nt()`) or an explicit subset (via direct construction, Example 3).
- A null cell on input stays null on output; `strip()` never fabricates or
  discards nullity.

## See Also

- [`DataFrameNoteAccessor::lstrip`](lstrip.md) / [`DataFrameNoteAccessor::rstrip`](rstrip.md):
  the one-sided equivalents.
- [`DataFrame::nt`](../dataframe/nt.md): the usual entry point into this
  accessor, and the `features()`-first pattern this page's Example 3
  deliberately bypasses.
- [`DataFrameNoteAccessor::pad`](pad.md): the inverse operation — add
  characters to reach a target width instead of removing them.

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
