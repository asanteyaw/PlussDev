<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23e356d1f482 -->

# Slate::append

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::append(const Slate& other) const
```

## What It Does

Concatenates `other` onto the end of `*this`, returning a new, longer slate. This
is a strict operation: `*this` and `other` must have the same [`kind()`](kind.md),
and for `Series`-kind slates, the same concrete scalar dtype and platform too —
`append()` never coerces or promotes types to make two mismatched slates fit
together.

## Parameters

**other** (`const Slate&`)
    The slate to append. Must match `*this` exactly in payload kind (and, for
    `Series`, scalar dtype and platform).

## Returns

`Slate`
    A new slate, length `this->size() + other.size()`, holding `*this`'s entries
    followed by `other`'s.

## Examples

Example 1. Append two compatible `Series`-kind slates.

```cpp
using namespace pluss::table;

auto first = Slate(pluss::make_series({10.0, 20.0}));
auto second = Slate(pluss::make_series({30.0, 40.0}));
auto out = first.append(second);
std::cout << out;
```

### Output

```text
0  10
1  20
2  30
3  40

ptype: Series
```

Example 2. Appending slates of different payload kinds throws.

```cpp
auto notes = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = first.append(notes);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::append: payload kinds must match
```

## Notes

- Requires identical `kind()` between `*this` and `other`; throws
  `"Slate::append: payload kinds must match"` otherwise.
- For `Series`-kind slates, also requires identical concrete scalar dtype
  (`"Slate::append: Series dtypes must match"`) and platform
  (`"Slate::append: Series platforms must match"`).
- Validity masks combine correctly even when only one side has one — a side with
  no explicit mask is treated as fully valid.
- Also requires both slates to agree on whether they carry case labels at all
  (`"Slate::append: both Slates must either have case labels or both be
  unlabeled"`); if both do, the labels are appended too.

## See Also

- [`Slate::take`](take.md): combine with `append()` for more elaborate
  reassembly.
- [`Slate::cast`](cast.md) / [`Slate::astype`](astype.md): convert dtype first
  if two `Series`-kind slates have mismatched scalar types.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html">
    <span class="label">Previous</span>
    <span class="title">any</span>
  </a>
  <a class="next-link" href="arrange.html">
    <span class="label">Next</span>
    <span class="title">arrange</span>
  </a>
</div>
