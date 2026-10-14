<!-- pluss-hand-authored -->

# SlateNoteAccessor::fullmatch

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::fullmatch(std::string_view pattern) const -> Slate
```

## What It Does

Checks each cell of the bound slate against a regular expression, requiring
`pattern` to match the **entire** cell's text — not just a prefix, the way
[`matches()`](matches.md) does. A direct forward to
[`Slate::fullmatch()`](../slate/fullmatch.md). Requires the bound slate to be
`Note`-kind.

## Parameters

**pattern** (`std::string_view`)

A regular expression (ECMAScript-flavored, via `std::regex`).

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate, holding the
    elementwise full-match result.

## Examples

Example 1. Only a cell that's fully consumed by the pattern counts as `true`.

```cpp
using namespace pluss::table;

auto codes = Slate(pluss::make_note({"SKU-1001", "SKU-42", "SKU-4"}));
auto out = codes.nt().fullmatch("SKU-[0-9]{2}");
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

`"SKU-1001"` is `false` — the pattern only accounts for `"SKU-10"`, and the leftover
`"01"` means the pattern doesn't consume the whole string. `"SKU-42"` is exactly two
digits after `SKU-` with nothing left over, so it's `true`. Compare with
[`matches()`](matches.md), which reports `"SKU-1001"` as `true` for the same pattern
since it only checks a prefix.

Example 2. A non-`Note` slate throws — the same message
[`Slate::fullmatch()`](../slate/fullmatch.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().fullmatch("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::fullmatch: expected Note payload
```

## Notes

- `SlateNoteAccessor::fullmatch()` is a one-line forward to
  [`Slate::fullmatch()`](../slate/fullmatch.md) — identical behavior and throws.
- An invalid regex pattern throws at call time.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Slate::fullmatch`](../slate/fullmatch.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::matches`](matches.md): the prefix-only counterpart.
- [`SlateNoteAccessor::extract`](extract.md): pull out capture groups instead of
  just a boolean result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
