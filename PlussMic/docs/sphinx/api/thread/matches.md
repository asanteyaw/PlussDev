<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 496e4813e614 -->

# Thread::matches

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::matches(std::string_view pattern) const -> Thread
```

## What It Does

Checks each cell of a `Note` (string) thread against a regular expression, replacing
each cell with a `Bool`. `matches()` is a **start-anchored search**, not a full-string
match: it reports `true` as soon as `pattern` matches a *prefix* of the cell's text
— anything left over after the match doesn't matter. For a check that requires the
pattern to consume the entire string, see [`fullmatch()`](fullmatch.md). Requires the
thread to be `Note`-kind.

## Parameters

**pattern** (`std::string_view`)

A regular expression (ECMAScript-flavored, via `std::regex`). Internally,
`matches()` wraps it as `^(?:pattern)` before searching.

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as `*this`, holding the
    elementwise match result.

## Examples

Example 1. A pattern matching only part of the string still counts as `true`.

```cpp
using namespace pluss::table;

auto codes = Thread(pluss::make_note({"SKU-1001", "SKU-42", "SKU-4"}));
auto out = codes.matches("SKU-[0-9]{2}");
std::cout << out;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

`"SKU-1001"` matches `true` even though the pattern only asks for exactly two
digits after `SKU-` — the prefix `"SKU-10"` already satisfies it, and the trailing
`"01"` is simply ignored. `"SKU-4"` has only one digit after `SKU-`, so it can't
satisfy even the two-digit prefix requirement, and comes back `false`. Compare with
[`fullmatch()`](fullmatch.md), which requires the whole string to be consumed and
would report `"SKU-1001"` as `false`.

Example 2. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.matches("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::matches: expected Note payload
```

## Notes

- An invalid regex pattern throws at call time (from the internal pattern
  compilation), independent of the thread's contents.
- Changes the thread's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Thread::fullmatch`](fullmatch.md): the whole-string-must-match counterpart.
- [`Thread::extract`](extract.md): pull out capture groups instead of just a
  boolean result.
- [`Thread::starts`](starts.md): a literal (non-regex) prefix check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="matmul.html">
    <span class="label">Next</span>
    <span class="title">matmul</span>
  </a>
</div>
