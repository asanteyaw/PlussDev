<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a7a135bca270 -->

# Thread::fullmatch

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::fullmatch(std::string_view pattern) const -> Thread
```

## What It Does

Checks each cell of a `Note` (string) thread against a regular expression, requiring
`pattern` to match the **entire** cell's text — not just a prefix, the way
[`matches()`](matches.md) does. Requires the thread to be `Note`-kind.

## Parameters

**pattern** (`std::string_view`)

A regular expression (ECMAScript-flavored, via `std::regex`).

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as `*this`, holding the
    elementwise full-match result.

## Examples

Example 1. Only a cell that's fully consumed by the pattern counts as `true`.

```cpp
using namespace pluss::table;

auto codes = Thread(pluss::make_note({"SKU-1001", "SKU-42", "SKU-4"}));
auto out = codes.fullmatch("SKU-[0-9]{2}");
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

`"SKU-1001"` is now `false` — the pattern only accounts for `"SKU-10"`, and the
leftover `"01"` means the pattern doesn't consume the whole string. `"SKU-42"` is
exactly two digits after `SKU-` with nothing left over, so it's `true`. Compare with
[`matches()`](matches.md), which reported `"SKU-1001"` as `true` for the same
pattern since it only checks a prefix.

Example 2. A non-`Note` thread throws.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.fullmatch("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::fullmatch: expected Note payload
```

## Notes

- An invalid regex pattern throws at call time.
- Changes the thread's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Thread::matches`](matches.md): the prefix-only counterpart.
- [`Thread::extract`](extract.md): pull out capture groups instead of just a
  boolean result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
