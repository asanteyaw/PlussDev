<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c03ff63c1a83 -->

# Slate::starts

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::starts(std::string_view prefix) const -> Slate
```

## What It Does

Checks a `Note` (string) slate's cells for a literal, case-sensitive prefix match,
replacing each cell with a `Bool` — `true` where the cell's text begins with
`prefix`, `false` where it doesn't. The payload kind changes from `Note` to `Series`
(`Bool`). Requires the slate to be `Note`-kind.

`starts()` has exactly one overload and takes no options — always a plain,
case-sensitive, literal check. There's no regex mode; use [`matches()`](matches.md)
with a pattern anchored at the start for anything more flexible.

## Parameters

**prefix** (`std::string_view`)

The literal text to check for at the start of each cell. An empty prefix matches
every non-null cell.

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as `*this`, holding the
    elementwise prefix check.

## Examples

Example 1. Check a text slate for a literal prefix.

```cpp
using namespace pluss::table;

auto codes = Slate(pluss::make_note({"SKU-1001", "sku-1002", "REF-2003"}));
auto out = codes.starts("SKU-");
std::cout << out;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

Row `1` (`"sku-1002"`) doesn't match — the comparison is case-sensitive, so a
lowercase `"sku-"` isn't a match for `"SKU-"`.

Example 2. A non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.starts("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::starts: expected Note payload
```

## Notes

- Case-sensitive, literal comparison only — no regex mode, no case-insensitivity
  toggle. Normalize with [`lower()`](lower.md)/[`casefold()`](casefold.md) first, or
  use [`matches()`](matches.md), for anything more flexible.
- Changes the slate's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Slate::ends`](ends.md): the same check anchored at the end instead of the
  start.
- [`Slate::matches`](matches.md): a regex-anchored-at-start check, for patterns
  more flexible than a literal prefix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
